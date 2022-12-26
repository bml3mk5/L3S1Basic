/// @file dispsetbox.cpp
///
/// @brief 表示設定ダイアログ
///
#include "dispsetbox.h"
#include <wx/statline.h>
#include <wx/colordlg.h>
#include "main.h"
#include "config.h"
#include "colortag.h"

// Attach Event
BEGIN_EVENT_TABLE(DisplaySettingBox, wxDialog)
	EVT_BUTTON(IDC_BTN_CL_C_LINENUM, DisplaySettingBox::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_STATEMENT, DisplaySettingBox::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_QUOTED, DisplaySettingBox::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_COMMENT, DisplaySettingBox::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_C_DATALINE, DisplaySettingBox::OnChangeColor)
	EVT_BUTTON(IDC_BTN_CL_R_LINENUM, DisplaySettingBox::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_STATEMENT, DisplaySettingBox::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_QUOTED, DisplaySettingBox::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_COMMENT, DisplaySettingBox::OnRevertColor)
	EVT_BUTTON(IDC_BTN_CL_R_DATALINE, DisplaySettingBox::OnRevertColor)
END_EVENT_TABLE()

DisplaySettingBox::DisplaySettingBox(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("Display Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
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

	gszr = new wxGridSizer(5, 3, 2, 2);
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
	// Set from config file
	chkAddSpace->SetValue(gConfig.DoesAddSpaceAfterColon());

	// Set color
	wxColour color;
	gColorTag.Get(COLOR_TAG_LINENUMBER, color);
	txtClLinenum->SetForegroundColour(color);
	gColorTag.Get(COLOR_TAG_STATEMENT, color);
	txtClStatement->SetForegroundColour(color);
	gColorTag.Get(COLOR_TAG_QUOTED, color);
	txtClQuoted->SetForegroundColour(color);
	gColorTag.Get(COLOR_TAG_COMMENT, color);
	txtClComment->SetForegroundColour(color);
	gColorTag.Get(COLOR_TAG_DATALINE, color);
	txtClDataline->SetForegroundColour(color);

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
	// Set to config file
	gConfig.DoesAddSpaceAfterColon(chkAddSpace->GetValue());

	// Set color
	wxColour color;
	color = txtClLinenum->GetForegroundColour();
	gColorTag.Set(COLOR_TAG_LINENUMBER, color);
	color = txtClStatement->GetForegroundColour();
	gColorTag.Set(COLOR_TAG_STATEMENT, color);
	color = txtClQuoted->GetForegroundColour();
	gColorTag.Set(COLOR_TAG_QUOTED, color);
	color = txtClComment->GetForegroundColour();
	gColorTag.Set(COLOR_TAG_COMMENT, color);
	color = txtClDataline->GetForegroundColour();
	gColorTag.Set(COLOR_TAG_DATALINE, color);

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

/// 色を選択 ダイアログ表示
void DisplaySettingBox::OnChangeColor(wxCommandEvent &event)
{
	wxTextCtrl *txt = NULL;
	switch(event.GetId()) {
	case IDC_BTN_CL_C_LINENUM:
		txt = txtClLinenum;
		break;
	case IDC_BTN_CL_C_STATEMENT:
		txt = txtClStatement;
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
void DisplaySettingBox::OnRevertColor(wxCommandEvent &event)
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
	gColorTag.GetDefault(id, color);
	txt->SetForegroundColour(color);
	txt->Refresh();
}
