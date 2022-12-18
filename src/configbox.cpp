/// @file configbox.cpp
///
/// @brief 設定ダイアログ
///
#include "configbox.h"

// Attach Event
BEGIN_EVENT_TABLE(ConfigBox, wxDialog)
END_EVENT_TABLE()

ConfigBox::ConfigBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Configure"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
	, ParseParam()
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	wxGridSizer *gszr;
	wxBoxSizer *hbox;

	wxBoxSizer *vboxNL = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Input File")), wxVERTICAL);
	gszr = new wxFlexGridSizer(1, 2, 0, 0);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("Text File")), flags);
	chkEofTextRead = new wxCheckBox(this, IDC_CHK_TEXTREAD_EOF, _("Read Until End of File Mark"));
	gszr->Add(chkEofTextRead, flags);

	vboxNL->Add(gszr, flags);

	szrAll->Add(vboxNL);


	vboxNL = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Output File")), wxVERTICAL);
	gszr = new wxFlexGridSizer(6, 2, 0, 0);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("Intermediate Lang.")), flags);
	chkEofBinary = new wxCheckBox(this, IDC_CHK_BINARY_EOF, _("Add End of File"));
	gszr->Add(chkEofBinary, flags);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("Ascii Text")), flags);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	radNewLineAscii[0] = new wxRadioButton(this, IDC_RADIO_ASCII_CR, _("CR"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	radNewLineAscii[1] = new wxRadioButton(this, IDC_RADIO_ASCII_LF, _("LF"));
	radNewLineAscii[2] = new wxRadioButton(this, IDC_RADIO_ASCII_CRLF, _("CR+LF"));
	hbox->Add(radNewLineAscii[0], flags);
	hbox->Add(radNewLineAscii[1], flags);
	hbox->Add(radNewLineAscii[2], flags);
	gszr->Add(hbox, flags);

	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	chkEofAscii = new wxCheckBox(this, IDC_CHK_ASCII_EOF, _("Add End of File"));
	gszr->Add(chkEofAscii, flags);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("UTF-8 Text")), flags);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	radNewLineUtf8[0] = new wxRadioButton(this, IDC_RADIO_UTF8_CR, _("CR"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	radNewLineUtf8[1] = new wxRadioButton(this, IDC_RADIO_UTF8_LF, _("LF"));
	radNewLineUtf8[2] = new wxRadioButton(this, IDC_RADIO_UTF8_CRLF, _("CR+LF"));
	hbox->Add(radNewLineUtf8[0], flags);
	hbox->Add(radNewLineUtf8[1], flags);
	hbox->Add(radNewLineUtf8[2], flags);
	gszr->Add(hbox, flags);

	gszr->Add(new wxStaticText(this, wxID_ANY, _T("")), flags);
	chkBom = new wxCheckBox(this, IDC_CHK_BOM, _("Include BOM"));
	gszr->Add(chkBom, flags);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("Start Address (L3 only)")), flags);
	comStartAddr = new wxComboBox(this, IDC_COMBO_STARTADDR, _T(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	gszr->Add(comStartAddr, flags);
	vboxNL->Add(gszr, flags);

	szrAll->Add(vboxNL);

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int ConfigBox::ShowModal()
{
	init_dialog();
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void ConfigBox::AddStartAddrItems(const int *items, size_t count)
{
	wxString str;

	if (count == 0) return;

	comStartAddr->Clear();
	for(size_t i=0; i<count; i++) {
		str.Printf(_T("&H%04X"), items[i]);
		comStartAddr->Insert(str, (int)i);
	}
	if ((size_t)mStartAddr < count) {
		comStartAddr->Select(mStartAddr);
	} else {
		comStartAddr->Select(0);
	}
}

void ConfigBox::init_dialog()
{
	chkEofTextRead->SetValue(mEofTextRead);
	chkEofBinary->SetValue(mEofBinary);
	if (0 <= mNewLineAscii && mNewLineAscii <= 2) {
		radNewLineAscii[mNewLineAscii]->SetValue(true);
	}
	chkEofAscii->SetValue(mEofAscii);
	if (0 <= mNewLineUtf8 && mNewLineUtf8 <= 2) {
		radNewLineUtf8[mNewLineUtf8]->SetValue(true);
	}
	chkBom->SetValue(mIncludeBOM);
	comStartAddr->Select(mStartAddr);
}

void ConfigBox::term_dialog()
{
	mEofTextRead = chkEofTextRead->GetValue();
	mEofBinary = chkEofBinary->GetValue();
	for(int i=0; i<3; i++) {
		if (radNewLineAscii[i]->GetValue()) {
			mNewLineAscii = i;
			break;
		}
	}
	mEofAscii = chkEofAscii->GetValue();
	for(int i=0; i<3; i++) {
		if (radNewLineUtf8[i]->GetValue()) {
			mNewLineUtf8 = i;
			break;
		}
	}
	mIncludeBOM = chkBom->GetValue();
	mStartAddr = comStartAddr->GetSelection();
}
