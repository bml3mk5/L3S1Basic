/// @file main.cpp
///
/// @brief 本体
///
#include "main.h"
#include "configbox.h"
#include "dispsetbox.h"
#include "fontminibox.h"
#include "chartypebox.h"
#include "tapebox.h"
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include "mymenu.h"
#include "config.h"
#include "parse_l3s1basic.h"
#include "parse_msxbasic.h"
#include "res/l3s1basic.xpm"
#include "version.h"

#define BASIC_TRANS \
	_("can't open file '%s'") \
	_("can't create file '%s'") \
	_("can't close file descriptor %d") \
	_("can't read from file descriptor %d") \
	_("can't write to file descriptor %d") \
	_("can't flush file descriptor %d") \
	_("can't seek on file descriptor %d") \
	_("can't get seek position on file descriptor %d")

IMPLEMENT_APP(BasicApp)

BasicApp::BasicApp()
{
	frame = NULL;
}

bool BasicApp::OnInit()
{
	SetAppPath();
	SetAppName(_T(APPLICATION_NAME));

	if (!wxApp::OnInit()) {
		return false;
	}

	// load ini file
	gConfig.Load(ini_path + GetAppName() + _T(".ini"));

	// set locale search path and catalog name
	wxString locale_name = gConfig.GetLanguage();
	int lang_num = 0;
	if (locale_name.IsEmpty()) {
		lang_num = wxLocale::GetSystemLanguage();
	} else {
		const wxLanguageInfo * const lang = wxLocale::FindLanguageInfo(locale_name);
		if (lang) {
			lang_num = lang->Language;
		} else {
			lang_num = wxLANGUAGE_UNKNOWN;
		}
	}
	if (mLocale.Init(lang_num, wxLOCALE_LOAD_DEFAULT)) {
		mLocale.AddCatalogLookupPathPrefix(res_path + _T("lang"));
		mLocale.AddCatalogLookupPathPrefix(_T("lang"));
		mLocale.AddCatalog(_T(APPLICATION_NAME));
	}
	if (mLocale.IsLoaded(_T(APPLICATION_NAME))) {
		locale_name = mLocale.GetCanonicalName();
	} else {
		locale_name = wxT("");
	}

	frame = new BasicFrame(GetAppName(), wxSize(720, 600));
	if (!frame->IsOk()) {
		return false;
	}
	frame->Show(true);
	SetTopWindow(frame);

	if (!frame->Init(in_file)) {
		return false;
	}

	return true;
}

#define OPTION_VERBOSE "verbose"

void BasicApp::OnInitCmdLine(wxCmdLineParser &parser)
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

bool BasicApp::OnCmdLineParsed(wxCmdLineParser &parser)
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

void BasicApp::MacOpenFile(const wxString &fileName)
{
	if (frame) {
		frame->OpenDataFile(fileName);
	}
}

void BasicApp::MacOpenFiles(const wxArrayString &fileNames)
{
	if (frame) {
		frame->OpenDataFile(fileNames.Item(0));
	}
}

int BasicApp::OnExit()
{
	// save ini file
	gConfig.Save();

	return 0;
}

void BasicApp::SetAppPath()
{
	app_path = wxFileName::FileName(argv[0]).GetPath(wxPATH_GET_SEPARATOR);
#ifdef __WXOSX__
	if (app_path.Find(_T("MacOS")) >= 0) {
		wxFileName file = wxFileName::FileName(app_path+"../../../");
		file.Normalize(wxPATH_NORM_ALL);
		ini_path = file.GetPath(wxPATH_GET_SEPARATOR);
		file = wxFileName::FileName(app_path+"../../Contents/Resources/");
		file.Normalize(wxPATH_NORM_ALL);
		res_path = file.GetPath(wxPATH_GET_SEPARATOR);
	} else
#endif
	{
		ini_path = app_path;
		res_path = app_path;
	}
}

const wxString &BasicApp::GetAppPath()
{
	return app_path;
}

const wxString &BasicApp::GetIniPath()
{
	return ini_path;
}

const wxString &BasicApp::GetResPath()
{
	return res_path;
}

BasicFrame *BasicApp::GetFrame()
{
	return frame;
}

//
// Frame
//
// Attach Event
BEGIN_EVENT_TABLE(BasicFrame, wxFrame)
	// menu event
	EVT_MENU(wxID_EXIT,  BasicFrame::OnQuit)
	EVT_MENU(wxID_ABOUT, BasicFrame::OnAbout)

	EVT_MENU(IDM_OPEN_FILE, BasicFrame::OnOpenFile)
	EVT_MENU(IDM_CLOSE_FILE, BasicFrame::OnCloseFile)

	EVT_MENU(IDM_EXPORT_BASICBINTAPE, BasicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXTTAPE, BasicFrame::OnExportFile)

	EVT_MENU(IDM_EXPORT_BASICBINDISK, BasicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXTDISK, BasicFrame::OnExportFile)

	EVT_MENU(IDM_EXPORT_BASICBIN, BasicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_ASCIITXT, BasicFrame::OnExportFile)
	EVT_MENU(IDM_EXPORT_UTF8TEXT, BasicFrame::OnExportFile)

	EVT_MENU_RANGE(IDM_RECENT_FILE_0, IDM_RECENT_FILE_0 + MAX_RECENT_FILES - 1, BasicFrame::OnOpenRecentFile)

	EVT_MENU(IDM_MACHINE_L3S1, BasicFrame::OnChangeMachine)
	EVT_MENU(IDM_MACHINE_MSX, BasicFrame::OnChangeMachine)

	EVT_MENU(IDM_CONFIGURE, BasicFrame::OnConfigure)
	EVT_MENU(IDM_DISP_SETTINGS, BasicFrame::OnDispSettings)
	EVT_MENU(IDM_TAPE_SETTINGS, BasicFrame::OnTapeSettings)

	EVT_MENU_OPEN(BasicFrame::OnMenuOpen)
END_EVENT_TABLE()

// 翻訳用
#define DIALOG_BUTTON_STRING _("OK"),_("Cancel")
#define APPLE_MENU_STRING _("Hide l3s1basic"),_("Hide Others"),_("Show All"),_("Quit l3s1basic"),_("Services"),_("Preferences…"),_("Window"),_("Minimize"),_("Zoom"),_("Bring All to Front")

BasicFrame::BasicFrame(const wxString& title, const wxSize& size)
       : wxFrame(NULL, -1, title, wxDefaultPosition, size)
{
	// icon
#ifdef __WXMSW__
	SetIcon(wxIcon(_T(APPLICATION_NAME)));
#elif defined(__WXGTK__) || defined(__WXMOTIF__)
	SetIcon(wxIcon(APPLICATION_XPMICON_NAME));
#endif
	//
	mOk = false;

	// initialize
	psCollection.SetAppPath(wxGetApp().GetResPath());
	psCollection.Set(eL3S1Basic, new ParseL3S1Basic(&psCollection));
	psCollection.Set(eMSXBasic, new ParseMSXBasic(&psCollection));
	for(int i=0; i<eMachineCount; i++) {
		if (!(psCollection.Get(i)->Init())) {
			return;
		}
	}
	ps = psCollection.Get(gConfig.GetCurrentMachine());

	// menu
	menuFile = new MyMenu;
	menuMachine = new MyMenu;
	menuSettings = new MyMenu;
	menuHelp = new MyMenu;
	MyMenu *smenu;

	// file menu
	menuFile->Append( IDM_OPEN_FILE, _("&Open...\tCTRL+O") );
	menuFile->Append( IDM_CLOSE_FILE, _("&Close") );
	menuFile->AppendSeparator();
	smenu = new MyMenu;
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
	menuRecentFiles = new MyMenu();
	UpdateMenuRecentFiles();
	menuFile->AppendSubMenu(menuRecentFiles, _("&Reccent Files") );
	menuFile->AppendSeparator();
	menuFile->Append( wxID_EXIT, _("E&xit\tALT+F4") );
	// machine menu
	menuMachine->AppendRadioItem( IDM_MACHINE_L3S1, psCollection.Get(eL3S1Basic)->GetMachineName() );
	menuMachine->AppendRadioItem( IDM_MACHINE_MSX, psCollection.Get(eMSXBasic)->GetMachineName() );
	// settings menu
	menuSettings->Append( IDM_CONFIGURE, _("&File Settings...") );
	menuSettings->Append( IDM_DISP_SETTINGS, _("&Display Settings...") );
	menuSettings->Append( IDM_TAPE_SETTINGS, _("&Tape Image Settings...") );
	// help menu
	menuHelp->Append( wxID_ABOUT, _("&About...") );

	// menu bar
	MyMenuBar *menuBar = new MyMenuBar;
	menuBar->Append( menuFile, _("&File") );
	menuBar->Append( menuMachine, _("&Machine") );
	menuBar->Append( menuSettings, _("&Settings") );
#if defined(__WXOSX__) && wxCHECK_VERSION(3,1,2)
	menuBar->Append( new wxMenu, _("&Window") );
#endif
	menuBar->Append( menuHelp, _("&Help") );

	SetMenuBar( menuBar );

	// control panel
	panel = new BasicPanel(this);

	// drag and drop
	SetDropTarget(new BasicFileDropTarget(this));

	// select machine
	if (menuMachine) {
		wxMenuItem *item = menuMachine->FindItem(IDM_MACHINE_L3S1 + gConfig.GetCurrentMachine());
		if (item) {
			item->Check();
		}
	}

	mOk = true;
}

BasicFrame::~BasicFrame()
{
	// save ini file
//	gConfig.SetFilePath(file_path);
//	gConfig.SetParam(ps->GetParam());
}

/// 初期化
bool BasicFrame::Init(const wxString &in_file)
{
	// load ini file
//	file_path = gConfig.GetFilePath();
//	ps->SetParam(gConfig.GetParam());

	// set combo box on panel
	wxArrayString basic_types;
	ps->GetBasicTypes(basic_types);
	panel->AddBasicType(basic_types);
	wxArrayString char_types;
	ps->GetCharTypes(char_types);
	panel->AddCharType(char_types);
	panel->UpdateControls(NULL);

	UpdateMenu();

	// open file
	if (!in_file.IsEmpty()) {
		OpenDataFile(in_file);
	}

	return true;
}

/// リスタート
bool BasicFrame::Restart()
{
	// set combo box on panel
	wxArrayString basic_types;
	ps->GetBasicTypes(basic_types);
	panel->AddBasicType(basic_types);
	wxArrayString char_types;
	ps->GetCharTypes(char_types);
	panel->AddCharType(char_types);
	panel->RestartTextFont();
	panel->UpdateControls(NULL);

	UpdateMenu();

	return true;
}

/// メニュー更新
void BasicFrame::OnMenuOpen(wxMenuEvent& event)
{
	wxMenu *menu = event.GetMenu();

	if (menu == NULL) return;

	if (menu == menuFile) {	// File...
		UpdateMenu();
	}
}
/// ドロップされたファイルを開く
void BasicFrame::OpenDroppedFile(const wxString &path)
{
	CloseDataFile();
	OpenDataFile(path);
}

void BasicFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
 	CloseDataFile();
	Close(true);
}

void BasicFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	BasicAbout(this, wxID_ANY).ShowModal();
}

void BasicFrame::OnOpenFile(wxCommandEvent& WXUNUSED(event))
{
	BasicFileDialog *dlg = new BasicFileDialog(
		_("Open file"),
//		ps->GetConfigParam()->GetFilePath(),
		gConfig.GetRecentFilePath(),
		wxEmptyString,
		ps->GetOpenFileExtensions(),
		wxFD_OPEN);

	int rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		OpenDataFile(path);
	}
}

void BasicFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
	CloseDataFile();
}

/// エクスポート
void BasicFrame::OnExportFile(wxCommandEvent& event)
{
	int id = event.GetId();

	ExportFile(id);
}

/// 最近使用したファイル
void BasicFrame::OnOpenRecentFile(wxCommandEvent& event)
{
	wxMenuItem *item = menuRecentFiles->FindItem(event.GetId());
	if (!item) return;
	wxFileName path = item->GetItemLabel();
	CloseDataFile();
	OpenDataFile(path.GetFullPath());
}

/// 機種変更
void BasicFrame::OnChangeMachine(wxCommandEvent& event)
{
	if (!mOk) {
		return;
	}

	int idx = event.GetId() - IDM_MACHINE_L3S1;
	if (ps == psCollection.Get(idx)) {
		return;
	}
	// 切換
	wxString path;
	if (ps->IsOpenedDataFile()) {
		path = ps->GetFileFullPath();
		CloseDataFile();
	}

	ps = psCollection.Get(idx);

	Restart();

	if (!path.IsEmpty()) {
		OpenDataFile(path);
	}

	gConfig.SetCurrentMachine((enMachines)idx);
}

/// 設定ダイアログ
void BasicFrame::OnConfigure(wxCommandEvent& WXUNUSED(event))
{
	ConfigBox cfgbox(this, wxID_ANY, psCollection);

	// set combo box on configbox
	if (cfgbox.ShowModal() == wxID_OK) {
		cfgbox.GetParams(psCollection);
	}
}

/// 表示設定ダイアログ
void BasicFrame::OnDispSettings(wxCommandEvent& WXUNUSED(event))
{
	DisplaySettingBox dispbox(this, wxID_ANY);

	if (dispbox.ShowModal() == wxID_OK) {
		ReloadData();
	}
}

// テープイメージ設定ダイアログ
void BasicFrame::OnTapeSettings(wxCommandEvent& event)
{
	TapeBox tapebox(this, wxID_ANY, ps);

	PsFileType *opened_flags = ps->GetOpenedDataTypePtr();
	if (opened_flags) {
		tapebox.SetInternalName(opened_flags->GetInternalName());
	}

	if (tapebox.ShowModal() == wxID_OK) {
		if (opened_flags) {
			opened_flags->SetInternalName(tapebox.GetInternalName());
		}
	}
}

/// メニューの更新
void BasicFrame::UpdateMenu()
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
void BasicFrame::UpdateMenuRecentFiles()
{
	// メニューを更新
	wxArrayString names;
	gConfig.GetRecentFiles(names);
	for(int i=0; i<MAX_RECENT_FILES && i<(int)names.Count(); i++) {
		if (menuRecentFiles->FindItem(IDM_RECENT_FILE_0 + i)) menuRecentFiles->Delete(IDM_RECENT_FILE_0 + i);
		menuRecentFiles->Append(IDM_RECENT_FILE_0 + i, names[i]);
	}
}

/// 指定したファイルを開く
void BasicFrame::OpenDataFile(const wxString &path)
{
	wxString title;

	// set recent file path
//	wxString file_path = wxFileName::FileName(path).GetPath();
	gConfig.SetRecentFilePath(path);

	panel->SetTextInfo(_("Now Processing..."));
	Update();

	wxString basic_type = panel->GetBasicType(1);
	PsFileType file_type;
	file_type.SetMachineAndBasicType(ps->GetMachineType(basic_type), basic_type, ps->IsExtendedBasic(basic_type));

	if (!ps->OpenDataFile(path, file_type)) {
		panel->SetTextInfo(wxEmptyString);
		return;
	}

	// update window
	title = wxGetApp().GetAppName() + _T(" - ") + path;
	SetTitle(title);
	UpdateMenu();

	// update panel
	PsFileType *opened_flags = ps->GetOpenedDataTypePtr();
	// 内部ファイル名
	if (opened_flags && opened_flags->GetInternalName().IsEmpty()) {
		opened_flags->SetInternalName(wxFileName::FileName(path).GetName());	
	}

	panel->UpdateControls(opened_flags);
	panel->SetBasicType(0, ps->GetOpenedBasicType());
	panel->SetBasicType(1, ps->GetOpenedBasicType());
	panel->SetBasicType(2, ps->GetOpenedBasicType());

	wxArrayString lines;
	wxString char_type = ps->GetParsedData(lines);
	panel->SetCharType(2, char_type);
	panel->SetTextInfo(char_type, gConfig.GetCurrentParam().GetColorTag(), lines);

	gConfig.AddRecentFile(path, true, false);
	UpdateMenuRecentFiles();
}

/// ファイルを閉じる
void BasicFrame::CloseDataFile()
{
	if (ps == NULL) return;

	ps->CloseDataFile();

	// update window
	wxString title = wxGetApp().GetAppName();
	SetTitle(title);
	UpdateMenu();

	// update panel
	panel->SetTextInfo(wxEmptyString);
	panel->UpdateControls(NULL);
}

/// エクスポート
void BasicFrame::ExportFile(int id)
{
	int rc;
	wxString file_base;
	wxString wild_card;
	PsFileType file_type;

	if (!ps->IsOpenedDataFile()) return;

	file_base = ps->GetFileNameBase();

	PsFileType *opened_flags = ps->GetOpenedDataTypePtr();
	bool enable = true;

	TapeBox intnamebox(this, IDD_INTNAMEBOX, ps);

	switch(id) {
		case IDM_EXPORT_BASICBIN:
			file_base += ps->GetExportBasicBinaryFileExtension();
			wild_card = ps->GetExportBasicBinaryFileExtensions();
			file_type.SetTypeFlag(psBinary, true);
			break;
		case IDM_EXPORT_BASICBINTAPE:
			file_base += ps->GetExportBasicBinaryTapeImageExtension();
			wild_card = ps->GetExportBasicBinaryTapeImageExtensions();
			file_type.SetTypeFlag(psBinary | psTapeImage, true);
			intnamebox.SetInternalName(opened_flags->GetInternalName());
			if (intnamebox.ShowModal() == wxID_OK) {
				file_type.SetInternalName(intnamebox.GetInternalName());
				opened_flags->SetInternalName(intnamebox.GetInternalName());
			} else {
				enable = false;
			}
			break;
		case IDM_EXPORT_BASICBINDISK:
			file_base += ps->GetExportBasicBinaryDiskImageExtension();
			wild_card = ps->GetExportBasicBinaryDiskImageExtensions();
			file_type.SetTypeFlag(psBinary | psDiskImage, true);
			break;
		case IDM_EXPORT_ASCIITXT:
			file_base += ps->GetExportBasicAsciiFileExtension();
			wild_card = ps->GetExportBasicAsciiFileExtensions();
			file_type.SetTypeFlag(psAscii, true);
			break;
		case IDM_EXPORT_ASCIITXTTAPE:
			file_base += ps->GetExportBasicAsciiTapeImageExtension();
			wild_card = ps->GetExportBasicAsciiTapeImageExtensions();
			file_type.SetTypeFlag(psAscii | psTapeImage, true);
			intnamebox.SetInternalName(opened_flags->GetInternalName());
			if (intnamebox.ShowModal() == wxID_OK) {
				file_type.SetInternalName(intnamebox.GetInternalName());
				opened_flags->SetInternalName(intnamebox.GetInternalName());
			} else {
				enable = false;
			}
			break;
		case IDM_EXPORT_ASCIITXTDISK:
			file_base += ps->GetExportBasicAsciiDiskImageExtension();
			wild_card = ps->GetExportBasicAsciiDiskImageExtensions();
			file_type.SetTypeFlag(psAscii | psDiskImage, true);
			break;
		case IDM_EXPORT_UTF8TEXT:
			file_base += ps->GetExportUTF8TextFileExtension();
			wild_card = ps->GetExportUTF8TextFileExtensions();
			file_type.SetTypeFlag(psAscii | psUTF8, true);
			file_type.SetCharType(panel->GetCharType(2));
			break;
		default:
			file_base += _T("");
			wild_card = _("All Files (*.*)|*.*");
			file_type.SetTypeFlag(psBinary, true);
			break;
	}
	if (!enable) return;

	BasicFileDialog *dlg = new BasicFileDialog(
		_("Export File"),
//		ps->GetConfigParam()->GetFilePath(),
		gConfig.GetRecentFilePath(),
		file_base,
		wild_card,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	rc = dlg->ShowModal();
	wxString path = dlg->GetPath();

	delete dlg;

	if (rc == wxID_OK) {
		wxString basic_type = panel->GetBasicType(2);
		file_type.SetMachineAndBasicType(ps->GetMachineType(basic_type), basic_type, ps->IsExtendedBasic(basic_type));
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
		panel->SetTextInfo(nchar_type, gConfig.GetCurrentParam().GetColorTag(), nlines);
	}
}

/// ファイルを読み直す
void BasicFrame::ReloadBinaryData(int type, int mask, const wxString &basic_type)
{
	if (ps == NULL) return;
	if (ps->ReloadOpenedBinaryData(type, mask, basic_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, gConfig.GetCurrentParam().GetColorTag(), nlines);
	}
}

/// ファイルを読み直す
void BasicFrame::ReloadOpendData(int type, int mask, const wxString &char_type)
{
	if (ps == NULL) return;
	if (ps->ReloadOpendAsciiData(type, mask, char_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, gConfig.GetCurrentParam().GetColorTag(), nlines);
	}
}

/// ファイルを読み直す
void BasicFrame::ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type)
{
	if (ps == NULL) return;
	if (ps->ReloadParsedData(type, mask, char_type, basic_type)) {
		wxArrayString nlines;
		wxString nchar_type = ps->GetParsedData(nlines);
		panel->SetTextInfo(nchar_type, gConfig.GetCurrentParam().GetColorTag(), nlines);
	}
}
/// 再表示
void BasicFrame::ReloadData()
{
	ReloadParsedData(0, 0, wxEmptyString);
}

/// 設定パラメータ
ConfigParam *BasicFrame::GetConfigParam()
{
	return ps->GetConfigParam();
}

//
// Control Panel
//
// Attach Event
BEGIN_EVENT_TABLE(BasicPanel, wxPanel)
	// event
	EVT_SIZE(BasicPanel::OnSize)

	EVT_COMBOBOX(IDC_COMBO_BASICTYPE_I, BasicPanel::OnSelectBasicTypeI)

	EVT_RADIOBUTTON(IDC_RADIO_ASCII_I, BasicPanel::OnSelectAsciiI)
	EVT_RADIOBUTTON(IDC_RADIO_UTF8_I, BasicPanel::OnSelectUTF8I)

	EVT_COMBOBOX(IDC_COMBO_CHARTYPE_I, BasicPanel::OnSelectCharTypeI)

	EVT_COMBOBOX(IDC_COMBO_DBASICTYPE, BasicPanel::OnSelectDBasicType)
	EVT_COMBOBOX(IDC_COMBO_DCHARTYPE, BasicPanel::OnSelectDCharType)

	EVT_BUTTON(IDC_BUTTON_FONT, BasicPanel::OnClickFont)

	EVT_BUTTON(IDC_BUTTON_EXPORT, BasicPanel::OnClickExport)
	
END_EVENT_TABLE()

BasicPanel::BasicPanel(BasicFrame *parent)
       : wxPanel(parent)
{
	frame    = parent;
	textInfo = NULL;

	wxSize  frame_size = parent->GetClientSize();
	long style;

	wxSizerFlags flagsM = wxSizerFlags().Expand().Border(wxALL, 0);
	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxALL, 2);
	wxBoxSizer *hbox, *hboxa, *hboxb;
	wxBoxSizer *vbox, *vboxa, *vboxb;
	wxSize size;
	size.x = -1; size.y = -1;
	wxString dummystr = wxT("wwwwwwwwwwwwww");

	szrAll = new wxBoxSizer(wxVERTICAL);

	hbox = new wxBoxSizer(wxHORIZONTAL);

	// 入力ファイル情報
	vbox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Input File Information")), wxVERTICAL);

	radBinaryI   = new wxRadioButton(this, IDC_RADIO_BINARY_I, _("BASIC Intermediate Language"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	vbox->Add(radBinaryI, flagsW);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("        ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("BASIC Type")), flagsW);
	comBasicTypeI = new wxComboBox(this, IDC_COMBO_BASICTYPE_I, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comBasicTypeI, flagsW);
	vbox->Add(hboxb, 0);

	radAsciiI = new wxRadioButton(this, IDC_RADIO_ASCII_I, _("Ascii Text"), wxDefaultPosition, wxDefaultSize, 0);
	radUTF8I  = new wxRadioButton(this, IDC_RADIO_UTF8_I , _("UTF-8 Text"), wxDefaultPosition, wxDefaultSize, 0);
	vbox->Add(radAsciiI, flagsW);
	vbox->Add(radUTF8I, flagsW);

	hboxb = new wxBoxSizer(wxHORIZONTAL);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("        ")), flagsW);
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
	vbox->Add(new wxStaticText(this, wxID_ANY, _T("--->")), flagsW);
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
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("        ")), flagsW);
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
	hboxb->Add(new wxStaticText(this, wxID_ANY, _T("        ")), flagsW);
	hboxb->Add(new wxStaticText(this, wxID_ANY, _("Text Type")), flagsW);
	comCharTypeO = new wxComboBox(this, IDC_COMBO_CHARTYPE_O, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hboxb->Add(comCharTypeO, flagsW);
	vboxa->Add(hboxb, 0);

	hboxa->Add(vboxa, 0);

	// export button
	vboxa = new wxBoxSizer(wxVERTICAL);

	wxSize size_btn(-1, 48);
	btnExport = new wxButton(this, IDC_BUTTON_EXPORT, _("Export"), wxDefaultPosition, size_btn, 0);
	vboxa->Add(btnExport, wxSizerFlags().Border(wxLEFT | wxRIGHT, 16));
	hboxa->Add(vboxa, 0);

	vbox->Add(hboxa, 0);
	hbox->Add(vbox, flagsW);

	szrAll->Add(hbox, 0);

	// テキスト表示
	vboxb = new wxBoxSizer(wxVERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Display Type")), flagsW);
	comDBasicType = new wxComboBox(this, IDC_COMBO_DBASICTYPE, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hbox->Add(comDBasicType, flagsW);
	hbox->Add(new wxStaticText(this, wxID_ANY, _T("  ")), flagsW);
	comDCharType = new wxComboBox(this, IDC_COMBO_DCHARTYPE, _T(""), wxDefaultPosition, size, 1, &dummystr, wxCB_DROPDOWN | wxCB_READONLY);
	hbox->Add(comDCharType, flagsW);
	hbox->Add(new wxStaticText(this, wxID_ANY, _T("  ")), flagsW);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Font")), flagsW);
	style = wxTE_READONLY;
	size.x = DEFAULT_TEXTWIDTH * 1.5;
	textFont = new wxTextCtrl(this, IDC_TEXT_FONT, wxEmptyString, wxDefaultPosition, size, style);
	btnFont = new wxButton(this, IDC_BUTTON_FONT, _("Change"));
	hbox->Add(textFont, flagsW);
	hbox->Add(btnFont, flagsW);
	vboxb->Add(hbox, flagsW);

	szrAll->Add(vboxb, flagsM);

	wxBoxSizer *vboxc = new wxBoxSizer(wxVERTICAL);
	size.x = frame_size.x; size.y = -1;
	textInfo = new MyTextCtrl(this, IDC_TEXT_INFO, wxEmptyString, wxDefaultPosition, size);
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
	RestartTextFont();

	// drop target
	textInfo->SetDropTarget(new BasicFileDropTarget(frame));
}

BasicPanel::~BasicPanel()
{
	// save ini file
//	gConfig.SetFontName(mFontName);
//	gConfig.SetFontSize(mFontSize);
}

/// リサイズ
void BasicPanel::OnSize(wxSizeEvent& event)
{
	wxSize size = event.GetSize();

	if (textInfo) textInfo->SetSize(size.x, size.y - textInfo->GetPosition().y);
}

/// BASIC種類選択
void BasicPanel::OnSelectBasicTypeI(wxCommandEvent& event)
{
	frame->ReloadBinaryData(psBinary, psAscii, GetBasicType(0));
}
/// BASIC中間言語ラジオボタン選択
void BasicPanel::OnSelectBinaryI(wxCommandEvent& event)
{
	frame->ReloadBinaryData(psBinary, psAscii, GetBasicType(0));
}
/// Asciiラジオボタン選択
void BasicPanel::OnSelectAsciiI(wxCommandEvent& event)
{
	frame->ReloadOpendData(psAscii, psUTF8, _T(""));
}
/// UTF8ラジオボタン選択
void BasicPanel::OnSelectUTF8I(wxCommandEvent& event)
{
	frame->ReloadOpendData(psAscii | psUTF8, 0, GetCharType(0));
}
/// 文字種類選択
void BasicPanel::OnSelectCharTypeI(wxCommandEvent& event)
{
	radUTF8I->SetValue(true);
	frame->ReloadOpendData(psAscii | psUTF8, 0, GetCharType(0));
}
/// 表示するBASIC種類選択
void BasicPanel::OnSelectDBasicType(wxCommandEvent& event)
{
	comBasicTypeO->SetSelection(comDBasicType->GetSelection());
	frame->ReloadParsedData(0, 0, GetCharType(1), GetBasicType(1));
}
/// 表示する文字種類選択
void BasicPanel::OnSelectDCharType(wxCommandEvent& event)
{
	comCharTypeO->SetSelection(comDCharType->GetSelection());
	frame->ReloadParsedData(psAscii | psUTF8, 0, GetCharType(1));
}
/// フォントボタン選択
void BasicPanel::OnClickFont(wxCommandEvent& event)
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
	FontMiniBox fontbox(this, wxID_ANY, GetFont());
//	fontbox.SetFontName(mFontName);
//	fontbox.SetFontSize(mFontSize);
	fontbox.SetFontName(frame->GetConfigParam()->GetFontName());
	fontbox.SetFontSize(frame->GetConfigParam()->GetFontSize());

	if (fontbox.ShowModal() == wxID_OK) {
		wxFont new_font = wxFont(fontbox.GetFontSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, fontbox.GetFontName());
		if (new_font.IsOk()) {
//			mFontName = new_font.GetFaceName();
//			mFontSize = new_font.GetPointSize();
			frame->GetConfigParam()->SetFontName(new_font.GetFaceName());
			frame->GetConfigParam()->SetFontSize(new_font.GetPointSize());
			mFontFixed = new_font;
		}

		// set font
		SetTextFontName();
		textInfo->SetFont(mFontFixed);
	}
#endif
}
/// エクスポートボタン選択
void BasicPanel::OnClickExport(wxCommandEvent& event)
{
	int id = 0;
	if (radBinaryO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_BASICBIN;
	} else if (radBinaryTapeO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_BASICBINTAPE;
	} else if (radBinaryDiskO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_BASICBINDISK;
	} else if (radAsciiO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_ASCIITXT;
	} else if (radAsciiTapeO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_ASCIITXTTAPE;
	} else if (radAsciiDiskO->GetValue()) {
		id = BasicFrame::IDM_EXPORT_ASCIITXTDISK;
	} else if (radUTF8O->GetValue()) {
		id = BasicFrame::IDM_EXPORT_UTF8TEXT;
	}

	frame->ExportFile(id);
}

/// コントロールを更新
void BasicPanel::UpdateControls(PsFileType *file_type)
{
	radBinaryI->Enable(false);
	radAsciiI->Enable(false);
	radUTF8I->Enable(false);
	radBinaryI->SetValue(true);
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
void BasicPanel::AddBasicType(const wxArrayString &items)
{
	mOrigBasicTypes.Empty();
	comBasicTypeI->Clear();
	comBasicTypeO->Clear();
	comDBasicType->Clear();
	for(size_t i=0; i<items.GetCount(); i++) {
		mOrigBasicTypes.Add(items[i]);
		comBasicTypeI->Insert(wxGetTranslation(items[i]), (int)i);
		comBasicTypeO->Insert(wxGetTranslation(items[i]), (int)i);
		comDBasicType->Insert(wxGetTranslation(items[i]), (int)i);
	}
	comBasicTypeI->Select(0);
	comBasicTypeO->Select(0);
	comDBasicType->Select(0);
}
/// Basic type を返す
int BasicPanel::GetBasicTypeNum(int n)
{
	int type = -1;
	wxComboBox *cb;
	switch(n) {
	case 1:
		cb = comDBasicType;
		break;
	case 2:
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
wxString BasicPanel::GetBasicType(int n)
{
	int type = GetBasicTypeNum(n);
	if (type >= 0) {
		return mOrigBasicTypes[type];
	} else {
		return _T("");
	}
}
/// Basic typeをセット
void BasicPanel::SetBasicType(int n, int pos)
{
	if (pos < 0) return;

	switch(n) {
	case 1:
		comDBasicType->Select(pos);
		break;
	case 2:
		comBasicTypeO->Select(pos);
		break;
	default:
		comBasicTypeI->Select(pos);
		break;
	}
}
/// Basic typeをセット
void BasicPanel::SetBasicType(int n, const wxString &basic_type)
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
void BasicPanel::AddCharType(const wxArrayString &items)
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
wxString BasicPanel::GetCharType(int n)
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
void BasicPanel::SetCharType(int n, int pos)
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
/// Char typeをセット
void BasicPanel::SetCharType(int n, const wxString &char_type)
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

/// 文字列を情報ウィンドウにセット
/// @param[in] str : 文字列
void BasicPanel::SetTextInfo(const wxString &str)
{
	textInfo->SetValue(str);
}
/// 文字列を情報ウィンドウにセット
/// @param[in] char_type : 文字種別
/// @param[in] str : 文字列
void BasicPanel::SetTextInfo(const wxString &char_type, const wxString &str)
{
	SetCharType(1, char_type);
	textInfo->SetLine(str);
}
/// 文字列を情報ウィンドウにセット
/// @param[in] color_tag : 色情報
/// @param[in] str : 文字列
void BasicPanel::SetTextInfo(const MyColorTag &color_tag, const wxString &str)
{
	textInfo->SetColorTag(color_tag);
	textInfo->SetLine(str);
}
/// 文字列を情報ウィンドウにセット
/// @param[in] char_type : 文字種別
/// @param[in] color_tag : 色情報
/// @param[in] lines : 文字列リスト
void BasicPanel::SetTextInfo(const wxString &char_type, const MyColorTag &color_tag, const wxArrayString &lines)
{
	SetCharType(1, char_type);
	textInfo->SetColorTag(color_tag);
	textInfo->SetLines(lines);
}
/// フォント名を表示
void BasicPanel::SetTextFontName()
{
	wxString point_str;

	point_str.Printf(_T(" (%dpt)"), frame->GetConfigParam()->GetFontSize());

	textFont->SetValue(frame->GetConfigParam()->GetFontName() + point_str);
}
/// フォントを設定
void BasicPanel::SetInitialFont()
{
	// load ini file
//	mFontName = gConfig.GetFontName();
//	mFontSize = gConfig.GetFontSize();
	wxString fontName = frame->GetConfigParam()->GetFontName();
	int fontSize = frame->GetConfigParam()->GetFontSize();

	if (fontName.IsEmpty()) {
		mFontFixed = textInfo->GetFont();
	} else {
		mFontFixed = wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, fontName);
	}
	if (!mFontFixed.IsOk()) {
		mFontFixed = wxFont();
	}
	fontName = mFontFixed.GetFaceName();
	fontSize = mFontFixed.GetPointSize();

	frame->GetConfigParam()->SetFontName(fontName);
	frame->GetConfigParam()->SetFontSize(fontSize);
}

/// フォントの再設定
void BasicPanel::RestartTextFont()
{
	SetInitialFont();
	SetTextFontName();
	textInfo->SetFont(mFontFixed);
}

//
// File Dialog
//
BasicFileDialog::BasicFileDialog(const wxString& message, const wxString& defaultDir, const wxString& defaultFile, const wxString& wildcard, long style)
            : wxFileDialog(NULL, message, defaultDir, defaultFile, wildcard, style)
{
}

//
// File Drag and Drop
//
BasicFileDropTarget::BasicFileDropTarget(BasicFrame *parent)
			: frame(parent)
{
}

bool BasicFileDropTarget::OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames)
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
BasicAbout::BasicAbout(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("About..."), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrLeft   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrRight  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrMain   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrAll    = new wxBoxSizer(wxVERTICAL);

	szrLeft->Add(new wxStaticBitmap(this, wxID_ANY,
		wxBitmap(l3s1basic_xpm), wxDefaultPosition, wxSize(64, 64))
		, flags);

	wxString str = _T("");
	str += _T(APPLICATION_FULLNAME);
	str += _T(", Version ");
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
