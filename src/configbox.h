/// @file configbox.h
///
/// @brief 入出力設定ダイアログ
///
#ifndef CONFIGBOX_H
#define CONFIGBOX_H

#include "common.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dialog.h>
#include "parseparam.h"
#include "parse.h"
#include "config.h"

class wxNotebook;

/// 入出力設定サブコントロール
class ConfigParamCtrl : public wxPanel, public ParseParam
{
private:
	wxCheckBox    *chkEofTextRead;
	wxCheckBox    *chkEofBinary;
	wxRadioButton *radNewLineAscii[3];
	wxCheckBox    *chkEofAscii;
	wxRadioButton *radNewLineUtf8[3];
	wxCheckBox    *chkBom;
	wxStaticText  *lblStartAddr;
	wxComboBox    *comStartAddr;

public:
	ConfigParamCtrl();
	ConfigParamCtrl(wxWindow* parent, wxWindowID id, ConfigParam &cp, Parse &ps);

	enum {
		IDC_CHK_TEXTREAD_EOF = 1,
		IDC_CHK_BINARY_EOF,
		IDC_RADIO_ASCII_CR,
		IDC_RADIO_ASCII_LF,
		IDC_RADIO_ASCII_CRLF,
		IDC_CHK_ASCII_EOF,
		IDC_RADIO_UTF8_CR,
		IDC_RADIO_UTF8_LF,
		IDC_RADIO_UTF8_CRLF,
		IDC_CHK_BOM,
		IDC_COMBO_STARTADDR,
	};

	/// functions
	//@{
	void Initialize();
	void Terminate();
	void AddStartAddrItems(const wxString &title, const int *items, size_t count);
	//@}
};

/// 入出力設定ダイアログ
class ConfigBox : public wxDialog
{
private:
	wxNotebook *book;
	ConfigParamCtrl *ctrlParam[eMachineCount];

	void init_dialog();
	void term_dialog();

public:
	ConfigBox(wxWindow* parent, wxWindowID id, ParseCollection &coll);

	enum {
		IDC_CTRL_L3S1 = 21,
		IDC_CTRL_MSX,
	};

	/// functions
	//@{
	int ShowModal();
//	void AddStartAddrItems(int index, const wxString &title, const int *items, size_t count);
//	ParseParam &GetParam(int index);
//	void SetParam(int index, const ParseParam &param);
	void GetParams(ParseCollection &coll) const;
	//@}

	/// event procedures
	//@{
	//@}

	DECLARE_EVENT_TABLE()
};

#endif /* CONFIGBOX_H */
