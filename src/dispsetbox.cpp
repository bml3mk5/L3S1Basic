/// @file dispsetbox.cpp
///
/// @brief 表示設定ダイアログ
///
#include "dispsetbox.h"
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/colordlg.h>
#include "main.h"
#include "colortag.h"
#include "version.h"

// Attach Event
BEGIN_EVENT_TABLE(DisplaySettingCtrl, wxPanel)
	EVT_BUTTON(IDC_BTN_CL_C_LINENUM, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_STATEMENT, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_VARIABLE, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_QUOTED, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_COMMENT, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_DATALINE, DisplaySettingCtrl::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_R_LINENUM, DisplaySettingCtrl::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_STATEMENT, DisplaySettingCtrl::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_VARIABLE, DisplaySettingCtrl::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_QUOTED, DisplaySettingCtrl::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_COMMENT, DisplaySettingCtrl::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_DATALINE, DisplaySettingCtrl::OnRevertColor)
END_EVENT_TABLE()

DisplaySettingCtrl::DisplaySettingCtrl(wxWindow* parent, wxWindowID id, ConfigParam *config)
	: wxPanel(parent, id)
	, pConfig(config)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
	wxGridSizer *gszr;

	// --------------------
	// Display
	// --------------------

	chkAddSpace = new wxCheckBox(this, IDC_CHK_ADDSPACE, _("Insert a space after colon and comma."));
	szrAll->Add(chkAddSpace, flags);

	// Color

	szrAll->Add(new wxStaticLine(this, wxID_ANY), flags);
	szrAll->Add(new wxStaticText(this, wxID_ANY, _("Text Color")), flags);

	gszr = new wxGridSizer(6, 3, 2, 2);
	wxButton *btn;
	long style = wxTE_CENTRE | wxTE_NO_VSCROLL | wxTE_READONLY;
	txtClLinenum = new wxTextCtrl(this, IDC_TXT_CL_LINENUM, _("Line Number"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClLinenum, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_LINENUM, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_LINENUM, _("Revert"));
	gszr->Add(btn, flags);

	txtClStatement = new wxTextCtrl(this, IDC_TXT_CL_STATEMENT, _("Statement"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClStatement, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_STATEMENT, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_STATEMENT, _("Revert"));
	gszr->Add(btn, flags);

	txtClVariable = new wxTextCtrl(this, IDC_TXT_CL_VARIABLE, _("Variable"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClVariable, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_VARIABLE, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_VARIABLE, _("Revert"));
	gszr->Add(btn, flags);

	txtClQuoted = new wxTextCtrl(this, IDC_TXT_CL_QUOTED, _("Quoted String"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClQuoted, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_QUOTED, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_QUOTED, _("Revert"));
	gszr->Add(btn, flags);

	txtClComment = new wxTextCtrl(this, IDC_TXT_CL_COMMENT, _("Comment"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClComment, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_COMMENT, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_COMMENT, _("Revert"));
	gszr->Add(btn, flags);

	txtClDataline = new wxTextCtrl(this, IDC_TXT_CL_DATALINE, _("\"DATA\" Line"), wxDefaultPosition, wxDefaultSize, style);
	gszr->Add(txtClDataline, flags);
	btn = new wxButton(this, IDC_BTN_CL_C_DATALINE, _("Change"));
	gszr->Add(btn, flags);
	btn = new wxButton(this, IDC_BTN_CL_R_DATALINE, _("Revert"));
	gszr->Add(btn, flags);

	szrAll->Add(gszr, flags);

	SetSizerAndFit(szrAll);
}

void DisplaySettingCtrl::Initialize()
{
	// Set from config file
	chkAddSpace->SetValue(pConfig->EnableAddSpaceAfterColon());

	// Set color
	const MyColorTag *pColorTag = &pConfig->GetColorTag();
	wxColour color;
	pColorTag->Get(COLOR_TAG_LINENUMBER, color);
	txtClLinenum->SetForegroundColour(color);
	pColorTag->Get(COLOR_TAG_STATEMENT, color);
	txtClStatement->SetForegroundColour(color);
	pColorTag->Get(COLOR_TAG_VARIABLE, color);
	txtClVariable->SetForegroundColour(color);
	pColorTag->Get(COLOR_TAG_QUOTED, color);
	txtClQuoted->SetForegroundColour(color);
	pColorTag->Get(COLOR_TAG_COMMENT, color);
	txtClComment->SetForegroundColour(color);
	pColorTag->Get(COLOR_TAG_DATALINE, color);
	txtClDataline->SetForegroundColour(color);
}

void DisplaySettingCtrl::Terminate()
{
	// Set to config file
	pConfig->EnableAddSpaceAfterColon(chkAddSpace->GetValue());

	// Set color
	MyColorTag *pColorTag = &pConfig->GetColorTag();
	wxColour color;
	color = txtClLinenum->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_LINENUMBER, color);
	color = txtClStatement->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_STATEMENT, color);
	color = txtClVariable->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_VARIABLE, color);
	color = txtClQuoted->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_QUOTED, color);
	color = txtClComment->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_COMMENT, color);
	color = txtClDataline->GetForegroundColour();
	pColorTag->Set(COLOR_TAG_DATALINE, color);
}

/// 色を選択 ダイアログ表示
void DisplaySettingCtrl::OnChangeColor(wxCommandEvent &event)
{
	wxTextCtrl *txt = NULL;
	switch(event.GetId()) {
	case IDC_BTN_CL_C_LINENUM:
		txt = txtClLinenum;
		break;
	case IDC_BTN_CL_C_STATEMENT:
		txt = txtClStatement;
		break;
	case IDC_BTN_CL_C_VARIABLE:
		txt = txtClVariable;
		break;
	case IDC_BTN_CL_C_QUOTED:
		txt = txtClQuoted;
		break;
	case IDC_BTN_CL_C_COMMENT:
		txt = txtClComment;
		break;
	case IDC_BTN_CL_C_DATALINE:
		txt = txtClDataline;
		break;
	}
	if (!txt) return;

	wxColourData cdata;
	cdata.SetCustomColour(0, txt->GetForegroundColour());
	wxColourDialog dlg(this, &cdata);
	if (dlg.ShowModal() == wxID_OK) {
		txt->SetForegroundColour(dlg.GetColourData().GetColour());
		txt->Refresh();
	}
}

/// 色を戻す
void DisplaySettingCtrl::OnRevertColor(wxCommandEvent &event)
{
	wxTextCtrl *txt = NULL;
	int id = -1;
	switch(event.GetId()) {
	case IDC_BTN_CL_R_LINENUM:
		txt = txtClLinenum;
		id = COLOR_TAG_LINENUMBER;
		break;
	case IDC_BTN_CL_R_STATEMENT:
		txt = txtClStatement;
		id = COLOR_TAG_STATEMENT;
		break;
	case IDC_BTN_CL_R_VARIABLE:
		txt = txtClVariable;
		id = COLOR_TAG_VARIABLE;
		break;
	case IDC_BTN_CL_R_QUOTED:
		txt = txtClQuoted;
		id = COLOR_TAG_QUOTED;
		break;
	case IDC_BTN_CL_R_COMMENT:
		txt = txtClComment;
		id = COLOR_TAG_COMMENT;
		break;
	case IDC_BTN_CL_R_DATALINE:
		txt = txtClDataline;
		id = COLOR_TAG_DATALINE;
		break;
	}
	if (!txt) return;

	wxColour color;
	pConfig->GetColorTag().GetDefault(id, color);
	txt->SetForegroundColour(color);
	txt->Refresh();
}

//

BEGIN_EVENT_TABLE(DisplaySettingBox, wxDialog)
END_EVENT_TABLE()

DisplaySettingBox::DisplaySettingBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Display Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	book = new wxNotebook(this, wxID_ANY);
	szrAll->Add(book, flags);

	ctrlParam[0] = new DisplaySettingCtrl(book, IDC_CTRL_L3S1, &gConfig.GetParam(0));
	book->AddPage(ctrlParam[0], _("L3/S1 BASIC"));

	ctrlParam[1] = new DisplaySettingCtrl(book, IDC_CTRL_MSX, &gConfig.GetParam(1));
	book->AddPage(ctrlParam[1], _("MSX BASIC"));

	// 言語

	szrAll->Add(new wxStaticLine(this, wxID_ANY), flags);

	wxBoxSizer *szrH = new wxBoxSizer(wxHORIZONTAL);
	szrH->Add(new wxStaticText(this, wxID_ANY, wxT("Language")), flags);
	comLanguage = new wxChoice(this, IDC_COMBO_LANGUAGE);
	szrH->Add(comLanguage, flags);
	szrAll->Add(szrH, flags);

	// OK and Cancel Buttons

	wxSizer *szrButtons = CreateButtonSizer(wxOK|wxCANCEL);
	szrAll->Add(szrButtons, flags);

	init_dialog();

	SetSizerAndFit(szrAll);
}

int DisplaySettingBox::ShowModal()
{
	int rc = wxDialog::ShowModal();
	if (rc == wxID_OK) {
		term_dialog();
	}
	return rc;
}

void DisplaySettingBox::init_dialog()
{
	// color
	for(int i=0; i<eMachineCount; i++) {
		ctrlParam[i]->Initialize();
	}

	// select the tab
	book->SetSelection(gConfig.GetCurrentMachine());

	// language
	wxArrayString langs;
	wxTranslations *t = wxTranslations::Get();
	if (t) {
		langs = t->GetAvailableTranslations(wxT(APPLICATION_NAME));
	}
	langs.Insert(_("System Dependent"), 0); 
	langs.Insert(_("Unknown"), 1);
	comLanguage->Append(langs);

	int sel = 0;
	if (!gConfig.GetLanguage().IsEmpty()) {
		sel = langs.Index(gConfig.GetLanguage());
	}
	if (sel < 0) {
		sel = 1;
	}
	comLanguage->SetSelection(sel);
}

void DisplaySettingBox::term_dialog()
{
	// color
	for(int i=0; i<eMachineCount; i++) {
		ctrlParam[i]->Terminate();
	}

	// language
	int sel = comLanguage->GetSelection();
	wxString lang;
	switch(sel) {
	case 0:
		break;
	case 1:
		lang = wxT("unknown");
		break;
	default:
		lang = comLanguage->GetString(sel);
		break;
	}
	gConfig.SetLanguage(lang);
}
