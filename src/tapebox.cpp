/// @file tapebox.cpp
///
/// @brief テープイメージ設定ボックス
///
#include "tapebox.h"
#include "configbox.h"

//
// Tape image setting box
//
// Attach Event
BEGIN_EVENT_TABLE(TapeBox, wxDialog)
//	EVT_CHECKBOX(IDC_CHECK_USETAPE, TapeBox::OnCheckUseTape)
	EVT_BUTTON(wxID_OK, TapeBox::OnClickOk)
END_EVENT_TABLE()

TapeBox::TapeBox(wxWindow* parent, wxWindowID id, Parse *new_ps)
	: wxDialog(parent, id, _("Tape image"), wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	ps = new_ps;

	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSize size;

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Input internal name for tape image.")), flags);
	szrAll->Add(new wxStaticText(this, wxID_ANY, wxString::Format(_("(max %d charactors)"), ps->GetInternalNameSize())), flags);
	size.x = DEFAULT_TEXTWIDTH; size.y = -1;
//	chkUseTape = new wxCheckBox(this, IDC_CHECK_USETAPE, _("Save as a tape image"), wxDefaultPosition, wxDefaultSize);
//	szrAll->Add(chkUseTape, flags);
//	wxStaticText *sta = new wxStaticText(this, wxID_ANY, _("Internal file name (6 bytes)"));
//	szrAll->Add(sta, flags);
	txtIntName = new wxTextCtrl(this, IDC_TEXT_INTNAME, wxEmptyString, wxDefaultPosition, size, wxTE_PROCESS_ENTER);
	txtIntName->SetMaxLength(ps->GetInternalNameSize());
	szrAll->Add(txtIntName, flags);
	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int TapeBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void TapeBox::init_dialog()
{
//	if (chkUseTape->GetValue()) {
//		txtIntName->Enable(true);
//	} else {
//		txtIntName->Enable(false);
//	}
	wxString str;
	ps->ConvAsciiToUTF8OneLine(mInternalName, ps->GetCharType(0), str);
	txtIntName->SetValue(str);
}

void TapeBox::term_dialog()
{
	mInternalName += wxString(wxChar(' '), ps->GetInternalNameSize());
	mInternalName = mInternalName.Left(ps->GetInternalNameSize());
}

void TapeBox::OnCheckUseTape(wxCommandEvent &event)
{
	if (event.IsChecked()) {
		txtIntName->Enable(true);
	} else {
		txtIntName->Enable(false);
	}
}

void TapeBox::OnClickOk(wxCommandEvent &event)
{
	if (txtIntName->IsEnabled() && txtIntName->IsEmpty()) {
		wxMessageBox(_("Please input internal file name.") // 内部ファイル名を入力してください。
			, _("Error"), wxOK | wxICON_ERROR, this);
		return;
	}
	int rc = ps->ConvUTF8ToAsciiOneLine(ps->GetCharType(0), txtIntName->GetValue(), mInternalName);
	if (rc != 0) {
		wxMessageBox(_("Internal file name accepts only alphabets and digits.") // 内部ファイル名は半角英数字で入力してください。
			, _("Error"), wxOK | wxICON_ERROR, this);
		return;
	}
	EndModal(wxID_OK);
}

//bool TapeBox::UseTape() const
//{
//	return chkUseTape->GetValue();
//}

void TapeBox::SetInternalName(const wxString &val)
{
	mInternalName = val.Left(ps->GetInternalNameSize());
}

wxString TapeBox::GetInternalName() const
{
	return mInternalName;
}

