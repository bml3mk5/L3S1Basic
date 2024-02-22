/// @file parsetape_msxbasic.cpp
///
/// @brief テープイメージパーサー
///
#include "parse_msxbasic.h"
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/arrimpl.cpp>

/// テープイメージのフォーマットチェック
bool ParseMSXBasic::CheckTapeDataFormat(PsFileInput &in_data, PsFileOutput &out_data)
{
	bool st = true;

//	in_data.SetTypeFlag(psAscii, false);

	// テープイメージから実データを取り出す
	st = ReadTapeToRealData(in_data, out_data);
	if (st != true) {
		// エラー
		mErrInfo.SetInfo(__LINE__, psError, psErrBasic);
		mErrInfo.ShowMsgBox();
		return false;
	}
	return true;
}

/// テープイメージから実ファイルを取り出す
bool ParseMSXBasic::ReadTapeToRealData(PsFileInput &in_data, PsFileOutput &out_data)
{
//	wxString name = _("Tape->RealData");

	wxUint8 vals[VALS_SIZE + 1];
	size_t vlen;

	// fmsx casette image
	in_data.SeekStartPos(8);

	in_data.Read(vals, 16);
	// イメージ内のファイル名
	out_data.SetInternalName(&vals[10], 6);

	// skip indentifier
	in_data.SeekStartPos(32);

	vlen = 1;
	if (in_data.GetTypeFlag(psAscii)) {
		// アスキーの時 テキスト中にヘッダがあるのでとり除く
		size_t  valpos = VALS_SIZE;
		size_t  valsta = VALS_SIZE;
		size_t  vallen = VALS_SIZE;
		size_t  valend = VALS_SIZE;
		vals[VALS_SIZE] = 0;

		memset((void *)vals, 0, sizeof(vals));

		for(;;) {
			if (valsta > 0) {
				// shift
				for(size_t i=0; i<(vallen + 1 - valsta); i++)
					vals[i] = vals[i + valsta];
				// read
				vlen = valend - valsta;
				vallen = in_data.Read(&vals[vlen], VALS_SIZE - vlen);
				if (vallen + vlen < VALS_SIZE) {
					valend = vallen + vlen;
				} else {
					valend = VALS_SIZE;
				}
				vallen = VALS_SIZE;
				valpos -= valsta;
				valsta = 0;
			}

			// ヘッダをとり除く
			for(; valpos + 32 <= vallen && valpos < valend; valpos++) { 
				if (memcmp(&vals[valpos], FMSX_HEADER, 8) == 0) {
					// delete header
					// shift
					for(size_t i=valpos; i<(vallen-8); i++)
						vals[i] = vals[i + 8];

					valend -= 8;
					vallen -= 8;
					break;
				} else if (vals[valpos] == 0x1a) {
					// end of text
					valend = valpos;
					break;
				}
			}

			// 出力
			if (valsta < valpos) {
				out_data.Write(&vals[valsta], valpos - valsta);
			}
			if (valpos >= valend) {
				// end of line
				break;
			}
			valsta = valpos;
		}

	} else {
		// バイナリの時 そのまま出力
		while(vlen > 0) {
			vlen = in_data.Read(vals, VALS_SIZE);
			if (vlen == 0) break;
			out_data.Write(vals, vlen);
		}

	}

	out_data.SetTypeFlag(psTapeImage, true);

	return true;
}

/// 実データをテープイメージにして出力
bool ParseMSXBasic::WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file)
{
	wxUint8 vals[260];
	size_t len = 1;

	if (out_file.GetTypeFlag(psAscii)) {
		// アスキー形式

		// header
		PutCasetteImageHeader(out_file);
		// body
		// 256バイトごとにギャップを入れる
		while(len > 0) {
			len = in_file.Read(vals, 256);
			if (len > 0) out_file.Write(vals, len);
			if (len < 256) break;
			out_file.Write(FMSX_HEADER, 8);
		}

		// footer
		PutCasetteImageFooter(out_file, len);

	} else if (!out_file.GetTypeFlag(psUTF8)) {
		// バイナリ形式

		// header
		PutCasetteImageHeader(out_file);

		// body
		// 1バイト目はスキップ
		in_file.Seek(1);
		while(len > 0) {
			len = in_file.Read(vals, 256);
			if (len > 0) out_file.Write(vals, len);
		}

		// footer
		PutCasetteImageFooter(out_file, 7);

	} else {
		// そのまま出力
		out_file.Write(in_file);
	}

	return true;
}

/// カセットイメージのヘッダを出力
/// @param[in,out] out_file データ
void ParseMSXBasic::PutCasetteImageHeader(PsFileOutput &out_file)
{
	wxUint8 vals[8];

	// header
	out_file.Write(FMSX_HEADER, 8);
	if (out_file.GetTypeFlag(psAscii)) {
		// ascii 0xea
		out_file.Write(CAS_HEADER_ASCII, 10);
//	} else if (0) {
//		// machine language
//		out_file.Write(CAS_HEADER_MACHINE, 10);
	} else {
		// basic intermediate language 0xd3
		out_file.Write(CAS_HEADER_BASIC, 10);
	}

	// internal name
	AscStrToBytes(out_file.GetInternalName(), vals, 6);
	out_file.Write(vals, 6);

	out_file.Write(FMSX_HEADER, 8);
}

/// カセットイメージのフッタを出力
/// @param[in,out] out_file データ
/// @param[in]     len      データ長さ
void ParseMSXBasic::PutCasetteImageFooter(PsFileOutput &out_file, size_t len)
{
	wxUint8 val;

	if (out_file.GetTypeFlag(psAscii)) {
		// ascii
		val = EOF_CODEN;
		for(; len < 256; len++) {
			out_file.Write(&val, 1);
		}
	} else {
		// basic
		val = 0x00;
		for(size_t i=0; i < len; i++) {
			out_file.Write(&val, 1);
		}
	}
}

/// @brief 内部ファイル名のキャラクターコードをUTF-8に変換
///
/// @param[in] src 内部ファイル名
/// @param[in] len 長さ
/// @return 変換後の文字列
wxString ParseMSXBasic::ConvInternalName(const wxUint8 *src, size_t len)
{
	return wxT("");
}
