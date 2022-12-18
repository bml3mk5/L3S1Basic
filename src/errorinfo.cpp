/// @file errorinfo.cpp
///
/// @brief エラー情報
///
#include "errorinfo.h"

PsErrInfo::PsErrInfo()
{
	mType = psOK;
	mCode = psErrNone;
	mMsg = _T("");
	mLine = 0;
}

PsErrInfo::~PsErrInfo()
{
}

/// エラーメッセージ
wxString PsErrInfo::ErrMsg(PsErrCode code)
{
	wxString msg;
	switch(code) {
		case psErrFileNotFound:
			// ファイルがみつかりません。
			msg = _("File not found.");
			break;
		case psErrCannotOpen:
			// ファイルを開けません。
			msg = _("Cannot open file.");
			break;
		case pwErrCannotWrite:
			// ファイルを出力できません。
			msg = _("Cannot write file.");
			break;
		case psErrFileIsEmpty:
			// ファイルが空です。
			msg = _("File is empty.");
			break;
		case pwErrSameFile:
			// 同じファイルを指定することはできません。
			msg = _("Cannot specify the same file.");
			break;
		case psErrFileIsEncrypted:
			// ファイルはおそらく暗号化されています。
			msg = _("File is probably encrypted.");
			break;
		case psErrBasic:
			// ファイルはおそらくBASICではありません。
			msg = _("File is probably no BASIC data.");
			break;
		case psErrParseBasic:
			// 解析を中止します。解析できないBASICコードが多すぎます。
			msg = _("Parse stopped. Too many invalid BASIC codes exist.");
			break;
		case psErrInvalidString:
			// 解析に中止します。解析できない文字が多すぎます。
			msg = _("Parse stopped. Too many invalid charactors exist.");
			break;
		case psInfoFileInApp:
			// ファイルをこのアプリケーションと同じ場所においてください。
			msg = _("Please put on the file as same place as this application.");
			break;
		case psInfoContinue:
			// 処理を続行します。
			msg = _("Continue this process.");
			break;
		default:
			msg = _("Unknown error.");
			break;
	}

	return msg;
}

/// エラー情報セット
void PsErrInfo::SetInfo(int line, PsErrType type, PsErrCode code, const wxString &msg)
{
		mType = type;
		mCode = code;
		mMsg = ErrMsg(code);
		if (!msg.IsEmpty()) {
			mMsg += _T(" (") + msg + _T(")");
		}
		mLine = line;
}

void PsErrInfo::SetInfo(int line, PsErrType type, PsErrCode code1, PsErrCode code2, const wxString &msg)
{
		mType = type;
		mCode = code1;
		mMsg = ErrMsg(code1);
		if (!msg.IsEmpty()) {
			mMsg += _T(" (") + msg + _T(")");
		}
		mMsg += _T("\n") + ErrMsg(code2);
		mLine = line;
}

/// gui メッセージBOX
void PsErrInfo::ShowMsgBox(wxWindow *win)
{
	switch(mType) {
		case psError:
			wxMessageBox(mMsg, _("Error"), wxOK | wxICON_ERROR, win);
			break;
		case psWarning:
			wxMessageBox(mMsg, _("Warning"), wxOK | wxICON_WARNING, win);
			break;
		default:
			break;
	}
}
