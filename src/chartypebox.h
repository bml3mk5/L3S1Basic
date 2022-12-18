/// @file chartypebox.h
///
/// @brief 文字タイプダイアログ
///

#ifndef _CHARTYPEBOX_H_
#define _CHARTYPEBOX_H_

#include "common.h"
#include <wx/wx.h>

/// 文字タイプボックス
class CharTypeBox : public wxDialog
{
private:
	wxComboBox *comCharType;

	wxArrayString mOrigCharTypes;

	int mSelectedPos;
	// parameter

	void init_dialog() {};
	void term_dialog();

public:
	CharTypeBox(wxWindow* parent, wxWindowID id);

	enum {
		IDC_COMBO_CHARTYPE = 1
	};

	/// @name functions
	//@{
	int ShowModal();
	void AddCharType(const wxArrayString &items);
	wxString GetCharType();
	void SetCharType(int pos);
	void SetCharType(const wxString &char_type);
	//@}

	// event procedures

	// properties

	DECLARE_EVENT_TABLE()
};

#endif /* _CHARTYPEBOX_H_ */

