/// @file parse.h
///
/// @brief パーサー
///
#ifndef _PARSE_H_
#define _PARSE_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/datstrm.h>
#include <wx/dynarray.h>
#include <wx/regex.h>
#include "errorinfo.h"
#include "maptable.h"
#include "fileinfo.h"
#include "parseresult.h"
#include "parseparam.h"

/// パーサークラス
class Parse : public ParseParam
{
private:
	wxString mAppPath;
	bool mBigEndian;

	PsErrInfo mErrInfo;	///< エラー情報

	PsFileInputInfo  mInFile;		///< 入力ファイル情報
	PsFileOutputInfo mOutFile;		///< 出力ファイル情報

	int mNextAddress;		///< 次アドレス

	long mPrevLineNumber;

	PsFileData mParsedData;	///< 画面表示用データバッファ

	CodeMapTable mCharCodeTbl;	///< 文字コード変換テーブル
	CodeMapTable mBasicCodeTbl;	///< BASICコード変換テーブル

	wxRegEx reAlpha;	///< "^[a-zA-Z]+"
	wxRegEx reNumber;	///< "^[0-9]+"

	wxRegEx reAlphaNumeric;	///< "^[0-9a-zA-Z]+"

	wxRegEx reNumberReal;	///< "^[0-9]+([.][0-9]+)?[#!]?"
	wxRegEx reNumberExp;	///< "^[0-9]+([.][0-9]+)?[dDeE][+-]?[0-9]+"

	wxRegEx reNumberReald;	///< "^[.][0-9]+[#!]?"
	wxRegEx reNumberExpd;	///< "^[.][0-9]+[dDeE][+-]?[0-9]+"

	wxRegEx reOcta;		///< "^[0-7]+"
	wxRegEx reHexa;		///< "^[0-9a-fA-F]+"
	wxRegEx reSpace;		///< "^  +"

	/// 入力データのフォーマットチェック
	bool CheckDataFormat(PsFileInputInfo &in_file_info);
	/// テープイメージのフォーマットチェック
	bool CheckTapeDataFormat(PsFileInput &in_data, PsFileOutput &out_data);
	/// 中間言語形式データのフォーマットチェック
	bool CheckBinaryDataFormat(PsFileInput &in_data);
	/// アスキー形式データのフォーマットチェック
	bool CheckAsciiDataFormat(PsFileInput &in_data);
	/// 中間言語形式データの読み直し
	bool ReloadBinaryData(PsFileInput &in_file, PsFileType &out_type);
	/// アスキー形式データの読み直し
	bool ReloadAsciiData(PsFileInput &in_file, PsFileType &out_type);
	/// テープイメージから実データを取り出す
	bool ReadTapeToRealData(PsFileInput &in_data, PsFileOutput &out_data);
	/// 中間言語からアスキー形式テキストに変換
	bool ReadBinaryToAscii(PsFileInput &in_file, PsFileData &out_data, ParseResult *result = NULL);
	/// アスキー形式から中間言語に変換
	bool ParseAsciiToBinary(PsFileData &in_data, PsFileOutput &out_file, ParseResult *result = NULL);
	/// 変数文字列を変数文字に変換
	void ParseVariableString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result = NULL);
	/// 数値文字列を数値に変換
	void ParseNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_real, wxRegEx &re_exp, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result = NULL);
	/// 行番号文字列を数値に変換
	void ParseLineNumberString(const wxString &in_data, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re_int, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result = NULL);
	/// 8進or16進文字列を文字に変換
	void ParseOctHexString(const wxString &in_data, const wxString &octhexhed, int base, size_t &pos, int row, long line_number, const wxString &name, wxRegEx &re, wxUint8 *body, size_t body_size, size_t &body_len, bool &empstr, ParseResult *result = NULL);
	/// アスキー形式1行を中間言語に変換
	bool ParseAsciiToBinaryOneLine(PsFileType &in_file_type, wxString &in_data, PsFileOutput &out_file, int row = 0, ParseResult *result = NULL);
	/// アスキー形式テキストを読む
	int  ReadAsciiText(PsFileInput &in_data, PsFileData &out_data, bool to_utf8 = false);
	/// アスキー文字列を出力
	size_t WriteAsciiString(size_t len, const wxString &in_line, wxFile *out_data);
	/// テキストを出力
	bool WriteText(PsFileData &in_data, PsFileOutput &out_file);
	/// テキストを出力
//	bool WriteText(wxArrayString &inlines, PsFileOutput &out_file);
	/// 実データをテープイメージにして出力
	bool WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file);
	/// アスキー形式からUTF-8テキストに変換
	bool ConvAsciiToUTF8(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
	/// UTF-8テキストからアスキー形式に変換
	bool ConvUTF8ToAscii(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
	/// 内部ファイル名のかなを変換
	wxString ConvInternalName(const wxUint8 *src, size_t len);
	/// 文字コード変換テーブルの読み込み
	PsErrType LoadCharCodeTable();
	/// BASICコード変換テーブルの読み込み
	PsErrType LoadBasicCodeTable();
	/// 16進文字列をバイト列に変換
	int  HexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// アスキー文字列をバイト列に変換
	int  AscStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// 整数バイト列を数値に変換
	long BytesToLong(const wxUint8 *src, size_t src_len);
	/// 整数バイト列を数値に変換
	bool BytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
	/// 浮動小数点バイト列を文字列に変換
	bool FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
	/// 数値を整数バイト列に変換
	bool LongToBytes(long src, wxUint8 *dst, size_t dst_len);
	/// 数値文字列をバイト列に変換
	int  NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err = NULL);
	/// 8/16進文字列が範囲内か
	bool CheckOctHexStr(int type, const wxString &src);
	/// 行番号文字列をバイト列に変換
	int  LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err = NULL);

public:
	Parse(const wxString &path);
	~Parse();
	bool Init();
	/// 指定したファイルを開く
	bool OpenDataFile(const wxString &in_file_name);
	/// ファイルを閉じる
	void CloseDataFile();
	/// ファイル開いているか
	bool IsOpenedDataFile();
	/// 開いているファイル種類
	PsFileType *GetOpenedDataTypePtr();
	/// 開いているファイル情報
	PsFileInfo *GetOpenedDataInfoPtr();
	/// ファイル名
	wxString GetFileNameBase();
	/// 出力ファイルを開く
	bool OpenOutFile(const wxString &out_file_name, PsFileType &file_type);
	/// 出力ファイルを閉じる
	void CloseOutFile();
	/// エクスポート
	bool ExportData();
	/// 画面表示用データを返す
	wxString &GetParsedData(wxArrayString &lines);
	/// 文字種類を返す
	void GetCharTypes(wxArrayString &char_types);
	const wxString &GetCharType(size_t index) const;
	/// BASIC種類を返す
	void GetBasicTypes(wxArrayString &basic_types);
//	/// BASIC種類をセット
//	void SetBasicType(const wxString &basic_type);
	/// 入力ファイルのBASIC種類を返す
	wxString &GetOpenedBasicType();
	/// 入力ファイルの形式を変更して読み直す
	bool ReloadOpendAsciiData(int type, int mask, const wxString &char_type);
	/// 表示用データの形式を変更して読み直す
	bool ReloadParsedData(int type, int mask, const wxString &char_type);
	/// 中間言語形式データの読み直し
	bool ReloadOpenedBinaryData(int type, int mask, const wxString &basic_type);
	/// スタートアドレスのリストへのポインタを返す
	int *GetStartAddrsPtr(size_t *count);
	/// 行番号を返す
	long GetLineNumber(const wxString &line, size_t *next_pos = NULL);
	/// レポート
	void Report(ParseResult &result, wxArrayString &line);
	/// アスキー形式1行からUTF-8テキストに変換
	int  ConvAsciiToUTF8OneLine(size_t row, const wxString &in_line, const wxString &out_type, wxString &out_line, ParseResult *result = NULL);
	/// UTF-8テキスト1行からアスキー形式に変換
	int  ConvUTF8ToAsciiOneLine(const wxString &in_type, size_t row, const wxString &in_line, wxString &out_line, ParseResult *result = NULL);
};

#endif /* _PARSE_H_ */
