/// @file errorinfo.h
///
/// @brief エラー情報
///
#ifndef _ERRORINFO_H_
#define _ERRORINFO_H_

#include "common.h"
#include <wx/wx.h>

/// エラータイプ
typedef enum enumPsErrType {
	psOK = 0,
	psError,
	psWarning
} PsErrType;

/// エラーコード
typedef enum enumPsErrCode {
	psErrNone = 0,
	psErrFileNotFound,
	psErrCannotOpen,
	pwErrCannotWrite,
	psErrFileIsEmpty,
	pwErrSameFile,
	psErrFileIsEncrypted,
	psErrBasic,
	psErrParseBasic,
	psErrInvalidString,
	psInfoFileInApp,
	psInfoContinue,
	psErrUnknown
} PsErrCode;

/// エラー情報保存用
class PsErrInfo
{
private:
	PsErrType mType;
	PsErrCode mCode;
	wxString  mMsg;
	int       mLine;

public:
	PsErrInfo();
	~PsErrInfo();

	/// エラーメッセージ
	wxString ErrMsg(PsErrCode code);
	/// エラー情報セット
	void SetInfo(int line, PsErrType type, PsErrCode code, const wxString &msg = wxEmptyString);
	/// エラー情報セット
	void SetInfo(int line, PsErrType type, PsErrCode code1, PsErrCode code2, const wxString &msg = wxEmptyString);

	/// gui メッセージBOX
	void ShowMsgBox(wxWindow *win = 0);

};

#endif /* _ERRORINFO_H_ */
