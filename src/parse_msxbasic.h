/// @file parse_msxbasic.h
///
/// @brief MSX-BASICパーサー
///
#ifndef _PARSE_MSXBASIC_H_
#define _PARSE_MSXBASIC_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/datstrm.h>
#include <wx/dynarray.h>
#include <wx/regex.h>
#include "bsstring.h"
#include "errorinfo.h"
#include "maptable.h"
#include "fileinfo.h"
#include "parseresult.h"
#include "parseparam.h"
#include "parse.h"

/// casetteヘッダ種類
#define CAS_HEADER_BASIC	"\xD3\xD3\xD3\xD3\xD3\xD3\xD3\xD3\xD3\xD3"
#define CAS_HEADER_ASCII	"\xEA\xEA\xEA\xEA\xEA\xEA\xEA\xEA\xEA\xEA"
#define CAS_HEADER_MACHINE	"\xD0\xD0\xD0\xD0\xD0\xD0\xD0\xD0\xD0\xD0"

/// fMSX-DOS Casetteヘッダ
#define FMSX_HEADER "\x1F\xA6\xDE\xBA\xCC\x13\x7D\x74"

/// パーサークラス
class ParseMSXBasic : public Parse
{
protected:
	/// 初期設定
	enum enMSXDefaultConfigs {
		eMSXDefaultNewLineAscii = 2,	// CR+LF
		eMSXDefaultStartAddr = 0
	};

	/// スタートアドレス
	enum enMSXAddrCount {
		eMSXStartAddrCount = 1,
	};
	static const int iStartAddr[eMSXStartAddrCount];

	/// キャラクターコードテーブルファイル名
	wxString GetCharCodeTableFileName() const;
	/// BASICコードテーブルファイル名
	wxString GetBasicCodeTableFileName() const;
	/// 初期設定値をセット
	void SetDefaultConfigParam();

	/// 入力データのフォーマットチェック
	bool CheckDataFormat(PsFileInputInfo &in_file_info);
	/// テープイメージのフォーマットチェック
	bool CheckTapeDataFormat(PsFileInput &in_data, PsFileOutput &out_data);
//	/// 中間言語形式データのフォーマットチェック
//	bool CheckBinaryDataFormat(PsFileInput &in_data);
//	/// アスキー形式データのフォーマットチェック
//	bool CheckAsciiDataFormat(PsFileInput &in_data);
//	/// 中間言語形式データの読み直し
//	bool ReloadBinaryData(PsFileInput &in_file, PsFileType &out_type);
//	/// アスキー形式データの読み直し
//	bool ReloadAsciiData(PsFileInput &in_file, PsFileType &out_type);
	/// テープイメージから実データを取り出す
	bool ReadTapeToRealData(PsFileInput &in_data, PsFileOutput &out_data);
//	/// 中間言語からアスキー形式テキストに変換
//	bool ReadBinaryToAscii(PsFileInput &in_file, PsFileData &out_data, ParseResult *result = NULL);
//	/// 中間言語からアスキー形式色付きテキストに変換
//	bool ReadBinaryToAsciiColored(PsFileInput &in_file, PsFileData &out_data, ParseResult *result = NULL);
	/// 中間言語1行分を解析する
	int  ReadBinaryToSymbolsOneLine(PsFileInput &in_file, PsFileType &out_type, int phase, PsSymbolSentence &sentence, ParseResult *result = NULL);
	/// アスキー形式から中間言語に変換
	bool ParseAsciiToBinary(PsFileData &in_data, PsFileData &out_data, ParseResult *result = NULL);
	/// アスキー形式を解析して色付けする
	bool ParseAsciiToColored(PsFileData &in_data, PsFileData &out_data, ParseResult *result = NULL);
	/// 変数文字列を変数文字に変換
	void ParseVariableString(const wxString &in_data, wxRegEx &re, PsSymbol &body, ParseResult *result = NULL);
	/// 数値文字列を数値に変換
	void ParseNumberString(const wxString &in_data, wxRegEx &re_real, wxRegEx &re_exp, PsSymbol &body, ParseResult *result = NULL);
	/// 行番号文字列を数値に変換
	void ParseLineNumberString(const wxString &in_data, wxRegEx &re_int, PsSymbol &body, ParseResult *result = NULL);
	/// 8進or16進文字列を文字に変換
	void ParseOctHexString(const wxString &in_data, const wxString &octhexhed, const wxString &octhexcode, int base, wxRegEx &re, PsSymbol &body, ParseResult *result = NULL);
//	/// アスキー形式1行を中間言語に変換
//	bool ParseAsciiToBinaryOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result = NULL);
//	/// アスキー形式1行を解析して色付けする
//	bool ParseAsciiToColoredOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result = NULL);
	/// アスキー形式1行を解析する
	bool ParseAsciiToSymbolsOneLine(PsFileType &in_file_type, wxString &in_data, PsFileType &out_type, PsSymbolSentence &sentence, ParseResult *result = NULL);
//	/// アスキー形式テキストを読む
//	int  ReadAsciiText(PsFileInput &in_data, PsFileData &out_data, bool to_utf8 = false);
//	/// アスキー文字列を出力
//	size_t WriteAsciiString(size_t len, const wxString &in_line, wxFile *out_data);
	/// テキストを出力
	bool WriteText(PsFileData &in_data, PsFileOutput &out_file);
	/// バイナリを出力
	bool WriteBinary(PsFileData &in_data, PsFileOutput &out_file);
	/// 実データをテープイメージにして出力
	bool WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file);
//	/// アスキー形式からUTF-8テキストに変換
//	bool ConvAsciiToUTF8(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
//	/// UTF-8テキストからアスキー形式に変換
//	bool ConvUTF8ToAscii(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
	/// 内部ファイル名のかなを変換
	wxString ConvInternalName(const wxUint8 *src, size_t len);
	/// カセットイメージのヘッダを出力
	void PutCasetteImageHeader(PsFileOutput &out_file);
	/// カセットイメージのフッタを出力
	void PutCasetteImageFooter(PsFileOutput &out_file, size_t len);
//	/// 文字コード変換テーブルの読み込み
//	PsErrType LoadCharCodeTable();
//	/// BASICコード変換テーブルの読み込み
//	PsErrType LoadBasicCodeTable();
//	/// 16進文字列をバイト列に変換
//	int  HexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
//	/// アスキー文字列をバイト列に変換
//	int  AscStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
//	/// 整数バイト列を数値に変換
//	long BytesToLong(const wxUint8 *src, size_t src_len);
//	/// 整数バイト列を数値に変換
//	bool BytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
	/// 浮動小数点バイト列を文字列に変換
	bool FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
//	/// 数値を整数バイト列に変換
//	bool LongToBytes(long src, wxUint8 *dst, size_t dst_len);
	/// 数値を整数バイト文字列に変換
	bool LongToBinStr(long src, BinString &dst, int len);
	BinString LongToBinStr(long src, int len);
	/// 数値文字列をバイト列に変換
	int  NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err = NULL);
	/// 数値文字列をバイト文字列に変換
	int  NumStrToBinStr(const wxString &src, BinString &dst, int *err = NULL);
	/// 8/16進文字列をバイト列に変換
	int  OctHexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// 8/16進文字列が範囲内か
	bool CheckOctHexStr(int type, const wxString &src, long *val = NULL);
	/// 行番号文字列をバイト列に変換
	int  LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int memory_address = -1, int *err = NULL);
	/// 行番号文字列をバイト文字列に変換
	int  LineNumStrToBinStr(const wxString &src, BinString &dst, int memory_address = -1, int *err = NULL);
	/// 行先頭の行番号をバイト文字列に変換
	BinString HomeLineNumToBinStr(long line_number, int memory_address = -1, int *err = NULL);

public:
	ParseMSXBasic(ParseCollection *collection);
	~ParseMSXBasic();

//	/// ファイルを閉じる
//	void CloseDataFile();
//	/// ファイル開いているか
//	bool IsOpenedDataFile();
//	/// 開いているファイル種類
//	PsFileType *GetOpenedDataTypePtr();
//	/// 開いているファイル情報
//	PsFileInfo *GetOpenedDataInfoPtr();
//	/// ファイル名
//	wxString GetFileNameBase();
//	/// 出力ファイルを開く
//	bool OpenOutFile(const wxString &out_file_name, PsFileType &file_type);
//	/// 出力ファイルを閉じる
//	void CloseOutFile();
//	/// エクスポート
//	bool ExportData();
//	/// 画面表示用データを返す
//	wxString &GetParsedData(wxArrayString &lines);
//	/// 文字種類を返す
//	void GetCharTypes(wxArrayString &char_types);
//	const wxString &GetCharType(size_t index) const;
//	/// BASIC種類を返す
//	void GetBasicTypes(wxArrayString &basic_types);
//	/// BASIC種類をセット
//	void SetBasicType(const wxString &basic_type);
//	/// 入力ファイルのBASIC種類を返す
//	wxString &GetOpenedBasicType();
//	/// 入力ファイルの形式を変更して読み直す
//	bool ReloadOpendAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type = wxEmptyString);
//	/// 表示用データの形式を変更して読み直す
//	bool ReloadParsedAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type);
//	/// 表示用データの形式を変更して読み直す
//	bool ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type);
//	bool ReloadParsedData(PsFileType &out_type);
//	/// 中間言語形式データの読み直し
//	bool ReloadOpenedBinaryData(int type, int mask, const wxString &basic_type);
	/// スタートアドレスのリストへのポインタを返す
	const int *GetStartAddrsPtr() const;
	/// スタートアドレスのリスト数を返す
	int GetStartAddrCount() const;
//	/// 行番号を返す
//	long GetLineNumber(const wxString &line, size_t *next_pos = NULL);
//	/// レポート
//	void Report(ParseResult &result, wxArrayString &line);
//	/// アスキー形式1行からUTF-8テキストに変換
//	int  ConvAsciiToUTF8OneLine(size_t row, const wxString &in_line, const wxString &out_type, wxString &out_line, ParseResult *result = NULL);
//	/// UTF-8テキスト1行からアスキー形式に変換
//	int  ConvUTF8ToAsciiOneLine(const wxString &in_type, size_t row, const wxString &in_line, wxString &out_line, ParseResult *result = NULL);
	/// テープイメージの内部ファイル名の最大文字数を返す
	int GetInternalNameSize() const;
	/// BASICが拡張BASICかどうか
	bool IsExtendedBasic(const wxString &basic_type);
	/// マシンタイプの判別
	int GetMachineType(const wxString &basic_type);
	/// 機種名を返す
	wxString GetMachineName() const;
	/// ファイルオープン時の拡張子リストを返す
	const wxChar *GetOpenFileExtensions() const;
};

#endif /* _PARSE_MSXBASIC_H_ */
