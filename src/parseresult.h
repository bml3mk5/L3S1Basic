/// @file  parseresult.h
///
/// @brief 解析結果保存クラス
///
#ifndef _PARSERESULT_H_
#define _PARSERESULT_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>
#include <wx/hashmap.h>

/// エラーコード
typedef enum enumPrErrCode {
	prErrNone = 0,
	prErrInvalidChar,
	prErrInvalidUTF8Code,
	prErrInvalidToUTF8Code,
	prErrInvalidBasicCode,
	prErrStopInvalidUTF8Code,
	prErrStopInvalidToUTF8Code,
	prErrStopInvalidBasicCode,
	prErrOverflow,
	prErrInvalidNumber,
	prErrInvalidLineNumber,
	prErrDiscontLineNumber,
	prErrDuplicateLineNumber,
	prErrEraseCodeFE,
} PrErrCode;

WX_DECLARE_HASH_MAP(int, int, wxIntegerHash, wxIntegerEqual, LineNumberMap);

/// 位置情報
class ParsePosition
{
protected:
	wxString mName;			///< 名称
	long mLineNumber;		///< BASIC行番号
	LineNumberMap mLineNumberMap;	///< 重複チェック用

public:
	size_t mRow;			///< 行
	size_t mCol;			///< 列

public:
	ParsePosition();
	ParsePosition(size_t row, size_t col, long line_number, const wxString &name);
	~ParsePosition();
	void Empty();
	void SetRow(size_t val);
	void SetCol(size_t val);
	int  SetLineNumber(long val);
	void SetName(const wxString &val);
	size_t GetRow() const;
	size_t GetCol() const;
	long GetLineNumber() const;
	const wxString &GetName() const;
};

/// 解析結果保存アイテムクラス
class ParseResultItem : public ParsePosition
{
protected:
	PrErrCode mErrorCode;
	int mValue;
public:
	ParseResultItem();
	ParseResultItem(const ParsePosition &pos, PrErrCode error_code);
	ParseResultItem(const ParsePosition &pos, PrErrCode error_code, int value);
//	ParseResultItem(size_t row, size_t col, long line_number, const wxString &name, PrErrCode error_code);
	void SetErrorCode(PrErrCode val) { mErrorCode = val; }
	PrErrCode GetErrorCode() const { return mErrorCode; }
	int GetValue() const { return mValue; }
	wxString GetValueString() const;
};
WX_DECLARE_OBJARRAY(ParseResultItem, ParseResultItems);

/// 解析結果保存クラス
class ParseResult
{
private:
	ParseResultItems mItems;
public:
	ParseResult();
	void Add(const ParsePosition &pos, PrErrCode error_code);
	void Add(const ParsePosition &pos, PrErrCode error_code, int value);
//	void Add(size_t row, size_t col, size_t line_number, const wxString &name, PrErrCode error_code);
	void Empty();
	size_t GetCount();
	/// エラーメッセージ
	wxString ErrMsg(PrErrCode code);
	/// レポート出力
	void Report(wxArrayString &lines);
};

#endif /* _PARSERESULT_H_ */
