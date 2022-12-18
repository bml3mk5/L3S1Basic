/// @file intnamebox.cpp
///
/// @brief 内部ファイル名ダイアログ
///

#include "intnamebox.h"

#define DUMMY_STRING _("Required information entry is empty."), _("'%s' is invalid")

IntNameValidator::IntNameValidator()
	: wxTextValidator(wxFILTER_EMPTY | wxFILTER_INCLUDE_CHAR_LIST)
{
	wxTextValidator::SetCharIncludes(" !#$%&'*+,-./0123456789:<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_abcdefghijklmnopqrstuvwxyz{|}~");
}

// Attach Event
BEGIN_EVENT_TABLE(IntNameBox, wxDialog)
	EVT_BUTTON(wxID_OK, IntNameBox::OnOK)
END_EVENT_TABLE()

IntNameBox::IntNameBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Internal Name"), wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSize size;
	long style = 0;
	IntNameValidator validate;

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Input internal name for tape image.")), flags);
	size.x = DEFAULT_TEXTWIDTH; size.y = -1;
	txtIntName = new wxTextCtrl(this, IDC_TEXT_INTNAME, wxEmptyString, wxDefaultPosition, size, style, validate);
	txtIntName->SetMaxLength(8);
	szrAll->Add(txtIntName, flags);
	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int IntNameBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void IntNameBox::term_dialog()
{
}

void IntNameBox::OnOK(wxCommandEvent& event)
{
	if (Validate() && TransferDataFromWindow()) {
		if (IsModal()) {
			EndModal(wxID_OK);
		} else {
			SetReturnCode(wxID_OK);
			this->Show(false);
		}
	}
}

void IntNameBox::SetInternalName(const wxString &item)
{
	txtIntName->SetValue(item);
}
wxString IntNameBox::GetInternalName() const
{
	wxString val = txtIntName->GetValue();
	int len = (int)val.Length();
	if (len < 8) {
		val += wxString(' ', 8 - len);
	} else if (len > 8) {
		val = val.Mid(0, 8);
	}
	return val;
}
