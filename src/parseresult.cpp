/// @file parseresult.cpp
///
/// @brief 解析結果保存クラス
///
#include "parseresult.h"
#include <wx/arrimpl.cpp>

ParseResultItem::ParseResultItem()
{
	line = 0;
	pos = 0;
	line_number = 0;
	error_code = prErrNone;
	name.Empty();
}
ParseResultItem::ParseResultItem(long new_line, long new_pos, long new_line_number, const wxString &new_name, PrErrCode new_error_code)
{
	line = new_line;
	pos = new_pos;
	line_number = new_line_number;
	error_code = new_error_code;
	name = new_name;
}

WX_DEFINE_OBJARRAY(ParseResultItems);

ParseResult::ParseResult()
{
	Empty();
}
void ParseResult::Add(long new_line, long new_pos, long new_line_number, const wxString &new_name, PrErrCode new_error_code)
{
	ParseResultItem *item = new ParseResultItem(new_line, new_pos, new_line_number, new_name, new_error_code);
	items.Add(item);
}
void ParseResult::Empty()
{
	items.Empty();
}
size_t ParseResult::GetCount()
{
	return items.GetCount();
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
			msg = _("Line number is smaller than above it.");
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
	for (size_t i=0; i<items.GetCount(); i++) {
		itm = &items.Item(i);
		msg = _("Linenumber:");
		numstr.Printf(_T("%ld"), itm->line_number);
		msg += numstr;
		msg += _T(" (") + _("Row:");
		numstr.Printf(_T("%ld"), itm->line);
		msg += numstr;
		msg += _T(" ") + _("Col:");
		numstr.Printf(_T("%ld"), itm->pos);
		msg += numstr;
		msg += _T(") [") + itm->name + _T("] ");
		msg += ErrMsg(itm->error_code);

		lines.Add(msg);
	}
}
