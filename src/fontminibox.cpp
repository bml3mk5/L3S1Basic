/// @file fontminibox.cpp
///
/// @brief フォントミニダイアログ
///

#include "fontminibox.h"
#include <wx/fontenum.h>

// Attach Event
BEGIN_EVENT_TABLE(FontMiniBox, wxDialog)
	EVT_TEXT(IDC_COMBO_FONTSIZE, FontMiniBox::OnTextSize)
END_EVENT_TABLE()

FontMiniBox::FontMiniBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Font"), wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSize size;

	wxTextValidator tVali(wxFILTER_NUMERIC);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	size.x = DEFAULT_TEXTWIDTH; size.y = -1;
	comFontName = new wxComboBox(this, IDC_COMBO_FONTNAME, wxEmptyString, wxDefaultPosition, size, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	comFontSize = new wxComboBox(this, IDC_COMBO_FONTSIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN, tVali);
	hbox->Add(comFontName, flags);
	hbox->Add(comFontSize, flags);
	szrAll->Add(hbox, flags);
	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int FontMiniBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void FontMiniBox::init_dialog()
{
	mFontNames = wxFontEnumerator::GetFacenames();
	mFontNames.Sort();

	wxString size;
	mFontSizes.Empty();
	for(int i=4; i<=24; i++) {
		size.Printf(_T("%d"), i);
		mFontSizes.Add(size);
	}

	comFontName->Clear();
	comFontName->Insert(mFontNames, 0);
	comFontSize->Clear();
	comFontSize->Insert(mFontSizes, 0);

	comFontName->SetValue(mSelectedName);
	size.Printf(_T("%d"), mSelectedSize);
	comFontSize->SetValue(size);
}

void FontMiniBox::term_dialog()
{
	mSelectedName = comFontName->GetValue();
	long val;
	comFontSize->GetValue().ToLong(&val);
	if (val < 1) val = 1;
	if (val > 99) val = 99;
	mSelectedSize = (int)val;
}

void FontMiniBox::OnTextSize(wxCommandEvent& event)
{
	wxString mInputSize = event.GetString().Left(2);
	comFontSize->ChangeValue(mInputSize);
}
