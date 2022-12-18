/// @file main.cpp
///
/// @brief 本体
///
#include "main.h"
#include "configbox.h"
#include "fontminibox.h"
#include "chartypebox.h"
#include "intnamebox.h"
#include <wx/cmdline.h>
#include <wx/filename.h>
#include "res/l3s1basic.xpm"

#define L3BASIC_TRANS \
	_("can't open file '%s'") \
	_("can't create file '%s'") \
	_("can't close file descriptor %d") \
	_("can't read from file descriptor %d") \
	_("can't write to file descriptor %d") \
	_("can't flush file descriptor %d") \
	_("can't seek on file descriptor %d") \
	_("can't get seek position on file descriptor %d")

IMPLEMENT_APP(L3basicApp)

L3basicApp::L3basicApp() : mLocale(wxLANGUAGE_DEFAULT)
{
	frame = NULL;
}

bool L3basicApp::OnInit()
{
	SetAppPath();
	SetAppName(_T("l3s1basic"));

	// load ini file
	mConfig.Load(ini_path + GetAppName() + _T(".ini"));

	// set locale search path and catalog name
	mLocale.AddCatalogLookupPathPrefix(res_path + _T("lang"));
	mLocale.AddCatalogLookupPathPrefix(_T("lang"));
	mLocale.AddCatalog(_T("l3s1basic"));

	if (!wxApp::OnInit()) {
		return false;
	}

	frame = new L3basicFrame(GetAppName(), wxSize(640, 480) );
	frame->Show(true);
	SetTopWindow(frame);

	if (!frame->Init(in_file)) {
		return false;
	}

	return true;
}

#define OPTION_VERBOSE "verbose"

void L3basicApp::OnInitCmdLine(wxCmdLineParser &parser)
{
	// the standard command line options
	static const wxCmdLineEntryDesc cmdLineDesc[] = {
		{
			wxCMD_LINE_SWITCH, "h", "help",
			"show this help message",
			wxCMD_LINE_VAL_NONE,
			wxCMD_LINE_OPTION_HELP
		},

#if wxUSE_LOG
		{
			wxCMD_LINE_SWITCH, NULL, OPTION_VERBOSE,
			"generate verbose log messages",
			wxCMD_LINE_VAL_NONE,
			0x0
		},
#endif // wxUSE_LOG
	    {
			wxCMD_LINE_PARAM, NULL, NULL,
			"input file",
			wxCMD_LINE_VAL_STRING,
			wxCMD_LINE_PARAM_OPTIONAL
		},

		// terminator
		wxCMD_LINE_DESC_END
	};

	parser.SetDesc(cmdLineDesc);
}

bool L3basicApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
#if wxUSE_LOG
	if ( parser.Found(OPTION_VERBOSE) ) {
		wxLog::SetVerbose(true);
	}
#endif // wxUSE_LOG
	if (parser.GetParamCount() > 0) {
		in_file = parser.GetParam(0);
	}
	return true;
}

void L3basicApp::MacOpenFile(const wxString &fileName)
{
	if (frame) {
		frame->OpenDataFile(fileName);
	}
}

int L3basicApp::OnExit()
{
	// save ini file
	mConfig.Save();

	return 0;
}

void L3basicApp::SetAppPath()
{
	app_path = wxFileName::FileName(argv[0]).GetPath(wxPATH_GET_SEPARATOR);
#ifdef __WXOSX__
	if (app_path.Find(_T("MacOS")) >= 0) {
		wxFileName file = wxFileName::FileName(app_path+"../../../");
		file.Normalize();
		ini_path = file.GetPath(wxPATH_GET_SEPARATOR);
		file = wxFileName::FileName(app_path+"../../Contents/Resources/");
		file.Normalize();
		res_path = file.GetPath(wxPATH_GET_SEPARATOR);
	} else
#endif
	{
		ini_path = app_path;
		res_path = app_path;
	}
}

const wxString &L3basicApp::GetAppPath()
{
	return app_path;
}

const wxString &L3basicApp::GetIniPath()
{
	return ini_path;
}

const wxString &L3basicApp::GetResPath()
{
	return res_path;
}

Config *L3basicApp::GetConfig()
{
	return &mConfig;
}

//
// Frame
//
// Attach Event
BEGIN_EVENT_TABLE(L3basicFrame, wxFrame)
	// menu event
	EVT_MENU(wxID_EXIT,  L3basicFrame::OnQuit)
	EVT_MENU(wxID_ABOUT, L3basicFrame::OnAbout)

	EVT_MENU(IDM_OPEN_FILE, L3basicFrame::OnOpenFile)
	EVT_MENU(IDM_CLOSE_FILE, L3basicFrame::OnCloseFile)

	EVT_MENU(IDM_EXPORT_BASICBINTAPE, L3basicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXTTAPE, L3basicFrame::OnExportFile)

	EVT_MENU(IDM_EXPORT_BASICBINDISK, L3basicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXTDISK, L3basicFrame::OnExportFile)

	EVT_MENU(IDM_EXPORT_BASICBIN, L3basicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXT, L3basicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_UTF8TEXT, L3basicFrame::OnExportFile)

	EVT_MENU_RANGE(IDM_RECENT_FILE_0, IDM_RECENT_FILE_0 + MAX_RECENT_FILES - 1, L3basicFrame::OnOpenRecentFile)

	EVT_MENU(IDM_CONFIGURE, L3basicFrame::OnConfigure)

	EVT_MENU_OPEN(L3basicFrame::OnMenuOpen)
END_EVENT_TABLE()

// 翻訳用
#define DIALOG_BUTTON_STRING _("OK"),_("Cancel")
#define APPLE_MENU_STRING _("Hide l3s1basic"),_("Hide Others"),_("Show All"),_("Quit l3s1basic"),_("Services"),_("Preferences…")

L3basicFrame::L3basicFrame(const wxString& title, const wxSize& size)
       : wxFrame(NULL, -1, title, wxDefaultPosition, size)
{
	// icon
#ifdef __WXMSW__
	SetIcon(wxIcon(_T("l3basic")));
#elif defined(__WXGTK__) || defined(__WXMOTIF__)
	SetIcon(wxIcon(l3basic_xpm));
#endif
	//
	ps = NULL;

	// menu
	menuFile = new wxMenu;
	menuOther = new wxMenu;
	menuHelp = new wxMenu;
	wxMenu *smenu;

	// file menu
	menuFile->Append( IDM_OPEN_FILE, _("&Open...") );
	menuFile->Append( IDM_CLOSE_FILE, _("&Close") );
	menuFile->AppendSeparator();
	smenu = new wxMenu;
	smenu->Append( IDM_EXPORT_BASICBIN, _("&BASIC Intermediate Language...") );
	smenu->Append( IDM_EXPORT_ASCIITXT, _("&Ascii Text...") );
	smenu->Append( IDM_EXPORT_UTF8TEXT, _("&UTF-8 Text...") );
	smenu->AppendSeparator();
	smenu->Append( IDM_EXPORT_BASICBINDISK, _("For Disk(&Binary)...") );
	smenu->Append( IDM_EXPORT_ASCIITXTDISK, _("For Disk(&Ascii)...") );
	smenu->AppendSeparator();
	smenu->Append( IDM_EXPORT_BASICBINTAPE, _("Tape Image(&Binary)...") );
	smenu->Append( IDM_EXPORT_ASCIITXTTAPE, _("Tape Image(&Ascii)...") );
	menuFile->Append( IDM_EXPORT_FILE, _("&Export To"), smenu );
	menuFile->AppendSeparator();
	menuRecentFiles = new wxMenu();
	UpdateMenuRecentFiles();
	menuFile->AppendSubMenu(menuRecentFiles, _("&Reccent Files") );
	menuFile->AppendSeparator();
	menuFile->Append( wxID_EXIT, _("E&xit") );
	// other menu
	menuOther->Append( IDM_CONFIGURE, _("&Configure...") );
	// help menu
	menuHelp->Append( wxID_ABOUT, _("&About...") );

	// menu bar
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, _("&File") );
	menuBar->Append( menuOther, _("&Other") );
	menuBar->Append( menuHelp, _("&Help") );

	SetMenuBar( menuBar );

	// control panel
	panel = new L3basicPanel(this);
	// config box
	cfgbox = new ConfigBox(this, IDD_CONFIGBOX);
	// char type box
//	ctypebox = new CharTypeBox(this, IDD_CHARTYPEBOX);

	// drag and drop
	SetDropTarget(new L3basicFileDropTarget(this));
}

L3basicFrame::~L3basicFrame()
{
	// save ini file
	Config *ini = wxGetApp().GetConfig();
	ini->SetFilePath(file_path);
	ini->SetParam(ps->GetParam());

//	delete ctypebox;
	delete cfgbox;
	delete ps;
}

bool L3basicFrame::Init(const wxString &in_file)
{
	// initialize
	ps = new Parse(wxGetApp().GetResPath());
	if (!ps->Init()) {
		return false;
	}
	// load ini file
	Config *ini = wxGetApp().GetConfig();
	file_path = ini->GetFilePath();
	ps->SetParam(ini->GetParam());

	// set combo box on panel
	wxArrayString basic_types;
	ps->GetBasicTypes(basic_types);
	panel->AddBasicType(basic_types);
	wxArrayString char_types;
	ps->GetCharTypes(char_types);
	panel->AddCharType(char_types);
	panel->Update(NULL);

	// set combo box on configbox
	int *items;
	size_t count;
	items = ps->GetStartAddrsPtr(&count);
	cfgbox->AddStartAddrItems(items, count);

	UpdateMenu();

	// open file
	if (!in_file.IsEmpty()) {
		OpenDataFile(in_file);
	}
	return true;
}

/// メニュー更新
void L3basicFrame::OnMenuOpen(wxMenuEvent& event)
{
	wxMenu *menu = event.GetMenu();

	if (menu == NULL) return;

	if (menu == menuFile) {	// File...
		UpdateMenu();
	}
}
/// ドロップされたファイルを開く
void L3basicFrame::OpenDroppedFile(const wxString &path)
{
	CloseDataFile();
	OpenDataFile(path);
}

void L3basicFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
 	CloseDataFile();
	Close(true);
}

void L3basicFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	L3basicAbout(this, wxID_ANY).ShowModal();
}

void L3basicFrame::OnOpenFile(wxCommandEvent& WXUNUSED(event))
{
	L3basicFileDialog *dlg = new L3basicFileDialog(
		_("Open file"),
		file_path,
		wxEmptyString,
		_("Supported files (*.bin;*.bas;*.txt;*.dat;*.l3)|*.bin;*.bas;*.txt;*.dat;*.l3|All files (*.*)|*.*"),
		wxFD_OPEN);

	int rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		OpenDataFile(path);
	}
}

void L3basicFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
	CloseDataFile();
}

/// エクスポート
void L3basicFrame::OnExportFile(wxCommandEvent& event)
{
	int id = event.GetId();

	ExportFile(id);
}

/// 最近使用したファイル
void L3basicFrame::OnOpenRecentFile(wxCommandEvent& event)
{
	wxMenuItem *item = menuRecentFiles->FindItem(event.GetId());
	if (!item) return;
	wxFileName path = item->GetItemLabel();
	CloseDataFile();
	OpenDataFile(path.GetFullPath());
}

/// 設定ダイアログ
void L3basicFrame::OnConfigure(wxCommandEvent& WXUNUSED(event))
{
	cfgbox->SetParam(ps->GetParam());

	cfgbox->ShowModal();

	ps->SetParam(cfgbox->GetParam());
}

/// メニューの更新
void L3basicFrame::UpdateMenu()
{
	// Export to ...
	bool opened = ps->IsOpenedDataFile();

	menuFile->Enable(IDM_CLOSE_FILE, opened);
	menuFile->Enable(IDM_EXPORT_BASICBINTAPE, opened);
	menuFile->Enable(IDM_EXPORT_ASCIITXTTAPE, opened);
	menuFile->Enable(IDM_EXPORT_BASICBINDISK, opened);
	menuFile->Enable(IDM_EXPORT_ASCIITXTDISK, opened);
	menuFile->Enable(IDM_EXPORT_BASICBIN, opened);
	menuFile->Enable(IDM_EXPORT_ASCIITXT, opened);
	menuFile->Enable(IDM_EXPORT_UTF8TEXT, opened);
}

/// 最近使用したファイル一覧を更新
void L3basicFrame::UpdateMenuRecentFiles()
{
	// メニューを更新
	wxArrayString names;
	Config *ini = wxGetApp().GetConfig();
	ini->GetRecentFiles(names);
	for(int i=0; i<MAX_RECENT_FILES && i<(int)names.Count(); i++) {
		if (menuRecentFiles->FindItem(IDM_RECENT_FILE_0 + i)) menuRecentFiles->Delete(IDM_RECENT_FILE_0 + i);
		menuRecentFiles->Append(IDM_RECENT_FILE_0 + i, names[i]);
	}
}

/// 指定したファイルを開く
void L3basicFrame::OpenDataFile(const wxString &path)
{
	wxString title;

	// set recent file path
	file_path = wxFileName::FileName(path).GetPath(wxPATH_GET_SEPARATOR);

	panel->SetTextInfo(_("Now Processing..."));
	Update();

	if (!ps->OpenDataFile(path)) {
		panel->SetTextInfo(wxEmptyString);
		return;
	}

	// update window
	title = wxGetApp().GetAppName() + _T(" - ") + path;
	SetTitle(title);
	UpdateMenu();

	// update panel
	PsFileType *opened_flags = ps->GetOpenedDataTypePtr();
	opened_flags->SetInternalName(wxFileName::FileName(path).GetName());	

	panel->Update(opened_flags);
	panel->SetBasicType(0, ps->GetOpenedBasicType());
	panel->SetBasicType(1, ps->GetOpenedBasicType());
//	panel->GetTextName()->SetValue(path);

	wxArrayString lines;
	wxString char_type = ps->GetParsedData(lines);
	panel->SetCharType(2, char_type);
	panel->SetTextInfo(char_type, lines);

	Config *ini = wxGetApp().GetConfig();
	ini->AddRecentFile(path);
	UpdateMenuRecentFiles();
}

/// ファイルを閉じる
void L3basicFrame::CloseDataFile()
{
	if (ps == NULL) return;

	ps->CloseDataFile();

	// update window
	wxString title = wxGetApp().GetAppName();
	SetTitle(title);
	UpdateMenu();

	// update panel
	panel->Update(NULL);
//	panel->GetTextName()->SetValue(wxT(""));
}

/// エクスポート
void L3basicFrame::ExportFile(int id)
{
	int rc;
	wxString file_base;
	wxString wild_card;
	PsFileType file_type;

	if (!ps->IsOpenedDataFile()) return;

	file_base = ps->GetFileNameBase();

	PsFileType *opened_flags = ps->GetOpenedDataTypePtr();
	bool enable = true;

	IntNameBox intnamebox(this, IDD_INTNAMEBOX);

	switch(id) {
		case IDM_EXPORT_BASICBIN:
			file_base += _T(".bin");
			wild_card = _("BASIC File (*.bin)|*.bin|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psBinary, true);
			break;
		case IDM_EXPORT_BASICBINTAPE:
			file_base += _T(".l3");
			wild_card = _("Tape Image File (*.l3)|*.l3|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psBinary | psTapeImage, true);
			intnamebox.SetInternalName(opened_flags->GetInternalName());
			if (intnamebox.ShowModal() == wxID_OK) {
				file_type.SetInternalName(intnamebox.GetInternalName());
			} else {
				enable = false;
			}
			break;
		case IDM_EXPORT_BASICBINDISK:
			file_base += _T(".BAS");
			wild_card = _("BASIC File (*.bas)|*.bas|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psBinary | psDiskImage, true);
			break;
		case IDM_EXPORT_ASCIITXT:
			file_base += _T(".bas");
			wild_card = _("ASCII File (*.bas)|*.bas|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psAscii, true);
			break;
		case IDM_EXPORT_ASCIITXTTAPE:
			file_base += _T(".l3");
			wild_card = _("Tape Image File (*.l3)|*.l3|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psAscii | psTapeImage, true);
			intnamebox.SetInternalName(opened_flags->GetInternalName());
			if (intnamebox.ShowModal() == wxID_OK) {
				file_type.SetInternalName(intnamebox.GetInternalName());
			} else {
				enable = false;
			}
			break;
		case IDM_EXPORT_ASCIITXTDISK:
			file_base += _T(".BAS");
			wild_card = _("ASCII File (*.bas)|*.bas|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psAscii | psDiskImage, true);
			break;
		case IDM_EXPORT_UTF8TEXT:
			file_base += _T(".txt");
			wild_card = _("TEXT File (*.txt)|*.txt|All Files (*.*)|*.*");
			file_type.SetTypeFlag(psAscii | psUTF8, true);
			file_type.SetCharType(panel->GetCharType(2));
			// show char type box
//			rc = ctypebox->ShowModal();
//			file_type.SetCharType(ctypebox->GetCharType());
			break;
		default:
			file_base += _T("");
			wild_card = _("All Files (*.*)|*.*");
			file_type.SetTypeFlag(psBinary, true);
			break;
	}
	if (!enable) return;

	L3basicFileDialog *dlg = new L3basicFileDialog(
		_("Export File"),
		file_path,
		file_base,
		wild_card,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		wxString basic_type = panel->GetBasicType(1);
		file_type.SetBasicType(basic_type);
		if (!ps->OpenOutFile(path, file_type)) {
			return;
		}
		panel->SetTextInfo(_("Now Processing..."));
		Update();

		ps->ExportData();
		ps->CloseOutFile();

		// output report
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, nlines);
	}
}

/// ファイルを読み直す
void L3basicFrame::ReloadBinaryData(int type, int mask, const wxString &basic_type)
{
	if (ps == NULL) return;
	if (ps->ReloadOpenedBinaryData(type, mask, basic_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, nlines);
	}
}

/// ファイルを読み直す
void L3basicFrame::ReloadOpendData(int type, int mask, const wxString &char_type)
{
	if (ps == NULL) return;
	if (ps->ReloadOpendAsciiData(type, mask, char_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, nlines);
	}
}

/// ファイルを読み直す
void L3basicFrame::ReloadParsedData(int type, int mask, const wxString &char_type)
{
	if (ps == NULL) return;
	if (ps->ReloadParsedData(type, mask, char_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, nlines);
	}
}

//
// Control Panel
//
// Attach Event
BEGIN_EVENT_TABLE(L3basicPanel, wxPanel)
	// event
	EVT_SIZE(L3basicPanel::OnSize)

	EVT_COMBOBOX(IDC_COMBO_BASICTYPE_I, L3basicPanel::OnSelectBasicTypeI)

	EVT_RADIOBUTTON(IDC_RADIO_ASCII_I, L3basicPanel::OnSelectAsciiI)
	EVT_RADIOBUTTON(IDC_RADIO_UTF8_I, L3basicPanel::OnSelectUTF8I)

	EVT_COMBOBOX(IDC_COMBO_CHARTYPE_I, L3basicPanel::OnSelectCharTypeI)

	EVT_COMBOBOX(IDC_COMBO_DCHARTYPE, L3basicPanel::OnSelectDCharType)

	EVT_BUTTON(IDC_BUTTON_FONT, L3basicPanel::OnClickFont)

	EVT_BUTTON(IDC_BUTTON_EXPORT, L3basicPanel::OnClickExport)
	
END_EVENT_TABLE()

L3basicPanel::L3basicPanel(L3basicFrame *parent)
       : wxPanel(parent)
{
	frame    = parent;
//	textName = NULL;
	textInfo = NULL;

	wxSize  frame_size = parent->GetClientSize();
	long style;

	wxSizerFlags flagsM = wxSizerFlags().Expand().Border(wxALL, 0);
	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxALL, 2);
	wxBoxSizer *hbox, *hboxa, *hboxb;
	wxBoxSizer *vbox, *vboxa, *vboxb;
	wxSize size;
//	size.x = DEFAULT_TEXTWIDTH; size.y = -1;
	size.x = -1; size.y = -1;
	wxString dummystr = wxT("wwwwwwwwwwwwwwww");

	szrAll = new wxBoxSizer(wxVERTICAL);

	hbox = new wxBoxSizer(wxHORIZONTAL);

	// 入力ファイル情報
	vbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Input File Information")), wxVERTICAL);

//	style = wxTE_READONLY;
//	textName = new wxTextCtrl(this, IDC_TEXT_NAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, style);
//	vbox->Add(textName, flagsW);

	radBinaryI   = new wxRadioButton(this, IDC_RADIO_BINARY_I, _("BASIC Intermediate Language"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE | wxRB_GROUP);
	vbox->Add(radBinaryI, flagsW);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("    ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("BASIC Type")), flagsW);
	comBasicTypeI = new wxComboBox(this, IDC_COMBO_BASICTYPE_I, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comBasicTypeI, flagsW);
	vbox->Add(hboxb, 0);

	radAsciiI = new wxRadioButton(this, IDC_RADIO_ASCII_I, _("Ascii Text"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE);
	radUTF8I  = new wxRadioButton(this, IDC_RADIO_UTF8_I , _("UTF-8 Text"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE);
	vbox->Add(radAsciiI, flagsW);
	vbox->Add(radUTF8I, flagsW);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("    ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("Text Type")), flagsW);
	comCharTypeI = new wxComboBox(this, IDC_COMBO_CHARTYPE_I, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comCharTypeI, flagsW);
	vbox->Add(hboxb, 0);

	hbox->Add(vbox, flagsW);

	// セパレータ
	vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(this, wxID_ANY, _T("")), flagsW);
	vbox->Add(new wxStaticText(this, wxID_ANY, _T("")), flagsW);
	vbox->Add(new wxStaticText(this, wxID_ANY, _T("")), flagsW);
	vbox->Add(new wxStaticText(this, wxID_ANY, _T("->")), flagsW);
	hbox->Add(vbox, flagsW);

	// 出力ファイル情報
	vbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Output File Information")), wxVERTICAL);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	radBinaryO     = new wxRadioButton(this, IDC_RADIO_BINARY_O, _("BASIC Intermediate Language"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	radBinaryDiskO = new wxRadioButton(this, IDC_RADIO_BIN_DI_O, _("For Disk (Binary)"), wxDefaultPosition, wxDefaultSize, 0);
	radBinaryTapeO = new wxRadioButton(this, IDC_RADIO_BIN_TA_O, _("Tape Image (Binary)"), wxDefaultPosition, wxDefaultSize, 0);
	hboxb->Add(radBinaryO, flagsW);
	hboxb->Add(radBinaryDiskO, flagsW);
	hboxb->Add(radBinaryTapeO, flagsW);
	vbox->Add(hboxb, 0);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("    ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("BASIC Type")), flagsW);
	comBasicTypeO = new wxComboBox(this, IDC_COMBO_BASICTYPE_O, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comBasicTypeO, flagsW);
	vbox->Add(hboxb, 0);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	radAsciiO     = new wxRadioButton(this, IDC_RADIO_ASCII_O, _("Ascii Text"), wxDefaultPosition, wxDefaultSize, 0);
	radAsciiDiskO = new wxRadioButton(this, IDC_RADIO_ASC_D_O, _("For Disk (Ascii)"), wxDefaultPosition, wxDefaultSize, 0);
	radAsciiTapeO = new wxRadioButton(this, IDC_RADIO_ASC_T_O, _("Tape Image (Ascii)"), wxDefaultPosition, wxDefaultSize, 0);
	hboxb->Add(radAsciiO, flagsW);
	hboxb->Add(radAsciiDiskO, flagsW);
	hboxb->Add(radAsciiTapeO, flagsW);
	vbox->Add(hboxb, 0);

	hboxa = new wxBoxSizer(wxHORIZONTAL);
	vboxa = new wxBoxSizer(wxVERTICAL);

	radUTF8O      = new wxRadioButton(this, IDC_RADIO_UTF8_O , _("UTF-8 Text"), wxDefaultPosition, wxDefaultSize, 0);
	vboxa->Add(radUTF8O, flagsW);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("    ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("Text Type")), flagsW);
	comCharTypeO = new wxComboBox(this, IDC_COMBO_CHARTYPE_O, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comCharTypeO, flagsW);
	vboxa->Add(hboxb, 0);

	hboxa->Add(vboxa, 0);

	// export button
	btnExport = new wxButton(this, IDC_BUTTON_EXPORT, _("Export"), wxDefaultPosition, size, 0);
	hboxa->Add(btnExport, flagsW);

	vbox->Add(hboxa, 0);
	hbox->Add(vbox, flagsW);

	szrAll->Add(hbox, 0);

	// テキスト表示
	vboxb = new wxBoxSizer(wxVERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Display Type")), flagsW);
	comDCharType = new wxComboBox(this, IDC_COMBO_DCHARTYPE, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hbox->Add(comDCharType, flagsW);
	hbox->Add(new wxStaticText(this, wxID_ANY, _T("  ")), flagsW);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Font")), flagsW);
	style = wxTE_READONLY;
	size.x = DEFAULT_TEXTWIDTH * 2;
	textFont = new wxTextCtrl(this, IDC_TEXT_FONT, wxEmptyString, wxDefaultPosition, size, style);
	btnFont = new wxButton(this, IDC_BUTTON_FONT, _("Change"));
	hbox->Add(textFont, flagsW);
	hbox->Add(btnFont, flagsW);
	vboxb->Add(hbox, flagsW);

	szrAll->Add(vboxb, flagsM);

	wxBoxSizer *vboxc = new wxBoxSizer(wxVERTICAL);
	style = wxTE_READONLY | wxTE_MULTILINE;
	size.x = frame_size.x; size.y = -1;
	textInfo = new wxTextCtrl(this, IDC_TEXT_INFO, wxEmptyString, wxDefaultPosition, size, style);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(textInfo, flagsM);
	vboxc->Add(hbox, flagsM);

	szrAll->Add(vboxc, flagsM);
	szrAll->SetSizeHints(this);

	SetSizerAndFit(szrAll);
	Layout();

	// adjust window width
	wxSize fsz = frame->GetClientSize();
	wxSize psz = this->GetSize();
	if (fsz.x < psz.x) fsz.x = psz.x;
	if (fsz.y < psz.y) fsz.y = psz.y;
	frame->SetClientSize(fsz);

	// set font name
	SetInitialFont();
	SetTextFontName();
	textInfo->SetFont(fontFixed);
}

L3basicPanel::~L3basicPanel()
{
	// save ini file
	Config *ini = wxGetApp().GetConfig();
	ini->SetFontName(mFontName);
	ini->SetFontSize(mFontSize);
}

/// リサイズ
void L3basicPanel::OnSize(wxSizeEvent& event)
{
	wxSize size = event.GetSize();

//	if (textName) textName->SetSize(size.x, -1);
	if (textInfo) textInfo->SetSize(size.x, size.y - textInfo->GetPosition().y);
}

/// BASIC種類選択
void L3basicPanel::OnSelectBasicTypeI(wxCommandEvent& event)
{
	frame->ReloadBinaryData(psBinary, psAscii, GetBasicType(0));
}
/// BASIC中間言語ラジオボタン選択
void L3basicPanel::OnSelectBinaryI(wxCommandEvent& event)
{
	radAsciiI->SetValue(false);
	radUTF8I->SetValue(false);
	frame->ReloadBinaryData(psBinary, psAscii, GetBasicType(0));
}
/// Asciiラジオボタン選択
void L3basicPanel::OnSelectAsciiI(wxCommandEvent& event)
{
	radBinaryI->SetValue(false);
	radUTF8I->SetValue(false);
	frame->ReloadOpendData(psAscii, psUTF8, _T(""));
}
/// UTF8ラジオボタン選択
void L3basicPanel::OnSelectUTF8I(wxCommandEvent& event)
{
	radBinaryI->SetValue(false);
	radAsciiI->SetValue(false);
	frame->ReloadOpendData(psAscii | psUTF8, 0, GetCharType(0));
}
/// 文字種類選択
void L3basicPanel::OnSelectCharTypeI(wxCommandEvent& event)
{
	radBinaryI->SetValue(false);
	radAsciiI->SetValue(false);
	radUTF8I->SetValue(true);
	frame->ReloadOpendData(psAscii | psUTF8, 0, GetCharType(0));
}
/// 表示する文字種類選択
void L3basicPanel::OnSelectDCharType(wxCommandEvent& event)
{
	frame->ReloadParsedData(psAscii | psUTF8, 0, GetCharType(1));
}
/// フォントボタン選択
void L3basicPanel::OnClickFont(wxCommandEvent& event)
{
#ifdef USE_FONTDIALOG
	wxFontData fdata;

	// font dialog
	fdata.SetInitialFont(fontFixed);
	wxFontDialog *fontdlg = new wxFontDialog(this, fdata);

	if (fontdlg->ShowModal() == wxID_OK) {

		fdata = fontdlg->GetFontData();
		fontFixed = fdata.GetChosenFont();

		mFontName = fontFixed.GetFaceName();
		mFontSize = fontFixed.GetPointSize();

		// set font
		SetTextFontName();
		textInfo->SetFont(fontFixed);
	}

	delete fontdlg;
#else
	FontMiniBox fontbox(this, wxID_ANY);
	fontbox.SetFontName(mFontName);
	fontbox.SetFontSize(mFontSize);

	if (fontbox.ShowModal() == wxID_OK) {
		wxFont new_font = wxFont(fontbox.GetFontSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, fontbox.GetFontName());
		if (new_font.IsOk()) {
			mFontName = new_font.GetFaceName();
			mFontSize = new_font.GetPointSize();
			fontFixed = new_font;
		}

		// set font
		SetTextFontName();
		textInfo->SetFont(fontFixed);
	}
#endif
}
/// エクスポートボタン選択
void L3basicPanel::OnClickExport(wxCommandEvent& event)
{
	int id = 0;
	if (radBinaryO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_BASICBIN;
	} else if (radBinaryTapeO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_BASICBINTAPE;
	} else if (radBinaryDiskO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_BASICBINDISK;
	} else if (radAsciiO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_ASCIITXT;
	} else if (radAsciiTapeO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_ASCIITXTTAPE;
	} else if (radAsciiDiskO->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_ASCIITXTDISK;
	} else if (radUTF8O->GetValue()) {
		id = L3basicFrame::IDM_EXPORT_UTF8TEXT;
	}

	frame->ExportFile(id);
}

/// コントロールを更新
void L3basicPanel::Update(PsFileType *file_type)
{
	radBinaryI->Enable(false);
	radAsciiI->Enable(false);
	radUTF8I->Enable(false);
	radBinaryI->SetValue(false);
	radAsciiI->SetValue(false);
	radUTF8I->SetValue(false);
	comBasicTypeI->Enable(false);
	comCharTypeI->Enable(false);
	btnExport->Enable(false);

	if (file_type == NULL) return;

	if (file_type->GetTypeFlag(psAscii)) {
		radAsciiI->Enable(true);
		radUTF8I->Enable(true);
		comCharTypeI->Enable(true);
		btnExport->Enable(true);
		if (file_type->GetTypeFlag(psUTF8)) {
			// infile is ascii utf8
			radUTF8I->SetValue(true);
			SetCharType(0, file_type->GetCharType());
		} else {
			// infile is ascii text
			radAsciiI->SetValue(true);
		}
	} else {
		// infile is binary
		radBinaryI->Enable(true);
		radBinaryI->SetValue(true);
		comBasicTypeI->Enable(true);
		btnExport->Enable(true);
	}
}

/// Basic type を更新
void L3basicPanel::AddBasicType(const wxArrayString &items)
{
	mOrigBasicTypes.Empty();
	comBasicTypeI->Clear();
	comBasicTypeO->Clear();
	for(size_t i=0; i<items.GetCount(); i++) {
		mOrigBasicTypes.Add(items[i]);
		comBasicTypeI->Insert(wxGetTranslation(items[i]), (int)i);
		comBasicTypeO->Insert(wxGetTranslation(items[i]), (int)i);
	}
	comBasicTypeI->Select(0);
	comBasicTypeO->Select(0);
}
/// Basic type を返す
int L3basicPanel::GetBasicTypeNum(int n)
{
	int type = -1;
	wxComboBox *cb;
	switch(n) {
	case 1:
		cb = comBasicTypeO;
		break;
	default:
		cb = comBasicTypeI;
		break;
	}

	if (0 < mOrigBasicTypes.Count() && 0 <= cb->GetSelection()) {
		type = cb->GetSelection();
	}
	return type;
}
/// Basic type を返す
wxString L3basicPanel::GetBasicType(int n)
{
	int type = GetBasicTypeNum(n);
	if (type >= 0) {
		return mOrigBasicTypes[type];
	} else {
		return _T("");
	}
}
/// Basic typeをセット
void L3basicPanel::SetBasicType(int n, int pos)
{
	if (pos < 0) return;

	switch(n) {
	case 1:
		comBasicTypeO->Select(pos);
		break;
	default:
		comBasicTypeI->Select(pos);
		break;
	}
}
/// Basic typeをセット
void L3basicPanel::SetBasicType(int n, const wxString &basic_type)
{
	int i = 0;
	for(; i<(int)mOrigBasicTypes.GetCount(); i++) {
		if (mOrigBasicTypes[i] == basic_type) {
			break;
		}
	}
	if (i >= (int)mOrigBasicTypes.GetCount()) {
		i = -1;
	}
	SetBasicType(n, i);
}

/// Char type を更新
void L3basicPanel::AddCharType(const wxArrayString &items)
{
	mOrigCharTypes.Empty();
	comCharTypeI->Clear();
	comCharTypeO->Clear();
	comDCharType->Clear();
	for(size_t i=0; i<items.GetCount(); i++) {
		mOrigCharTypes.Add(items[i]);
		comCharTypeI->Insert(wxGetTranslation(items[i]), (int)i);
		comCharTypeO->Insert(wxGetTranslation(items[i]), (int)i);
		comDCharType->Insert(wxGetTranslation(items[i]), (int)i);
	}
	comCharTypeI->Select(0);
	comCharTypeO->Select(0);
	comDCharType->Select(0);
}
/// Char type を返す
wxString L3basicPanel::GetCharType(int n)
{
	wxComboBox *cb;
	switch(n) {
	case 1:
		cb = comDCharType;
		break;
	case 2:
		cb = comCharTypeO;
		break;
	default:
		cb = comCharTypeI;
		break;
	}
	return mOrigCharTypes[cb->GetSelection()];
}
/// Char typeをセット
void L3basicPanel::SetCharType(int n, int pos)
{
	wxComboBox *cb;
	switch(n) {
	case 1:
		cb = comDCharType;
		break;
	case 2:
		cb = comCharTypeO;
		break;
	default:
		cb = comCharTypeI;
		break;
	}
	cb->Select(pos);
}
void L3basicPanel::SetCharType(int n, const wxString &char_type)
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
	SetCharType(n, (int)i);
}

/// 情報をセット
void L3basicPanel::SetTextInfo(const wxString &str)
{
	textInfo->SetValue(str);
}
void L3basicPanel::SetTextInfo(const wxString &char_type, const wxString &str)
{
	SetCharType(1, char_type);
	textInfo->SetValue(str);
}
void L3basicPanel::SetTextInfo(const wxString &char_type, const wxArrayString &lines)
{
	SetCharType(1, char_type);

	wxString str;

	for(size_t i=0; i<lines.GetCount(); i++) {
		str += lines[i] + _T("\n");
	}
	textInfo->SetValue(str);
}
/// フォント名を表示
void L3basicPanel::SetTextFontName()
{
	wxString point_str;

	point_str.Printf(_T(" (%dpt)"), mFontSize);

	textFont->SetValue(mFontName + point_str);
}
/// フォントを設定
void L3basicPanel::SetInitialFont()
{
	// load ini file
	Config *ini = wxGetApp().GetConfig();
	mFontName = ini->GetFontName();
	mFontSize = ini->GetFontSize();

	if (mFontName.IsEmpty()) {
		fontFixed = textInfo->GetFont();
	} else {
		fontFixed = wxFont(mFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, mFontName);
	}
	if (!fontFixed.IsOk()) {
		fontFixed = wxFont();
	}
	mFontName = fontFixed.GetFaceName();
	mFontSize = fontFixed.GetPointSize();
}

//
// File Dialog
//
L3basicFileDialog::L3basicFileDialog(const wxString& message, const wxString& defaultDir, const wxString& defaultFile, const wxString& wildcard, long style)
            : wxFileDialog(NULL, message, defaultDir, defaultFile, wildcard, style)
{
}

//
// File Drag and Drop
//
L3basicFileDropTarget::L3basicFileDropTarget(L3basicFrame *parent)
			: frame(parent)
{
}

bool L3basicFileDropTarget::OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames)
{
	if (filenames.Count() > 0) {
		wxString name = filenames.Item(0);
		frame->OpenDroppedFile(name);
	}
    return true;
}

//
// About dialog
//
L3basicAbout::L3basicAbout(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("About..."), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrLeft   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrRight  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrMain   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrAll    = new wxBoxSizer(wxVERTICAL);

	szrLeft->Add(new wxStaticBitmap(this, wxID_ANY,
		wxBitmap(l3basic_xpm), wxDefaultPosition, wxSize(64, 64))
		, flags);

	wxString str = _T("");
	str += _T("L3S1basic, Version ");
	str += _T(APPLICATION_VERSION);
	str += _T(" \"");
	str += _T(PLATFORM);
	str += _T("\"\n\n");
	str	+= _T("using ");
	str += wxVERSION_STRING;
	str += _T("\n\n");
	str	+= _T(APP_COPYRIGHT);

	szrRight->Add(new wxStaticText(this, wxID_ANY, str), flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK);
	szrMain->Add(szrLeft, flags);
	szrMain->Add(szrRight, flags);
	szrAll->Add(szrMain, flags);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}
