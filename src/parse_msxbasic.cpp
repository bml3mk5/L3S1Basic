/// @file parse_msxbasic.cpp
///
/// @brief MSX-BASICパーサー
///
#include "parse_msxbasic.h"
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/arrimpl.cpp>
#include "main.h"
#include "config.h"
//#include "msxspecs.h"
#include "pssymbol.h"

/// マシンタイプ
#define MACHINE_TYPE_MSX 1

enum en_basic_code {
	CODE_MEMORYADDR = 0x0d,
	CODE_LINENUMBER = 0x0e,
	CODE_NUMBER8BIT = 0x0f,
	CODE_NUMBER0 = 0x11,
	CODE_NUMBER9 = 0x1a,
	CODE_NUMBER16BIT = 0x1c,
	CODE_FLOAT = 0x1d,
	CODE_DOUBLE = 0x1f,
};

#define MSX_CHAR_CODE_TABLE  "msx_char_code.dat"
#define MSX_BASIC_CODE_TABLE "msx_basic_code.dat"

//////////////////////////////////////////////////////////////////////

/// スタートアドレス
const int ParseMSXBasic::iStartAddr[eMSXStartAddrCount] = {
	0x8000	// 0x8000
};

//////////////////////////////////////////////////////////////////////
///
/// パーサークラス
///
ParseMSXBasic::ParseMSXBasic(ParseCollection *collection)
	: Parse(collection, &gConfig.GetParam(eMSXBasic))
{
	mBigEndian = false;	// Z80なので
}

ParseMSXBasic::~ParseMSXBasic()
{
}

/// キャラクターコードテーブルファイル名
wxString ParseMSXBasic::GetCharCodeTableFileName() const
{
	return MSX_CHAR_CODE_TABLE;
}

/// BASICコードテーブルファイル名
wxString ParseMSXBasic::GetBasicCodeTableFileName() const
{
	return MSX_BASIC_CODE_TABLE;
}

/// 初期設定値をセット
void ParseMSXBasic::SetDefaultConfigParam()
{
	pConfig->SetNewLineAscii(eMSXDefaultNewLineAscii);
	pConfig->SetStartAddr(eMSXDefaultStartAddr);
}

/// 入力データのフォーマットチェック
/// @param[in] in_file_info : 入力ファイルの情報
/// @return true/false
bool ParseMSXBasic::CheckDataFormat(PsFileInputInfo &in_file_info)
{
	wxUint8 hsign[32];
	wxUint8 *hp = NULL;
	bool st = true;
	PsFileFsInput in_file(in_file_info);
	PsFileStrOutput out_data;

	size_t len = in_file.Read(hsign, 32);

	if (len == 0) {
		// file is empty
		mErrInfo.SetInfo(__LINE__, psError, psErrFileIsEmpty);
		mErrInfo.ShowMsgBox();
		return false;
	}

	in_file.SetTypeFlag(psAscii, true);

	// テープイメージヘッダがあるか先頭から8バイトを検索
	hp = hsign;
	bool is_tape = false;
	if (memcmp(hsign, FMSX_HEADER, 8) == 0) {
		// fmsx casette image
		is_tape = true;
		in_file.SetTypeFlag(psTapeImage, true);
		hp = &hsign[8];
	}

	if (memcmp(hp, CAS_HEADER_BASIC, 10) == 0) {
		// basic intermediate language
		is_tape = true;
		in_file.SetTypeFlag(psAscii, false);

	} else if (memcmp(hp, CAS_HEADER_ASCII, 10) == 0) {
		// ascii
		is_tape = true;
		in_file.SetTypeFlag(psAscii, true);

	}

	out_data.SetType(in_file.GetType());
	if (is_tape) {
		// テープイメージから実データを取り出してバッファに入れる
		in_file.SeekStartPos(0);
		st = CheckTapeDataFormat(in_file, out_data);
		if (!st) return st;
	} else {
		// ファイルデータをそのままバッファに入れる
		in_file.SeekStartPos(0);
		out_data.Write(in_file);
	}

	PsFileStrInput in_data(out_data);
	out_data.Clear();

	len = in_data.Read(hsign, 32);

	if (!in_data.GetTypeFlag(psAscii)) {
		// basic intermediate language
		in_data.SeekStartPos(0);
		st = CheckBinaryDataFormat(in_data);

	} else if (hsign[0] == 0xff) {
		// basic intermediate language
		in_data.SetTypeFlag(psAscii, false);
		in_data.SeekStartPos(1);
		st = CheckBinaryDataFormat(in_data);

	} else if (hsign[0] == 0xfe) {
		// basic intermediate language (encrypted)
		in_data.SetTypeFlag(psAscii, false);
		in_data.SetTypeFlag(psEncrypted, true);
		// error
		mErrInfo.SetInfo(__LINE__, psError, psErrFileIsEncrypted);
		mErrInfo.ShowMsgBox();
		return false;

	} else if (memcmp(hsign, BOM_CODE, 3) == 0) {
		// UTF-8 text with BOM?
		in_data.SeekStartPos(3);
		st = CheckAsciiDataFormat(in_data);
		in_data.SetTypeFlag(psUTF8BOM, true);

	} else if (hsign[0] != 0x0a && hsign[0] != 0x0d && hsign[0] != 0x09 && hsign[0] < 0x20) {
		// ummm machine code???
		in_data.SetTypeFlag(psAscii, false);
		// error
		mErrInfo.SetInfo(__LINE__, psError, psErrBasic);
		mErrInfo.ShowMsgBox();
		return false;

	} else {
		// propably text
		in_data.SeekStartPos(0);
		st = CheckAsciiDataFormat(in_data);
	}

	in_file_info.SetData(in_data);

	return st;
}

/// 中間言語を解析する
/// @param[in]  in_file  入力ファイル
/// @param[in]  out_type 出力属性
/// @param[in]  phase    フェーズ
/// @param[out] sentence 出力データ
/// @param[in,out] result 結果格納用
/// @return フェーズ
int ParseMSXBasic::ReadBinaryToSymbolsOneLine(PsFileInput &in_file, PsFileType &out_type, int phase, PsSymbolSentence &sentence, ParseResult *result)
{
	bool extend_basic = out_type.GetTypeFlag(psExtendBasic);	// DISK BASICか
	mMachineType = out_type.GetMachineType();
	wxString basic_type = out_type.GetBasicType();

	// body
	long next_addr;				// next address
	int exists;
	wxUint8 vals[10];
	long vall;
	wxUint32 area = 0;
	int linenumber_area = 0;
	int charnumber_area = 0;
	int contstate_area = 0;
	int vlen = 0;
//	int llen = 0;
	wxString chrstr;
	CodeMapItem *item;
	int error_count = 0;	// エラー発生数
	PsSymbol word;

	while(!in_file.Eof() && phase >= PHASE_NONE) {
		memset(vals, 0, sizeof(vals));
		if (error_count > 20) {
			// エラーが多いので解析中止
			phase = PHASE_STOPPED;
			break;
		}
		switch(phase) {
		case PHASE_LINE_NUMBER:
			// get next address
			in_file.Read(vals, 4);
			next_addr = BytesToLong(vals, 2);
			if (next_addr == 0) {
				// end
				phase = PHASE_END;
				break;
			}

			word.Empty();
			mPos.mRow++;
			mPos.mCol = 0;
			area &= ~STATEMENT_AREA;

			// get line number
			exists = mPos.SetLineNumber(BytesToLong(&vals[2], 2));
			if (exists >= 0) {
				// 同じ行番号がある
				if (result) result->Add(mPos, prErrDuplicateLineNumber, exists + 1);
			}

			word.Set(wxString::Format(_T("%ld "), mPos.GetLineNumber()),
				BinString(vals, 4));
			word.SetType(PsSymbol::HOME_LINE_NUMBER);

			// next phase
			sentence.Add(word);
			word.Empty();
			phase = PHASE_BODY;
			break;

		case PHASE_BODY:
			// get 10 chars

			vlen = (int)in_file.Read(vals, 10);

			if (vlen == 0) {
				// end of file
				phase = PHASE_END;
				break;
			}

			if (vals[0] == 0) {
				// end of line
				phase = PHASE_EOL;

				sentence.Add(word);

				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;
				break;
			}

			if (vals[0] == 0x22) {
				// double quote
				if (!(area & QUOTED_AREA)) {
					if (!(area & COMMENT_AREA)) {
						sentence.Add(word);
						word.Empty();
					}
				}
				word.Append(vals[0], vals[0]);
				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;
				if (area & QUOTED_AREA) {
					area &= ~QUOTED_AREA;
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				} else {
					area |= QUOTED_AREA;
					word.SetType(area);
				}
			} else if (area & (QUOTED_AREA | COMMENT_AREA)) {
				// quoted string or REM line
				word.Append(vals[0], vals[0]);
				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;

			} else if (area & ALLDATA_AREA) {
				// all DATA line
				word.Append(vals[0], vals[0]);
				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;

			} else if (area & DATA_AREA) {
				// DATA line
				if (vals[0] == 0x3a) {
					area &= ~DATA_AREA;	// end of data area
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				}
				word.Append(vals[0], vals[0]);
				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;

			} else if (vals[0] == CODE_MEMORYADDR) {
				// goto memory address
				// TODO: must be calc address 
				sentence.Add(word);
				word.Empty();
				vall = BytesToLong(&vals[1], 2);
				word.SetBitType(PsSymbol::LINE_NUMBER);
				word.Append(wxString::Format(_T("%ld"),vall),
					BinString(vals, 3));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(3-vlen, wxFromCurrent);
				mPos.mCol+=3;

			} else if (vals[0] == CODE_LINENUMBER) {
				// goto line number
				sentence.Add(word);
				word.Empty();
				vall = BytesToLong(&vals[1], 2);
				word.SetBitType(PsSymbol::LINE_NUMBER);
				word.Append(wxString::Format(_T("%ld"),vall),
					BinString(vals, 3));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(3-vlen, wxFromCurrent);
				mPos.mCol+=3;

			} else if (vals[0] == CODE_NUMBER8BIT) {
				// number (10 - 255)
				sentence.Add(word);
				word.Empty();
				word.Append(wxString::Format(_T("%d"),vals[1]),
					BinString(vals, 2));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(2-vlen, wxFromCurrent);
				mPos.mCol+=2;

			} else if (CODE_NUMBER0 <= vals[0] && vals[0] <= CODE_NUMBER9) {
				// number (0 - 9)
				sentence.Add(word);
				word.Empty();
				word.Append(wxString::Format(_T("%d"),vals[0]-CODE_NUMBER0),
					BinString(vals, 1));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(1-vlen, wxFromCurrent);
				mPos.mCol++;

			} else if (vals[0] == CODE_NUMBER16BIT) {
				// number (256 - 32767)
				sentence.Add(word);
				word.Empty();
				vall = BytesToLong(&vals[1], 2);
				word.Append(wxString::Format(_T("%ld"),vall),
					BinString(vals, 3));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(3-vlen, wxFromCurrent);
				mPos.mCol+=3;

			} else if (vals[0] == CODE_FLOAT) {
				// float (4bytes)
				sentence.Add(word);
				word.Empty();
				FloatBytesToStr(&vals[1], 4, chrstr);
				word.Append(chrstr,
					BinString(vals, 5));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(5-vlen, wxFromCurrent);
				mPos.mCol+=5;

			} else if (vals[0] == CODE_DOUBLE) {
				// double (8bytes)
				sentence.Add(word);
				word.Empty();
				FloatBytesToStr(&vals[1], 8, chrstr);
				word.Append(chrstr,
					BinString(vals, 9));
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
				in_file.Seek(9-vlen, wxFromCurrent);
				mPos.mCol+=9;

			} else {
				// find command statement
				mBasicCodeTbl.FindSectionByType(mMachineType);
				item = mBasicCodeTbl.FindByCode(vals);
				if (item == NULL && extend_basic) {
					mBasicCodeTbl.FindSection(basic_type);
					item = mBasicCodeTbl.FindByCode(vals);
				}
				if (item != NULL) {
					// found the BASIC sentence
					if ((item->GetAttr() | item->GetAttr2()) & CodeMapItem::ATTR_INNERSENTENCE) {
						if (!(area & STATEMENT_AREA)) {
							// is not statement
							item = NULL;
						}
					}
				}
				if (item != NULL) {
					// process the BASIC sentence
					if (area & VARIABLE_AREA) {
						// end of variable area
						sentence.Add(word);
						word.Empty();
						area &= ~VARIABLE_AREA;
						word.SetType(area);
					}

					wxUint32 attr = item->GetAttr() | item->GetAttr2();
//					if (attr & CodeMapItem::ATTR_COLON) { // ELSE statement
//						llen = (int)body.Len() - 1;
//						if (llen >= 0 && body.GetChar(llen) == ':') {
//							body = body.Left(llen); // trim last char ':'
//						}
//					}
					if (attr & CodeMapItem::ATTR_ALLDATA) { // all DATA statement
						if (!(area & ALLDATA_AREA)) {
							area |= ALLDATA_AREA;
							sentence.Add(word);
							word.Empty();
							word.SetType(area);
						}
					} else if (attr & CodeMapItem::ATTR_DATA) { // DATA statement
						if (!(area & DATA_AREA)) {
							area |= DATA_AREA;
							sentence.Add(word);
							word.Empty();
							word.SetType(area);
						}
					} else if (attr & CodeMapItem::ATTR_COMMENT) { // ' REM statement
						if (!(area & COMMENT_AREA)) {
							area |= COMMENT_AREA;
							sentence.Add(word);
							word.Empty();
							word.SetType(area);
						}
					}

					if (linenumber_area && (attr & CodeMapItem::ATTR_CONTLINENUMBER) != 0) {
						// 行番号指定は続く
						linenumber_area = 1;
					} else if (linenumber_area && (attr & CodeMapItem::ATTR_CONTONELINENUMBER) != 0) {
						// 最初の数値のみ行番号
						linenumber_area = 2;
					} else if ((attr & (CodeMapItem::ATTR_CONTONELINENUMBER | CodeMapItem::ATTR_ONELINENUMBER)) == CodeMapItem::ATTR_ONELINENUMBER) {
						// 最初の数値のみ行番号
						linenumber_area = 2;
					} else if ((attr & (CodeMapItem::ATTR_CONTLINENUMBER | CodeMapItem::ATTR_ONELINENUMBER | CodeMapItem::ATTR_LINENUMBER)) == CodeMapItem::ATTR_LINENUMBER) {
						// 行番号はその命令内の数値全て
						linenumber_area = 1;
					} else {
						linenumber_area = 0;
					}

					// 数値を文字列として出力するか
					charnumber_area = ((attr & CodeMapItem::ATTR_CHARNUMBER) != 0) ? 1 : 0;
					// 続く文字列はステートメントとするか(CALL文)
					contstate_area =  ((attr & CodeMapItem::ATTR_CONTSTATEMENT) != 0) ? 1 : 0;

					if ((area & (ALLDATA_AREA | DATA_AREA | COMMENT_AREA)) == 0) {
						if (!(area & SENTENCE_AREA)) {
							area |= SENTENCE_AREA;
							sentence.Add(word);
							word.Empty();
							word.SetType(area);
						}
						if (attr & CodeMapItem::ATTR_NOSTATEMENT) {
							area &= ~STATEMENT_AREA;
						} else {
							area |= STATEMENT_AREA;
						}
					}
					if (attr & (CodeMapItem::ATTR_OCTSTRING | CodeMapItem::ATTR_HEXSTRING)) {
						// 8進数, 16進数
						sentence.Add(word);
						word.Empty();
						area |= SENTENCE_AREA;
						word.SetType(area);
					}

					word.Append(item->GetStr(),
						BinString(vals, item->GetCodeLength()));
					in_file.Seek((int)(item->GetCodeLength())-vlen, wxFromCurrent);
					mPos.mCol+=item->GetCodeLength();

					if (attr & CodeMapItem::ATTR_OCTSTRING) {
						// octet
						vall = BytesToLong(&vals[1], 2);
						word.Append(wxString::Format(_T("%lo"),vall),
							BinString(vals, 3));
						in_file.Seek(2, wxFromCurrent);
						word.SetBitType(PsSymbol::OCTSTRING);
						mPos.mCol+=2;
					} else if (attr & CodeMapItem::ATTR_HEXSTRING) {
						// hex
						vall = BytesToLong(&vals[1], 2);
						word.Append(wxString::Format(_T("%lX"),vall),
							BinString(vals, 3));
						in_file.Seek(2, wxFromCurrent);
						word.SetBitType(PsSymbol::HEXSTRING);
						mPos.mCol+=2;
					}

				} else {
					chrstr = _T("");
					if (area & SENTENCE_AREA) {
						area &= ~SENTENCE_AREA;
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}

					if (vals[0] == 0x3a) {
						// colon
						area &= ~STATEMENT_AREA;
						linenumber_area = 0;
						charnumber_area = 0;
						word.Append(vals[0], vals[0]);
						in_file.Seek(1-vlen, wxFromCurrent);
						mPos.mCol++;

					} else if (vals[0] == 0x20) {
						// space
						word.Append(vals[0], vals[0]);
						in_file.Seek(1-vlen, wxFromCurrent);
						mPos.mCol++;

					} else if (vals[0] < 0x20 || 0x80 <= vals[0]) {
						// Unknown command (error?)
						error_count++;
						// for parse result
						if (result) {
							result->Add(mPos, prErrInvalidBasicCode);
						}
						word.Append(vals[0], vals[0]);
						in_file.Seek(1-vlen, wxFromCurrent);
						mPos.mCol++;

					} else if (area & VARIABLE_AREA) {
						// now variable
						if ((0x30 <= vals[0] && vals[0] <= 0x39) || (0x41 <= vals[0] && vals[0] <= 0x5a)) {
							// variable name
							word.Append(vals[0], vals[0]);
							in_file.Seek(1-vlen, wxFromCurrent);
							mPos.mCol++;
						} else if (vals[0] == '%' || vals[0] == '#' || vals[0] == '!' || vals[0] == '$') {
							// end of variable name
							word.Append(vals[0], vals[0]);
							in_file.Seek(1-vlen, wxFromCurrent);
							mPos.mCol++;

							if (contstate_area) {
								word.SetType(SENTENCE_AREA);
								contstate_area = 0;
							}
							sentence.Add(word);
							word.Empty();

							area &= ~VARIABLE_AREA;
							word.SetType(area);
						} else {
							// non variable area
							if (contstate_area) {
								word.SetType(SENTENCE_AREA);
								contstate_area = 0;
							}
							sentence.Add(word);
							word.Empty();

							area &= ~VARIABLE_AREA;
							word.SetType(area);

							word.Append(vals[0], vals[0]);
							in_file.Seek(1-vlen, wxFromCurrent);
							mPos.mCol++;
						}

					} else if (0x30 <= vals[0] && vals[0] <= 0x39) {
						// line number or constant value
						int n = 0;
						for(; 0x30 <= vals[n] && vals[n] <= 0x39 && n < 10; n++) {}
						word.Append(vals, n, vals, n);
						if (linenumber_area) {
							word.SetBitType(PsSymbol::LINE_NUMBER);
							if (linenumber_area == 2) {
								linenumber_area = 0;
							}
						}
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
						in_file.Seek(n-vlen, wxFromCurrent);
						mPos.mCol+=n;

					} else if (0x41 <= vals[0] && vals[0] <= 0x5a) { 
						// variable name first
						sentence.Add(word);
						word.Empty();

						area |= VARIABLE_AREA;
						word.SetType(area);
						word.Append(vals[0], vals[0]);
						in_file.Seek(1-vlen, wxFromCurrent);
						mPos.mCol++;

					} else {
						word.Append(vals[0], vals[0]);
						in_file.Seek(1-vlen, wxFromCurrent);
						mPos.mCol++;

					}
				}
			}
			break;
		}
	}
	return phase;
}

/// アスキー形式/UTF-8テキストから中間言語に変換して出力
/// @param[in]  in_data  入力データ
/// @param[out] out_data 変換後データ
/// @param[in,out] result 結果格納用
/// @return true/false
bool ParseMSXBasic::ParseAsciiToBinary(PsFileData &in_data, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Ascii->Binary"));

	mNextAddress = iStartAddr[pConfig->GetStartAddr()];

	// header
	out_data.Add("\xff", 1);
	mNextAddress += 1;

	// body
	mPrevLineNumber = -1;
	for(mPos.mRow = 0; mPos.mRow < in_data.GetCount(); mPos.mRow++) {
		if (!ParseAsciiToBinaryOneLine(in_data.GetType(), in_data[mPos.mRow], out_data, result)) {
			break;
		}
		if (result && result->GetCount() > ERROR_STOPPED_COUNT) {
			// エラーが多いので中止
			result->Add(mPos, prErrStopInvalidBasicCode);
			break;
		}
	}

	// footer
	out_data.Add("\x00\x00", 2);

	// ファイル終端コードを出力
	if ((!out_data.GetTypeFlag(psTapeImage) && pConfig->GetEofBinary())) {
		out_data.Add(EOF_CODE, 1);
	}

	return true;

}

/// アスキー形式を解析して色付けする
/// @param[in]  in_data  入力データ
/// @param[out] out_data 変換後データ
/// @param[in,out] result 結果格納用
/// @return true/false
bool ParseMSXBasic::ParseAsciiToColored(PsFileData &in_data, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Parse Ascii"));

	// body
	mPrevLineNumber = -1;
	for(mPos.mRow = 0; mPos.mRow < in_data.GetCount(); mPos.mRow++) {
		if (!ParseAsciiToColoredOneLine(in_data.GetType(), in_data[mPos.mRow], out_data, result)) {
			break;
		}
	}

	return true;
}

/// 変数文字列を変数文字に変換
/// @param[in]     in_data   入力文字
/// @param[in]     re        正規表現
/// @param[out]    body      出力文字
/// @param[in,out] result    結果格納用
void ParseMSXBasic::ParseVariableString(const wxString &in_data, wxRegEx &re, PsSymbol &body, ParseResult *result)
{
	size_t re_start, re_len;
	wxString chrstr;
	if (re.Matches(in_data)) {
		// 変数名
		re.GetMatch(&re_start,&re_len);
		chrstr = in_data.Mid(re_start, re_len + 1);
		if (re_len > 16) {
			// 変数名は16文字までが有効（エラーにしない）
		}
		// 末尾
		if (!reVariEnd.Matches(chrstr.Right(1))) {
			chrstr = chrstr.Left(re_len);
		} else {
			re_len++;
		}
		body.Append(chrstr, chrstr);
		mPos.mCol += re_len;

	} else {
		// 変数名がおかしい（ここにはこない）
		mPos.mCol++;
	}
}

/// 数値文字列を数値に変換
/// @param[in]     in_data   入力文字
/// @param[in]     re_real   整数表記の正規表現
/// @param[in]     re_exp    指数表記の正規表現
/// @param[out]    body      出力文字
/// @param[in,out] result    結果格納用
void ParseMSXBasic::ParseNumberString(const wxString &in_data, wxRegEx &re_real, wxRegEx &re_exp, PsSymbol &body, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
	int len = 0;
	int err = 0;
	wxString numstr;
	BinString numstrb;
	bool match = false;

	if (re_exp.Matches(in_data)) {
		// 指数表記
		match = true;
		re_exp.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);
	} else if (re_real.Matches(in_data)) {
		// 整数 or 実数表記
		match = true;
		re_real.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);
	}

	if (match) {
		len = NumStrToBinStr(numstr, numstrb, &err);

		if (err != 0) {
			// overflow
			if (result) result->Add(mPos, prErrOverflow);
		}
		if (len == 0) {
			// cannot convert
			numstrb = numstr;
			if (result) result->Add(mPos, prErrInvalidNumber);
		}

		body.Append(numstr, numstrb);
		mPos.mCol += re_len;

	} else {
		if (result) result->Add(mPos, prErrInvalidNumber);
		mPos.mCol++;
	}
}

/// 行番号文字列を数値に変換
/// @param[in]     in_data   入力文字
/// @param[in]     re        正規表現
/// @param[out]    body      出力文字
/// @param[in,out] result    結果格納用
void ParseMSXBasic::ParseLineNumberString(const wxString &in_data, wxRegEx &re_int, PsSymbol &body, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
	int len = 0;
	int err = 0;
	wxString numstr;
	BinString numstrb;

	if (re_int.Matches(in_data)) {
		// 整数
		re_int.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		len = LineNumStrToBinStr(numstr, numstrb, mNextAddress, &err);

		if (err != 0) {
			// overflow
			if (result) result->Add(mPos, prErrOverflow);
		}
		if (len == 0) {
			// cannot convert
			numstrb = numstr;
			if (result) result->Add(mPos, prErrInvalidNumber);
		}

		body.Append(numstr, numstrb);

		mPos.mCol += re_len;
	} else {
		if (result) result->Add(mPos, prErrInvalidNumber);
		mPos.mCol++;
	}
}

/// 8進or16進文字列を文字に変換
/// @param[in]     in_data   入力文字
/// @param[in]     octhexhed 文字列のヘッダ "&H"or"&O"
/// @param[in]     octhexcode 8進or16進コード 0x0b/0x0c
/// @param[in]     base      8 or 16
/// @param[in]     re        正規表現
/// @param[out]    body      出力文字データ
/// @param[in,out] result    結果格納用
void ParseMSXBasic::ParseOctHexString(const wxString &in_data, const wxString &octhexhed, const wxString &octhexcode, int base, wxRegEx &re, PsSymbol &body, ParseResult *result)
{
	size_t re_start, re_len;
	BinString numstr = in_data.Mid(octhexhed.Length());
	BinString numstrb;

	if (re.Matches(numstr)) {
		// 数値OK
		re.GetMatch(&re_start,&re_len);
		numstr = numstr.Mid(re_start, re_len);
		long val = 0;
		int rc = CheckOctHexStr(base, numstr, &val);
		if (!rc) {
			// overflow
			if (result) result->Add(mPos, prErrOverflow);
		}

		LongToBinStr(val, numstrb, 2);

		body.Append(numstr, numstrb);
		mPos.mCol += re_len;
	} else {
		// 数値NG
		if (result) result->Add(mPos, prErrInvalidNumber);
		mPos.mCol++;
	}
}

/// アスキー形式1行を解析する
/// @param[in]  in_file_type 入力ファイル形式
/// @param[in]  in_data      入力データ
/// @param[in]  out_type     出力データ形式
/// @param[out] sentence     出力データ
/// @param[in,out] result    結果格納用
/// @return 解析終了する場合false
bool ParseMSXBasic::ParseAsciiToSymbolsOneLine(PsFileType &in_file_type, wxString &in_data, PsFileType &out_type, PsSymbolSentence &sentence, ParseResult *result)
{
	if (in_data.IsEmpty()) {
		return false;
	}

	bool extend_basic = out_type.GetTypeFlag(psExtendBasic);	// DISK BASICか
	mMachineType = out_type.GetMachineType();
	wxString basic_type = out_type.GetBasicType();

	wxUint32 area = 0;
	int linenumber_area = 0;
	int charnumber_area = 0;
	int contstate_area = 0;

	CodeMapItem *item;

	PsSymbol word;

	BinString in_str;
	wxUint8 in_chr;
	mPos.mCol = 0;

	// line number
	int exists = mPos.SetLineNumber(GetLineNumber(in_data, &mPos.mCol));
	if (exists >= 0){
		// 同じ行番号がある
		if (result) result->Add(mPos, prErrDuplicateLineNumber, exists + 1);
	}
	if (mPos.GetLineNumber() < mPrevLineNumber) {
		// 行番号が前行より小さい
		if (result) result->Add(mPos, prErrDiscontLineNumber);
	}
	mPrevLineNumber = mPos.GetLineNumber();

	word.AppendAscStr(in_data.Left(mPos.mCol));
	word.AppendBinStr(HomeLineNumToBinStr(mPos.GetLineNumber(), mNextAddress));

	word.SetType(PsSymbol::HOME_LINE_NUMBER);
	sentence.Add(word);
	word.Empty();

	// body
	while(mPos.mCol < in_data.Len()) {
		in_str = in_data.Mid(mPos.mCol);
		in_chr = in_str.At(0);

		if (in_chr == EOF_CODEN) {
			// end of text
			break;
		}

		if (in_chr == 0x22) {
			// quote
			if (!(area & QUOTED_AREA)) {
				if (!(area & COMMENT_AREA)) {
					sentence.Add(word);
					word.Empty();
				}
			}
			word.Append(in_chr, in_chr);
			mPos.mCol++;
			if (area & QUOTED_AREA) {
				area &= ~QUOTED_AREA;
				sentence.Add(word);
				word.Empty();
				word.SetType(area);
			} else {
				area |= QUOTED_AREA;
				word.SetType(area);
			}
			continue;
		}

		// find command
		if (!(area & (COMMENT_AREA | DATA_AREA | ALLDATA_AREA | QUOTED_AREA))) {
			// search command
			mBasicCodeTbl.FindSectionByType(mMachineType);
			item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			if (item == NULL && extend_basic) {
				mBasicCodeTbl.FindSection(basic_type);
				item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			}
			if (item != NULL) {
				// found the BASIC sentence
				if ((item->GetAttr() | item->GetAttr2()) & CodeMapItem::ATTR_INNERSENTENCE) {
					if (!(area & STATEMENT_AREA)) {
						// is not statement
						item = NULL;
					}
				}
			}
			if (item != NULL) {
				// process the BASIC sentence
				wxUint32 attr = item->GetAttr() | item->GetAttr2();
				if (attr & CodeMapItem::ATTR_COMMENT) {
					// comment area
					if (!(area & COMMENT_AREA)) {
						area |= COMMENT_AREA;

						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}
				}
				if (attr & CodeMapItem::ATTR_DATA) {
					// :または行末までDATA
					if (!(area & DATA_AREA)) {
						area |= DATA_AREA;
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}
				} else if (attr & CodeMapItem::ATTR_ALLDATA) {
					// 行末までDATA
					if (!(area & ALLDATA_AREA)) {
						area |= ALLDATA_AREA;
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}
				}
				if (area & VARIABLE_AREA) {
					// 変数終わり
					area &= ~VARIABLE_AREA;
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				}
				if (!(area & (COMMENT_AREA | DATA_AREA | ALLDATA_AREA))) {
					if (!(area & SENTENCE_AREA)) {
						area |= SENTENCE_AREA;
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}
					if (attr & CodeMapItem::ATTR_NOSTATEMENT) {
						area &= ~STATEMENT_AREA;
					} else {
						area |= STATEMENT_AREA;
					}
				}

				if (attr & (CodeMapItem::ATTR_OCTSTRING | CodeMapItem::ATTR_HEXSTRING)) {
					// 8進数, 16進数
					sentence.Add(word);
					word.Empty();
					area |= SENTENCE_AREA;
					word.SetType(area);
				}

				word.Append(in_str.Left(item->GetStr().Len()),
					BinString(item->GetCode(), item->GetCodeLength()));

				if (linenumber_area && (attr & CodeMapItem::ATTR_CONTLINENUMBER) != 0) {
					// 行番号指定は続く
					linenumber_area = 1;
				} else if (linenumber_area && (attr & CodeMapItem::ATTR_CONTONELINENUMBER) != 0) {
					// 最初の数値のみ行番号
					linenumber_area = 2;
				} else if ((attr & (CodeMapItem::ATTR_CONTONELINENUMBER | CodeMapItem::ATTR_ONELINENUMBER)) == CodeMapItem::ATTR_ONELINENUMBER) {
					// 最初の数値のみ行番号
					linenumber_area = 2;
				} else if ((attr & (CodeMapItem::ATTR_CONTLINENUMBER | CodeMapItem::ATTR_ONELINENUMBER | CodeMapItem::ATTR_LINENUMBER)) == CodeMapItem::ATTR_LINENUMBER) {
					// 行番号はその命令内の数値全て
					linenumber_area = 1;
				} else {
					linenumber_area = 0;
				}

				// 数値を文字列として出力するか
				charnumber_area = ((attr & CodeMapItem::ATTR_CHARNUMBER) != 0) ? 1 : 0;
				// 続く文字列はステートメントとするか(CALL文)
				contstate_area =  ((attr & CodeMapItem::ATTR_CONTSTATEMENT) != 0) ? 1 : 0;

				if (attr & CodeMapItem::ATTR_OCTSTRING) {
					// 8進数
					ParseOctHexString(in_str, item->GetStr(), item->GetCode(), 8, reOcta, word, result);

					word.SetBitType(PsSymbol::OCTSTRING);
					sentence.Add(word);

					area &= ~SENTENCE_AREA;
					word.Empty();
					word.SetType(area);
				}
				if (attr & CodeMapItem::ATTR_HEXSTRING) {
					// 16進数
					ParseOctHexString(in_str, item->GetStr(), item->GetCode(), 16, reHexa, word, result);

					word.SetBitType(PsSymbol::HEXSTRING);
					sentence.Add(word);

					area &= ~SENTENCE_AREA;
					word.Empty();
					word.SetType(area);
				}

				mPos.mCol += item->GetStr().Len();

			} else {
				// non BASIC sentence
				if (area & SENTENCE_AREA) {
					area &= ~SENTENCE_AREA;
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				}

				if (in_chr == 0x3a) {
					// colon
					area &= ~STATEMENT_AREA;

					linenumber_area = 0;
					charnumber_area = 0;
					sentence.Add(word);
					word.Empty();
					word.SetType(area);

					word.Append(in_str[0], in_str[0]);
					mPos.mCol++;

				} else if (in_chr == 0x20) {
					// 空白
					word.Append(in_chr, in_chr);
					mPos.mCol++;

				} else if (in_chr >= 0x80) {
					// unknown char ?(error)
					if (result) result->Add(mPos, prErrInvalidChar);
					word.Append(in_chr, in_chr);
					mPos.mCol++;

				} else if (in_chr == 0x2e) {
					// ピリオドの場合 小数点か？
					if (linenumber_area) {
						// 行番号は指定できないはず
						if (result) result->Add(mPos, prErrInvalidLineNumber);
						mPos.mCol++;
					} else {
						// 数値を変換
						sentence.Add(word);
						word.Empty();
						ParseNumberString(in_str, reNumberReald, reNumberExpd, word, result);
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}

				} else if (reAlpha.Matches(in_str.Left(1))) {
					// 変数の場合
					linenumber_area = 0;
					sentence.Add(word);
					word.Empty();
					ParseVariableString(in_str, reAlphaNumeric, word, result);
					if (contstate_area) {
						word.SetType(SENTENCE_AREA);
						contstate_area = 0;
					} else {
						word.SetBitType(VARIABLE_AREA);
					}
					sentence.Add(word);
					word.Empty();
					word.SetType(area);

				} else if (reNumber.Matches(in_str.Left(1))) {
					// 数値の場合
					if (linenumber_area) {
						// 行番号の場合
						sentence.Add(word);
						word.Empty();
						ParseLineNumberString(in_str, reNumber, word, result);
						word.SetBitType(PsSymbol::LINE_NUMBER);
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					} else {
						// 数値を変換
						sentence.Add(word);
						word.Empty();
						ParseNumberString(in_str, reNumberReal, reNumberExp, word, result);
						if (charnumber_area) {
							word.SetBitType(PsSymbol::CHAR_NUMBER);
						}
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
					}
					if (linenumber_area == 2) {
						// 最初の数値のみ行番号
						linenumber_area = 0;
					}

				} else {
					word.Append(in_chr, in_chr);
					mPos.mCol++;

				}
			}
		} else {
			// COMMENT_AREA | DATA_AREA | ALLDATA_AREA | QUOTED_AREA
			if (in_chr == 0x3a) {
				// colon
				if (area & DATA_AREA) {
					area &= ~DATA_AREA;
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				}
			}

			word.Append(in_chr, in_chr);
			mPos.mCol++;
		}
	}

	sentence.Add(word);

	return true;
}

/// テキストを出力
/// @param[in]  in_data      入力データ
/// @param[out] out_file     出力ファイル
/// @return true/false
bool ParseMSXBasic::WriteText(PsFileData &in_data, PsFileOutput &out_file)
{
	// UTF-8に変換するか
	bool utf8_mode = out_file.GetTypeFlag(psUTF8);
	wxArrayString *inlines = &in_data.GetData();

	if (!out_file.IsOpened()) {
		return true;
	}

	if (!utf8_mode) {
		// アスキー そのまま出力
		for(size_t row = 0; row < inlines->GetCount(); row++) {
			out_file.Write(inlines->Item(row));	// 変換しない
			out_file.Write(cNLChr[pConfig->GetNewLineAscii()]); // 改行
		}

	} else {
		// UTF-8
		if (out_file.GetTypeFlag(psUTF8BOM)) {
			out_file.Write((const wxUint8 *)BOM_CODE, 3); // BOM
		}

		for(size_t row = 0; row < inlines->GetCount(); row++) {
			out_file.WriteUTF8(inlines->Item(row));	// UTF-8に変換して出力
			out_file.Write(cNLChr[pConfig->GetNewLineUtf8()]);	// 改行
		}
	}
	return true;
}

/// バイナリを出力
/// @param[in]  in_data      入力データ
/// @param[out] out_file     出力ファイル
/// @return true/false
bool ParseMSXBasic::WriteBinary(PsFileData &in_data, PsFileOutput &out_file)
{
	wxArrayString *inlines = &in_data.GetData();

	if (!out_file.IsOpened()) {
		return true;
	}

	// body
	for(size_t row = 0; row < inlines->GetCount(); row++) {
		out_file.Write(inlines->Item(row));
	}

	return true;
}

/// スタートアドレスのリストへのポインタを返す
const int *ParseMSXBasic::GetStartAddrsPtr() const
{
	return iStartAddr;
}

/// スタートアドレスのリスト数を返す
int ParseMSXBasic::GetStartAddrCount() const
{
	return eMSXStartAddrCount;
}

/// 浮動小数点バイト列を文字列に変換
/// @param[in]  src     浮動小数点バイト列
/// @param[in]  src_len srcの長さ
/// @param[out] dst     文字列
/// @return     true
bool ParseMSXBasic::FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst)
{
	// 指数部が0の時は"0"
	if ((src[0] & 0x7f) == 0) {
		dst = _T("0");
		return true;
	}

	// 指数表記にするか
	bool expview = true;
	// 指数
	int ex = (int)(src[0] & 0x7f) - 64;
	if (-1 <= ex && ex <= 13) {
		expview = false;
	} else {
		ex--;
	}

	dst = _T("");
	// 符号
	if (src[0] & 0x80) {
		dst += _T("-");
	}
	
	// 仮数部の0をトリミング
	int len = (int)src_len * 2 - 1;
	for(;len >= 2; len--) {
		if (src[len >> 1] & (0xf0 >> ((len & 1) << 2))) break; 
	}

	if (expview) {
		// 指数表記
		// 仮数部(BCD)
		for(int i=2; i <= len; i++) {
			if (i == 3) dst += _T(".");
			dst += wxString::Format(_T("%d"), (src[i >> 1] & (0xf0 >> ((i & 1) << 2))) >> ((1 - (i & 1)) << 2));
		}
		dst += _T("E");
		dst += wxString::Format(_T("%+03d"), ex);
	} else {
		// 実数表記
		// 仮数部(BCD)
		int imin = ex < 0 ? ex + 2 : 2;
		int imax = ex >= len ? ex + 1 : len;
		for(int i=imin; i <= imax; i++) {
			if (i == (ex + 2)) dst += _T(".");
			if (2 <= i && i <= len) {
				dst += wxString::Format(_T("%d"), (src[i >> 1] & (0xf0 >> ((i & 1) << 2))) >> ((1 - (i & 1)) << 2));
			} else {
				dst += _T("0");
			}
		}
		if (src_len == 8) {
			dst += _T("#");	// 倍精度
		} else {
			dst += _T("!");	// 単精度
		}
	}
	return true;
}

/// 数値を整数バイト文字列に変換
/// @param[in]  src     数値
/// @param[out] dst     バイト列(追記)
/// @param[in]  len     桁数
/// @return             true
bool ParseMSXBasic::LongToBinStr(long src, BinString &dst, int len)
{
	wxUint8 buf[4];
	bool rc = LongToBytes(src, buf, len);
	dst.Append(buf, len);
	return rc;
}

/// 数値を整数バイト文字列に変換
/// @param[in]  src     数値
/// @param[in]  len     桁数
/// @return             整数バイト列
BinString ParseMSXBasic::LongToBinStr(long src, int len)
{
	BinString dst;
	LongToBinStr(src, dst, len);
	return dst;
}

/// 数値文字列をバイト列に変換
/// @param[in]  src     数値文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ
int ParseMSXBasic::NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err)
{
	bool is_integer = false;
	wxRegEx reExp(_T("[E.!#]"));
	wxString str = src.Upper();
	wxUint8 buf[10];
	int len = 0;

	long num = 0;

	if (!reExp.Matches(str)) {
		// 整数?
		if (str.Len() <= 6) {
			// 整数?
			str.ToLong(&num);
			if (-32768 <= num && num < 32768) {
				// 2バイト整数
				is_integer = true;
			}
		}
	}
	memset(buf, 0, sizeof(buf));
	if(is_integer) {
		// 整数
		if (num < 0) {
			buf[len] = '-';
			len++;
		}
		num = abs(num);
		if (num < 10) {
			// 0 - 9
			buf[len] = (num + CODE_NUMBER0) & 0xff;
			len++;
		} else if (num < 256) {
			// 10 - 255
			buf[len] = CODE_NUMBER8BIT;
			len++;
			LongToBytes(num, &buf[len], 1);
			len++;
		} else {
			// 256 - 32767
			buf[len] = CODE_NUMBER16BIT;
			len++;
			LongToBytes(num, &buf[len], 2);
			len+=2;
		}
	} else {
		// 実数
		if (str.Right(1) == _T("!") || str.Right(1) == _T("#")) {
			str = str.Left(str.Len()-1);
		}

		int epos = str.Find(_T("E"));
		wxString vtval;
		long exval;
		if (epos != wxNOT_FOUND) {
			vtval = str.Left(epos);
			str.Mid(epos+1).ToLong(&exval);
		} else {
			vtval = str;
			exval = 0;
		}
		// shift point
		int ppos = vtval.Find(_T("."));
		if (ppos == wxNOT_FOUND) {
			exval += str.Len();
		} else {
			exval += ppos;
			if (ppos > 0) vtval = vtval.Left(ppos) + vtval.Mid(ppos+1);
			else vtval = vtval.Mid(ppos+1);
		}
		// trim left zero
		while(vtval.Len() > 0) {
			if (vtval.Left(1) != _T("0")) break;
			exval--;
			vtval = vtval.Mid(1);
			if (exval <= -63) break;
		}
		// trim right zero
		while(vtval.Len() > 0) {
			if (vtval.Right(1) != _T("0")) break;
			vtval = vtval.Left(vtval.Len()-1);
		}

		// "0"の場合
		if (vtval == _T("0")) {
			// 整数の0で表記
			buf[len] = CODE_NUMBER0;
			len++;
			goto FIN;
		}

		size_t cols = 0;
		if (vtval.Len() <= 6) {
			// 単精度
			buf[len] = CODE_FLOAT;
			len++;
			cols = 6;
		} else if (vtval.Len() <= 14) {
			// 倍精度
			buf[len] = CODE_DOUBLE;
			len++;
			cols = 14;
		} else {
			// オーバーフロー
			return 0;
		}
		// 指数部
		buf[len] = (exval+64) & 0xff;
		len++;
		// 仮数部(BCD)
		for(size_t i=0; i<cols; i+=2) {
			long v1 = 0;
			long v2 = 0;
			if (i < vtval.Len()) vtval.Mid(i,1).ToLong(&v1); 
			if (i+1 < vtval.Len()) vtval.Mid(i+1,1).ToLong(&v2); 
			buf[len] = (v1 * 16 + v2) & 0xff;
			len++;
		}
	}
FIN:
	if ((size_t)len > dst_len) {
		// buffer overflow
		return 0;
	}
	memcpy(dst, buf, len);
	return len;
}

/// 数値文字列をバイト文字列に変換
/// @param[in]  src     数値文字列
/// @param[out] dst     バイト列(追記)
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ
int ParseMSXBasic::NumStrToBinStr(const wxString &src, BinString &dst, int *err)
{
	wxUint8 buf[10];
	int len = NumStrToBytes(src, buf, 10, err);
	dst.Append(buf, len);
	return len;
}

/// 8/16進文字列をバイト列に変換
/// @param[in]  src     8/16進文字列(0x0bまたは0x0cで始まること)
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @return     >0: 書き込んだ長さ / 0: エラー
int ParseMSXBasic::OctHexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len)
{
	int base = 10;
	int len = 0;
	wxString str;

	if (dst_len < 3) {
		return 0;
	}

	if (src[0] == 0x0b) {
		base = 8;
		str = src.Mid(1);
	} else if (src[0] == 0x0c) {
		base = 16;
		str = src.Mid(1);
	} else {
		return 0;
	}

	long num;
	if (!str.ToLong(&num, base)) {
		return 0;
	}
	if (num < 0 || 65535 < num) {
		// overflow
		return 0;
	}

	if (base == 8) {
		dst[len] = 0x0b;
		len++;
	} else {
		dst[len] = 0x0c;
		len++;
	}
	LongToBytes(num, &dst[len], 2);
	len += 2;

	return len;
}

/// 8/16進文字列が範囲内か
/// @param[in]  base    8 or 16
/// @param[in]  src     8/16進文字列
/// @param[in]  val     変換後の値
/// @return     false:オーバーフロー
bool ParseMSXBasic::CheckOctHexStr(int base, const wxString &src, long *val)
{
	long num;
	if (!src.ToLong(&num, base)) {
		return false;
	}
	if (num < 0 || 65535 < num) {
		// overflow
		return false;
	}
	if (val) *val = num;
	return true;
}

/// 行番号文字列をバイト列に変換
/// @param[in]  src            行番号(数値)文字列
/// @param[out] dst            バイト列
/// @param[in]  dst_len        dstの長さ
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ 0:上記以外のエラーで書き込めない
int ParseMSXBasic::LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int memory_address, int *err)
{
	long num;
	int len = 0;

	if (dst_len < 3) {
		return 0;
	}

	if (!src.ToLong(&num)) {
		return 0;
	}
	if (num > 65535) {
		if (err) *err = 1;
		num = 65535;
	}
	if (num < 0) {
		if (err) *err = 2;
		num = 0;
	}
	dst[len] = CODE_LINENUMBER;
	len++;
	LongToBytes(num, &dst[len], 2);
	len += 2;

	return len;
}

/// 行番号文字列をバイト文字列に変換
/// @param[in]  src            行番号(数値)文字列
/// @param[out] dst            バイト文字列
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ 0:上記以外のエラーで書き込めない
int ParseMSXBasic::LineNumStrToBinStr(const wxString &src, BinString &dst, int memory_address, int *err)
{
	wxUint8 buf[4];
	int len = LineNumStrToBytes(src, buf, 4, memory_address, err);
	dst.Append(buf, len);
	return len;
}

/// 行先頭の行番号をバイト文字列に変換
/// @param[in]  line_number    行番号
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     バイト列 4bytes(xxyy) xx:メモリアドレス yy:行番号 
BinString ParseMSXBasic::HomeLineNumToBinStr(long line_number, int memory_address, int *err)
{
	BinString dst;
//	int len = 0;

	if (line_number > 65535) {
		if (err) *err = 1;
		line_number = 65535;
	}
	if (line_number < 0) {
		if (err) *err = 2;
		line_number = 0;
	}

	LongToBinStr(memory_address, dst, 2);
	LongToBinStr(line_number, dst, 2);

	return dst;
}

/// テープイメージの内部ファイル名の最大文字数を返す
int ParseMSXBasic::GetInternalNameSize() const
{
	return 6;
}

/// BASICが拡張BASICかどうか
bool ParseMSXBasic::IsExtendedBasic(const wxString &basic_type)
{
	return false;
}

/// マシンタイプの判別
int ParseMSXBasic::GetMachineType(const wxString &basic_type)
{
	return MACHINE_TYPE_MSX;
}

/// 機種名を返す
wxString ParseMSXBasic::GetMachineName() const
{
	return _("MSX BASIC");
}

/// ファイルオープン時の拡張子リストを返す
const wxChar *ParseMSXBasic::GetOpenFileExtensions() const
{
#if defined(__WXMSW__)
	return _("Supported files|*.cas;*.bin;*.bas;*.txt;*.dat|All files|*.*");
#else
	return _("Supported files|*.cas;*.CAS;*.bin;*.BIN;*.bas;*.BAS;*.txt;*.TXT;*.dat;*.DAT|All files|*.*");
#endif
}
