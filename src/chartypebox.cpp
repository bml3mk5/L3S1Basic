/// @file chartypebox.cpp
///
/// @brief 文字タイプダイアログ
///

#include "chartypebox.h"

//
// Char type box
//
// Attach Event
BEGIN_EVENT_TABLE(CharTypeBox, wxDialog)
END_EVENT_TABLE()

CharTypeBox::CharTypeBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Charactor Type"), wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSize size;

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Choose charactor type.")), flags);
	size.x = DEFAULT_TEXTWIDTH; size.y = -1;
	comCharType = new wxComboBox(this, IDC_COMBO_CHARTYPE, wxEmptyString, wxDefaultPosition, size, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	szrAll->Add(comCharType, flags);
	wxSizer *szrButtons = CreateButtonSizer(wxOK);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);

	mSelectedPos = 0;
}

int CharTypeBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void CharTypeBox::term_dialog()
{
	mSelectedPos = comCharType->GetSelection();
}

void CharTypeBox::AddCharType(const wxArrayString &items)
{
	mOrigCharTypes.Empty();
	comCharType->Clear();
	for(size_t i=0; i<items.GetCount(); i++) {
		mOrigCharTypes.Add(items[i]);
		comCharType->Insert(wxGetTranslation(items[i]), (int)i);
	}
	comCharType->Select(0);
}
/// Char type を返す
wxString CharTypeBox::GetCharType()
{
	return mOrigCharTypes[mSelectedPos];
}
/// Char typeをセット
void CharTypeBox::SetCharType(int pos)
{
	mSelectedPos = pos;
	comCharType->Select(pos);
}
void CharTypeBox::SetCharType(const wxString &char_type)
{
	size_t i = 0;
	for(; i<mOrigCharTypes.GetCount(); i++) {
		if (mOrigCharTypes[i] == char_type) {
			break;
		}
	}
	if (i >= mOrigCharTypes.GetCount()) {
		i = 0;
	}
	SetCharType((int)i);
}

