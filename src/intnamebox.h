/// @file intnamebox.h
///
/// @brief 内部ファイル名ダイアログ
///

#ifndef _INTNAMEBOX_H_
#define _INTNAMEBOX_H_

#include "common.h"
#include <wx/wx.h>

/// バリデータ
class IntNameValidator : public wxTextValidator
{
public:
	IntNameValidator();
};

/// 内部ファイル名ボックス
class IntNameBox : public wxDialog
{
private:
	wxTextCtrl *txtIntName;

	// parameter

	void init_dialog() {};
	void term_dialog();

public:
	IntNameBox(wxWindow* parent, wxWindowID id);

	enum {
		IDC_TEXT_INTNAME = 1
	};

	/// @name functions
	//@{
	int ShowModal();
	//@}

	// event procedures
	void OnOK(wxCommandEvent& event);

	// properties
	void SetInternalName(const wxString &item);
	wxString GetInternalName() const;

	DECLARE_EVENT_TABLE()
};

#endif /* _INTNAMEBOX_H_ */

