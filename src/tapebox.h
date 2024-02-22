/// @file tapebox.h
///
/// @brief テープイメージ設定ボックス
///
#ifndef TAPEBOX_H
#define TAPEBOX_H

#include "common.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/dynarray.h>
#include <wx/dnd.h>
#include "parse.h"

/// テープイメージ設定ボックス
class TapeBox : public wxDialog
{
private:
//	wxCheckBox *chkUseTape;
	wxTextCtrl *txtIntName;
	wxString	mInternalName;

	Parse *ps;

	// parameter

	void init_dialog();
	void term_dialog();

public:
	TapeBox(wxWindow* parent, wxWindowID id, Parse *new_ps);

	enum {
		IDC_CHECK_USETAPE = 1,
		IDC_TEXT_INTNAME
	};

	/// @name functions
	//@{
	int ShowModal();
	//@}
	// event procedures
	//@{
	void OnCheckUseTape(wxCommandEvent& event);
	void OnClickOk(wxCommandEvent& event);
	//@}
	// properties
	//@{
//	bool UseTape() const;
	void SetInternalName(const wxString &val);
	wxString GetInternalName() const;
	//@}

	DECLARE_EVENT_TABLE()
};

#endif /* TAPEBOX_H */

