/// @file configbox.cpp
///
/// @brief 入出力設定ダイアログ
///
#include "configbox.h"
#include <wx/statline.h>
#include <wx/colordlg.h>
#include <wx/notebook.h>
#include "main.h"
#include "basicspecs.h"


ConfigParamCtrl::ConfigParamCtrl()
	: wxPanel()
	, ParseParam()
{
}

ConfigParamCtrl::ConfigParamCtrl(wxWindow* parent, wxWindowID id, ConfigParam &cp, Parse &ps)
	: wxPanel(parent, id)
	, ParseParam(cp.GetParam())
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	wxGridSizer *gszr;
	wxBoxSizer *hbox;

	// --------------------
	// Input file

	wxBoxSizer *vboxNL = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Input File")), wxVERTICAL);
	gszr = new wxFlexGridSizer(1, 2, 0, 0);

	gszr->Add(new wxStaticText(this, wxID_ANY, _("Text File")), flags);
	chkEofTextRead = new wxCheckBox(this, IDC_CHK_TEXTREAD_EOF, _("Read Until End of File Mark"));
	gszr->Add(chkEofTextRead, flags);

	vboxNL->Add(gszr, flags);

	szrAll->Add(vboxNL, flags);

	// --------------------
	// Output file

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

	lblStartAddr = new wxStaticText(this, wxID_ANY, wxEmptyString);
	gszr->Add(lblStartAddr, flags);
	comStartAddr = new wxComboBox(this, IDC_COMBO_STARTADDR, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	gszr->Add(comStartAddr, flags);
	vboxNL->Add(gszr, flags);

	AddStartAddrItems(ps.GetStartAddrTitle(), ps.GetStartAddrsPtr(), ps.GetStartAddrCount());

	szrAll->Add(vboxNL, flags);

	SetSizerAndFit(szrAll);
}

void ConfigParamCtrl::Initialize()
{
	chkEofTextRead->SetValue(mEofTextRead);
	chkEofBinary->SetValue(mEofBinary);
	if (CR <= mNewLineAscii && mNewLineAscii <= CRLF) {
		radNewLineAscii[mNewLineAscii]->SetValue(true);
	}
	chkEofAscii->SetValue(mEofAscii);
	if (CR <= mNewLineUtf8 && mNewLineUtf8 <= CRLF) {
		radNewLineUtf8[mNewLineUtf8]->SetValue(true);
	}
	chkBom->SetValue(mIncludeBOM);
	comStartAddr->Select(mStartAddr);
}

void ConfigParamCtrl::Terminate()
{
	mEofTextRead = chkEofTextRead->GetValue();
	mEofBinary = chkEofBinary->GetValue();
	for(int i=CR; i<=CRLF; i++) {
		if (radNewLineAscii[i]->GetValue()) {
			mNewLineAscii = i;
			break;
		}
	}
	mEofAscii = chkEofAscii->GetValue();
	for(int i=CR; i<=CRLF; i++) {
		if (radNewLineUtf8[i]->GetValue()) {
			mNewLineUtf8 = i;
			break;
		}
	}
	mIncludeBOM = chkBom->GetValue();
	mStartAddr = comStartAddr->GetSelection();
}

void ConfigParamCtrl::AddStartAddrItems(const wxString &title, const int *items, size_t count)
{
	wxString str;

	lblStartAddr->SetLabelText(title);

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


// Attach Event
BEGIN_EVENT_TABLE(ConfigBox, wxDialog)
END_EVENT_TABLE()

ConfigBox::ConfigBox(wxWindow* parent, wxWindowID id, ParseCollection &coll)
	: wxDialog(parent, id, _("File Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	book = new wxNotebook(this, wxID_ANY);
	szrAll->Add(book, flags);

	ctrlParam[0] = new ConfigParamCtrl(book, IDC_CTRL_L3S1, gConfig.GetParam(eL3S1Basic), *coll.Get(eL3S1Basic));
	book->AddPage(ctrlParam[0], _("L3/S1 BASIC"));

	ctrlParam[1] = new ConfigParamCtrl(book, IDC_CTRL_MSX, gConfig.GetParam(eMSXBasic), *coll.Get(eMSXBasic));
	book->AddPage(ctrlParam[1], _("MSX BASIC"));

	// OK and Cancel Buttons

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

//void ConfigBox::AddStartAddrItems(int index, const wxString &title, const int *items, size_t count)
//{
//	ctrlParam[index]->AddStartAddrItems(title, items, count);
//}

void ConfigBox::init_dialog()
{
	for(int i= 0; i<eMachineCount; i++) {
		ctrlParam[i]->Initialize();
	}
	// select the tab
	book->SetSelection(gConfig.GetCurrentMachine());
}

void ConfigBox::term_dialog()
{
	for(int i= 0; i<eMachineCount; i++) {
		ctrlParam[i]->Terminate();
	}
}

//ParseParam &ConfigBox::GetParam(int index)
//{
//	return ctrlParam[index]->GetParam();
//}

//void ConfigBox::SetParam(int index, const ParseParam &param)
//{
//	ctrlParam[index]->SetParam(param);
//}

void ConfigBox::GetParams(ParseCollection &coll) const
{
	for(int i= 0; i<eMachineCount; i++) {
		gConfig.GetParam(i).SetParam(ctrlParam[i]->GetParam());
//		coll.Get(i)->SetParam(ctrlParam[i]->GetParam());
	}
}
