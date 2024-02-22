/// @file dispsetbox.h
///
/// @brief 表示設定ダイアログ
///
#ifndef DISPSETBOX_H
#define DISPSETBOX_H

#include "common.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dialog.h>
#include "config.h"

class wxNotebook;

/// 表示設定コントロール
class DisplaySettingCtrl : public wxPanel
{
private:
	ConfigParam   *pConfig;

	wxCheckBox    *chkAddSpace;
	wxTextCtrl    *txtClLinenum;
	wxTextCtrl    *txtClStatement;
	wxTextCtrl    *txtClVariable;
	wxTextCtrl    *txtClQuoted;
	wxTextCtrl    *txtClComment;
	wxTextCtrl    *txtClDataline;

public:
	DisplaySettingCtrl(wxWindow* parent, wxWindowID id, ConfigParam *config);

	enum {
		IDC_CHK_ADDSPACE = 1,
		IDC_TXT_CL_LINENUM,
		IDC_BTN_CL_C_LINENUM,
		IDC_BTN_CL_R_LINENUM,
		IDC_TXT_CL_STATEMENT,
		IDC_BTN_CL_C_STATEMENT,
		IDC_BTN_CL_R_STATEMENT,
		IDC_TXT_CL_VARIABLE,
		IDC_BTN_CL_C_VARIABLE,
		IDC_BTN_CL_R_VARIABLE,
		IDC_TXT_CL_QUOTED,
		IDC_BTN_CL_C_QUOTED,
		IDC_BTN_CL_R_QUOTED,
		IDC_TXT_CL_COMMENT,
		IDC_BTN_CL_C_COMMENT,
		IDC_BTN_CL_R_COMMENT,
		IDC_TXT_CL_DATALINE,
		IDC_BTN_CL_C_DATALINE,
		IDC_BTN_CL_R_DATALINE,
	};

	/// functions
	//@{
	void Initialize();
	void Terminate();
	//@}

	/// event procedures
	//@{
	void OnChangeColor(wxCommandEvent &event);
	void OnRevertColor(wxCommandEvent &event);
	//@}

	DECLARE_EVENT_TABLE()
};

/// 表示設定ダイアログ
class DisplaySettingBox : public wxDialog
{
private:
	wxNotebook *book;
	DisplaySettingCtrl *ctrlParam[eMachineCount];

	wxChoice      *comLanguage;

	void init_dialog();
	void term_dialog();

public:
	DisplaySettingBox(wxWindow* parent, wxWindowID id);

	enum {
		IDC_COMBO_LANGUAGE = 51,
		IDC_CTRL_L3S1,
		IDC_CTRL_MSX
	};

	/// functions
	//@{
	int ShowModal();
	//@}

	DECLARE_EVENT_TABLE()
};

#endif /* DISPSETBOX_H */
