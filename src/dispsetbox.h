/// @file dispsetbox.h
///
/// @brief 表示設定ダイアログ
///
#ifndef DISPSETBOX_H
#define DISPSETBOX_H

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include "parseparam.h"

/// 表示設定ダイアログ
class DisplaySettingBox : public wxDialog
{
private:
	wxCheckBox    *chkAddSpace;
	wxTextCtrl    *txtClLinenum;
	wxTextCtrl    *txtClStatement;
	wxTextCtrl    *txtClQuoted;
	wxTextCtrl    *txtClComment;
	wxTextCtrl    *txtClDataline;
	wxChoice      *comLanguage;

	void init_dialog();
	void term_dialog();

public:
	DisplaySettingBox(wxWindow* parent, wxWindowID id);

	enum {
		IDC_CHK_ADDSPACE = 1,
		IDC_TXT_CL_LINENUM,
		IDC_BTN_CL_C_LINENUM,
		IDC_BTN_CL_R_LINENUM,
		IDC_TXT_CL_STATEMENT,
		IDC_BTN_CL_C_STATEMENT,
		IDC_BTN_CL_R_STATEMENT,
		IDC_TXT_CL_QUOTED,
		IDC_BTN_CL_C_QUOTED,
		IDC_BTN_CL_R_QUOTED,
		IDC_TXT_CL_COMMENT,
		IDC_BTN_CL_C_COMMENT,
		IDC_BTN_CL_R_COMMENT,
		IDC_TXT_CL_DATALINE,
		IDC_BTN_CL_C_DATALINE,
		IDC_BTN_CL_R_DATALINE,
		IDC_COMBO_LANGUAGE,
	};

	/// functions
	//@{
	int ShowModal();
	//@}

	/// event procedures
	//@{
	void OnChangeColor(wxCommandEvent &event);
	void OnRevertColor(wxCommandEvent &event);
	//@}

	DECLARE_EVENT_TABLE()
};

#endif /* DISPSETBOX_H */
