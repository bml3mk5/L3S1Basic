/// @file main.h
///
/// @brief 本体
///
#ifndef MAIN_H
#define MAIN_H

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>
#include <wx/dnd.h>
#include <wx/fontdlg.h>
#include "mytextctrl.h"
#include "parse.h"
#include "config.h"

class BasicApp;
class BasicFrame;
class BasicPanel;
class BasicFileDialog;
class BasicFileDropTarget;

class MyMenu;

//class CharTypeBox;
//class ConfigBox;

/// メインWindow
class BasicApp: public wxApp
{
private:
	wxString app_path;
	wxString ini_path;
	wxString res_path;
	wxLocale mLocale;
	BasicFrame *frame;
	wxString in_file;

	void SetAppPath();
public:
	BasicApp();
	bool OnInit();
	void OnInitCmdLine(wxCmdLineParser &parser);
	bool OnCmdLineParsed(wxCmdLineParser &parser);
	void MacOpenFile(const wxString &fileName);
	void MacOpenFiles(const wxArrayString &fileNames);
	int  OnExit();
	const wxString &GetAppPath();
	const wxString &GetIniPath();
	const wxString &GetResPath();

	BasicFrame *GetFrame();
};

DECLARE_APP(BasicApp)

/// メインFrame
class BasicFrame: public wxFrame
{
private:
	// gui
	MyMenu *menuFile;
	MyMenu *menuMachine;
	MyMenu *menuSettings;
	MyMenu *menuHelp;
	MyMenu *menuRecentFiles;
	BasicPanel *panel;

	ParseCollection psCollection;
	Parse *ps;

//	ConfigBox *cfgbox;
//	CharTypeBox *ctypebox;

//	wxString file_path;
	bool mOk;


public:

    BasicFrame(const wxString& title, const wxSize& size);
	~BasicFrame();

	bool Init(const wxString &in_file);
	bool Restart();

	/// @name event procedures
	//@{
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnOpenFile(wxCommandEvent& event);
	void OnCloseFile(wxCommandEvent& event);

	void OnExportFile(wxCommandEvent& event);

	void OnOpenRecentFile(wxCommandEvent& event);

	void OnChangeMachine(wxCommandEvent& event);

	void OnConfigure(wxCommandEvent& event);
	void OnDispSettings(wxCommandEvent& event);
	void OnTapeSettings(wxCommandEvent& event);

	void OnMenuOpen(wxMenuEvent& event);
	//@}

	/// @name functions
	//@{
	void UpdateMenu();
	void UpdateMenuRecentFiles();
	void OpenDataFile(const wxString &path);
	void CloseDataFile();
	void ExportFile(int id);
	void OpenDroppedFile(const wxString &path);
	void ReloadBinaryData(int type, int mask, const wxString &basic_type);
	void ReloadOpendData(int type, int mask, const wxString &char_type);
//	void ReloadParsedAsciiData(int type, int mask, const wxString &char_type, const wxString &basic_type);
	void ReloadParsedData(int type, int mask, const wxString &char_type, const wxString &basic_type = wxEmptyString);
	void ReloadData();
	//@}

	/// @name properties
	//@{
	BasicPanel *GetBasicPanel() { return panel; }
	Parse *GetParse() { return ps; }
	bool IsOk() const { return mOk; }
	ConfigParam *GetConfigParam();
	//@}

	enum en_menu_id
	{
		// menu id
		IDM_EXIT = 1,
		IDM_OPEN_FILE,
		IDM_CLOSE_FILE,
		IDM_EXPORT_FILE,
		IDM_EXPORT_BASICBIN,
		IDM_EXPORT_BASICBINTAPE,
		IDM_EXPORT_BASICBINDISK,
		IDM_EXPORT_ASCIITXT,
		IDM_EXPORT_ASCIITXTTAPE,
		IDM_EXPORT_ASCIITXTDISK,
		IDM_EXPORT_UTF8TEXT,
		IDM_MACHINE_L3S1,
		IDM_MACHINE_MSX,
		IDM_CONFIGURE,
		IDM_DISP_SETTINGS,
		IDM_TAPE_SETTINGS,

		IDD_CONFIGBOX,
		IDD_CHARTYPEBOX,
		IDD_INTNAMEBOX,
		IDD_TAPEBOX,

		IDM_RECENT_FILE_0 = 50,
	};

	DECLARE_EVENT_TABLE()
};

/// メインPanel
class BasicPanel: public wxPanel
{
private:
	BasicFrame *frame;

	wxBoxSizer *szrAll;

//	wxTextCtrl *textName;
	MyTextCtrl *textInfo;

	wxRadioButton *radBinaryI;
	wxRadioButton *radAsciiI;
	wxRadioButton *radUTF8I;

	wxComboBox *comBasicTypeI;
	wxComboBox *comCharTypeI;

	wxRadioButton *radBinaryO;
	wxRadioButton *radBinaryTapeO;
	wxRadioButton *radBinaryDiskO;
	wxRadioButton *radAsciiO;
	wxRadioButton *radAsciiTapeO;
	wxRadioButton *radAsciiDiskO;
	wxRadioButton *radUTF8O;

	wxComboBox *comBasicTypeO;
	wxComboBox *comCharTypeO;

	wxButton *btnExport;

	wxComboBox *comDBasicType;
	wxComboBox *comDCharType;

	wxTextCtrl *textFont;
	wxButton *btnFont;

//	wxString mFontName;
//	int      mFontSize;
	wxFont mFontFixed;

	wxArrayString mOrigBasicTypes;
	wxArrayString mOrigCharTypes;

	void SetInitialFont();
public:
	BasicPanel(BasicFrame *parent);
	~BasicPanel();

	/// @name event procedures
	//@{
	void OnSize(wxSizeEvent& event);
	void OnSelectBasicTypeI(wxCommandEvent& event);
	void OnSelectBinaryI(wxCommandEvent& event);
	void OnSelectAsciiI(wxCommandEvent& event);
	void OnSelectUTF8I(wxCommandEvent& event);
	void OnSelectCharTypeI(wxCommandEvent& event);
	void OnSelectDBasicType(wxCommandEvent& event);
	void OnSelectDCharType(wxCommandEvent& event);
	void OnClickFont(wxCommandEvent& event);
	void OnClickExport(wxCommandEvent& event);
	//@}
	/// @name functions
	//@{
	void UpdateControls(PsFileType *file_type);
	void AddBasicType(const wxArrayString &items);
	int  GetBasicTypeNum(int n);
	wxString GetBasicType(int n);
	void SetBasicType(int n, int pos);
	void SetBasicType(int n, const wxString &basic_type);

	void AddCharType(const wxArrayString &items);
	wxString GetCharType(int n);
	void SetCharType(int n, int pos);
	void SetCharType(int n, const wxString &char_type);

	void SetTextInfo(const wxString &str);
	void SetTextInfo(const wxString &char_type, const wxString &str);
	void SetTextInfo(const MyColorTag &color_tag, const wxString &str);
	void SetTextInfo(const wxString &char_type, const MyColorTag &color_tag, const wxArrayString &lines);
	void SetTextFontName();
	void RestartTextFont();
	//@}
	/// @name properties
	//@{
//	wxTextCtrl *GetTextName() { return textName; }
	MyTextCtrl *GetTextInfo() { return textInfo; }

//	void SetFontName(const wxString &val) { mFontName = val; }
//	void SetFontSize(int val) { mFontSize = val; }
//	wxString &GetFontName() { return mFontName; }
//	int GetFontSize() { return mFontSize; }
	//@}

	enum {
		IDC_TEXT_NAME = 1,
		IDC_TEXT_INFO,
		//
		IDC_RADIO_BINARY_I,
		IDC_RADIO_ASCII_I,
		IDC_RADIO_UTF8_I,
		IDC_COMBO_BASICTYPE_I,
		IDC_COMBO_CHARTYPE_I,
		//
		IDC_RADIO_BINARY_O,
		IDC_RADIO_BIN_TA_O,
		IDC_RADIO_BIN_DI_O,
		IDC_RADIO_ASCII_O,
		IDC_RADIO_ASC_T_O,
		IDC_RADIO_ASC_D_O,
		IDC_RADIO_UTF8_O,
		IDC_COMBO_BASICTYPE_O,
		IDC_COMBO_CHARTYPE_O,

		IDC_BUTTON_EXPORT,

		IDC_COMBO_DBASICTYPE,
		IDC_COMBO_DCHARTYPE,

		IDC_TEXT_FONT,
		IDC_BUTTON_FONT
	};

	DECLARE_EVENT_TABLE()
};

/// ファイルダイアログ
class BasicFileDialog: public wxFileDialog
{
public:
	BasicFileDialog(const wxString& message, const wxString& defaultDir = wxEmptyString, const wxString& defaultFile = wxEmptyString, const wxString& wildcard = wxFileSelectorDefaultWildcardStr, long style = wxFD_DEFAULT_STYLE);

};

/// ファイル ドラッグ＆ドロップ
class BasicFileDropTarget : public wxFileDropTarget
{
    BasicFrame *frame;
public:
    BasicFileDropTarget(BasicFrame *parent);
    bool OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames);
};

/// About dialog
class BasicAbout : public wxDialog
{
public:
	BasicAbout(wxWindow* parent, wxWindowID id);
};

#endif /* MAIN_H */

