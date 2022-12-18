/// @file  parseresult.h
///
/// @brief 解析結果保存クラス
///
#ifndef _PARSERESULT_H_
#define _PARSERESULT_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>

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
	prErrEraseCodeFE,
} PrErrCode;

/// 解析結果保存アイテムクラス
class ParseResultItem {
public:
	long line;
	long pos;
	long line_number;
	PrErrCode error_code;
	wxString name;
public:
	ParseResultItem();
	ParseResultItem(long new_line, long new_pos, long new_line_number, const wxString &new_name, PrErrCode new_error_code);
};
WX_DECLARE_OBJARRAY(ParseResultItem, ParseResultItems);

/// 解析結果保存クラス
class ParseResult {
private:
	ParseResultItems items;
public:
	ParseResult();
	void Add(long new_line, long new_pos, long new_line_number, const wxString &new_name, PrErrCode new_error_code);
	void Empty();
	size_t GetCount();
	/// エラーメッセージ
	wxString ErrMsg(PrErrCode code);
	/// レポート出力
	void Report(wxArrayString &lines);
};

#endif /* _PARSERESULT_H_ */
