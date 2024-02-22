/// @file parseresult.cpp
///
/// @brief 解析結果保存クラス
///
#include "parseresult.h"
#include <wx/arrimpl.cpp>

//////////////////////////////////////////////////////////////////////

ParsePosition::ParsePosition()
{
	mLineNumber = 0;
	mRow = 0;
	mCol = 0;
}

ParsePosition::ParsePosition(size_t row, size_t col, long line_number, const wxString &name)
{
	mName = name;
	mLineNumber = line_number;
	mLineNumberMap[line_number] = (int)mRow;
	mRow = row;
	mCol = col;
}

ParsePosition::~ParsePosition()
{
}

void ParsePosition::Empty()
{
	mName.Empty();
	mLineNumber = 0;
	mLineNumberMap.clear();
	mRow = 0;
	mCol = 0;
}

void ParsePosition::SetRow(size_t val)
{
	mRow = val;
}

void ParsePosition::SetCol(size_t val)
{
	mCol = val;
}

/// 行番号をセット
/// 同時に重複しているかチェック
/// @return >=0 : すでに存在する場合行を返す
int ParsePosition::SetLineNumber(long val)
{
	int exists_row = -1;
	mLineNumber = val;
	LineNumberMap::const_iterator cit = mLineNumberMap.find(val);
	if (cit != mLineNumberMap.end()) {
		// already exists
		exists_row = mLineNumberMap[val];
	}
	mLineNumberMap[val] = (int)mRow;
	return exists_row;
}

void ParsePosition::SetName(const wxString &val)
{
	mName = val;
}

size_t ParsePosition::GetRow() const
{
	return mRow;
}

size_t ParsePosition::GetCol() const
{
	return mCol;
}

long ParsePosition::GetLineNumber() const
{
	return mLineNumber;
}

const wxString &ParsePosition::GetName() const
{
	return mName;
}

//////////////////////////////////////////////////////////////////////

ParseResultItem::ParseResultItem() : ParsePosition()
{
	mErrorCode = prErrNone;
	mValue = -1;
	mName.Empty();
}

ParseResultItem::ParseResultItem(const ParsePosition &pos, PrErrCode error_code)
	: ParsePosition(pos)
{
	mErrorCode = error_code;
	mValue = -1;
}

ParseResultItem::ParseResultItem(const ParsePosition &pos, PrErrCode error_code, int value)
	: ParsePosition(pos)
{
	mErrorCode = error_code;
	mValue = value;
}

#if 0
ParseResultItem::ParseResultItem(size_t row, size_t col, long line_number, const wxString &name, PrErrCode error_code)
	: ParsePosition(row, col, line_number, name)
{
	mErrorCode = error_code;
}
#endif
wxString ParseResultItem::GetValueString() const
{
	return wxString::Format(_T("%d"), mValue);
}

//////////////////////////////////////////////////////////////////////

WX_DEFINE_OBJARRAY(ParseResultItems);

//////////////////////////////////////////////////////////////////////

ParseResult::ParseResult()
{
	Empty();
}
void ParseResult::Add(const ParsePosition &pos, PrErrCode error_code)
{
	ParseResultItem *item = new ParseResultItem(pos, error_code);
	mItems.Add(item);
}
void ParseResult::Add(const ParsePosition &pos, PrErrCode error_code, int value)
{
	ParseResultItem *item = new ParseResultItem(pos, error_code, value);
	mItems.Add(item);
}
#if 0
void ParseResult::Add(size_t row, size_t col, size_t line_number, const wxString &name, PrErrCode error_code)
{
	ParseResultItem *item = new ParseResultItem(row, col, line_number, name, error_code);
	mItems.Add(item);
}
#endif
void ParseResult::Empty()
{
	mItems.Empty();
}
size_t ParseResult::GetCount()
{
	return mItems.GetCount();
}

/// エラーメッセージ
wxString ParseResult::ErrMsg(PrErrCode code)
{
	wxString msg;
	switch(code) {
		case prErrInvalidChar:
			//変換できない文字があります。
			msg = _("Invalid character exists.");
			break;
		case prErrInvalidUTF8Code:
			// 変換できないUTF-8文字があります。
			msg = _("Invalid UTF-8 code exists.");
			break;
		case prErrInvalidToUTF8Code:
			// UTF-8へ変換できない文字があります。
			msg = _("Invalid character which cannot convert to UTF-8 exists.");
			break;
		case prErrInvalidBasicCode:
			// 変換できないBASICコードがあります。
			msg = _("Invalid BASIC code exists.");
			break;
		case prErrStopInvalidUTF8Code:
			// 変換できないUTF-8文字が多くあるため、変換を中止しました。
			msg = _("Aborted converting because too many invalid UTF-8 code exist.");
			break;
		case prErrStopInvalidToUTF8Code:
			// UTF-8へ変換できない文字が多くあるため、変換を中止しました。
			msg = _("Aborted converting because too many chars cannot convert to UTF-8.");
			break;
		case prErrStopInvalidBasicCode:
			// 変換できないBASICコードが多くあるため、変換を中止しました。
			msg = _("Aborted converting because too many invalid BASIC code exists.");
			break;
		case prErrOverflow:
			// オーバーフローです。
			msg = _("Overflow.");
			break;
		case prErrInvalidNumber:
			// 不正な数値が指定されています。
			msg = _("Invalid number is specified.");
			break;
		case prErrInvalidLineNumber:
			// 不正な行番号が指定されています。
			msg = _("Invalid line number is specified.");
			break;
		case prErrDiscontLineNumber:
			// 行番号が前行より小さくなっています。
			msg = _("The line number is smaller than above it.");
			break;
		case prErrDuplicateLineNumber:
			// 行番号が重複しています。
			msg = _("The line number duplicate another line:");
			break;
		case prErrEraseCodeFE:
			// キャラクタコード&HFEは削除されます。
			msg = _("Character code &HFE is erased.");
			break;
		default:
			msg = _("Unknown error.");
			break;
	}

	return msg;
}

/// レポート出力
void ParseResult::Report(wxArrayString &lines)
{
	wxString msg;
	wxString numstr;
	ParseResultItem *itm;
	for (size_t i=0; i<mItems.GetCount(); i++) {
		itm = &mItems.Item(i);
		msg = _("Linenumber:");
		numstr.Printf(_T("%ld"), (wxInt32)(itm->GetLineNumber()));
		msg += numstr;
		msg += _T(" (") + _("Row:");
		numstr.Printf(_T("%lu"), (wxUint32)(itm->GetRow() + 1));
		msg += numstr;
		msg += _T(" ") + _("Col:");
		numstr.Printf(_T("%lu"), (wxUint32)(itm->GetCol() + 1));
		msg += numstr;
		msg += _T(") [") + itm->GetName() + _T("] ");
		msg += ErrMsg(itm->GetErrorCode());
		if (itm->GetValue() >= 0) {
			msg += itm->GetValueString();
		}

		lines.Add(msg);
	}
}
