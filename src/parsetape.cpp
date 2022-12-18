/// @file parsetape.cpp
///
/// @brief テープイメージパーサー
///
#include "parse.h"
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/arrimpl.cpp>

static const wxUint8 cmt_ident[4] = { 0xff, 0x01, 0x3c, 0x00 };

/// テープイメージのフォーマットチェック
bool Parse::CheckTapeDataFormat(PsFileInput &in_data, PsFileOutput &out_data)
{
	bool st = true;

	in_data.SetTypeFlag(psAscii, false);

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
bool Parse::ReadTapeToRealData(PsFileInput &in_data, PsFileOutput &out_data)
{
	wxString name = _("Tape->RealData");

	wxUint8 vals[260];

	int vlen = 0;

	int phase = 0;

	int header_type;
	int data_len;

	wxString file_name;	///< イメージ内のファイル名

	// parse start
	bool rc = true;
	while(!in_data.Eof() && phase >= 0) {
		memset(vals, 0, sizeof(vals));
		switch(phase) {
		case 0:
			// find indentifier
			vlen = (int)in_data.Read(vals, 3);
			if (vlen < 3) {
				// end
				phase = -1;
				break;
			}
			if (memcmp(vals, cmt_ident, 3) == 0) {
				// found
				in_data.Read(&vals[3], 1);
				header_type = vals[3];
				switch(header_type) {
					case 0:
						// file name section
						phase = 1;
						break;
					case 1:
						// body data section
						phase = 2;
						break;
					case 0xff:
						// footer data section
						phase = 3;
						break;
					default:
						// ?? error?
						break;
				}
			}
			if (phase == 0) {
				// continue reading
				in_data.Seek(-2, wxFromCurrent);
			}
			break;

		case 1:
			// file name section
			vlen = (int)in_data.Read(vals, 1);
			if (vlen == 0) {
				// end
				phase = -1;
				break;
			}
			data_len = vals[0];
			vlen = (int)in_data.Read(vals, data_len);
			// file name pos0-7
			out_data.SetInternalName(vals, 8);
			// file type pos8
			if (vals[8] != 0) {
				// This is not BASIC file.
				rc = false;
				phase = -1;
				break;
			}
			// file type2 pos9,10
			if (vals[9] == 0xff && vals[10] == 0xff) {
				// ascii
				out_data.SetTypeFlag(psAscii, true);
			} else if (vals[9] == 0 && vals[10] == 0) {
				// binary
				out_data.SetTypeFlag(psAscii, false);
			} else {
				// Invalid parameter.
				rc = false;
				phase = -1;
				break;
			}

			vlen = (int)in_data.Read(vals, 3);
			// crc 0 ignore
			// footer 1-2

			// read next section
			phase = 0;
			break;

		case 2:
			// body data section
			vlen = (int)in_data.Read(vals, 1);
			if (vlen == 0) {
				// end
				phase = -1;
				break;
			}
			data_len = vals[0];
			vlen = (int)in_data.Read(vals, data_len);
			// datas
			out_data.Write(vals, data_len);

			vlen = (int)in_data.Read(vals, 3);
			// crc 0 ignore
			// footer 1-2

			// read next section
			phase = 0;
			break;

		case 3:
			// footer data section
			vlen = (int)in_data.Read(vals, 1);
			if (vlen == 0) {
				// end
				phase = -1;
				break;
			}

			vlen = (int)in_data.Read(vals, 3);
			// crc 0 ignore
			// footer 1-2

			// end of reading file
			phase = -1;
			break;
		}
	}
	if (rc) {
		out_data.SetTypeFlag(psTapeImage, true);
	}
	return rc;
}

/// 実データをテープイメージにして出力
bool Parse::WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file)
{
	bool rc = true;
	int hlen = 0x5a;
	int dlen = 0;
	int chk_sum = 0;
	int phase = 0;
	wxString sdata;
	const char *cdata;
	wxUint8 buf[260];

	wxUint8 odata[520];
	int opos;

	while (phase >= 0) {
		switch(phase) {
			case 0:
				// file name section
				chk_sum = 0;
				opos = 0;

				memset(&odata[opos], 0xff, hlen); opos += hlen;
				memcpy(&odata[opos], &cmt_ident[1], 2); opos += 2;
				// header_type
				odata[opos++] = 0;
				chk_sum += 0;
				// 20バイト
				dlen = 20;
				odata[opos++] = (dlen & 0xff);
				chk_sum += dlen;
				// file name
				sdata = out_file.GetInternalName();
				if (sdata.Length() < 8) {
					sdata = "NONAME  ";
				}
				cdata = sdata.To8BitData();
				for(int i=0; i<8; i++) {
					odata[opos++] = (cdata[i] & 0xff);
					chk_sum += cdata[i];
				}
				// file type (BASIC)
				odata[opos++] = 0;
				chk_sum += 0;
				// file type 2
				if (in_file.GetTypeFlag(psAscii)) {
					memset(&odata[opos], 0xff, 2); opos += 2;
					chk_sum += 0xff + 0xff;
				} else {
					memset(&odata[opos], 0, 2); opos += 2;
					chk_sum += 0;
				}
				memset(&odata[opos], 0, dlen - 11); opos += (dlen - 11);
				chk_sum += 0;
				// chk sum
				odata[opos++] = (chk_sum & 0xff);
				// 0 x4
				memset(&odata[opos], 0, 4); opos += 4;

				out_file.Write(odata, opos);

				phase = 1;
				break;

			case 1:
				// body data
				chk_sum = 0;
				opos = 0;

				memset(&odata[opos], 0xff, hlen); opos += hlen;
				memcpy(&odata[opos], &cmt_ident[1], 2); opos += 2;
				// header_type
				odata[opos++] = 1;
				chk_sum += 1;
				// 255バイト最大
				dlen = (int)in_file.Read(buf, 255);
				if (dlen == 0) {
					// no data
					phase = 2;
					break;
				}
				odata[opos++] = (dlen & 0xff);
				chk_sum += dlen;
				for(int i=0; i<dlen; i++) {
					odata[opos++] = buf[i];
					chk_sum += buf[i];
				}
				// chk sum
				odata[opos++] = (chk_sum & 0xff);
				// 0 x4
				memset(&odata[opos], 0, 4); opos += 4;

				out_file.Write(odata, opos);

				if (!in_file.GetTypeFlag(psAscii)) {
					// gap reset when it's binary save
					hlen = 10;
				}

				if (dlen < 255 || in_file.Eof()) {
					phase = 2;
				}
				break;
			case 2:
				// footer data
				chk_sum = 0;
				opos = 0;

				memset(&odata[opos], 0xff, hlen); opos += hlen;
				memcpy(&odata[opos], &cmt_ident[1], 2); opos += 2;
				// header_type
				odata[opos++] = 0xff;
				chk_sum += 0xff;
				//
				dlen = 0;
				odata[opos++] = (dlen & 0xff);
				chk_sum += dlen;
				// chk sum
				odata[opos++] = (chk_sum & 0xff);
				// 0 x4
				memset(&odata[opos], 0, 4); opos += 4;

				out_file.Write(odata, opos);

				phase = -1;
				break;
		}
	}

	return rc;
}

/// @brief 内部ファイル名のキャラクターコードを変換するテーブル
static const wxString chr2utf8tbl[128] = {
	_T("年"),_T("月"),_T("日"),_T("市"),_T("区"),_T("町"),_T("を"),_T("ぁ"),_T("ぃ"),_T("ぅ"),_T("ぇ"),_T("ぉ"),_T("ゃ"),_T("ゅ"),_T("ょ"),_T("っ"),
	_T("π"),_T("あ"),_T("い"),_T("う"),_T("え"),_T("お"),_T("か"),_T("き"),_T("く"),_T("け"),_T("こ"),_T("さ"),_T("し"),_T("す"),_T("せ"),_T("そ"),
	_T("〒"),_T("。"),_T("「"),_T("」"),_T("、"),_T("・"),_T("ヲ"),_T("ァ"),_T("ィ"),_T("ゥ"),_T("ェ"),_T("ォ"),_T("ャ"),_T("ュ"),_T("ョ"),_T("ッ"),
	_T("ー"),_T("ア"),_T("イ"),_T("ウ"),_T("エ"),_T("オ"),_T("カ"),_T("キ"),_T("ク"),_T("ケ"),_T("コ"),_T("サ"),_T("シ"),_T("ス"),_T("セ"),_T("ソ"),
	_T("タ"),_T("チ"),_T("ツ"),_T("テ"),_T("ト"),_T("ナ"),_T("ニ"),_T("ヌ"),_T("ネ"),_T("ノ"),_T("ハ"),_T("ヒ"),_T("フ"),_T("ヘ"),_T("ホ"),_T("マ"),
	_T("ミ"),_T("ム"),_T("メ"),_T("モ"),_T("ヤ"),_T("ユ"),_T("ヨ"),_T("ラ"),_T("リ"),_T("ル"),_T("レ"),_T("ロ"),_T("ワ"),_T("ン"),_T("゛"),_T("゜"),
	_T("た"),_T("ち"),_T("つ"),_T("て"),_T("と"),_T("な"),_T("に"),_T("ぬ"),_T("ね"),_T("の"),_T("は"),_T("ひ"),_T("ふ"),_T("へ"),_T("ほ"),_T("ま"),
	_T("み"),_T("む"),_T("め"),_T("も"),_T("や"),_T("ゆ"),_T("よ"),_T("ら"),_T("り"),_T("る"),_T("れ"),_T("ろ"),_T("わ"),_T("ん"),_T("■"),_T("　")
};

/// @brief 内部ファイル名のキャラクターコードをUTF-8に変換
///
/// @param[in] src 内部ファイル名
/// @param[in] len 長さ
/// @return 変換後の文字列
wxString Parse::ConvInternalName(const wxUint8 *src, size_t len)
{
	wxString dst = _T("");

	for(size_t i=0; i<len; i++) {
		const wxUint8 p = src[i];
		if (p >= 0x80 && p <= 0xff) {
			dst += chr2utf8tbl[p-0x80];
		} else if (p >= 0x20 && p <= 0x7f) {
			dst += wxString::FromUTF8((const char *)&p, 1);
		} else {
			dst += _T("?");
		}
	}
	return dst;
}