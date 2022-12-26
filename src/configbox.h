/// @file configbox.h
///
/// @brief 入出力設定ダイアログ
///
#ifndef CONFIGBOX_H
#define CONFIGBOX_H

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include "parseparam.h"

/// 入出力設定ダイアログ
class ConfigBox : public wxDialog, public ParseParam
{
private:
	wxCheckBox    *chkEofTextRead;
	wxCheckBox    *chkEofBinary;
	wxRadioButton *radNewLineAscii[3];
	wxCheckBox    *chkEofAscii;
	wxRadioButton *radNewLineUtf8[3];
	wxCheckBox    *chkBom;
	wxComboBox    *comStartAddr;

	void init_dialog();
	void term_dialog();

public:
	ConfigBox(wxWindow* parent, wxWindowID id);

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
	int ShowModal();
	void AddStartAddrItems(const int *items, size_t count);
	//@}

	/// event procedures
	//@{
	//@}

	DECLARE_EVENT_TABLE()
};

#endif /* CONFIGBOX_H */
