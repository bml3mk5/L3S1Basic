/// @file parse_l3s1basic.cpp
///
/// @brief L3/S1 BASICパーサー
///
#include "parse_l3s1basic.h"
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/arrimpl.cpp>
#include "main.h"
#include "config.h"
#include "l3float.h"
//#include "l3specs.h"

/// マシンタイプ
#define MACHINE_TYPE_L3 1
#define MACHINE_TYPE_S1 2

#define L3_START_ADDR_MESSAGE _("Start Address (L3 only)")

enum en_basic_code {
	CODE_S1_NUMERIC = 0xfe,
	CODE_LINENUMBER = 0xf2,
	CODE_NUMBER1 = 0x01,
	CODE_NUMBER2 = 0x02,
	CODE_FLOAT = 0x04,
	CODE_DOUBLE = 0x08,
};

#define L3_CHAR_CODE_TABLE  "l3s1_char_code.dat"
#define L3_BASIC_CODE_TABLE "l3s1_basic_code.dat"

//////////////////////////////////////////////////////////////////////

/// スタートアドレス(L3用)
const int ParseL3S1Basic::iStartAddr[eL3StartAddrCount] = {
	0x0768,	// newon11 0x0400 + 0x368
	0x0B68,	// newon15 0x0800 + 0x368
	0x2768,	// newon 3 0x2400 + 0x368
	0x4768 	// newon 7 0x4400 + 0x368
};

//////////////////////////////////////////////////////////////////////
///
/// パーサークラス
///
ParseL3S1Basic::ParseL3S1Basic(ParseCollection *collection)
	: Parse(collection, &gConfig.GetParam(eL3S1Basic))
{
	mBigEndian = true;	// 6809なので
}

ParseL3S1Basic::~ParseL3S1Basic()
{
}

/// キャラクターコードテーブルファイル名
wxString ParseL3S1Basic::GetCharCodeTableFileName() const
{
	return L3_CHAR_CODE_TABLE;
}

/// BASICコードテーブルファイル名
wxString ParseL3S1Basic::GetBasicCodeTableFileName() const
{
	return L3_BASIC_CODE_TABLE;
}

/// 初期設定値をセット
void ParseL3S1Basic::SetDefaultConfigParam()
{
	pConfig->SetNewLineAscii(eL3DefaultNewLineAscii);
	pConfig->SetStartAddr(eL3DefaultStartAddr);
}

/// 入力データのフォーマットチェック
/// @param[in] in_file_info : 入力ファイルの情報
/// @return true/false
bool ParseL3S1Basic::CheckDataFormat(PsFileInputInfo &in_file_info)
{
	wxUint8 hsign[6];
	bool st = true;
	PsFileFsInput in_file(in_file_info);
	PsFileStrOutput out_data;

	size_t len = in_file.Read(hsign, 3);

	if (len == 0) {
		// file is empty
		mErrInfo.SetInfo(__LINE__, psError, psErrFileIsEmpty);
		mErrInfo.ShowMsgBox();
		return false;
	}

	// テープイメージヘッダがあるか先頭から512バイトを検索
	bool is_tape = false;
	for(;!in_file.Eof() && len < 512; ) {
		if (memcmp(hsign, CMT_HEADER_GAP, 3) == 0) {
			is_tape = true;
			break;
		}
		in_file.Seek(-2, wxFromCurrent);
		in_file.Read(hsign, 3);
		len++;
	}
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
	in_data.SetType(in_file_info.GetType());
	out_data.Clear();

	// バッファにいれたデータの先頭を見る
	len = in_data.Read(hsign, 6);

	if (hsign[0] == 0xff) {
		// basic intermediate language
		in_data.SeekStartPos(3);
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
int ParseL3S1Basic::ReadBinaryToSymbolsOneLine(PsFileInput &in_file, PsFileType &out_type, int phase, PsSymbolSentence &sentence, ParseResult *result)
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

			} else if (mMachineType == MACHINE_TYPE_S1 && vals[0] == CODE_S1_NUMERIC) {
				if (area & SENTENCE_AREA) {
					area &= ~SENTENCE_AREA;	// end of sentence area
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
				}
				// S1 BASIC has numeric converted to binary.
				if (vals[1] == CODE_NUMBER1) {
					// integer number 1byte abs(0 - 255)
					word.Append(wxString::Format(_T("%d"),vals[2]),
						BinString(vals, 3));
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
					in_file.Seek(3-vlen, wxFromCurrent);
					mPos.mCol+=3;

				} else if (vals[1] == CODE_NUMBER2) {
					// integer number 2bytes abs(256 - 32767)
					vall = BytesToLong(&vals[2], 2);
					word.Append(wxString::Format(_T("%ld"),vall),
						BinString(vals, 4));
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
					in_file.Seek(4-vlen, wxFromCurrent);
					mPos.mCol+=4;

				} else if (vals[1] == CODE_FLOAT) {
					// float (4bytes) abs
					FloatBytesToStr(&vals[2], 4, chrstr);
					word.Append(chrstr,
						BinString(vals, 6));
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
					in_file.Seek(6-vlen, wxFromCurrent);
					mPos.mCol+=6;

				} else if (vals[1] == CODE_DOUBLE) {
					// double (8bytes) abs
					FloatBytesToStr(&vals[2], 8, chrstr);
					word.Append(chrstr,
						BinString(vals, 10));
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
					in_file.Seek(10-vlen, wxFromCurrent);
					mPos.mCol+=10;

				} else if (vals[1] == CODE_LINENUMBER) {
					// goto gosub integer number 2bytes
					vall = BytesToLong(&vals[2], 2);
					word.Append(wxString::Format(_T("%ld"),vall),
						BinString(vals, 4));
					word.SetBitType(PsSymbol::LINE_NUMBER);
					sentence.Add(word);
					word.Empty();
					word.SetType(area);
					in_file.Seek(4-vlen, wxFromCurrent);
					mPos.mCol+=4;

				}
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

					if (mMachineType != MACHINE_TYPE_S1) {
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

					word.Append(item->GetStr(),
						BinString(vals, item->GetCodeLength()));
					in_file.Seek((int)(item->GetCodeLength())-vlen, wxFromCurrent);
					mPos.mCol+=item->GetCodeLength();

					// &O,&Hの後を数値文字列として処理
					if (attr & CodeMapItem::ATTR_OCTSTRING) {
						// octet
						vall = OctHexStrToBinStr(vals, 10, word);
						in_file.Seek(vall, wxFromCurrent);
						word.SetBitType(PsSymbol::OCTSTRING);
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
						mPos.mCol+=vall;
					} else if (attr & CodeMapItem::ATTR_HEXSTRING) {
						// hex
						vall = OctHexStrToBinStr(vals, 10, word);
						in_file.Seek(vall, wxFromCurrent);
						word.SetBitType(PsSymbol::HEXSTRING);
						sentence.Add(word);
						word.Empty();
						word.SetType(area);
						mPos.mCol+=vall;
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
bool ParseL3S1Basic::ParseAsciiToBinary(PsFileData &in_data, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Ascii->Binary"));

	mNextAddress = (out_data.GetMachineType() == MACHINE_TYPE_L3 ? iStartAddr[pConfig->GetStartAddr()] : 1);

	// header
	out_data.Add("\xff\xff\xff", 3);

	// body
	mHasCodeFe = false;
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
bool ParseL3S1Basic::ParseAsciiToColored(PsFileData &in_data, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Parse Ascii"));

	// body
	mHasCodeFe = false;
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
void ParseL3S1Basic::ParseVariableString(const wxString &in_data, wxRegEx &re, PsSymbol &body, ParseResult *result)
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
void ParseL3S1Basic::ParseNumberString(const wxString &in_data, wxRegEx &re_real, wxRegEx &re_exp, PsSymbol &body, ParseResult *result)
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

		if (mMachineType == MACHINE_TYPE_S1) {
			// S1はバイナリにする
			body.Append(numstr, numstrb);
		} else {
			// L3は数値文字列のまま
			body.Append(numstr, numstr);
		}
		mPos.mCol += re_len;

	} else {
		if (result) result->Add(mPos, prErrInvalidNumber);
		mPos.mCol++;
	}

	// 数値後のスペースはトリミングする
	if (mMachineType == MACHINE_TYPE_S1 && reSpace.Matches(in_data.Mid(re_len))) {
		// S1モードでスペースが連続する場合はスペース１つ
		reSpace.GetMatch(&re_start,&re_len);
		mPos.mCol += (re_len - 1);
	}
}

/// 行番号文字列を数値に変換
/// @param[in]     in_data   入力文字
/// @param[in]     re        正規表現
/// @param[out]    body      出力文字
/// @param[in,out] result    結果格納用
void ParseL3S1Basic::ParseLineNumberString(const wxString &in_data, wxRegEx &re_int, PsSymbol &body, ParseResult *result)
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

		if (mMachineType == MACHINE_TYPE_S1) {
			// S1はバイナリにする
			body.Append(numstr, numstrb);
		} else {
			// L3は数値文字列のまま
			body.Append(numstr, numstr);
		}
		mPos.mCol += re_len;
	} else {
		if (result) result->Add(mPos, prErrInvalidNumber);
		mPos.mCol++;
	}

	// 数値後のスペースはトリミングする
	if (mMachineType == MACHINE_TYPE_S1 && reSpace.Matches(in_data.Mid(re_len))) {
		// S1モードでスペースが連続する場合はスペース１つ
		reSpace.GetMatch(&re_start,&re_len);
		mPos.mCol += (re_len - 1);
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
void ParseL3S1Basic::ParseOctHexString(const wxString &in_data, const wxString &octhexhed, const wxString &octhexcode, int base, wxRegEx &re, PsSymbol &body, ParseResult *result)
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

		// L3/S1は数値文字列のまま
		body.Append(numstr, numstr);

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
bool ParseL3S1Basic::ParseAsciiToSymbolsOneLine(PsFileType &in_file_type, wxString &in_data, PsFileType &out_type, PsSymbolSentence &sentence, ParseResult *result)
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

//		if (in_chr == EOF_CODEN) {
//			// end of text
//			break;
//		}

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

//				if (attr & CodeMapItem::ATTR_COLON) {
//					word.AppendBinStr(':');
//				}
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
			if (mMachineType == MACHINE_TYPE_S1 && in_str[0] == CODE_S1_NUMERIC) {
				// S1では0xfeの文字は使用できない
				if (result && !mHasCodeFe) result->Add(mPos, prErrEraseCodeFE);
				mHasCodeFe = true;
				mPos.mCol++;
				continue;
			}
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
bool ParseL3S1Basic::WriteText(PsFileData &in_data, PsFileOutput &out_file)
{
	// UTF-8に変換するか
	bool utf8_mode = out_file.GetTypeFlag(psUTF8);
	wxArrayString *inlines = &in_data.GetData();

	if (!out_file.IsOpened()) {
		return true;
	}

	if (!utf8_mode) {
		// テープイメージの場合CR固定。ディスクイメージの場合CR+LF固定
		int nl = (out_file.GetTypeFlag(psTapeImage) ? 0 : (out_file.GetTypeFlag(psDiskImage) ? 2 : pConfig->GetNewLineAscii()));
//		size_t len = 0;
		if (inlines->GetCount() > 0 && !inlines->Item(0).IsEmpty()) {
			out_file.Write(cNLChr[nl]); // 1行目は必ず改行
		}
		for(size_t row = 0; row < inlines->GetCount(); row++) {
			out_file.Write(inlines->Item(row));	// 変換しない
			out_file.Write(cNLChr[nl]); // 改行
		}
		// ファイル終端コードを出力
		if ((!out_file.GetTypeFlag(psTapeImage) && pConfig->GetEofAscii())) {
			out_file.Write((const wxUint8 *)EOF_CODE, 1);
		}
	} else {
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
bool ParseL3S1Basic::WriteBinary(PsFileData &in_data, PsFileOutput &out_file)
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
const int *ParseL3S1Basic::GetStartAddrsPtr() const
{
	return iStartAddr;
}

/// スタートアドレスのリスト数を返す
int ParseL3S1Basic::GetStartAddrCount() const
{
	return eL3StartAddrCount;
}

/// スタートアドレスのタイトルを返す
wxString ParseL3S1Basic::GetStartAddrTitle() const
{
	return L3_START_ADDR_MESSAGE;
}

/// 浮動小数点バイト列を文字列に変換
/// @param[in]  src     浮動小数点バイト列
/// @param[in]  src_len srcの長さ
/// @param[out] dst     文字列
/// @return     true
bool ParseL3S1Basic::FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst)
{
	UINT192 cin;
	UINT192 cpo;
	DeciStr decs;

	L3Float::RealStrToUint192(src, (int)src_len, cin, cpo);
	L3Float::Uint192ToDeciStr(cin, cpo, src_len > 4 ? 16 : 6, decs);

	dst = wxString::From8BitData(decs.GetStr(0), decs.Length());
	return true;
}

/// 数値を整数バイト文字列に変換
/// @param[in]  src     数値
/// @param[out] dst     バイト列(追記)
/// @param[in]  len     桁数
/// @return             true
bool ParseL3S1Basic::LongToBinStr(long src, BinString &dst, int len)
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
BinString ParseL3S1Basic::LongToBinStr(long src, int len)
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
int ParseL3S1Basic::NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err)
{
	bool is_integer = false;
	wxRegEx reExp(_T("[DE.!#]"));
	wxString str = src.Upper();
	wxUint8 buf[12];
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
		num = abs((int)num);
		buf[len] = 0xfe;
		len++;
		if (num < 256) {
			// 0 - 255
			buf[len] = 0x01;
			len++;
			LongToBytes(num, &buf[len], 1);
			len++;
		} else {
			// 256 - 32767
			buf[len] = 0x02;
			len++;
			LongToBytes(num, &buf[len], 2);
			len+=2;
		}
	} else {
		// 実数
		buf[len] = 0xfe;
		len++;

		UINT192 cin;
		UINT192 cpo;
		int bytes = L3Float::DeciStrToUint192(str, (int)str.Length(), cin, cpo);
		bytes *= 4;
		buf[len] = (bytes & 0xf);
		len++;

		int rc = L3Float::Uint192ToRealStr(cin, cpo, &buf[len], bytes);
		len += bytes;
		if (rc != 0) {
			// オーバーフロー or アンダーフロー
			if (err) *err = rc;
		}
	}
	memcpy(dst, buf, len);
	return len;
}

/// 数値文字列をバイト文字列に変換
/// @param[in]  src     数値文字列
/// @param[out] dst     バイト列(追記)
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ
int ParseL3S1Basic::NumStrToBinStr(const wxString &src, BinString &dst, int *err)
{
	wxUint8 buf[10];
	int len = NumStrToBytes(src, buf, 10, err);
	dst.Append(buf, len);
	return len;
}

/// 8/16進文字列をバイト文字列に変換(L3用)
/// @param[in]  src     8/16進文字列
/// @param[in]  src_len srcの長さ
/// @param[out] dst     バイト文字列
/// @return     >0: 書き込んだ長さ / 0: エラー
int ParseL3S1Basic::OctHexStrToBinStr(const wxUint8 *src, size_t src_len, PsSymbol &dst)
{
	int base = 10;
	size_t pos = 0;
	size_t re_sta = 0;
	size_t re_len = 0;

	if (src_len < 2) {
		return 0;
	}

	if (src[0] == '&') {
		base = 8;
		pos = 1;
		if (src[1] == 'h' || src[1] == 'H') {
			base = 16;
			pos = 2;
		} else if (src[1] == 'o' || src[1] == 'O') {
			base = 16;
			pos = 2;
		}
	} else {
		return 0;
	}

	BinString bstr(&src[pos], src_len - pos);

	if (base == 8 && reOcta.Matches(bstr)) {
		reOcta.GetMatch(&re_sta, &re_len);
		dst.Append(&src[pos], re_len, &src[pos], re_len);

	} else if (base == 16 && reHexa.Matches(bstr)) {
		reHexa.GetMatch(&re_sta, &re_len);
		dst.Append(&src[pos], re_len, &src[pos], re_len);

	}

	return (int)re_len;
}

/// 8/16進文字列が範囲内か
/// @param[in]  base    8 or 16
/// @param[in]  src     8/16進文字列
/// @param[in]  val     変換後の値
/// @return     false:オーバーフロー
bool ParseL3S1Basic::CheckOctHexStr(int base, const wxString &src, long *val)
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
/// (S1-BASIC用)
/// @param[in]  src            行番号(数値)文字列
/// @param[out] dst            バイト列
/// @param[in]  dst_len        dstの長さ
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ 0:上記以外のエラーで書き込めない
int ParseL3S1Basic::LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int memory_address, int *err)
{
	long num;
	int len = 0;

	if (dst_len < 3) {
		return 0;
	}

	if (!src.ToLong(&num)) {
		return 0;
	}
	if (num > 65529) {
		if (err) *err = 1;
		num = 65529;
	}
	if (num < 0) {
		if (err) *err = 2;
		num = 0;
	}
	dst[len] = 0xfe;
	len++;
	dst[len] = 0xf2;
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
int ParseL3S1Basic::LineNumStrToBinStr(const wxString &src, BinString &dst, int memory_address, int *err)
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
BinString ParseL3S1Basic::HomeLineNumToBinStr(long line_number, int memory_address, int *err)
{
	BinString dst;
//	int len = 0;

	if (line_number > 65529) {
		if (err) *err = 1;
		line_number = 65529;
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
int ParseL3S1Basic::GetInternalNameSize() const
{
	return 8;
}

/// BASICが拡張BASICかどうか
bool ParseL3S1Basic::IsExtendedBasic(const wxString &basic_type)
{
	return (basic_type.Find("ROM") >= 0 ? false : true);
}

/// マシンタイプの判別
int ParseL3S1Basic::GetMachineType(const wxString &basic_type)
{
	int machine_type = 0;
	if (basic_type.Find("L3") >= 0) {
		machine_type = MACHINE_TYPE_L3;
	} else if (basic_type.Find("S1") >= 0) {
		machine_type = MACHINE_TYPE_S1;
	}
	return machine_type;
}

/// 機種名を返す
wxString ParseL3S1Basic::GetMachineName() const
{
	return _("L3/S1 BASIC");
}

/// ファイルオープン時の拡張子リストを返す
const wxChar *ParseL3S1Basic::GetOpenFileExtensions() const
{
#if defined(__WXMSW__)
	return _("Supported files|*.bin;*.bas;*.txt;*.dat;*.l3|All files|*.*");
#else
	return _("Supported files|*.bin;*.BIN;*.bas;*.BAS;*.txt;*.TXT;*.dat;*.DAT;*.l3;*.L3|All files|*.*");
#endif
}

/// BASICバイナリテープイメージエクスポート時のデフォルト拡張子を返す
const wxChar *ParseL3S1Basic::GetExportBasicBinaryTapeImageExtension() const
{
	return wxT(".l3");
}

/// BASICバイナリテープイメージエクスポート時の拡張子リストを返す
const wxChar *ParseL3S1Basic::GetExportBasicBinaryTapeImageExtensions() const
{
	return _("Tape Image (*.l3)|*.l3|All Files (*.*)|*.*");
}
