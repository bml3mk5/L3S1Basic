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
#include "pssymbol.h"
#include "config.h"

/// BOMコード
#define BOM_CODE "\xEF\xBB\xBF"

/// ファイル終端コード
#define EOF_CODE "\x1a"
#define EOF_CODEN 0x1a
/// エスケープコード
#define ESC_CODE "\x1b"
#define ESC_CODEN 0x1b

#if 0
/// パーサーに渡す属性
class ParseAttr
{
private:
	bool mAddSpaceAfterColon;

public:
	ParseAttr();
	~ParseAttr();

	bool EnableAddSpaceAfterColon() const { return mAddSpaceAfterColon; }
	void EnableAddSpaceAfterColon(bool val) { mAddSpaceAfterColon = val; }
};
#endif

#define ERROR_STOPPED_COUNT		50

class ParseCollection;

/// パーサークラス
class Parse
{
public:
	enum enParseAttrs {
		QUOTED_AREA		= 0x01,
		DATA_AREA		= 0x02,
		ALLDATA_AREA	= 0x04,
		COMMENT_AREA	= 0x08,
		SENTENCE_AREA	= 0x10,
		VARIABLE_AREA	= 0x20,
		STATEMENT_AREA	= 0x40,
	};
	enum enParsePhase {
		PHASE_STOPPED = -3,
		PHASE_END = -2,
		PHASE_EOL = -1,
		PHASE_NONE = 0,
		PHASE_LINE_NUMBER = 1,
		PHASE_BODY = 2,
	};
	enum enParseBuffer {
		VALS_SIZE =	1023
	};

	static const char *cNLChr[];	///< 改行コード

protected:
	ParseCollection *pColl;

	bool mBigEndian;

	PsErrInfo mErrInfo;	///< エラー情報

	PsFileInputInfo  mInFile;		///< 入力ファイル情報
	PsFileOutputInfo mOutFile;		///< 出力ファイル情報

	int mNextAddress;		///< 次アドレス

	ParsePosition mPos;		///< 処理中の位置情報
	long mPrevLineNumber;	///< 1つ前のBASIC行番号
	int mMachineType;		///< 処理中のマシンタイプ

	PsFileData mParsedData;	///< 画面表示用データバッファ

	ConfigParam *pConfig;	///< 設定

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
	
	wxRegEx reVariEnd;	///< [%$!#]

	/// キャラクターコードテーブルファイル名
	virtual wxString GetCharCodeTableFileName() const = 0;
	/// BASICコードテーブルファイル名
	virtual wxString GetBasicCodeTableFileName() const = 0;
	/// 初期設定値をセット
	virtual void SetDefaultConfigParam() = 0;

	/// 入力データのフォーマットチェック
	virtual bool CheckDataFormat(PsFileInputInfo &in_file_info) = 0;
	/// テープイメージのフォーマットチェック
	virtual bool CheckTapeDataFormat(PsFileInput &in_data, PsFileOutput &out_data) = 0;
	/// 中間言語形式データのフォーマットチェック
	virtual bool CheckBinaryDataFormat(PsFileInput &in_data);
	/// アスキー形式データのフォーマットチェック
	virtual bool CheckAsciiDataFormat(PsFileInput &in_data);
	/// 中間言語形式データの読み直し
	virtual bool ReloadBinaryData(PsFileInput &in_file, PsFileType &out_type);
	/// アスキー形式データの読み直し
	virtual bool ReloadAsciiData(PsFileInput &in_file, PsFileType &out_type);
	/// テープイメージから実データを取り出す
	virtual bool ReadTapeToRealData(PsFileInput &in_data, PsFileOutput &out_data) = 0;
	/// 中間言語からアスキー形式テキストに変換
	virtual bool ReadBinaryToAscii(PsFileInput &in_file, PsFileData &out_data, ParseResult *result = NULL);
	/// 中間言語からアスキー形式色付きテキストに変換
	virtual bool ReadBinaryToAsciiColored(PsFileInput &in_file, PsFileData &out_data, ParseResult *result = NULL);
	/// 中間言語1行分を解析する
	virtual int  ReadBinaryToSymbolsOneLine(PsFileInput &in_file, PsFileType &out_type, int phase, PsSymbolSentence &sentence, ParseResult *result = NULL) = 0;
	/// アスキー形式から中間言語に変換
	virtual bool ParseAsciiToBinary(PsFileData &in_data, PsFileData &out_data, ParseResult *result = NULL) = 0;
	/// アスキー形式を解析して色付けする
	virtual bool ParseAsciiToColored(PsFileData &in_data, PsFileData &out_data, ParseResult *result = NULL) = 0;
	/// アスキー形式1行を解析する
	virtual bool ParseAsciiToSymbolsOneLine(PsFileType &in_file_type, wxString &in_data, PsFileType &out_type, PsSymbolSentence &sentence, ParseResult *result = NULL) = 0;
	/// 1行分データを色付けして文字列にする
	virtual void DecorateSentenceToColored(const PsSymbolSentence &sentence, wxString &out_str, bool add_space_colon);
	/// 変数文字列を変数文字に変換
	virtual void ParseVariableString(const wxString &in_data, wxRegEx &re, PsSymbol &body, ParseResult *result = NULL) = 0;
	/// 数値文字列を数値に変換
	virtual void ParseNumberString(const wxString &in_data, wxRegEx &re_real, wxRegEx &re_exp, PsSymbol &body, ParseResult *result = NULL) = 0;
	/// 行番号文字列を数値に変換
	virtual void ParseLineNumberString(const wxString &in_data, wxRegEx &re_int, PsSymbol &body, ParseResult *result = NULL) = 0;
	/// 8進or16進文字列を文字に変換
	virtual void ParseOctHexString(const wxString &in_data, const wxString &octhexhed, const wxString &octhexcode, int base, wxRegEx &re, PsSymbol &body, ParseResult *result = NULL) = 0;
	/// アスキー形式1行を中間言語に変換
	virtual bool ParseAsciiToBinaryOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result = NULL);
	/// アスキー形式1行を解析して色付けする
	virtual bool ParseAsciiToColoredOneLine(PsFileType &in_file_type, wxString &in_data, PsFileData &out_data, ParseResult *result = NULL);
	/// アスキー形式テキストを読む
	virtual int  ReadAsciiText(PsFileInput &in_data, PsFileData &out_data, bool to_utf8 = false);
	/// アスキー文字列を出力
	virtual size_t WriteAsciiString(size_t len, const wxString &in_line, wxFile *out_data);
	/// テキストを出力
	virtual bool WriteText(PsFileData &in_data, PsFileOutput &out_file);
	/// バイナリを出力
	virtual bool WriteBinary(PsFileData &in_data, PsFileOutput &out_file);
	/// 実データをテープイメージにして出力
	virtual bool WriteTapeFromRealData(PsFileInput &in_file, PsFileOutput &out_file);
	/// アスキー形式からUTF-8テキストに変換
	virtual bool ConvAsciiToUTF8(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
	/// UTF-8テキストからアスキー形式に変換
	virtual bool ConvUTF8ToAscii(PsFileData &in_data, PsFileData *out_data, ParseResult *result = NULL);
	/// 内部ファイル名のかなを変換
	virtual wxString ConvInternalName(const wxUint8 *src, size_t len);
	/// カセットイメージのヘッダを出力
	virtual void PutCasetteImageHeader(PsFileOutput &out_data);
	/// カセットイメージのフッタを出力
	virtual void PutCasetteImageFooter(PsFileOutput &out_data, size_t len);
	/// 文字コード変換テーブルの読み込み
	virtual PsErrType LoadCharCodeTable();
	/// BASICコード変換テーブルの読み込み
	virtual PsErrType LoadBasicCodeTable();
	/// 16進文字列をバイト列に変換
	virtual int  HexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// アスキー文字列をバイト列に変換
	virtual int  AscStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// 整数バイト列を数値に変換
	virtual long BytesToLong(const wxUint8 *src, size_t src_len);
	/// 整数バイト列を数値に変換
	virtual bool BytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
	/// 浮動小数点バイト列を文字列に変換
	virtual bool FloatBytesToStr(const wxUint8 *src, size_t src_len, wxString &dst);
	/// 数値を整数バイト列に変換
	virtual bool LongToBytes(long src, wxUint8 *dst, size_t dst_len);
	/// 数値文字列をバイト列に変換
	virtual int  NumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int *err = NULL);
	/// 8/16進文字列をバイト列に変換
	virtual int  OctHexStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len);
	/// 8/16進文字列が範囲内か
	virtual bool CheckOctHexStr(int type, const wxString &src, long *val = NULL);
	/// 行番号文字列をバイト列に変換
	virtual int  LineNumStrToBytes(const wxString &src, wxUint8 *dst, size_t dst_len, int memory_address = -1, int *err = NULL);
	/// 行先頭の行番号をバイト文字列に変換
	virtual BinString HomeLineNumToBinStr(long line_number, int memory_address = -1, int *err = NULL);

public:
	Parse(ParseCollection *collection, ConfigParam *config);
	virtual ~Parse();
	/// 初期化
	virtual bool Init();
	/// 指定したファイルを開く
	virtual bool OpenDataFile(const wxString &in_file_name, PsFileType &file_type);
	/// ファイルを閉じる
	virtual void CloseDataFile();
	/// ファイル開いているか
	virtual bool IsOpenedDataFile();
	/// 開いているファイル種類
	virtual PsFileType *GetOpenedDataTypePtr();
	virtual const PsFileType *GetOpenedDataTypePtr() const;
	/// 開いているファイル情報
	virtual PsFileInfo *GetOpenedDataInfoPtr();
	/// ファイル名フルパス
	virtual wxString GetFileFullPath() const;
	/// ファイル名
	virtual wxString GetFileNameBase() const;
	/// 出力ファイルを開く
	virtual bool OpenOutFile(const wxString &out_file_name, PsFileType &file_type);
	/// 出力ファイルを閉じる
	virtual void CloseOutFile();
	/// エクスポート
	virtual bool ExportData();
	/// 画面表示用データを返す
	virtual wxString &GetParsedData(wxArrayString &lines);
	/// 文字種類を返す
	virtual void GetCharTypes(wxArrayString &char_types);
	virtual const wxString &GetCharType(size_t index) const;
	/// BASIC種類を返す
	virtual void GetBasicTypes(wxArrayString &basic_types);
//	/// BASIC種類をセット
//	virtual void SetBasicType(const wxString &basic_type);
	/// 入力ファイルのBASIC種類を返す
	virtual wxString &GetOpenedBasicType();
	/// 入力ファイルの形式を変更して読み直す
	virtual bool ReloadOpendAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type = wxEmptyString);
//	/// 表示用データの形式を変更して読み直す
//	virtual bool ReloadParsedAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type);
	/// 表示用データの形式を変更して読み直す
	virtual bool ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type);
	virtual bool ReloadParsedData(PsFileType &out_type);
	/// 中間言語形式データの読み直し
	virtual bool ReloadOpenedBinaryData(int type, int mask, const wxString &basic_type);
	/// スタートアドレスのリストへのポインタを返す
	virtual const int *GetStartAddrsPtr() const = 0;
	/// スタートアドレスのリスト数を返す
	virtual int GetStartAddrCount() const = 0;
	/// スタートアドレスのタイトルを返す
	virtual wxString GetStartAddrTitle() const;
	/// 行番号を返す
	virtual long GetLineNumber(const wxString &line, size_t *next_pos = NULL);
	/// レポート
	virtual void Report(ParseResult &result, wxArrayString &line);
	/// アスキー形式1行からUTF-8テキストに変換
	virtual int  ConvAsciiToUTF8OneLine(const wxString &in_line, const wxString &out_type, wxString &out_line, ParseResult *result = NULL);
	/// UTF-8テキスト1行からアスキー形式に変換
	virtual int  ConvUTF8ToAsciiOneLine(const wxString &in_type, const wxString &in_line, wxString &out_line, ParseResult *result = NULL);
	/// テープイメージの内部ファイル名を設定
	virtual void SetInternalName(const wxString &name);
	/// テープイメージの内部ファイル名を返す
	virtual const wxString &GetInternalName();
	/// テープイメージの内部ファイル名の最大文字数を返す
	virtual int GetInternalNameSize() const;
	/// BASICが拡張BASICかどうか
	virtual bool IsExtendedBasic(const wxString &basic_type) = 0;
	/// マシンタイプの判別
	virtual int GetMachineType(const wxString &basic_type) = 0;
	/// 機種名を返す
	virtual wxString GetMachineName() const = 0;
	/// 設定パラメータを返す
	virtual ConfigParam *GetConfigParam();
	/// ファイルオープン時の拡張子リストを返す
	virtual const wxChar *GetOpenFileExtensions() const;
	/// BASICバイナリファイルエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicBinaryFileExtension() const;
	/// BASICバイナリファイルエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicBinaryFileExtensions() const;
	/// BASICバイナリテープイメージエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicBinaryTapeImageExtension() const;
	/// BASICバイナリテープイメージエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicBinaryTapeImageExtensions() const;
	/// BASICバイナリディスクイメージエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicBinaryDiskImageExtension() const;
	/// BASICバイナリディスクイメージエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicBinaryDiskImageExtensions() const;
	/// BASICアスキーファイルエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicAsciiFileExtension() const;
	/// BASICアスキーファイルエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicAsciiFileExtensions() const;
	/// BASICアスキーテープイメージエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicAsciiTapeImageExtension() const;
	/// BASICアスキーテープイメージエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicAsciiTapeImageExtensions() const;
	/// BASICアスキーディスクイメージエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportBasicAsciiDiskImageExtension() const;
	/// BASICアスキーディスクイメージエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportBasicAsciiDiskImageExtensions() const;
	/// UTF-8テキストファイルエクスポート時のデフォルト拡張子を返す
	virtual const wxChar *GetExportUTF8TextFileExtension() const;
	/// UTF-8テキストファイルエクスポート時の拡張子リストを返す
	virtual const wxChar *GetExportUTF8TextFileExtensions() const;
};

/// パーサーのリスト
class ParseCollection
{
private:
	Parse *mColl[eMachineCount];

	wxString mAppPath;

public:
	ParseCollection();
	~ParseCollection();
	void Set(int idx, Parse *ps);
	Parse *Get(int idx);

	void SetAppPath(const wxString &path);
	const wxString &GetAppPath() const;
};

#endif /* _PARSE_H_ */
