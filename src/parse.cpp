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
#include "bsstring.h"
#include "main.h"

#define DATA_DIR _T("data")

/// 改行コード
const char *Parse::cNLChr[] = {
	"\r", "\n", "\r\n", NULL
};

#if 0
//////////////////////////////////////////////////////////////////////

ParseAttr::ParseAttr()
{
	mAddSpaceAfterColon = false;
}

ParseAttr::~ParseAttr()
{
}
#endif

//////////////////////////////////////////////////////////////////////
///
/// パーサークラス
///
Parse::Parse(ParseCollection *collection, ConfigParam *config)
{
	pColl = collection;
	pConfig = config;

	mNextAddress = 0;
	mPrevLineNumber = -1;
	mMachineType = 0;
}

Parse::~Parse()
{
}

/// 初期化
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

	// スタートアドレスが範囲外ならクリア
	if (pConfig->GetStartAddr() >= GetStartAddrCount()) {
		pConfig->SetStartAddr(0);
	}
	if (!pConfig->IsLoaded()) {
		// 初期設定値をセット
		SetDefaultConfigParam();
	}

	return true;
}

/// 指定したファイルを開く
/// @param[in] in_file_name 入力ファイルのパス
/// @param[in] file_type 入力ファイルの種類
/// @return true/false
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

/// 中間言語形式データのフォーマットチェック
/// @param[in] in_data 入力ファイル
/// @return true/false
bool Parse::CheckBinaryDataFormat(PsFileInput &in_data)
{
	PsFileData    out_data;
	ParseResult   result;
	wxArrayString basic_types;
//	ParseAttr     pattr;
	bool st = true;

//	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	in_data.SetTypeFlag(psAscii, false);

	// ファイルを解析する
	GetBasicTypes(basic_types);
	out_data.SetTypeFlag(psAscii, true);
	for(size_t i=0; i < basic_types.GetCount(); i++) {
		in_data.SeekStartPos();
		in_data.SetMachineAndBasicType(GetMachineType(basic_types[i]), basic_types[i], IsExtendedBasic(basic_types[i]));
		out_data.SetMachineAndBasicType(GetMachineType(basic_types[i]), basic_types[i], IsExtendedBasic(basic_types[i]));
		out_data.Empty();
		result.Empty();
		st = ReadBinaryToAsciiColored(in_data, out_data, &result);
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
/// @param[in] in_data 入力ファイル
/// @return true/false
bool Parse::CheckAsciiDataFormat(PsFileInput &in_data)
{
	PsFileData    out_data;
	PsFileData    tmp_data;
	ParseResult   result;
//	ParseAttr     pattr;
	bool st = false;
	int  sti;

//	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

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
		st = ParseAsciiToColored(tmp_data, out_data, &result);
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
		st = ParseAsciiToColored(tmp_data, out_data, &result);
		Report(result, mParsedData.GetData());
		// interace 文字に変換する
		mParsedData.SetTypeFlag(psAscii | psUTF8, true);
		mParsedData.SetCharType(GetCharType(0));
		st = ConvAsciiToUTF8(out_data, &mParsedData);
	}

	return true;
}

/// 入力ファイルの形式を変更して読み直す
/// @param[in] type 入力ファイルの形式(セットするフラグ)
/// @param[in] mask 入力ファイルの形式(クリアするフラグ)
/// @param[in] char_type 文字種類
/// @param[in] basic_type BASIC種類
/// @return true/false
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
		mInFile.SetMachineAndBasicType(GetMachineType(basic_type), basic_type, IsExtendedBasic(basic_type));
	}
	return ReloadAsciiData(mInFile, out_type);
}

#if 0
/// 表示用データの形式を変更して読み直す
/// @param[in] type 入力ファイルの形式(セットするフラグ)
/// @param[in] mask 入力ファイルの形式(クリアするフラグ)
/// @param[in] char_type 文字種類
/// @param[in] basic_type BASIC種類
/// @return true/false
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
		out_type.SetMachineAndBasicType(basic_type);
	}
	return ReloadAsciiData(mInFile, out_type);
}
#endif

/// 表示用データの形式を変更して読み直す
/// @param[in] type 入力ファイルの形式(セットするフラグ)
/// @param[in] mask 入力ファイルの形式(クリアするフラグ)
/// @param[in] char_type 文字種類
/// @param[in] basic_type BASIC種類
/// @return true/false
bool Parse::ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type)
{
	PsFileType out_type(mParsedData.GetType());

	out_type.SetTypeFlag(mask, false);
	out_type.SetTypeFlag(type, true);
	if (char_type != wxEmptyString) {
		out_type.SetCharType(char_type);
	}

	if (!mInFile.GetTypeFlag(psAscii)) {
		wxString type = mInFile.GetBasicType();
		out_type.SetMachineAndBasicType(GetMachineType(type), type, IsExtendedBasic(type));
	}
	if (basic_type != wxEmptyString) {
		out_type.SetMachineAndBasicType(GetMachineType(basic_type), basic_type, IsExtendedBasic(basic_type));
	}
	return ReloadParsedData(out_type);
}

/// 表示用データの形式を変更して読み直す
/// @param[in,out] out_type 出力データの形式
/// @return true/false
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

/// 中間言語形式データの読み直し
/// @param[in] type 入力ファイルの形式(セットするフラグ)
/// @param[in] mask 入力ファイルの形式(クリアするフラグ)
/// @param[in] basic_type BASIC種類
/// @return true/false
bool Parse::ReloadOpenedBinaryData(int type, int mask, const wxString &basic_type)
{
	if (!mInFile.Exist() || mInFile.GetTypeFlag(psAscii)) {
		// データがないかASCII形式の場合
		return false;
	}

	PsFileType out_type(mParsedData.GetType());

	mInFile.SetTypeFlag(mask, false);
	mInFile.SetTypeFlag(type, true);
	mInFile.SetMachineAndBasicType(GetMachineType(basic_type), basic_type, IsExtendedBasic(basic_type));
	out_type.SetMachineAndBasicType(GetMachineType(basic_type), basic_type, IsExtendedBasic(basic_type));

	return ReloadBinaryData(mInFile, out_type);
}

/// 中間言語形式データの読み直し
/// @param[in]     in_file  入力データの形式
/// @param[in,out] out_type 出力データの形式
/// @return true/false
bool Parse::ReloadBinaryData(PsFileInput &in_file, PsFileType &out_type)
{
	PsFileData out_data;
	ParseResult result;
//	ParseAttr pattr;

//	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

	out_data.Empty();
	mParsedData.Empty();
	result.Empty();

	out_data.SetType(out_type);
	in_file.SeekStartPos();
	if (in_file.GetBasicType() != out_type.GetBasicType()) {
		// 違うBASICのとき
		wxString basic_type = in_file.GetBasicType();
		// まず、ASCIIテキストにする
		PsFileData tmp_data;
		tmp_data.SetTypeFlag(psAscii, true);
		tmp_data.SetMachineAndBasicType(GetMachineType(basic_type), basic_type, IsExtendedBasic(basic_type));
		ReadBinaryToAscii(in_file, tmp_data, &result);
		// 色付け
		ParseAsciiToColored(tmp_data, out_data, &result);
		Report(result, mParsedData.GetData());
	} else {
		// 同じBASICのとき
		// 解析＆色付け
		ReadBinaryToAsciiColored(in_file, out_data, &result);
		Report(result, mParsedData.GetData());
	}
	// interace or noninterace 文字に変換する
	mParsedData.SetType(out_type);
	ConvAsciiToUTF8(out_data, &mParsedData);

	return true;
}

/// アスキー形式データの読み直し
/// @param[in]     in_file  入力データの形式
/// @param[in,out] out_type 出力データの形式
/// @return true/false
bool Parse::ReloadAsciiData(PsFileInput &in_file, PsFileType &out_type)
{
	PsFileData  tmp_data;
	PsFileData  in_data;
	PsFileData  out_data;
	ParseResult result;
//	ParseAttr   pattr;

//	pattr.DoesAddSpaceAfterColon(gConfig.DoesAddSpaceAfterColon());

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
		ParseAsciiToColored(tmp_data, out_data, &result);
		Report(result, mParsedData.GetData());
	} else {
		// ASCIIテキスト
		ReadAsciiText(in_file, in_data, false);
		out_data.SetType(out_type);
		ParseAsciiToColored(in_data, out_data, &result);
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

/// 開いているファイル種類
const PsFileType *Parse::GetOpenedDataTypePtr() const
{
	return (mInFile.Exist() ? &mInFile.GetType() : NULL);
}

/// 開いているファイル情報
PsFileInfo *Parse::GetOpenedDataInfoPtr()
{
	return &mInFile;
}

/// ファイル名フルパス
wxString Parse::GetFileFullPath() const
{
	return mInFile.GetFileFullPath();
}

/// ファイル名
wxString Parse::GetFileNameBase() const
{
	return mInFile.GetFileName();
}

/// 出力ファイルを開く
/// @param[in] out_file_name  出力ファイルのパス
/// @param[in] file_type      出力データの形式
/// @return true/false
/// @note mOutFile: 出力ファイル
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
		mOutFile.SetTypeFlag(psUTF8BOM, pConfig->GetIncludeBOM());
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
			PsFileData tmp_data;
			ReadAsciiText(mInFile, in_data, true);
			tmp_data.SetType(in_data.GetType());
			st = ConvUTF8ToAscii(in_data, &tmp_data, &result);
			out_data.SetTypeFlag(psAscii, true);
			st = ParseAsciiToBinary(tmp_data, out_data, &result);
			st = WriteBinary(out_data, out_file);
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
			st = ParseAsciiToBinary(in_data, out_data, &result);
			st = WriteBinary(out_data, out_file);
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
			st = ParseAsciiToBinary(in_data, out_data, &result);
			st = WriteBinary(out_data, out_file);
		}
	}

	// ファイルに出力
	PsFileStrInput in_file(out_file);
	PsFileFsOutput out(mOutFile.GetFile());
	out.SetType(mOutFile.GetType());
	if (mOutFile.GetTypeFlag(psTapeImage)) {
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
/// @param[in]  in_file  入力ファイル
/// @param[out] out_data 変換後データ
/// @param[in,out] result 結果格納用
/// @return true/false
bool Parse::ReadBinaryToAscii(PsFileInput &in_file, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Binary->Ascii"));

	PsSymbolSentence sentence;

	int phase = PHASE_LINE_NUMBER;

	// parse start

	while(!in_file.Eof() && phase >= PHASE_NONE) {
		phase = ReadBinaryToSymbolsOneLine(in_file, out_data, phase, sentence, result);
		if (phase == PHASE_EOL) {
			// end of line
			out_data.Add(sentence.JoinAscStr());

			// next phase
			sentence.Empty();
			phase = PHASE_LINE_NUMBER;
		}
		if (result && result->GetCount() > ERROR_STOPPED_COUNT) {
			// エラーが多いので中止
			phase = PHASE_STOPPED;
			break;
		}
	}
	if (phase == PHASE_STOPPED) {
		if (result) {
			result->Add(mPos, prErrStopInvalidBasicCode);
		}
		return false;
	}
	return true;
}

/// 中間言語からアスキー形式色付きテキストに変換
/// @param[in]  in_file  入力ファイル
/// @param[out] out_data 変換後データ
/// @param[in,out] result 結果格納用
/// @return true/false
bool Parse::ReadBinaryToAsciiColored(PsFileInput &in_file, PsFileData &out_data, ParseResult *result)
{
	mPos.Empty();
	mPos.SetName(_("Parse Binary"));

	PsSymbolSentence sentence;

	int phase = PHASE_LINE_NUMBER;

	// parse start

	while(!in_file.Eof() && phase >= PHASE_NONE) {
		phase = ReadBinaryToSymbolsOneLine(in_file, out_data, phase, sentence, result);
		if (phase == PHASE_EOL) {
			// end of line
			wxString body;
			DecorateSentenceToColored(sentence, body, pConfig->EnableAddSpaceAfterColon());
			out_data.Add(body);

			// next phase
			sentence.Empty();
			phase = PHASE_LINE_NUMBER;
		}
		if (result && result->GetCount() > ERROR_STOPPED_COUNT) {
			// エラーが多いので中止
			phase = PHASE_STOPPED;
			break;
		}
	}

	if (phase == PHASE_STOPPED) {
		if (result) {
			result->Add(mPos, prErrStopInvalidBasicCode);
		}
		return false;
	}
	return true;
}

/// アスキー形式1行を中間言語に変換して出力
/// @param[in]  in_file_type 入力ファイル形式
/// @param[in]  in_data      入力データ
/// @param[out] out_data     変換後データ
/// @param[in,out] result    結果格納用
/// @return true/false
bool Parse::ParseAsciiToBinaryOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result)
{
	PsSymbolSentence sentence;

	bool rc = ParseAsciiToSymbolsOneLine(in_file_type, in_data, out_data, sentence, result);

	// 次アドレスを更新
	mNextAddress += sentence.SelectedStrLen() + 1;

	if (sentence.Count() > 0) {
		// アドレスを更新
		sentence[0].SetBinStr(HomeLineNumToBinStr(mPos.GetLineNumber(), mNextAddress));
	}

	// ファイルに出力
	out_data.Add(sentence.JoinSelectedStr());
	out_data.Add("\0", 1);

	return rc;
}

/// アスキー形式1行を解析して色付けする
/// @param[in]  in_file_type 入力ファイル形式
/// @param[in]  in_data      入力データ
/// @param[out] out_data     変換後データ
/// @param[in,out] result    結果格納用
/// @return true/false
bool Parse::ParseAsciiToColoredOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result)
{
	PsSymbolSentence sentence;

	bool rc = ParseAsciiToSymbolsOneLine(in_file_type, in_data, out_data, sentence, result);

	// 色付けする
	wxString body;
	DecorateSentenceToColored(sentence, body, pConfig->EnableAddSpaceAfterColon());
	out_data.Add(body);

	return rc;
}

/// 1行分データを色付けして文字列にする
/// @param[in]  sentence        1行分データ
/// @param[out] out_str         結合した文字列
/// @param[in]  add_space_colon コロンの後ろにスペースを入れるか
void Parse::DecorateSentenceToColored(const PsSymbolSentence &sentence, wxString &out_str, bool add_space_colon)
{
	for(size_t i=0; i<sentence.Count(); i++) {
		wxUint32 area = sentence[i].GetType();

		if (area & (PsSymbol::HOME_LINE_NUMBER | PsSymbol::LINE_NUMBER)) {
			out_str += ESC_CODEN;
			out_str += wxT('l');
		} else if (area & COMMENT_AREA) {
			out_str += ESC_CODEN;
			out_str += wxT('c');
		} else if (area & QUOTED_AREA) {
			out_str += ESC_CODEN;
			out_str += wxT('q');
		} else if (area & (DATA_AREA | ALLDATA_AREA)) {
			out_str += ESC_CODEN;
			out_str += wxT('d');
		} else if (area & (PsSymbol::HEXSTRING | PsSymbol::OCTSTRING)) {
			out_str += ESC_CODEN;
			out_str += wxT('h');
		} else if (area & PsSymbol::COLON) {
			out_str += ESC_CODEN;
			out_str += wxT('y');
		} else if (area & SENTENCE_AREA) {
			out_str += ESC_CODEN;
			out_str += wxT('s');
		} else if (area & VARIABLE_AREA) {
			out_str += ESC_CODEN;
			out_str += wxT('v');
		}

		wxString str = sentence[i].GetAscStr();
		if (add_space_colon && (area & (COMMENT_AREA | QUOTED_AREA)) == 0) {
			str.Replace(wxT(":"), wxT(": "));
			str.Replace(wxT(","), wxT(", "));
		}

		out_str += str;

		if (area != 0) {
			out_str += ESC_CODEN;
			out_str += wxT('e');
		}
	}
}

/// アスキー形式テキストを読む
/// @param[in]  in_data      入力データ
/// @param[out] out_data     変換後データ
/// @return 2:7bit文字のみ 1: utf8に変換できる 0: utf8に変換できない
int Parse::ReadAsciiText(PsFileInput &in_data, PsFileData &out_data, bool to_utf8)
{
	int rc = 2;	// utf8に変換できない場合 0

	wxUint8 vals[VALS_SIZE + 1];
	size_t  valpos = VALS_SIZE;
	size_t  valsta = VALS_SIZE;
	size_t  vallen = VALS_SIZE;
	size_t  valend = VALS_SIZE;

	wxString body;
	int lf_len = 0;
	int phase = 0;
	bool eof_text_read = pConfig->GetEofTextRead();

//	vals[VALS_SIZE] = 0;
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
			} else if (eof_text_read && vals[valpos] == 0x1a) {
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
/// @param[in]  in_data      入力データ
/// @param[out] out_file     出力ファイル
/// @return true/false
bool Parse::WriteText(PsFileData &in_data, PsFileOutput &out_file)
{
	wxArrayString *inlines = &in_data.GetData();
	for(size_t row = 0; row < inlines->GetCount(); row++) {
		out_file.Write(inlines->Item(row));
	}
	return true;
}

/// バイナリを出力
/// @param[in]  in_data      入力データ
/// @param[out] out_file     出力ファイル
/// @return true/false
bool Parse::WriteBinary(PsFileData &in_data, PsFileOutput &out_file)
{
	wxArrayString *inlines = &in_data.GetData();
	for(size_t row = 0; row < inlines->GetCount(); row++) {
		out_file.Write(inlines->Item(row));
	}
	return true;
}

/// 実データをテープイメージにして出力
bool Parse::WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file)
{
	out_file.Write(in_file);
	return true;
}

/// アスキー文字列を出力
/// @param[in]  len          長さ
/// @param[in]  in_line      入力文字列
/// @param[out] out_file     出力ファイル
/// @return 追加した文字列の長さ
size_t Parse::WriteAsciiString(size_t len, const wxString &in_line, wxFile *out_data)
{
	out_data->Write(in_line, wxConvISO8859_1);
	len += in_line.Len();
	return len;
}

/// アスキー形式からUTF-8テキストに変換
/// @param[in]  in_data      入力データ
/// @param[out] out_data     変換後データ
/// @param[in,out] result    結果格納用
/// @return true/false
bool Parse::ConvAsciiToUTF8(PsFileData &in_data, PsFileData *out_data, ParseResult *result)
{
	// 変換タイプ設定
	wxString out_char_type;
	if (out_data->GetTypeFlag(psUTF8)) {
		// インターレースなどのUTF-8文字
		out_char_type = out_data->GetCharType();
	}

	mPos.Empty();
	mPos.SetName(_("Ascii->UTF8"));

	int error_count = 0;
	bool stopped = false;

	wxString body;

	for(mPos.mRow = 0; mPos.mRow < (size_t)in_data.GetCount(); mPos.mRow++) {
		if (error_count > 100) {
			stopped = true;
			break;
		}
		mPos.SetLineNumber(GetLineNumber(in_data[mPos.mRow]));

		error_count += ConvAsciiToUTF8OneLine(in_data[mPos.mRow], out_char_type, body, result);

		if (out_data) out_data->Add(body);
	}
	if (stopped) {
		if (result) {
			result->Add(mPos, prErrStopInvalidToUTF8Code);
		}
		return false;
	}
	return true;
}

/// アスキー形式1行からUTF-8テキストに変換
/// @param[in]  in_line      入力データ
/// @param[in]  out_type     出力形式
/// @param[out] out_line     変換後データ
/// @param[in,out] result    結果格納用
/// @return 0>エラーあり
int Parse::ConvAsciiToUTF8OneLine(const wxString &in_line, const wxString &out_type, wxString &out_line, ParseResult *result)
{
	// 変換タイプ設定
	bool through_mode = true;	// 半カナや記号をUTF-8に変換しないか

	if (!out_type.IsEmpty()) {
		// インターレースなどのUTF-8文字
		through_mode = false;
		mCharCodeTbl.FindSection(out_type);
	}

	int error_count = 0;

	wxUint8 val[4];
//	wxString body;
	wxString chrstr;
	CodeMapItem *item;

	out_line.Empty();
	for(mPos.mCol = 0; mPos.mCol < in_line.Len();) {
		chrstr.Empty();
		// バイト列に変換
		AscStrToBytes(in_line.Mid(mPos.mCol), val, sizeof(val));
		if (!through_mode) {
			// ESC CODEのとき
			if (memcmp(val, ESC_CODE, 1) == 0) {
				chrstr = in_line.Mid(mPos.mCol, 2);
				mPos.mCol += 2;
			}
			// キャラクターコードをUTF-8に変換
			item = mCharCodeTbl.FindByCode(val,CodeMapItem::ATTR_HIGHER,false);
			if (item != NULL) {
				chrstr = item->GetStr();
				mPos.mCol += (long)item->GetCodeLength();
			} else {
				if (val[0] >= 0x80) {
					if (result) {
						result->Add(mPos, prErrInvalidToUTF8Code);
					}
					error_count++;
				}
			}
		}
		if (chrstr.IsEmpty()) {
			chrstr = wxString(val, wxConvISO8859_1, 1);
			mPos.mCol++;
		}
		out_line += chrstr;
	}

	return error_count;
}

/// UTF-8テキストからアスキー形式に変換
/// @param[in]  in_data      入力データ
/// @param[out] out_data     変換後データ
/// @param[in,out] result    結果格納用
/// @return true/false
bool Parse::ConvUTF8ToAscii(PsFileData &in_data, PsFileData *out_data, ParseResult *result)
{
	// 変換タイプ設定
	wxString in_char_type;
	if (in_data.GetTypeFlag(psUTF8)) {
		// インターレースなどのUTF-8文字
		in_char_type = in_data.GetCharType();
	}

	mPos.Empty();
	mPos.SetName(_("UTF8->Ascii"));

	wxString body;

	int error_count = 0;
	bool stopped = false;

	for(mPos.mRow = 0; mPos.mRow < in_data.GetCount(); mPos.mRow++) {
		if (error_count > 100) {
			stopped = true;
			break;
		}
		mPos.SetLineNumber(GetLineNumber(in_data[mPos.mRow]));

		error_count += ConvUTF8ToAsciiOneLine(in_char_type, in_data[mPos.mRow], body, result);

		if (out_data) out_data->Add(body);
	}
	if (stopped) {
		if (result) {
			result->Add(mPos, prErrStopInvalidUTF8Code);
		}
		return false;
	}

	return true;

}

/// UTF-8テキスト１行からアスキー形式に変換
/// @param[in]  in_type      入力形式
/// @param[in]  in_line      入力データ
/// @param[out] out_line     変換後データ
/// @param[in,out] result    結果格納用
/// @return 0>エラーあり
int Parse::ConvUTF8ToAsciiOneLine(const wxString &in_type, const wxString &in_line, wxString &out_line, ParseResult *result)
{
	// 変換タイプ設定
	bool through_mode = true;	// UTF-8から半カナや記号に変換しないか
	if (!in_type.IsEmpty()) {
		// インターレースなどのUTF-8文字
		through_mode = false;
		mCharCodeTbl.FindSection(in_type);
	}

	wxCharBuffer vals;
	size_t vallen;
	BinString chrstr;
	CodeMapItem *item;

	int error_count = 0;

	out_line.Empty();
	vals = in_line.mb_str(wxConvUTF8);
	vallen = strlen(vals);
	for(mPos.mCol = 0; mPos.mCol < vallen;) {
		if (vals[mPos.mCol] < 0) { // upper 0x80
			// UTF-8 code?
			if ((wxUint8)vals[mPos.mCol] == 0xef && (wxUint8)vals[mPos.mCol+1] == 0xbb && (wxUint8)vals[mPos.mCol+2] == 0xbf) {
				// BOM?
				mPos.mCol += 3;
			} else {
				chrstr.Empty();
				if (!through_mode) {
					// UTF-8をキャラクターコードに変換
					item = mCharCodeTbl.FindByBytes((const wxUint8 *)((const char *)vals+mPos.mCol),CodeMapItem::ATTR_LOWER,false);
					if (item != NULL) {
						switch(item->GetFlags()) {
						case 1:
							// UTF-8 -> SJIS変換できる
							chrstr = BinString(item->GetBytes(), item->GetBytesLength());
							mPos.mCol += item->GetCodeLength();
							break;
						default:
							chrstr = BinString(item->GetCode(), item->GetCodeLength());
							mPos.mCol += item->GetBytesLength();
							break;
						}
					} else {
						// cannot convert
						if (result) {
							result->Add(mPos, prErrInvalidUTF8Code);
						}
						error_count++;
					}
				}
				if (chrstr.IsEmpty()) {
					chrstr = BinString((wxUint8)vals[mPos.mCol]);
					mPos.mCol++;
				}
				out_line += chrstr;
			}
		} else {
			// ascii char
			chrstr = BinString((wxUint8)vals[mPos.mCol]);
			mPos.mCol++;
			out_line += chrstr;
		}
	}

	return error_count;
}

/// @brief 内部ファイル名のキャラクターコードをUTF-8に変換
///
/// @param[in] src 内部ファイル名
/// @param[in] len 長さ
/// @return 変換後の文字列
wxString Parse::ConvInternalName(const wxUint8 *src, size_t len)
{
	return _T("?");
}

/// カセットイメージのヘッダを出力
/// @param[in,out] out_data データ
void Parse::PutCasetteImageHeader(PsFileOutput &out_data)
{
}

/// カセットイメージのフッタを出力
/// @param[in,out] out_data データ
/// @param[in]     len      データ長さ
void Parse::PutCasetteImageFooter(PsFileOutput &out_data, size_t len)
{
}

/// 文字コード変換テーブルの読み込み
PsErrType Parse::LoadCharCodeTable()
{
	wxTextFile file;
	wxFileName filename(pColl->GetAppPath() + DATA_DIR, GetCharCodeTableFileName());
	wxString path = filename.GetFullPath();

	if (!file.Open(path)) {
		mErrInfo.SetInfo(__LINE__, psError, psErrCannotOpen, psInfoFileInApp, GetCharCodeTableFileName());
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
	wxFileName filename(pColl->GetAppPath() + DATA_DIR, GetBasicCodeTableFileName());
	wxString path = filename.GetFullPath();

	if (!file.Open(path)) {
		mErrInfo.SetInfo(__LINE__, psError, psErrCannotOpen, psInfoFileInApp, GetBasicCodeTableFileName());
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
			mBasicCodeTbl.AddSection(mstr, GetMachineType(mstr));
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

/// 文字種類を返す
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

/// スタートアドレスのタイトルを返す
wxString Parse::GetStartAddrTitle() const
{
	return _("Start Address");
}

/// 行番号を返す
/// @param[in] line 文字列
/// @param[out] next_pos 処理した文字位置+1
/// @return 行番号
long Parse::GetLineNumber(const wxString &line, size_t *next_pos)
{
	long num = 0;

	// get line number
	int pos = line.Find(wxChar(' '));
	if (pos != wxNOT_FOUND) {
		wxRegEx re(_T("[0-9]+"));
		if (re.Matches(line.Left(pos))) {
			wxString sub = re.GetMatch(line.Left(pos));
			sub.ToLong(&num);
			if (next_pos) *next_pos = pos + 1;
		}
	} else {
		wxRegEx re(_T("^[0-9]+"));
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
	return false;
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
	return 0;
}

/// 8/16進文字列をバイト列に変換
/// @param[in]  src     8/16進文字列
/// @param[out] dst     バイト列
/// @param[in]  dst_len dstの長さ
/// @return     >0: 書き込んだ長さ / 0: エラー
int Parse::OctHexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len)
{
	return 0;
}

/// 8/16進文字列が範囲内か
/// @param[in]  base    8 or 16
/// @param[in]  src     8/16進文字列
/// @param[in]  val     変換後の値
/// @return     false:オーバーフロー
bool Parse::CheckOctHexStr(int base, const wxString &src, long *val)
{
	return true;
}

/// 行番号文字列をバイト列に変換
/// @param[in]  src            行番号(数値)文字列
/// @param[out] dst            バイト列
/// @param[in]  dst_len        dstの長さ
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     >0: 書き込んだ長さ 0:上記以外のエラーで書き込めない
int Parse::LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int memory_address, int *err)
{
	return 0;
}

/// 行先頭の行番号をバイト文字列に変換
/// @param[in]  line_number    行番号
/// @param[in]  memory_address メモリアドレス
/// @param[out] err     エラー 1:オーバーフロー 2:アンダーフロー
/// @return     バイト列 4bytes(xxyy) xx:メモリアドレス yy:行番号 
BinString Parse::HomeLineNumToBinStr(long line_number, int memory_address, int *err)
{
	return BinString();
}

/// テープイメージの内部ファイル名を設定
void Parse::SetInternalName(const wxString &name)
{
	PsFileType *type = GetOpenedDataTypePtr();
	if (type) type->SetInternalName(name);
}

/// テープイメージの内部ファイル名を返す
const wxString &Parse::GetInternalName()
{
	PsFileType *type = GetOpenedDataTypePtr();
	if (type) {
		return type->GetInternalName();
	} else {
		return mInFile.GetInternalName();
	}
}

/// テープイメージの内部ファイル名の最大文字数を返す
int Parse::GetInternalNameSize() const
{
	return 0;
}

/// 設定パラメータを返す
ConfigParam *Parse::GetConfigParam()
{
	return pConfig;
}

/// ファイルオープン時の拡張子リストを返す
const wxChar *Parse::GetOpenFileExtensions() const
{
	return wxT("Supported files|*.bin;*.bas;*.txt;*.dat|All files|*.*");
}

/// BASICバイナリファイルエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicBinaryFileExtension() const
{
	return wxT(".bin");
}

/// BASICバイナリファイルエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicBinaryFileExtensions() const
{
	return _("BASIC File (*.bin)|*.bin|All Files (*.*)|*.*");
}

/// BASICバイナリテープイメージエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicBinaryTapeImageExtension() const
{
	return wxT(".cas");
}

/// BASICバイナリテープイメージエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicBinaryTapeImageExtensions() const
{
	return _("Tape Image (*.cas)|*.cas|All Files (*.*)|*.*");
}

/// BASICバイナリディスクイメージエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicBinaryDiskImageExtension() const
{
	return wxT(".BAS");
}

/// BASICバイナリディスクイメージエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicBinaryDiskImageExtensions() const
{
	return _("DISK BASIC File (*.BAS)|*.BAS|DISK BASIC File (*.bas)|*.bas|All Files (*.*)|*.*");
}

/// BASICアスキーファイルエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicAsciiFileExtension() const
{
	return wxT(".bas");
}

/// BASICアスキーファイルエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicAsciiFileExtensions() const
{
	return _("ASCII File (*.bas)|*.bas|All Files (*.*)|*.*");
}

/// BASICアスキーテープイメージエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicAsciiTapeImageExtension() const
{
	return GetExportBasicBinaryTapeImageExtension();
}

/// BASICアスキーテープイメージエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicAsciiTapeImageExtensions() const
{
	return GetExportBasicBinaryTapeImageExtensions();
}

/// BASICアスキーディスクイメージエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportBasicAsciiDiskImageExtension() const
{
	return GetExportBasicBinaryDiskImageExtension();
}

/// BASICアスキーディスクイメージエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportBasicAsciiDiskImageExtensions() const
{
	return GetExportBasicBinaryDiskImageExtensions();
}

/// UTF-8テキストファイルエクスポート時のデフォルト拡張子を返す
const wxChar *Parse::GetExportUTF8TextFileExtension() const
{
	return wxT(".txt");
}

/// UTF-8テキストファイルエクスポート時の拡張子リストを返す
const wxChar *Parse::GetExportUTF8TextFileExtensions() const
{
	return _("Text File (*.txt)|*.txt|All Files (*.*)|*.*");
}

//////////////////////////////////////////////////////////////////////

ParseCollection::ParseCollection()
{
	for(int i=0; i<eMachineCount; i++) {
		mColl[i] = NULL;
	}
}

ParseCollection::~ParseCollection()
{
	for(int i=0; i<eMachineCount; i++) {
		delete mColl[i];
	}
}

void ParseCollection::Set(int idx, Parse *ps)
{
	mColl[idx] = ps;
}

Parse *ParseCollection::Get(int idx)
{
	return mColl[idx];
}

void ParseCollection::SetAppPath(const wxString &path)
{
	mAppPath = path;
}

const wxString &ParseCollection::GetAppPath() const
{
	return mAppPath;
}
