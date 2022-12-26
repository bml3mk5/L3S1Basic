/// @file parse.cpp
///
/// @brief パーサー
///
#include "parse.h"
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/arrimpl.cpp>
#include "main.h"
#include "config.h"
#include "l3float.h"
#include "l3specs.h"

#define DATA_DIR _T("data")
#define CHAR_CODE_TABLE _T("char_code.dat")
#define BASIC_CODE_TABLE _T("basic_code.dat")

#define VALS_SIZE	1023

/// 改行コード
static wxString nl_chr[] = {
	_T("\r"), _T("\n"), _T("\r\n")
};
/// ファイル終端コード
#define EOF_CODE "\x1a"
/// BOMコード
#define BOM_CODE "\xEF\xBB\xBF"
/// スタートアドレス(L3用)
static int iStartAddr[4] = {
	0x0768,	// newon11 0x0400 + 0x368
	0x0B68,	// newon15 0x0800 + 0x368
	0x2768,	// newon 3 0x2400 + 0x368
	0x4768 	// newon 7 0x4400 + 0x368
};

#define ESC_CODE "\x1b"
#define ESC_CODEN 0x1b

//////////////////////////////////////////////////////////////////////

ParseAttr::ParseAttr()
{
	mAddSpaceAfterColon = false;
}
ParseAttr::~ParseAttr()
{
}

//////////////////////////////////////////////////////////////////////
///
/// パーサークラス
///
Parse::Parse(const wxString &data_path) : ParseParam()
{
	mAppPath = data_path;
	mBigEndian = true;	// 6809なので
}

Parse::~Parse()
{
}

bool Parse::Init()
{
	// 文字コード変換テーブルの読み込み
	if (LoadCharCodeTable() != psOK) {
		return false;
	}
	// BASICコード変換テーブルの読み込み
	if (LoadBasicCodeTable() != psOK) {
		return false;
	}

	/// 正規表現の設定
	reAlpha.Compile(_T("^[a-zA-Z]+"));
	reNumber.Compile(_T("^[0-9]+"));

	reAlphaNumeric.Compile(_T("^[0-9a-zA-Z]+"));

	reNumberReal.Compile(_T("^[0-9]+([.][0-9]+)?[#!]?"));
	reNumberExp.Compile(_T("^[0-9]+([.][0-9]+)?[dDeE][+-]?[0-9]+"));

	reNumberReald.Compile(_T("^[.][0-9]+[#!]?"));
	reNumberExpd.Compile(_T("^[.][0-9]+[dDeE][+-]?[0-9]+"));

	reOcta.Compile(_T("^[0-7]+"));
	reHexa.Compile(_T("^[0-9a-fA-F]+"));
	reSpace.Compile(_T("^  +"));

	reVariEnd.Compile(_T("[%$#!]"));

	return true;
}

/// 指定したファイルを開く
bool Parse::OpenDataFile(const wxString &in_file_name, PsFileType &file_type)
{
	PsFileInputInfo in_file;
	if (!in_file.Open(in_file_name, wxFile::read)) {
		mErrInfo.SetInfo(__LINE__, psError, psErrFileNotFound);
		mErrInfo.ShowMsgBox();
		return false;
	}
	in_file.SetType(file_type);
	if (!CheckDataFormat(in_file)) {
		return false;
	}
	in_file.Close();

	CloseDataFile();
	mInFile = in_file;
//	mInFile.Open(wxFile::read);

	return true;
}

static const wxUint8 cmt_gap[] = { 0xff, 0x01, 0x3c };

/// 入力データのフォーマットチェック
bool Parse::CheckDataFormat(PsFileInputInfo &in_file_info)
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
		if (memcmp(hsign, cmt_gap, 3) == 0) {
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

/// 中間言語形式データのフォーマットチェック
bool Parse::CheckBinaryDataFormat(PsFileInput &in_data)
{
	PsFileData    out_data;
	ParseResult   result;
	wxArrayString basic_types;
	ParseAttr     pattr;
	bool st = true;

	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	in_data.SetTypeFlag(psAscii, false);

	// ファイルを解析する
	GetBasicTypes(basic_types);
	out_data.SetTypeFlag(psAscii, true);
	for(size_t i=0; i < basic_types.GetCount(); i++) {
		in_data.SeekStartPos();
		in_data.SetBasicType(basic_types[i]);
		out_data.SetBasicType(basic_types[i]);
		out_data.Empty();
		result.Empty();
		st = ReadBinaryToAsciiColored(in_data, out_data, pattr, &result);
		if (result.GetCount() > 0) {
			// 変換できないBASIC statementがある
			in_data.SetTypeFlag(psExtendBasic, true);
			out_data.SetTypeFlag(psExtendBasic, true);
		} else {
			break;
		}
	}
	if (st != true) {
		// エラー
		mErrInfo.SetInfo(__LINE__, psError, psErrParseBasic);
		mErrInfo.ShowMsgBox();
		return false;
	}

	mParsedData.Empty();
	Report(result, mParsedData.GetData());

	// UTF-8に変換
	mParsedData.SetType(out_data);
	mParsedData.SetTypeFlag(psUTF8, true);
	mParsedData.SetCharType(GetCharType(0));
	st = ConvAsciiToUTF8(out_data, &mParsedData);

	return true;
}

/// アスキー形式データのフォーマットチェック
bool Parse::CheckAsciiDataFormat(PsFileInput &in_data)
{
	PsFileData    out_data;
	PsFileData    tmp_data;
	ParseResult   result;
	ParseAttr     pattr;
	bool st = false;
	int  sti;

	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	out_data.Empty();
	mParsedData.Empty();

	if (in_data.GetTypeFlag(psTapeImage | psDiskImage)) {
		// テープ or ディスクの場合
		sti = 0;
	} else {
		// まずUTF-8として変換できるかを試みる
		in_data.SetTypeFlag(psAscii | psUTF8, true);
		in_data.SeekStartPos();
		sti = ReadAsciiText(in_data, out_data, true);
	}
	if (sti == 1) {
		// UTF-8らしい
		wxArrayString char_types;
		GetCharTypes(char_types);
		out_data.SetTypeFlag(psAscii | psUTF8, true);
		tmp_data.SetTypeFlag(psAscii, true);
		for(size_t i=0; i<char_types.GetCount(); i++) {
			// 一旦Asciiにして変換できるかどうか調べる
			result.Empty();
			tmp_data.Empty();
			out_data.SetCharType(char_types.Item(i));
			st = ConvUTF8ToAscii(out_data, &tmp_data, &result);
			if (result.GetCount() == 0) {
				// エラーなし
				break;
			}
		}
		if (st != true) {
			// エラー解析中止
			mErrInfo.SetInfo(__LINE__, psError, psErrInvalidString);
			mErrInfo.ShowMsgBox();
			return false;
		}
		out_data.Empty();
		st = ParseAsciiToColored(tmp_data, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
		mParsedData.SetType(in_data.GetType());
		mParsedData.SetCharType(out_data.GetCharType());
		in_data.SetCharType(out_data.GetCharType());
		// UTF-8文字に変換する
		st = ConvAsciiToUTF8(out_data, &mParsedData);
	} else {
		// UTF-8で読めない or 7ビット文字のみ アスキー形式
		in_data.SetTypeFlag(psAscii, true);
		in_data.SetTypeFlag(psUTF8, false);
		in_data.SeekStartPos();
		out_data.Empty();
		out_data.SetType(in_data.GetType());
		mParsedData.Empty();
		ReadAsciiText(in_data, tmp_data, false);
		st = ParseAsciiToColored(tmp_data, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
		// interace 文字に変換する
		mParsedData.SetTypeFlag(psAscii | psUTF8, true);
		mParsedData.SetCharType(GetCharType(0));
		st = ConvAsciiToUTF8(out_data, &mParsedData);
	}

	return true;
}

// 入力ファイルの形式を変更して読み直す
bool Parse::ReloadOpendAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type)
{
	if (!mInFile.Exist()) {
		// データがない
		return false;
	}

	PsFileType out_type(mParsedData.GetType());

	mInFile.SetTypeFlag(mask, false);
	mInFile.SetTypeFlag(type, true);
	if (char_type != wxEmptyString) {
		mInFile.SetCharType(char_type);
	}
	if (basic_type != wxEmptyString) {
		mInFile.SetBasicType(basic_type);
	}
	return ReloadAsciiData(mInFile, out_type);
}
#if 0
// 表示用データの形式を変更して読み直す
bool Parse::ReloadParsedAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type)
{
	if (!mInFile.Exist() || !mInFile.GetTypeFlag(psAscii)) {
		// データがない or Binaryのとき
		return false;
	}

	PsFileType out_type(mParsedData.GetType());

	out_type.SetTypeFlag(mask, false);
	out_type.SetTypeFlag(type, true);
	if (char_type != wxEmptyString) {
		out_type.SetCharType(char_type);
	}
	if (basic_type != wxEmptyString) {
		out_type.SetBasicType(basic_type);
	}
	return ReloadAsciiData(mInFile, out_type);
}
#endif
// 表示用データの形式を変更して読み直す
bool Parse::ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type)
{
	PsFileType out_type(mParsedData.GetType());

	out_type.SetTypeFlag(mask, false);
	out_type.SetTypeFlag(type, true);
	if (char_type != wxEmptyString) {
		out_type.SetCharType(char_type);
	}

	if (!mInFile.GetTypeFlag(psAscii)) {
		out_type.SetBasicType(mInFile.GetBasicType());
	}
	if (basic_type != wxEmptyString) {
		out_type.SetBasicType(basic_type);
	}
	return ReloadParsedData(out_type);
}
// 表示用データの形式を変更して読み直す
bool Parse::ReloadParsedData(PsFileType &out_type)
{
	if (!mInFile.Exist()) {
		// データがない
		return false;
	}

	if (mInFile.GetTypeFlag(psAscii)) {
		return ReloadAsciiData(mInFile, out_type);
	} else {
		return ReloadBinaryData(mInFile, out_type);
	}
}
// 中間言語形式データの読み直し
bool Parse::ReloadOpenedBinaryData(int type, int mask, const wxString &basic_type)
{
	if (!mInFile.Exist() || mInFile.GetTypeFlag(psAscii)) {
		// データがないかASCII形式の場合
		return false;
	}

	PsFileType out_type(mParsedData.GetType());

	mInFile.SetTypeFlag(mask, false);
	mInFile.SetTypeFlag(type, true);
	mInFile.SetBasicType(basic_type);
	out_type.SetBasicType(basic_type);

	return ReloadBinaryData(mInFile, out_type);
}

// 中間言語形式データの読み直し
bool Parse::ReloadBinaryData(PsFileInput &in_file, PsFileType &out_type)
{
	PsFileData out_data;
	ParseResult result;
	ParseAttr pattr;

	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	out_data.Empty();
	mParsedData.Empty();
	result.Empty();

	out_data.SetType(out_type);
	in_file.SeekStartPos();
	if (in_file.GetBasicType() != out_type.GetBasicType()) {
		// 違うBASICのとき
		// まず、ASCIIテキストにする
		PsFileData tmp_data;
		tmp_data.SetTypeFlag(psAscii, true);
		tmp_data.SetBasicType(in_file.GetBasicType());
		ReadBinaryToAscii(in_file, tmp_data, &result);
		// 色付け
		ParseAsciiToColored(tmp_data, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
	} else {
		// 同じBASICのとき
		// 解析＆色付け
		ReadBinaryToAsciiColored(in_file, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
	}
	// interace or noninterace 文字に変換する
	mParsedData.SetType(out_type);
	ConvAsciiToUTF8(out_data, &mParsedData);

	return true;
}

// アスキー形式データの読み直し
bool Parse::ReloadAsciiData(PsFileInput &in_file, PsFileType &out_type)
{
	PsFileData  tmp_data;
	PsFileData  in_data;
	PsFileData  out_data;
	ParseResult result;
	ParseAttr   pattr;

	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	tmp_data.Empty();
	in_data.Empty();
	out_data.Empty();
	mParsedData.Empty();
	result.Empty();

	//
	in_file.SeekStartPos();
	if (in_file.GetTypeFlag(psUTF8)) {
		// UTF-8テキスト
		ReadAsciiText(in_file, in_data, true);
		// 一旦ASCIIにする
		ConvUTF8ToAscii(in_data, &tmp_data, &result);
		out_data.SetType(out_type);
		ParseAsciiToColored(tmp_data, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
	} else {
		// ASCIIテキスト
		ReadAsciiText(in_file, in_data, false);
		out_data.SetType(out_type);
		ParseAsciiToColored(in_data, out_data, pattr, &result);
		Report(result, mParsedData.GetData());
	}
	// interace or noninterace 文字に変換する
	mParsedData.SetType(out_type);
	ConvAsciiToUTF8(out_data, &mParsedData);

	return true;
}

/// ファイルを閉じる & ストリームのデータをクリア
void Parse::CloseDataFile()
{
	if (mInFile.IsOpened()) {
		mInFile.Close();
	}
	mInFile.ClearData();
}

/// ファイル開いているか or ストリームにデータがあるか
bool Parse::IsOpenedDataFile()
{
	return mInFile.IsOpened() || mInFile.Exist();
}

/// 開いているファイル種類
PsFileType *Parse::GetOpenedDataTypePtr()
{
	return (mInFile.Exist() ? &mInFile.GetType() : NULL);
}

/// 開いているファイル情報
PsFileInfo *Parse::GetOpenedDataInfoPtr()
{
	return &mInFile;
}

/// ファイル名
wxString Parse::GetFileNameBase()
{
	wxString name = mInFile.GetFileName();
	return name;
}

/// 出力ファイルを開く
bool Parse::OpenOutFile(const wxString &out_file_name, PsFileType &file_type)
{
	// 入力ファイルと同じファイルはダメ
	wxFileName out_file(out_file_name);
	if (mInFile.IsSameFile(out_file)) {
		mErrInfo.SetInfo(__LINE__, psError, pwErrSameFile);
		mErrInfo.ShowMsgBox();
		return false;
	}
	// 出力ファイルオープン
	mOutFile.SetType(file_type);
	if (!mOutFile.Open(out_file_name, wxFile::write)) {
		mErrInfo.SetInfo(__LINE__, psError, pwErrCannotWrite);
		mErrInfo.ShowMsgBox();
		return false;
	}

	return true;
}

// 出力ファイルを閉じる
void Parse::CloseOutFile()
{
	if (mOutFile.IsOpened()) {
		mOutFile.Close();
	}
}

// エクスポート
bool Parse::ExportData()
{
	bool st = false;

	if (!mInFile.Exist()) {
		mParsedData.Empty();
		mParsedData.Add(_("No input file exist."));
		mParsedData.Add(_T(""));
		return st;
	}

	PsFileData in_data;
	PsFileData out_data;
	PsFileStrOutput out_file;
	ParseResult result;
//	ParseAttr pattr;

	in_data.Empty();
	out_data.Empty();
	result.Empty();

	mInFile.SeekStartPos();

	// BOM付きで出力するか
	if (mOutFile.GetTypeFlag(psAscii | psUTF8)) {
		mOutFile.SetTypeFlag(psUTF8BOM, mIncludeBOM);
	} else {
		mOutFile.SetTypeFlag(psUTF8BOM, false);
	}

	out_file.SetType(mOutFile.GetType());
	out_data.SetType(out_file.GetType());
	if (mInFile.GetTypeFlag(psUTF8)) {
		if (mOutFile.GetTypeFlag(psUTF8)) {
			// UTF-8テキストからUTF-8テキストに変換
			PsFileData tmp_data;
			ReadAsciiText(mInFile, in_data, true);
			st = ConvUTF8ToAscii(in_data, &tmp_data, &result);
			st = ConvAsciiToUTF8(tmp_data, &out_data);
			st = WriteText(out_data, out_file);
		} else if (mOutFile.GetTypeFlag(psAscii)) {
			// UTF-8テキストからアスキー形式に変換
			ReadAsciiText(mInFile, in_data, true);
			st = ConvUTF8ToAscii(in_data, &out_data, &result);
			st = WriteText(out_data, out_file);
		} else {
			// UTF-8テキストから中間言語に変換
			ReadAsciiText(mInFile, in_data, true);
			out_data.SetTypeFlag(psAscii, true);
			st = ConvUTF8ToAscii(in_data, &out_data, &result);
			st = ParseAsciiToBinary(out_data, out_file, &result);
		}
	} else if (mInFile.GetTypeFlag(psAscii)) {
		if (mOutFile.GetTypeFlag(psUTF8)) {
			// アスキー形式からUTF-8テキストに変換
			ReadAsciiText(mInFile, in_data, false);
			st = ConvAsciiToUTF8(in_data, &out_data);
			st = WriteText(out_data, out_file);
		} else if (mOutFile.GetTypeFlag(psAscii)) {
			// アスキー形式からアスキー形式に変換
			ReadAsciiText(mInFile, in_data, false);
			st = WriteText(in_data, out_file);
		} else {
			// アスキー形式から中間言語に変換
			ReadAsciiText(mInFile, in_data, false);
			st = ParseAsciiToBinary(in_data, out_file, &result);
		}
	} else {
		if (mOutFile.GetTypeFlag(psAscii)) {
			// 中間言語からアスキー形式/UTF-8テキストに変換
			in_data.SetType(mInFile.GetType());
			st = ReadBinaryToAscii(mInFile, in_data, &result);
			if (mOutFile.GetTypeFlag(psUTF8)) {
				// UTF-8テキストの場合
				st = ConvAsciiToUTF8(in_data, &out_data);
			} else {
				out_data = in_data;
			}
			st = WriteText(out_data, out_file);
		} else {
			// 中間言語からアスキー形式にしてまた中間言語に変換
			mOutFile.SetTypeFlag(psUTF8, false);
			in_data.SetType(mInFile.GetType());
			st = ReadBinaryToAscii(mInFile, in_data, &result);
			st = ParseAsciiToBinary(in_data, out_file, &result);
		}
	}

	// ファイルに出力
	PsFileStrInput in_file(out_file);
	PsFileFsOutput out(mOutFile.GetFile());
	out.SetType(mOutFile.GetType());
	if (mOutFile.GetInternalName().Length() > 0) {
		// テープイメージに変換して出力
		if (WriteTapeFromRealData(in_file, out)) {
			// 内部ファイル名を入力側に反映
			mInFile.SetInternalName(mOutFile.GetInternalName());
		}
	} else {
		// そのまま出力
		out.Write(in_file);
	}

	mParsedData.Empty();
	if (result.GetCount() > 0) {
		Report(result, mParsedData.GetData());
		st = false;
	} else {
		mParsedData.Add(_("Complete."));
		mParsedData.Add(_T(""));
	}
	return st;
}

/// 中間言語からアスキー形式テキストに変換
bool Parse::ReadBinaryToAscii(PsFileInput &in_file, PsFileData &out_data, ParseResult *result)
{
	bool extend_basic = out_data.GetTypeFlag(psExtendBasic);	// DISK BASICか
	int machine_type = out_data.GetMachineType();
	wxString basic_type = out_data.GetBasicType();

	wxString name = _("Binary->Ascii");

	long next_addr;				// next address
	long line_number = 0;		// line number
	// body
	wxUint8 vals[10];
	long vall;
	wxUint8 area = 0;
//	bool quoted = false;
//	bool data_area = false;
//	bool alldata_area = false;
//	bool comment_area = false;

	wxString body = _T("");
	wxString chrstr;
	CodeMapItem *item;

	int vrow = 0;
	int vcol = 0;
	int vlen = 0;

	int phase = 1;

	int error_count = 0;	// エラー発生数
	bool stopped = false;

	// parse start

	while(!in_file.Eof() && phase >= 0) {
		memset(vals, 0, sizeof(vals));
		if (error_count > 20) {
			// エラーが多いので解析中止
			stopped = true;
			break;
		}
		switch(phase) {
//		case 0:
//			// get header
//			in_file.file.Read((void *)hsign, 3);
//			// next phase
//			phase = 1;
//			break;
//
		case 1:
			// get next address
			in_file.Read(vals, 2);
			next_addr = BytesToLong(vals, 2);
			if (next_addr == 0) {
				// end
				phase = -1;
			} else {
				// next phase
				phase = 2;
				vrow++;
			}
			break;

		case 2:
			// get line number
			in_file.Read(vals, 2);
			line_number = BytesToLong(vals, 2);
			// next phase
			phase = 3;

			vcol = 0;
			break;

		case 3:
			// get 10 chars

			vlen = (int)in_file.Read(vals, 10);

			if (vlen == 0) {
				// end
				phase = -1;
				break;
			} else {
				if (vals[0] == 0) {
					// end of line
					phase = 4;
					in_file.Seek(1-vlen, wxFromCurrent);
					vcol++;
					break;
				} else if (vals[0] == 0x22) {
					// double quote
					chrstr = wxString::From8BitData((const char *)vals, 1);
					body += chrstr;
					in_file.Seek(1-vlen, wxFromCurrent);
					vcol++;
					area = (area & QUOTED_AREA) ? area & ~QUOTED_AREA : area | QUOTED_AREA;	// toggle quoted string
				} else {
					if (area & (QUOTED_AREA | ALLDATA_AREA | COMMENT_AREA)) {
						// quoted string or all DATA line or REM line
						chrstr = wxString::From8BitData((const char *)vals, 1);
						body += chrstr;
						in_file.Seek(1-vlen, wxFromCurrent);
						vcol++;
					} else if (area & DATA_AREA) {
						// DATA line
						if (vals[0] == 0x3a) {
							area &= ~DATA_AREA;	// end of data area
						}
						chrstr = wxString::From8BitData((const char *)vals, 1);
						body += chrstr;
						in_file.Seek(1-vlen, wxFromCurrent);
						vcol++;
					} else if (machine_type == MACHINE_TYPE_S1 && vals[0] == 0xfe) {
						// S1 BASIC has numeric converted to binary.
						if (vals[1] == 0x01) {
							// integer number 1byte abs(0 - 255)
							chrstr = wxString::Format(_T("%d"),vals[2]);
							body += chrstr;
							in_file.Seek(3-vlen, wxFromCurrent);
							vcol+=3;

						} else if (vals[1] == 0x02) {
							// integer number 2bytes abs(256 - 32767)
							vall = BytesToLong(&vals[2], 2);
							chrstr = wxString::Format(_T("%ld"),vall);
							body += chrstr;
							in_file.Seek(4-vlen, wxFromCurrent);
							vcol+=4;

						} else if (vals[1] == 0x04) {
							// float (4bytes) abs
							FloatBytesToStr(&vals[2], 4, chrstr);
							body += chrstr;
							in_file.Seek(6-vlen, wxFromCurrent);
							vcol+=6;

						} else if (vals[1] == 0x08) {
							// double (8bytes) abs
							FloatBytesToStr(&vals[2], 8, chrstr);
							body += chrstr;
							in_file.Seek(10-vlen, wxFromCurrent);
							vcol+=10;

						} else if (vals[1] == 0xf2) {
							// goto gosub integer number 2bytes
							vall = BytesToLong(&vals[2], 2);
							chrstr = wxString::Format(_T("%ld"),vall);
							body += chrstr;
							in_file.Seek(4-vlen, wxFromCurrent);
							vcol+=4;

						}
					} else {
						// find command statement
						mBasicCodeTbl.FindSectionByType(machine_type);
						item = mBasicCodeTbl.FindByCode(vals);
						if (item == NULL && extend_basic) {
							mBasicCodeTbl.FindSection(basic_type);
							item = mBasicCodeTbl.FindByCode(vals);
						}
						if (item != NULL) {
							wxUint32 attr = item->GetAttr();
							if (attr & CodeMapItem::ATTR_COLON) { // ELSE statement
								int len = (int)body.Len()-1;
								if (len >= 0 && body.GetChar(len) == ':') {
									body = body.Left(len); // trim last char ':'
								}
							}

							body += item->GetStr();
							in_file.Seek((int)(item->GetCodeLength())-vlen, wxFromCurrent);
							vcol+=item->GetCodeLength();

							if (attr & CodeMapItem::ATTR_ALLDATA) { // all DATA statement

								area |= ALLDATA_AREA;
							} else if (attr & CodeMapItem::ATTR_DATA) { // DATA statement

								area |= DATA_AREA;
							} else if (attr & CodeMapItem::ATTR_COMMENT) { // ' REM statement

								area |= COMMENT_AREA;
							}
						} else {
							chrstr = _T("");
							if (vals[0] < 0x20 || 0x80 <= vals[0]) {
								// Unknown command (error?)
								error_count++;
								// for parse result
								if (result) {
									result->Add(vrow, vcol, line_number, name, prErrInvalidBasicCode);
								}
							}
							// variable name
							chrstr = wxString::From8BitData((const char *)vals, 1);
							body += chrstr;
							in_file.Seek(1-vlen, wxFromCurrent);
							vcol++;
						}
					}
				}
			}
			break;

		case 4:
			// end of line
			chrstr.Printf(_T("%ld "),line_number);
			out_data.Add(chrstr + body);

			// clear val
			area = 0;
			body = _T("");

			// next phase
			phase = 1;
			break;

		}
	}
	if (stopped) {
		if (result) {
			result->Add(vrow, vcol, line_number, name, prErrStopInvalidBasicCode);
		}
		return false;
	}
	return true;

}

/// 中間言語からアスキー形式色付きテキストに変換
bool Parse::ReadBinaryToAsciiColored(PsFileInput &in_file, PsFileData &out_data, const ParseAttr &pattr, ParseResult *result)
{
	bool extend_basic = out_data.GetTypeFlag(psExtendBasic);	// DISK BASICか
	int machine_type = out_data.GetMachineType();
	wxString basic_type = out_data.GetBasicType();

	bool add_space_colon = pattr.DoesAddSpaceAfterColon();

	wxString name = _("Parse Binary");

	long next_addr;				// next address
	long line_number = 0;		// line number
	// body
	wxUint8 vals[10];
	long vall;
	wxUint8 area = 0;
	int  linenumber_area = 0;

	wxString body = _T("");
	wxString chrstr;
	CodeMapItem *item;

	int vrow = 0;
	int vcol = 0;
	int vlen = 0;
	int llen = 0;

	int phase = 1;

	int error_count = 0;	// エラー発生数
	bool stopped = false;

	// parse start

	while(!in_file.Eof() && phase >= 0) {
		memset(vals, 0, sizeof(vals));
		if (error_count > 20) {
			// エラーが多いので解析中止
			stopped = true;
			break;
		}
		switch(phase) {
//		case 0:
//			// get header
//			in_file.file.Read((void *)hsign, 3);
//			// next phase
//			phase = 1;
//			break;
//
		case 1:
			// get next address
			in_file.Read(vals, 2);
			next_addr = BytesToLong(vals, 2);
			if (next_addr == 0) {
				// end
				phase = -1;
			} else {
				// next phase
				phase = 2;
				vrow++;
			}
			break;

		case 2:
			// get line number
			in_file.Read(vals, 2);
			line_number = BytesToLong(vals, 2);
			// next phase
			phase = 3;

			vcol = 0;
			break;

		case 3:
			// get 10 chars

			vlen = (int)in_file.Read(vals, 10);

			if (vlen == 0) {
				// end
				phase = -1;
				break;
			} else {
				if (vals[0] == 0) {
					// end of line
					phase = 4;
					in_file.Seek(1-vlen, wxFromCurrent);
					vcol++;
					break;
				} else if (vals[0] == 0x22) {
					// double quote
					if (!(area & QUOTED_AREA)) {
						if (!(area & COMMENT_AREA)) {
							if (add_space_colon) {
								// add space after comma
								llen = (int)body.Len() - 1;
								if (llen >= 0 && (body.GetChar(llen) == ',')) {
									body += _T(" ");
								}
							}
							body += ESC_CODEN;
							body += wxT('q');
						}
					}
					chrstr = wxString::From8BitData((const char *)vals, 1);
					body += chrstr;
					in_file.Seek(1-vlen, wxFromCurrent);
					vcol++;
					if (area & QUOTED_AREA) {
						if (!(area & COMMENT_AREA)) {
							body += ESC_CODEN;
							body += wxT('e');
						}
						area &= ~QUOTED_AREA;
					} else {
						area |= QUOTED_AREA;
					}
				} else {
					if (area & (QUOTED_AREA | COMMENT_AREA)) {
						// quoted string or REM line
						chrstr = wxString::From8BitData((const char *)vals, 1);
						body += chrstr;
						in_file.Seek(1-vlen, wxFromCurrent);
						vcol++;
					} else if (area & ALLDATA_AREA) {
						// all DATA line
						if (add_space_colon && (area & QUOTED_AREA) == 0) {
							// add space after comma
							llen = (int)body.Len() - 1;
							if (llen >= 0 && (body.GetChar(llen) == ',')) {
								body += _T(" ");
							}
						}
						chrstr = wxString::From8BitData((const char *)vals, 1);
						body += chrstr;
						in_file.Seek(1-vlen, wxFromCurrent);
						vcol++;
					} else if (area & DATA_AREA) {
						// DATA line
						if (vals[0] == 0x3a) {
							area &= ~DATA_AREA;	// end of data area
							body += ESC_CODEN;
							body += wxT('e');
						}
						if (add_space_colon && (area & QUOTED_AREA) == 0) {
							// add space after comma
							llen = (int)body.Len() - 1;
							if (llen >= 0 && (body.GetChar(llen) == ',')) {
								body += _T(" ");
							}
						}
						chrstr = wxString::From8BitData((const char *)vals, 1);
						body += chrstr;
						in_file.Seek(1-vlen, wxFromCurrent);
						vcol++;
					} else if (machine_type == MACHINE_TYPE_S1 && vals[0] == 0xfe) {
						if (add_space_colon) {
							// add space after colon or comma
							llen = (int)body.Len() - 1;
							if (llen >= 0 && (body.GetChar(llen) == ':' || body.GetChar(llen) == ',')) {
								body += _T(" ");
							}
						}
						if (area & SENTENCE_AREA) {
							area &= ~SENTENCE_AREA;	// end of sentence area
							body += ESC_CODEN;
							body += wxT('e');
						}
						// S1 BASIC has numeric converted to binary.
						if (vals[1] == 0x01) {
							// integer number 1byte abs(0 - 255)
							chrstr = wxString::Format(_T("%d"),vals[2]);
							body += chrstr;
							in_file.Seek(3-vlen, wxFromCurrent);
							vcol+=3;

						} else if (vals[1] == 0x02) {
							// integer number 2bytes abs(256 - 32767)
							vall = BytesToLong(&vals[2], 2);
							chrstr = wxString::Format(_T("%ld"),vall);
							body += chrstr;
							in_file.Seek(4-vlen, wxFromCurrent);
							vcol+=4;

						} else if (vals[1] == 0x04) {
							// float (4bytes) abs
							FloatBytesToStr(&vals[2], 4, chrstr);
							body += chrstr;
							in_file.Seek(6-vlen, wxFromCurrent);
							vcol+=6;

						} else if (vals[1] == 0x08) {
							// double (8bytes) abs
							FloatBytesToStr(&vals[2], 8, chrstr);
							body += chrstr;
							in_file.Seek(10-vlen, wxFromCurrent);
							vcol+=10;

						} else if (vals[1] == 0xf2) {
							// goto gosub integer number 2bytes
							vall = BytesToLong(&vals[2], 2);
							body += ESC_CODEN;
							body += wxT('l');
							chrstr = wxString::Format(_T("%ld"),vall);
							body += chrstr;
							body += ESC_CODEN;
							body += wxT('e');
							in_file.Seek(4-vlen, wxFromCurrent);
							vcol+=4;

						}
					} else {
						// find command statement
						mBasicCodeTbl.FindSectionByType(machine_type);
						item = mBasicCodeTbl.FindByCode(vals);
						if (item == NULL && extend_basic) {
							mBasicCodeTbl.FindSection(basic_type);
							item = mBasicCodeTbl.FindByCode(vals);
						}
						if (item != NULL) {
							wxUint32 attr2 = item->GetAttr2();
							if (attr2 & CodeMapItem::ATTR_COLON) { // ELSE statement
								llen = (int)body.Len() - 1;
								if (llen >= 0 && body.GetChar(llen) == ':') {
									body = body.Left(llen); // trim last char ':'
								}
							}
							if (add_space_colon) {
								llen = (int)body.Len() - 1;
								if (llen >= 0 && (body.GetChar(llen) == ':' || body.GetChar(llen) == ',')) {
									body += _T(" ");
								}
							}

							if (attr2 & CodeMapItem::ATTR_ALLDATA) { // all DATA statement
								if (!(area & ALLDATA_AREA)) {
									area |= ALLDATA_AREA;
									body += ESC_CODEN;
									body += wxT('d');
								}
							} else if (attr2 & CodeMapItem::ATTR_DATA) { // DATA statement
								if (!(area & DATA_AREA)) {
									area |= DATA_AREA;
									body += ESC_CODEN;
									body += wxT('d');
								}
							} else if (attr2 & CodeMapItem::ATTR_COMMENT) { // ' REM statement
								if (!(area & COMMENT_AREA)) {
									area |= COMMENT_AREA;
									body += ESC_CODEN;
									body += wxT('c');
								}
							}
							if (machine_type != MACHINE_TYPE_S1) {
								if (linenumber_area && (attr2 & CodeMapItem::ATTR_CONTLINENUMBER) != 0) {
									// 行番号指定は続く
									linenumber_area = 1;
								} else if (linenumber_area && (attr2 & CodeMapItem::ATTR_CONTONELINENUMBER) != 0) {
									// 最初の数値のみ行番号
									linenumber_area = 2;
								} else if ((attr2 & (CodeMapItem::ATTR_CONTONELINENUMBER | CodeMapItem::ATTR_ONELINENUMBER)) == CodeMapItem::ATTR_ONELINENUMBER) {
									// 最初の数値のみ行番号
									linenumber_area = 2;
								} else if ((attr2 & (CodeMapItem::ATTR_CONTLINENUMBER | CodeMapItem::ATTR_ONELINENUMBER | CodeMapItem::ATTR_LINENUMBER)) == CodeMapItem::ATTR_LINENUMBER) {
									// 行番号はその命令内の数値全て
									linenumber_area = 1;
								} else {
									linenumber_area = 0;
								}
							}

							if ((area & (ALLDATA_AREA | DATA_AREA | COMMENT_AREA)) == 0) {
								if (!(area & SENTENCE_AREA)) {
									area |= SENTENCE_AREA;
									body += ESC_CODEN;
									body += wxT('v');
								}
							}

							body += item->GetStr();
							in_file.Seek((int)(item->GetCodeLength())-vlen, wxFromCurrent);
							vcol+=item->GetCodeLength();

						} else {
							chrstr = _T("");
							if (area & SENTENCE_AREA) {
								body += ESC_CODEN;
								body += wxT('e');
								area &= ~SENTENCE_AREA;
							}
							if (vals[0] < 0x20 || 0x80 <= vals[0]) {
								// Unknown command (error?)
								error_count++;
								// for parse result
								if (result) {
									result->Add(vrow, vcol, line_number, name, prErrInvalidBasicCode);
								}
								chrstr = wxString::From8BitData((const char *)vals, 1);
								body += chrstr;
								in_file.Seek(1-vlen, wxFromCurrent);
								vcol++;
							} else if (linenumber_area && 0x30 <= vals[0] && vals[0] <= 0x39) {
								int n = 0;
								for(; 0x30 <= vals[n] && vals[n] <= 0x39 && n < 10; n++) {}
								chrstr = wxString::From8BitData((const char *)vals, n);
								body += ESC_CODEN;
								body += wxT('l');
								body += chrstr;
								body += ESC_CODEN;
								body += wxT('e');
								in_file.Seek(n-vlen, wxFromCurrent);
								vcol+=n;
								if (linenumber_area == 2) {
									linenumber_area = 0;
								}
							} else {
								// variable name
								if (add_space_colon) {
									llen = (int)body.Len() - 1;
									if (llen >= 0 && (body.GetChar(llen) == ':' || body.GetChar(llen) == ',')) {
										body += _T(" ");
									}
								}
								chrstr = wxString::From8BitData((const char *)vals, 1);
								body += chrstr;
								in_file.Seek(1-vlen, wxFromCurrent);
								vcol++;
							}
						}
					}
				}
			}
			break;

		case 4:
			// end of line
			while(area) {
				if (area & 1) {
					body += ESC_CODEN;
					body += wxT('e');
				}
				area >>= 1;
			}

			chrstr.Printf(_T("%cl%ld %ce"),ESC_CODEN,line_number,ESC_CODEN);
			out_data.Add(chrstr + body);

			// clear val
			area = 0;
			linenumber_area = 0;
			body = _T("");

			// next phase
			phase = 1;
			break;

		}
	}
	if (stopped) {
		if (result) {
			result->Add(vrow, vcol, line_number, name, prErrStopInvalidBasicCode);
		}
		return false;
	}
	return true;

}

// アスキー形式/UTF-8テキストから中間言語に変換して出力
bool Parse::ParseAsciiToBinary(PsFileData &in_data, PsFileOutput &out_file, ParseResult *result)
{
	mNextAddress = (in_data.GetMachineType() == MACHINE_TYPE_L3 ? iStartAddr[mStartAddr] : 1);

//	wxString outline = _T("");

	// header
	if (out_file.IsOpened()) {
		out_file.Write((const wxUint8 *)"\xff\xff\xff", 3);
	}

	// body
	mPrevLineNumber = -1;
	for(size_t row = 0; row < in_data.GetCount(); row++) {
		ParseAsciiToBinaryOneLine(in_data.GetType(), in_data[row], out_file, (int)row+1, result);
	}

	// footer
	if (out_file.IsOpened()) {
		out_file.Write((const wxUint8 *)"\x00\x00", 2);
	}

	// ファイル終端コードを出力
	if ((!out_file.GetTypeFlag(psTapeImage) && mEofBinary)) {
		out_file.Write((const wxUint8 *)EOF_CODE, 1);
	}

	return true;

}

// アスキー形式を解析して色付けする
bool Parse::ParseAsciiToColored(PsFileData &in_data, PsFileData &out_data, const ParseAttr &pattr, ParseResult *result)
{
	// body
	mPrevLineNumber = -1;
	for(size_t row = 0; row < in_data.GetCount(); row++) {
		ParseAsciiToColoredOneLine(in_data.GetType(), in_data[row], out_data, pattr, (int)row+1, result);
	}

	return true;
}

/// 変数文字列を変数文字に変換
void Parse::ParseVariableString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result)
{
	size_t re_start, re_len;
	wxString chrstr;
	if (re.Matches(in_data)) {
		// 変数名
		re.GetMatch(&re_start,&re_len);
		chrstr = in_data.Mid(re_start, re_len);
		chrstr = chrstr.Upper();
		if (re_len > 16) {
			// 変数名は16文字までが有効（エラーにしない）
		}
		body_len += AscStrToBytes(chrstr, &body[body_len], body_size-body_len);
		pos += re_len;
		empstr = false;

	} else {
		// 変数名がおかしい（ここにはこない）
	}
}

/// 変数文字列を変数文字に変換
void Parse::ParseVariableString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxString &body, bool &empstr, ParseResult *result)
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
//		body += ESC_CODEN;
//		body += wxT('v');
		body += chrstr;
//		body += ESC_CODEN;
//		body += wxT('e');
		pos += re_len;
		empstr = false;

	} else {
		// 変数名がおかしい（ここにはこない）
	}
}

/// 数値文字列を数値に変換
void Parse::ParseNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_real, wxRegEx &re_exp, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
	int len = 0;
	int err = 0;
	wxString numstr;

	if (re_real.Matches(in_data)) {
		// 整数 or 実数表記
		re_real.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		len = NumStrToBytes(numstr, &body[body_len], body_size-body_len, &err);

		if (err != 0) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
		if (len == 0) {
			// cannot convert
			len = AscStrToBytes(numstr, &body[body_len], body_size-body_len);
			if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
		}
		body_len += len;
		pos += re_len;
		empstr = false;
	} else if (re_exp.Matches(in_data)) {
		// 指数表記
		int len, err;
		re_exp.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		len = NumStrToBytes(numstr, &body[body_len], body_size-body_len, &err);

		if (err != 0) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
		if (len == 0) {
			// cannot convert
			len = AscStrToBytes(numstr, &body[body_len], body_size-body_len);
			if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
		}
		body_len += len;
		pos += re_len;
		empstr = false;
	} else {
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
	}
	// 数値後のスペースはトリミングする
	if (reSpace.Matches(in_data.Mid(re_len))) {
		// S1モードでスペースが連続する場合はスペース１つ
		reSpace.GetMatch(&re_start,&re_len);
		body_len += AscStrToBytes(wxT(" "), &body[body_len], body_size-body_len);
		pos+=re_len;
	}
}

/// 数値文字列を数値に変換
void Parse::ParseNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_real, wxRegEx &re_exp, wxString &body, bool &empstr, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
//	int len = 0;
//	int err = 0;
	wxString numstr;

	if (re_real.Matches(in_data)) {
		// 整数 or 実数表記
		re_real.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		body += numstr;
		pos += re_len;
		empstr = false;
	} else if (re_exp.Matches(in_data)) {
		// 指数表記
		re_exp.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		body += numstr;
		pos += re_len;
		empstr = false;
	} else {
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
	}
//	// 数値後のスペースはトリミングする
//	if (reSpace.Matches(in_data.Mid(re_len))) {
//		// S1モードでスペースが連続する場合はスペース１つ
//		reSpace.GetMatch(&re_start,&re_len);
//		body_len += AscStrToBytes(wxT(" "), &body[body_len], body_size-body_len);
//		pos+=re_len;
//	}
}

/// 行番号文字列を数値に変換
void Parse::ParseLineNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_int, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
	int len = 0;
	int err = 0;
	wxString numstr;

	if (re_int.Matches(in_data)) {
		// 整数
		re_int.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		len = LineNumStrToBytes(numstr, &body[body_len], body_size-body_len, &err);

		if (err != 0) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
		if (len == 0) {
			// cannot convert
			len = AscStrToBytes(numstr, &body[body_len], body_size-body_len);
			if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
		}
		body_len += len;
		pos += re_len;
		empstr = false;
	} else {
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
	}
	// 数値後のスペースはトリミングする
	if (reSpace.Matches(in_data.Mid(re_len))) {
		// S1モードでスペースが連続する場合はスペース１つ
		reSpace.GetMatch(&re_start,&re_len);
		body_len += AscStrToBytes(wxT(" "), &body[body_len], body_size-body_len);
		pos+=re_len;
	}
}

/// 行番号文字列を数値に変換
void Parse::ParseLineNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_int, wxString &body, bool &empstr, ParseResult *result)
{
	size_t re_start = 0,re_len = 0;
//	int len = 0;
	int err = 0;
	wxString numstr;

	if (re_int.Matches(in_data)) {
		// 整数
		re_int.GetMatch(&re_start,&re_len);
		numstr = in_data.Mid(re_start, re_len);

		body += ESC_CODEN;
		body += wxT('l');
		body += numstr;
		body += ESC_CODEN;
		body += wxT('e');

		if (err != 0) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
		pos += re_len;
		empstr = false;
	} else {
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
	}
//	// 数値後のスペースはトリミングする
//	if (reSpace.Matches(in_data.Mid(re_len))) {
//		// S1モードでスペースが連続する場合はスペース１つ
//		reSpace.GetMatch(&re_start,&re_len);
//		body_len += AscStrToBytes(wxT(" "), &body[body_len], body_size-body_len);
//		pos+=re_len;
//	}
}

/// 8進or16進文字列を文字に変換
void Parse::ParseOctHexString(const wxString &in_data, const wxString &octhexhed, int base, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result)
{
	size_t re_start, re_len;
	wxString numstr = in_data.Mid(octhexhed.Length());

	if (re.Matches(numstr)) {
		// 数値OK
		re.GetMatch(&re_start,&re_len);
		numstr = numstr.Mid(re_start, re_len);
		int rc = CheckOctHexStr(base, numstr);
		if (!rc) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
		numstr = octhexhed + numstr;
		body_len += AscStrToBytes(numstr, &body[body_len], body_size-body_len);
		pos += numstr.Length();
		empstr = false;
	} else {
		// 数値NG
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
		empstr = true;
	}
}

/// 8進or16進文字列を文字に変換
void Parse::ParseOctHexString(const wxString &in_data, const wxString &octhexhed, int base, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxString &body, bool &empstr, ParseResult *result)
{
	size_t re_start, re_len;
	wxString numstr = in_data.Mid(octhexhed.Length());

	if (re.Matches(numstr)) {
		// 数値OK
		re.GetMatch(&re_start,&re_len);
		numstr = numstr.Mid(re_start, re_len);
		int rc = CheckOctHexStr(base, numstr);
		if (!rc) {
			// overflow
			if (result) result->Add(row+1,pos,line_number,name,prErrOverflow);
		}
//		body += ESC_CODEN;
//		body += wxT('v');
		body += octhexhed;
		pos += octhexhed.Length();
		body += ESC_CODEN;
		body += wxT('e');
		body += numstr;
		pos += numstr.Length();
		empstr = false;
	} else {
		// 数値NG
		if (result) result->Add(row+1,pos,line_number,name,prErrInvalidNumber);
		empstr = true;
	}
}

// アスキー形式1行を中間言語に変換して出力
bool Parse::ParseAsciiToBinaryOneLine(PsFileType &in_file_type, wxString &in_data, PsFileOutput &out_file, int row, ParseResult *result)
{
	if (in_data.IsEmpty()) {
		return true;
	}

	bool extend_basic = out_file.GetTypeFlag(psExtendBasic);	// DISK BASICか
	int machine_type = out_file.GetMachineType();
	wxString basic_type = out_file.GetBasicType();

	wxString name = _("Ascii->Binary");

	wxUint8 area = 0;
	int linenumber_area = 0;

	wxString chrstr;
	bool is_empty_chr;
	bool has_code_fe = false;

	CodeMapItem *item;

	long line_number;

	wxUint8 buf[8];

	wxUint8 body[VALS_SIZE+1];
	size_t  body_len = 0;

	wxString in_str;
	size_t pos = 0;

	// line number
	line_number = GetLineNumber(in_data, &pos);
	if (line_number < mPrevLineNumber) {
		// 行番号が前行より小さい
		if (result) result->Add(row+1,pos,line_number,name,prErrDiscontLineNumber);
	}
	mPrevLineNumber = line_number;

	// body
	while(pos < in_data.Len()) {
		in_str = in_data.Mid(pos);
		if (in_str[0] == 0x22) {
			// quote
			area = (area & QUOTED_AREA) ? (area & ~QUOTED_AREA) : (area | QUOTED_AREA);

		}
		chrstr.Empty();
		is_empty_chr = true;

		// find command
		if (!(area & (COMMENT_AREA | DATA_AREA | ALLDATA_AREA | QUOTED_AREA))) {
			// search command
			mBasicCodeTbl.FindSectionByType(machine_type);
			item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			if (item == NULL && extend_basic) {
				mBasicCodeTbl.FindSection(basic_type);
				item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			}
			if (item != NULL) {
				// find a BASIC sentence
				wxUint32 attr = item->GetAttr();
				if (attr & CodeMapItem::ATTR_COLON) {
					if (body_len > 0 || machine_type == MACHINE_TYPE_S1) {
						body_len += AscStrToBytes(_T(":"), &body[body_len], sizeof(body)-body_len);
					}
				}

				chrstr = wxString::From8BitData((const char *)item->GetCode(), item->GetCodeLength());
				is_empty_chr = false;

				if (attr & CodeMapItem::ATTR_COMMENT) {
					area |= COMMENT_AREA;
				}
				if (attr & CodeMapItem::ATTR_DATA) {
					// :または行末までDATA
					area |= DATA_AREA;
				} else if (attr & CodeMapItem::ATTR_ALLDATA) {
					// 行末までDATA
					area |= ALLDATA_AREA;
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
				if (attr & CodeMapItem::ATTR_OCTSTRING) {
					// 8進数
					ParseOctHexString(in_str, chrstr, 8, pos, row, line_number, name, reOcta, body, sizeof(body), body_len, is_empty_chr, result);
					chrstr.Empty();
				}
				if (attr & CodeMapItem::ATTR_HEXSTRING) {
					// 16進数
					ParseOctHexString(in_str, chrstr, 16, pos, row, line_number, name, reHexa, body, sizeof(body), body_len, is_empty_chr, result);
					chrstr.Empty();
				}
				if (!chrstr.IsEmpty()) {
					// BASIC中間コードを出力
					body_len += AscStrToBytes(chrstr, &body[body_len], sizeof(body)-body_len);
					pos += item->GetStr().Len();
					continue;
				}
			} else {
				// non BASIC sentence
				if (reAlpha.Matches(in_str.Left(1))) {
					// 変数の場合
					linenumber_area = 0;
					ParseVariableString(in_str, pos, row, line_number, name, reAlphaNumeric, body, sizeof(body), body_len, is_empty_chr, result);
				} else if (machine_type == MACHINE_TYPE_S1 && in_str[0] == 0x2e) {
					// S1モードでピリオドの場合
					if (linenumber_area) {
						// 行番号は指定できないはず
						if (result) result->Add(row+1,pos,line_number,name,prErrInvalidLineNumber);
					} else {
						// 数値を変換
						ParseNumberString(in_str, pos, row, line_number, name, reNumberReald, reNumberExpd, body, sizeof(body), body_len, is_empty_chr, result);
					}
				} else if (machine_type == MACHINE_TYPE_S1 && reNumber.Matches(in_str.Left(1))) {
					// S1モードで数値の場合
					if (linenumber_area) {
						// 行番号の場合
						ParseLineNumberString(in_str, pos, row, line_number, name, reNumber, body, sizeof(body), body_len, is_empty_chr, result);
					} else {
						// 数値を変換
						ParseNumberString(in_str, pos, row, line_number, name, reNumberReal, reNumberExp, body, sizeof(body), body_len, is_empty_chr, result);
					}
					if (linenumber_area == 2) {
						// 最初の数値のみ行番号
						linenumber_area = 0;
					}
				} else if (in_str[0] == 0x3a) {
					// colon
					linenumber_area = 0;
					area &= ~DATA_AREA;
				} else if ((wxUint32)in_str[0] >= 0x80) {
					// unknown char ?(error)
					if (result) result->Add(row+1,pos,line_number,name,prErrInvalidChar);
				}
			}
		} else {
			if (machine_type == MACHINE_TYPE_S1 && in_str[0] == 0xfe) {
				// S1では0xfeの文字は使用できない
				if (result && !has_code_fe) result->Add(row+1,pos,line_number,name,prErrEraseCodeFE);
				has_code_fe = true;
				pos++;
				continue;
			}
			if (in_str[0] == 0x3a) {
				// colon
				area &= ~DATA_AREA;
			}
		}
		if (is_empty_chr) {
			// 元の文字をそのまま入れる
			chrstr = in_str[0];
			body_len += AscStrToBytes(chrstr, &body[body_len], sizeof(body)-body_len);
			pos++;
		}
	}

	mNextAddress += body_len + 5;

	if (out_file.IsOpened()) {
		LongToBytes(mNextAddress, buf, 2);
		LongToBytes(line_number, &buf[2], 2);
		out_file.Write(buf, 4);
		out_file.Write(body, body_len);
		buf[0] = '\0';
		out_file.Write(buf, 1);
	}
	return true;
}


// アスキー形式1行を解析して色付けする
bool Parse::ParseAsciiToColoredOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, const ParseAttr &pattr, int row, ParseResult *result)
{
	if (in_data.IsEmpty()) {
		return true;
	}

	bool extend_basic = out_data.GetTypeFlag(psExtendBasic);	// DISK BASICか
	int machine_type = out_data.GetMachineType();
	wxString basic_type = out_data.GetBasicType();

	bool add_space_colon = pattr.DoesAddSpaceAfterColon();

	wxString name = _("Parse Ascii");

	wxUint8 area = 0;
	int linenumber_area = 0;

	wxString chrstr;
	bool is_empty_chr;
	bool has_code_fe = false;

	CodeMapItem *item;

	long line_number;

	wxString body;

	wxString in_str;
	size_t pos = 0;
	int llen = 0;

	// line number
	line_number = GetLineNumber(in_data, &pos);
	if (line_number < mPrevLineNumber) {
		// 行番号が前行より小さい
		if (result) result->Add(row+1,pos,line_number,name,prErrDiscontLineNumber);
	}
	mPrevLineNumber = line_number;

	body = wxUniChar(ESC_CODEN);
	body += wxT('l');
	body += in_data.Left(pos);
	body += ESC_CODEN;
	body += wxT('e');

	// body
	while(pos < in_data.Len()) {
		in_str = in_data.Mid(pos);
		if (in_str[0] == 0x22) {
			// quote
			if (!(area & QUOTED_AREA)) {
				if (!(area & COMMENT_AREA)) {
					if (add_space_colon) {
						llen = (int)body.Len() - 1;
						if (llen >= 0 && body.GetChar(llen) == ',') {
							body += _T(" ");
						}
					}
					body += ESC_CODEN;
					body += wxT('q');
				}
			}
			body += in_str[0];
			pos++;
			if (area & QUOTED_AREA) {
				if (!(area & COMMENT_AREA)) {
					body += ESC_CODEN;
					body += wxT('e');
				}
				area &= ~QUOTED_AREA;
			} else {
				area |= QUOTED_AREA;
			}
			continue;
		}
		chrstr.Empty();
		is_empty_chr = true;

		// find command
		if (!(area & (COMMENT_AREA | DATA_AREA | ALLDATA_AREA | QUOTED_AREA))) {
			// search command
			mBasicCodeTbl.FindSectionByType(machine_type);
			item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			if (item == NULL && extend_basic) {
				mBasicCodeTbl.FindSection(basic_type);
				item = mBasicCodeTbl.FindByStr(in_str.Upper(), true);
			}
			if (item != NULL) {
				// found a BASIC sentence
				if (add_space_colon) {
					llen = (int)body.Len() - 1;
					if (llen >= 0 && (body.GetChar(llen) == ':' || body.GetChar(llen) == ',')) {
						body += _T(" ");
					}
				}
				wxUint32 attr2 = item->GetAttr2();
				if (attr2 & CodeMapItem::ATTR_COMMENT) {
					if (!(area & COMMENT_AREA)) {
						area |= COMMENT_AREA;
						body += ESC_CODEN;
						body += wxT('c');
					}
				}
				if (attr2 & CodeMapItem::ATTR_DATA) {
					// :または行末までDATA
					if (!(area & DATA_AREA)) {
						area |= DATA_AREA;
						body += ESC_CODEN;
						body += wxT('d');
					}
				} else if (attr2 & CodeMapItem::ATTR_ALLDATA) {
					// 行末までDATA
					if (!(area & ALLDATA_AREA)) {
						area |= ALLDATA_AREA;
						body += ESC_CODEN;
						body += wxT('d');
					}
				}
				if (!(area & (COMMENT_AREA | DATA_AREA | ALLDATA_AREA))) {
					if (!(area & SENTENCE_AREA)) {
						area |= SENTENCE_AREA;
						body += ESC_CODEN;
						body += wxT('v');
					}
				}

				chrstr = in_str.Left(item->GetStr().Len());
				is_empty_chr = false;

				if (linenumber_area && (attr2 & CodeMapItem::ATTR_CONTLINENUMBER) != 0) {
					// 行番号指定は続く
					linenumber_area = 1;
				} else if (linenumber_area && (attr2 & CodeMapItem::ATTR_CONTONELINENUMBER) != 0) {
					// 最初の数値のみ行番号
					linenumber_area = 2;
				} else if ((attr2 & (CodeMapItem::ATTR_CONTONELINENUMBER | CodeMapItem::ATTR_ONELINENUMBER)) == CodeMapItem::ATTR_ONELINENUMBER) {
					// 最初の数値のみ行番号
					linenumber_area = 2;
				} else if ((attr2 & (CodeMapItem::ATTR_CONTLINENUMBER | CodeMapItem::ATTR_ONELINENUMBER | CodeMapItem::ATTR_LINENUMBER)) == CodeMapItem::ATTR_LINENUMBER) {
					// 行番号はその命令内の数値全て
					linenumber_area = 1;
				} else {
					linenumber_area = 0;
				}

				if (attr2 & CodeMapItem::ATTR_OCTSTRING) {
					// 8進数
					ParseOctHexString(in_str, chrstr, 8, pos, row, line_number, name, reOcta, body, is_empty_chr, result);
					chrstr.Empty();
					area &= ~SENTENCE_AREA;
				}
				if (attr2 & CodeMapItem::ATTR_HEXSTRING) {
					// 16進数
					ParseOctHexString(in_str, chrstr, 16, pos, row, line_number, name, reHexa, body, is_empty_chr, result);
					chrstr.Empty();
					area &= ~SENTENCE_AREA;
				}
				if (!chrstr.IsEmpty()) {
					// BASIC中間コードを出力
					body += chrstr;
					pos += item->GetStr().Len();
				}
				continue;
			} else {
				// non BASIC sentence
				if (area & SENTENCE_AREA) {
					body += ESC_CODEN;
					body += wxT('e');
					area &= ~SENTENCE_AREA;
				}
				if (add_space_colon) {
					llen = (int)body.Len() - 1;
					if (llen >= 0 && (body.GetChar(llen) == ':' || body.GetChar(llen) == ',')) {
						body += _T(" ");
					}
				}
				if (reAlpha.Matches(in_str.Left(1))) {
					// 変数の場合
					linenumber_area = 0;
					ParseVariableString(in_str, pos, row, line_number, name, reAlphaNumeric, body, is_empty_chr, result);
				} else if (in_str[0] == 0x2e) {
					// S1モードでピリオドの場合
					if (linenumber_area) {
						// 行番号は指定できないはず
						if (result) result->Add(row+1,pos,line_number,name,prErrInvalidLineNumber);
					} else {
						// 数値を変換
						ParseNumberString(in_str, pos, row, line_number, name, reNumberReald, reNumberExpd, body, is_empty_chr, result);
					}
				} else if (reNumber.Matches(in_str.Left(1))) {
					// S1モードで数値の場合
					if (linenumber_area) {
						// 行番号の場合
						ParseLineNumberString(in_str, pos, row, line_number, name, reNumber, body, is_empty_chr, result);
					} else {
						// 数値を変換
						ParseNumberString(in_str, pos, row, line_number, name, reNumberReal, reNumberExp, body, is_empty_chr, result);
					}
					if (linenumber_area == 2) {
						// 最初の数値のみ行番号
						linenumber_area = 0;
					}
				} else if (in_str[0] == 0x3a) {
					// colon
					linenumber_area = 0;
					if (area & DATA_AREA) {
						area &= ~DATA_AREA;
						body += ESC_CODEN;
						body += wxT('e');
					}
					body += in_str[0];
					pos++;
					continue;
				} else if ((wxUint32)in_str[0] >= 0x80) {
					// unknown char ?(error)
					if (result) result->Add(row+1,pos,line_number,name,prErrInvalidChar);
				}
			}
		} else {
			if (machine_type == MACHINE_TYPE_S1 && in_str[0] == 0xfe) {
				// S1では0xfeの文字は使用できない
				if (result && !has_code_fe) result->Add(row+1,pos,line_number,name,prErrEraseCodeFE);
				has_code_fe = true;
				pos++;
				continue;
			}
			if (add_space_colon && (area & (COMMENT_AREA | QUOTED_AREA)) == 0) {
				llen = (int)body.Len() - 1;
				if (llen >= 0 && body.GetChar(llen) == ',') {
					body += _T(" ");
				}
			}
			if (in_str[0] == 0x3a) {
				// colon
				if (area & DATA_AREA) {
					area &= ~DATA_AREA;
					body += ESC_CODEN;
					body += wxT('e');
					body += in_str[0];
					pos++;
					continue;
				}
			}
		}
		if (is_empty_chr) {
			// 元の文字をそのまま入れる
			body += in_str[0];
			pos++;
		}
	}

//	mNextAddress += body_len + 5;
	while(area) {
		if (area & 1) {
			body += ESC_CODEN;
			body += wxT('e');
		}
		area >>= 1;
	}
	out_data.Add(body);

	return true;
}


/// アスキー形式テキストを読む
/// @return 2:7bit文字のみ 1: utf8に変換できる 0: utf8に変換できない
int Parse::ReadAsciiText(PsFileInput &in_data, PsFileData &out_data, bool to_utf8)
{
	int rc = 2;	// utf8に変換できない場合 0

	wxUint8 vals[VALS_SIZE + 1];
	size_t  valpos = VALS_SIZE;
	size_t  valsta = VALS_SIZE;
	size_t  vallen = VALS_SIZE;
	size_t  valend = VALS_SIZE;
	vals[VALS_SIZE] = 0;

	wxString body;

	int lf_len = 0;

	int phase = 0;

	memset((void *)vals, 0, sizeof(vals));
//	inlines.Add(_T(""));

	while(phase >= 0) {
		switch(phase) {
		case 0:
			if (valsta > 0) {
				// shift
				for(size_t i=0; i<(vallen + 1 - valsta); i++)
					vals[i] = vals[i + valsta];
				// read
				size_t vlen = valend - valsta;
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
			phase = 1;
			break;

		case 1:
			if (valpos >= valend) {
				// end of line
				phase = 2;
				break;
			} else if (vals[valpos] == '\r' && vals[valpos+1] == '\n') {
				// 改行
				lf_len = 2;
				phase = 2;
				break;
			} else if (vals[valpos] == '\r' || vals[valpos] == '\n') {
				// 改行
				lf_len = 1;
				phase = 2;
				break;
			} else if (mEofTextRead && vals[valpos] == 0x1a) {
				// ファイル終端コード
				lf_len = 1;
				phase = -1;
				break;
			} else if (rc > 1 && vals[valpos] >= 0x80) {
				// Nonアスキー文字
				rc = 1;
			}

			valpos++;

			if (valpos + 32 > vallen) {
				// continue
				phase = 0;
			}
			break;

		case 2:
			// 1行出力
			vals[valpos] = 0;

			if (valsta < valpos) {	// 空行は省く
				if (to_utf8) {
					// utf8に変換を試みる
					body = wxString((const char *)&vals[valsta], wxConvUTF8);
					if (body.IsEmpty()) {
						// utf8に変換できない
						rc = 0;
						body = wxString((const char *)&vals[valsta], wxConvISO8859_1);
					}
				} else {
					body = wxString((const char *)&vals[valsta], wxConvISO8859_1);
				}
				// trim
				body.Trim(true).Trim(false);
				// add line
				out_data.Add(body);
			}
			if (valpos >= valend) {
				// end of line
				phase = -1;
			} else {
				// continue
				phase = 1;
				valpos += lf_len;
			}
			valsta = valpos;
			lf_len = 0;
			break;
		}
	}


	out_data.SetType(in_data.GetType());

	return rc;
}
/// テキストを出力
bool Parse::WriteText(PsFileData &in_data, PsFileOutput &out_file)
{
	// UTF-8に変換するか
	bool utf8_mode = out_file.GetTypeFlag(psUTF8);
	wxArrayString *inlines = &in_data.GetData();

	if (!out_file.IsOpened()) {
		return true;
	}

	if (!utf8_mode) {
		// テープイメージの場合CR固定。ディスクイメージの場合CR+LF固定
		int nl = (out_file.GetTypeFlag(psTapeImage) ? 0 : (out_file.GetTypeFlag(psDiskImage) ? 2 : mNewLineAscii));
//		size_t len = 0;
		if (inlines->GetCount() > 0 && !inlines->Item(0).IsEmpty()) {
			out_file.Write(nl_chr[nl]); // 1行目は必ず改行
		}
		for(size_t row = 0; row < inlines->GetCount(); row++) {
			out_file.Write(inlines->Item(row));	// 変換しない
			out_file.Write(nl_chr[nl]); // 改行
		
//			len = WriteAsciiString(len, inlines->Item(row), out_data);	// 変換しない
//			len = WriteAsciiString(len, nl_chr[nl], out_data);	// 改行
		}
		// ファイル終端コードを出力
		if ((!out_file.GetTypeFlag(psTapeImage) && mEofAscii)) {
			out_file.Write((const wxUint8 *)EOF_CODE, 1);
		}
	} else {
		if (out_file.GetTypeFlag(psUTF8BOM)) {
			out_file.Write((const wxUint8 *)BOM_CODE, 3); // BOM
		}

		for(size_t row = 0; row < inlines->GetCount(); row++) {
			out_file.WriteUTF8(inlines->Item(row));	// UTF-8に変換して出力
			out_file.Write(nl_chr[mNewLineUtf8]);	// 改行
		}
	}
	return true;
}

// アスキー文字列を出力
size_t Parse::WriteAsciiString(size_t len, const wxString &in_line, wxFile *out_data)
{
	out_data->Write(in_line, wxConvISO8859_1);
	len += in_line.Len();
	return len;
}

// アスキー形式からUTF-8テキストに変換
bool Parse::ConvAsciiToUTF8(PsFileData &in_data, PsFileData *out_data, ParseResult *result)
{
	// 変換タイプ設定
	wxString out_char_type;
	if (out_data->GetTypeFlag(psUTF8)) {
		// インターレースなどのUTF-8文字
		out_char_type = out_data->GetCharType();
	}

	wxString name = _("Ascii->UTF8");

	int error_count = 0;
	bool stopped = false;

	wxString body;

	size_t row = 0;

	for(row = 0; row < in_data.GetCount(); row++) {
		if (error_count > 100) {
			stopped = true;
			break;
		}
		error_count += ConvAsciiToUTF8OneLine(row, in_data[row], out_char_type, body, result);

		if (out_data) out_data->Add(body);
	}
	if (stopped) {
		if (result) {
			result->Add(row+1,1,GetLineNumber(in_data[row]),name,prErrStopInvalidToUTF8Code);
		}
		return false;
	}
	return true;
}

// アスキー形式1行からUTF-8テキストに変換
int Parse::ConvAsciiToUTF8OneLine(size_t row, const wxString &in_line, const wxString &out_type, wxString &out_line, ParseResult *result)
{
	// 変換タイプ設定
	bool through_mode = true;	// 半カナや記号をUTF-8に変換しないか

	if (!out_type.IsEmpty()) {
		// インターレースなどのUTF-8文字
		through_mode = false;
		mCharCodeTbl.FindSection(out_type);
	}

	wxString name = _("Ascii->UTF8");

	int error_count = 0;

	wxUint8 val[4];
	wxString body;
	wxString chrstr;
	CodeMapItem *item;

	out_line.Empty();
	for(size_t col = 0; col < in_line.Len();) {
		chrstr.Empty();
		// バイト列に変換
		AscStrToBytes(in_line.Mid(col), val, sizeof(val));
		if (!through_mode) {
			// ESC CODEのとき
			if (memcmp(val, ESC_CODE, 1) == 0) {
				chrstr = in_line.Mid(col, 2);
				col += 2;
			}
			// キャラクターコードをUTF-8に変換
			item = mCharCodeTbl.FindByCode(val,CodeMapItem::ATTR_HIGHER,false);
			if (item != NULL) {
				chrstr = item->GetStr();
				col += item->GetCodeLength();
			} else {
				if (val[0] >= 0x80) {
					if (result) {
						result->Add(row+1,col+1,GetLineNumber(in_line),name,prErrInvalidToUTF8Code);
					}
					error_count++;
				}
			}
		}
		if (chrstr.IsEmpty()) {
			chrstr = wxString(val, wxConvISO8859_1, 1);
			col++;
		}
		out_line += chrstr;
	}

	return error_count;
}

// UTF-8テキストからアスキー形式に変換
bool Parse::ConvUTF8ToAscii(PsFileData &in_data, PsFileData *out_data, ParseResult *result)
{
	// 変換タイプ設定
	wxString in_char_type;
	if (in_data.GetTypeFlag(psUTF8)) {
		// インターレースなどのUTF-8文字
		in_char_type = in_data.GetCharType();
	}

	wxString name = _("UTF8->Ascii");

	wxString body;

	int error_count = 0;
	bool stopped = false;

	size_t row = 0;

	for(row = 0; row < in_data.GetCount(); row++) {
		if (error_count > 100) {
			stopped = true;
			break;
		}
		error_count += ConvUTF8ToAsciiOneLine(in_char_type, row, in_data[row], body, result);

		if (out_data) out_data->Add(body);
	}
	if (stopped) {
		if (result) {
			result->Add(row+1,1,GetLineNumber(in_data[row]),name,prErrStopInvalidUTF8Code);
		}
		return false;
	}

	return true;

}

// UTF-8テキスト１行からアスキー形式に変換
int Parse::ConvUTF8ToAsciiOneLine(const wxString &in_type, size_t row, const wxString &in_line, wxString &out_line, ParseResult *result)
{
	// 変換タイプ設定
	bool through_mode = true;	// UTF-8から半カナや記号に変換しないか
	if (!in_type.IsEmpty()) {
		// インターレースなどのUTF-8文字
		through_mode = false;
		mCharCodeTbl.FindSection(in_type);
	}

	wxString name = _("UTF8->Ascii");

	wxCharBuffer vals;
	size_t vallen;
	wxString chrstr;
	CodeMapItem *item;

	int error_count = 0;

	size_t col = 0;

	out_line.Empty();
	vals = in_line.mb_str(wxConvUTF8);
	vallen = strlen(vals);
	for(col = 0; col < vallen;) {
		if (vals[col] < 0) { // upper 0x80
			// UTF-8 code?
			if ((wxUint8)vals[col] == 0xef && (wxUint8)vals[col+1] == 0xbb && (wxUint8)vals[col+2] == 0xbf) {
				// BOM?
				col += 3;
			} else {
				chrstr.Empty();
				if (!through_mode) {
					// UTF-8をキャラクターコードに変換
					item = mCharCodeTbl.FindByBytes((const wxUint8 *)((const char *)vals+col),CodeMapItem::ATTR_LOWER,false);
					if (item != NULL) {
						switch(item->GetFlags()) {
						case 1:
							// UTF-8 -> SJIS変換できる
							chrstr = wxString::From8BitData((const char *)item->GetBytes(), item->GetBytesLength());
							col += item->GetCodeLength();
							break;
						default:
							chrstr = wxString::From8BitData((const char *)item->GetCode(), item->GetCodeLength());
							col += item->GetBytesLength();
							break;
						}
					} else {
						// cannot convert
						if (result) {
							result->Add(row+1,col+1,GetLineNumber(in_line),name,prErrInvalidUTF8Code);
						}
						error_count++;
					}
				}
				if (chrstr.IsEmpty()) {
					chrstr = wxString::From8BitData((const char *)vals + col, 1);
					col++;
				}
				out_line += chrstr;
			}
		} else {
			// ascii char
			chrstr = wxString::From8BitData((const char *)vals + col, 1);
			col++;
			out_line += chrstr;
		}
	}

	return error_count;
}

/// 文字コード変換テーブルの読み込み
PsErrType Parse::LoadCharCodeTable()
{
	wxTextFile file;
	wxFileName filename(mAppPath + DATA_DIR, CHAR_CODE_TABLE);
	wxString path = filename.GetFullPath();

	if (!file.Open(path)) {
		mErrInfo.SetInfo(__LINE__, psError, psErrCannotOpen, psInfoFileInApp, CHAR_CODE_TABLE);
		mErrInfo.ShowMsgBox();
		return psError;
	}

	wxString line;
	for ( line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine() ) {
		// skip if comment line or empty
		line.Trim(false).Trim(true);
		if ( line.IsEmpty() || line[0] == wxChar('#') ) {
			continue;
		}

		// charset type code
		wxRegEx re(_T("^\\[(.+)\\]$"));
		if (re.Matches(line)) {
			wxString mstr = re.GetMatch(line, 1);
			mCharCodeTbl.AddSection(mstr, 0);
			continue;
		}

		int end_pos = line.Find(wxChar(','));
		wxString new_code_str;
		if (end_pos == wxNOT_FOUND) {
			new_code_str = line;
		} else {
			new_code_str = line.Left(end_pos);
		}
		new_code_str.Trim(true).Trim(false);
		if (new_code_str.Left(1) == wxT("*")) {
			// special function
			if (new_code_str.Left(5) == wxT("*SJIS")) {
				mCharCodeTbl.AddItem((const wxUint8 *)"SJIS", 4, wxEmptyString, wxEmptyString, wxEmptyString, 1);
			}

		} else {
			// add UTF-8 <-> CHR code item
			wxUint8 new_code[4];
			size_t new_code_len = HexStrToBytes(new_code_str, new_code, sizeof(new_code));
			line = line.Mid(end_pos+1);

			end_pos = line.Find(wxChar(','));
			if (end_pos == wxNOT_FOUND) {
				end_pos = (int)line.Len();
			}
			wxString new_str = line.Left(end_pos);
			int sep_st_pos = new_str.Find(wxChar('\''), false);
			int sep_ed_pos = new_str.Find(wxChar('\''), true);
			if (sep_st_pos != wxNOT_FOUND && sep_ed_pos != wxNOT_FOUND) {
				new_str = new_str.SubString(sep_st_pos+1, sep_ed_pos-1);
			} else {
				new_str.Empty();
			}
			line = line.Mid(end_pos+1);

			wxString new_attr = line;
			new_attr.Trim(true).Trim(false);

			mCharCodeTbl.AddItem(new_code, new_code_len, new_str, new_attr);
		}
	}

	file.Close();
	return psOK;
}

/// BASICコード変換テーブルの読み込み
PsErrType Parse::LoadBasicCodeTable()
{
	wxTextFile file;
	wxFileName filename(mAppPath + DATA_DIR, BASIC_CODE_TABLE);
	wxString path = filename.GetFullPath();

	if (!file.Open(path)) {
		mErrInfo.SetInfo(__LINE__, psError, psErrCannotOpen, psInfoFileInApp, BASIC_CODE_TABLE);
		mErrInfo.ShowMsgBox();
		return psError;
	}

	wxString line;
	for ( line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine() ) {
		// skip if comment line or empty
		line.Trim(false).Trim(true);
		if ( line.IsEmpty() || line[0] == wxChar('#') ) {
			continue;
		}

		// basic type
		wxRegEx re(_T("^\\[(.*)\\]$"));
		if (re.Matches(line)) {
			wxString mstr = re.GetMatch(line, 1);
			int machine_type = 0;
			SET_MACHINE_TYPE(mstr, machine_type);
			mBasicCodeTbl.AddSection(mstr, machine_type);
			continue;
		}

		int end_pos = line.Find(wxChar(','));
		if (end_pos == wxNOT_FOUND) {
			continue;
		}
		wxString new_code_str = line.Left(end_pos);
		new_code_str.Trim(true).Trim(false);
		wxUint8 new_code[4];
		size_t new_code_len = HexStrToBytes(new_code_str, new_code, sizeof(new_code));
		line = line.Mid(end_pos+1);

		end_pos = line.Find(wxChar(','));
		if (end_pos == wxNOT_FOUND) {
			end_pos = (int)line.Len();
		}
		wxString new_str = line.Left(end_pos);
		new_str.Trim(true).Trim(false);
		line = line.Mid(end_pos+1);

		end_pos = line.Find(wxChar(','));
		if (end_pos == wxNOT_FOUND) {
			end_pos = (int)line.Len();
		}
		wxString new_attr = line.Left(end_pos);
		new_attr.Trim(true).Trim(false);
		line = line.Mid(end_pos+1);

		wxString new_attr2 = line;
		new_attr2.Trim(true).Trim(false);
		if (new_attr2.Len() == 0) {
			new_attr2 = new_attr;
		}

		mBasicCodeTbl.AddItem(new_code, new_code_len, new_str, new_attr, new_attr2);
	}

	file.Close();
	return psOK;
}

/// 文字種類を返す
void Parse::GetCharTypes(wxArrayString &char_types)
{
	mCharCodeTbl.GetAllSectionNames(char_types);
}
const wxString &Parse::GetCharType(size_t index) const
{
	return mCharCodeTbl.GetSectionName(index);
}

/// BASIC種類を返す
void Parse::GetBasicTypes(wxArrayString &basic_types)
{
	mBasicCodeTbl.GetAllSectionNames(basic_types);
}

// BASIC種類をセット
//void Parse::SetBasicType(const wxString &basic_type)
//{
//	mInFile.type.SetBasicType(basic_type);
//	mOutFile.type.SetBasicType(basic_type);
//	if (!basic_type.IsEmpty()) {
//		mInFile.type.SetFlag(psExtendBasic, true);
//		mOutFile.type.SetFlag(psExtendBasic, true);
//	}
//}

/// 入力ファイルのBASIC種類を返す
wxString &Parse::GetOpenedBasicType()
{
	return mInFile.GetBasicType();
}

/// 画面表示用データを返す
wxString &Parse::GetParsedData(wxArrayString &lines)
{
	for(size_t i=0; i<mParsedData.GetCount(); i++) {
		lines.Add(mParsedData[i]);
	}
	return mParsedData.GetCharType();
}

/// スタートアドレスのリストへのポインタを返す
int *Parse::GetStartAddrsPtr(size_t *count)
{
	if (count) *count = sizeof(iStartAddr) / sizeof(int);
	return iStartAddr;
}

/// 行番号を返す
long Parse::GetLineNumber(const wxString &line, size_t *next_pos)
{
	long num = 0;

	// get line number
	int pos = line.Find(wxChar(' '));
	if (pos != wxNOT_FOUND) {
		line.Left(pos).ToLong(&num);
		if (next_pos) *next_pos = pos + 1;
	} else {
		wxRegEx re(_T("^\\d+"));
		if (re.Matches(line)) {
			wxString sub = re.GetMatch(line);
			sub.ToLong(&num);
			if (next_pos) *next_pos = sub.Len();
		}
	}
	return num;
}

/// レポート
void Parse::Report(ParseResult &result, wxArrayString &line)
{
	wxArrayString arr;
	result.Report(arr);
	if (arr.GetCount() == 0) return;

	line.Add(_("----- Result Report -----"));
	for(size_t i=0; i<arr.GetCount(); i++) {
		line.Add(arr[i]);
	}
	line.Add(_("-------------------------"));
	line.Add(_T(""));
}

/// 16進文字列をバイト列(可変)に変換(big endien)
/// @param[in]  src     16進文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @return     書き込んだ長さ
int Parse::HexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len)
{
	unsigned long val;

	if (src.ToULong(&val, 16) != true) {
		return 0;
	}

	size_t len;
	for(len = 3; len > 0; len--) {
		if (val >= (unsigned long)(1 << (len << 3))) {
			break;
		}
	}

	memset(dst, 0, dst_len);
	for(size_t i=0; i < dst_len && i <= len; i++) {
		dst[i] = (val >> ((len - i) << 3)) & 0xff;
	}

	return (int)(len + 1);
}

/// アスキー文字列をバイト列に変換
/// @param[in]  src     アスキー文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @return     書き込んだ長さ
int Parse::AscStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len)
{
	size_t len;
	wxChar c;

	memset(dst, 0, dst_len);
	for(len=0; len < dst_len && len < src.Len(); len++) {
		c = src[len];
		dst[len] = (wxUint8)c;
	}

	return (int)len;
}

/// 整数バイト列を数値に変換
/// @param[in]  src     整数バイト列
/// @param[in]  src_len srcの長さ
/// @return     数値
long Parse::BytesToLong(const wxUint8 *src, size_t src_len)
{
	long num = 0;

	src_len--;
	if (src_len > 3) src_len = 3;

	if (mBigEndian) {
		for(int i=0; i<=(int)src_len; i++) {
			num <<= 8;
			num += src[i];
		}
	} else {
		for(int i=(int)src_len; i>=0; i--) {
			num <<= 8;
			num += src[i];
		}
	}
	return num;
}

/// 整数バイト列を文字列に変換
/// @param[in]  src     整数バイト列
/// @param[in]  src_len srcの長さ
/// @param[out] dst     文字列
/// @return     true
bool Parse::BytesToStr(const wxUint8 *src, size_t src_len, wxString &dst)
{
	long num = BytesToLong(src, src_len);
	dst = wxString::Format("%ld", num);
	return true;
}

/// 浮動小数点バイト列を文字列に変換
/// @param[in]  src     浮動小数点バイト列
/// @param[in]  src_len srcの長さ
/// @param[out] dst     文字列
/// @return     true
bool Parse::FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst)
{
	UINT192 cin;
	UINT192 cpo;
	DeciStr decs;

	L3Float::RealStrToUint192(src, (int)src_len, cin, cpo);
	L3Float::Uint192ToDeciStr(cin, cpo, src_len > 4 ? 16 : 6, decs);

	dst = wxString::From8BitData(decs.GetStr(0), decs.Length());
	return true;
}

/// 数値を整数バイト列に変換
/// @param[in]  src     数値
/// @param[out] dst     整数バイト列
/// @param[in]  dst_len dstの長さ
/// @return     true
bool Parse::LongToBytes(long src, wxUint8 *dst, size_t dst_len)
{
	dst_len--;
	if (dst_len > 3) dst_len = 3;

	if (mBigEndian) {
		for(int i=(int)dst_len; i>=0; i--) {
			dst[i] = (src & 0xff);
			src >>= 8;
		}
	} else {
		for(int i=0; i<=(int)dst_len; i++) {
			dst[i] = (src & 0xff);
			src >>= 8;
		}
	}
	return true;
}

/// 数値文字列をバイト列に変換
/// @param[in]  src     数値文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ
int Parse::NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err)
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

/// 8/16進文字列が範囲内か
/// @param[in]  base    8 or 16
/// @param[in]  src     8/16進文字列
/// @return     false:オーバーフロー
bool Parse::CheckOctHexStr(int base, const wxString &src)
{
	long num;
	if (!src.ToLong(&num, base)) {
		return false;
	}
	if (num < 0 || 65535 < num) {
		// overflow
		return false;
	}
	return true;
}

/// 行番号文字列をバイト列に変換
/// @param[in]  src     行番号(数値)文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ 0:上記以外のエラーで書き込めない
int Parse::LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err)
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
