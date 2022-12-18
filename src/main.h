/// @file main.h
///
/// @brief 本体
///
#ifndef _L3S1BASIC_H_
#define _L3S1BASIC_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>
#include <wx/dnd.h>
#include <wx/fontdlg.h>
#include "config.h"
#include "parse.h"

class L3basicApp;
class L3basicFrame;
class L3basicPanel;
class L3basicFileDialog;
class L3basicFileDropTarget;

class CharTypeBox;

class ConfigBox;

/// メインWindow
class L3basicApp: public wxApp
{
private:
	wxString app_path;
	wxString ini_path;
	wxString res_path;
	wxLocale mLocale;
	Config   mConfig;
	L3basicFrame *frame;
	wxString in_file;

	void SetAppPath();
public:
	L3basicApp();
	bool OnInit();
	void OnInitCmdLine(wxCmdLineParser &parser);
	bool OnCmdLineParsed(wxCmdLineParser &parser);
	void MacOpenFile(const wxString &fileName);
	int  OnExit();
	const wxString &GetAppPath();
	const wxString &GetIniPath();
	const wxString &GetResPath();

	Config *GetConfig();
};

DECLARE_APP(L3basicApp)

/// メインFrame
class L3basicFrame: public wxFrame
{
private:
	// gui
	wxMenu *menuFile;
	wxMenu *menuOther;
	wxMenu *menuHelp;
	wxMenu *menuRecentFiles;
	L3basicPanel *panel;

	Parse *ps;

	ConfigBox *cfgbox;
//	CharTypeBox *ctypebox;

	wxString file_path;

public:

    L3basicFrame(const wxString& title, const wxSize& size);
	~L3basicFrame();

	bool Init(const wxString &in_file);

	/// @name event procedures
	//@{
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnOpenFile(wxCommandEvent& event);
	void OnCloseFile(wxCommandEvent& event);

	void OnExportFile(wxCommandEvent& event);

	void OnOpenRecentFile(wxCommandEvent& event);

	void OnConfigure(wxCommandEvent& event);

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
	void ReloadParsedData(int type, int mask, const wxString &char_type);
	//@}

	/// @name properties
	//@{
	L3basicPanel *GetL3basicPanel() { return panel; }
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
		IDM_CONFIGURE,

		IDD_CONFIGBOX,
		IDD_CHARTYPEBOX,
		IDD_INTNAMEBOX,

		IDM_RECENT_FILE_0 = 50,
	};

	DECLARE_EVENT_TABLE()
};

/// メインPanel
class L3basicPanel: public wxPanel
{
private:
	L3basicFrame *frame;

	wxBoxSizer *szrAll;

//	wxTextCtrl *textName;
	wxTextCtrl *textInfo;

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

	wxComboBox *comDCharType;

	wxTextCtrl *textFont;
	wxButton *btnFont;

	wxString mFontName;
	int      mFontSize;
	wxFont fontFixed;

	wxArrayString mOrigBasicTypes;
	wxArrayString mOrigCharTypes;

	void SetInitialFont();
public:
	L3basicPanel(L3basicFrame *parent);
	~L3basicPanel();

	/// @name event procedures
	//@{
	void OnSize(wxSizeEvent& event);
	void OnSelectBasicTypeI(wxCommandEvent& event);
	void OnSelectBinaryI(wxCommandEvent& event);
	void OnSelectAsciiI(wxCommandEvent& event);
	void OnSelectUTF8I(wxCommandEvent& event);
	void OnSelectCharTypeI(wxCommandEvent& event);
	void OnSelectDCharType(wxCommandEvent& event);
	void OnClickFont(wxCommandEvent& event);
	void OnClickExport(wxCommandEvent& event);
	//@}
	/// @name functions
	//@{
	void Update(PsFileType *file_type);
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
	void SetTextInfo(const wxString &char_type, const wxArrayString &lines);
	void SetTextFontName();
	//@}
	/// @name properties
	//@{
//	wxTextCtrl *GetTextName() { return textName; }
	wxTextCtrl *GetTextInfo() { return textInfo; }

	void SetFontName(const wxString &val) { mFontName = val; }
	void SetFontSize(int val) { mFontSize = val; }
	wxString &GetFontName() { return mFontName; }
	int GetFontSize() { return mFontSize; }
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

		IDC_COMBO_DCHARTYPE,

		IDC_TEXT_FONT,
		IDC_BUTTON_FONT
	};

	DECLARE_EVENT_TABLE()
};

/// ファイルダイアログ
class L3basicFileDialog: public wxFileDialog
{
public:
	L3basicFileDialog(const wxString& message, const wxString& defaultDir = wxEmptyString, const wxString& defaultFile = wxEmptyString, const wxString& wildcard = wxFileSelectorDefaultWildcardStr, long style = wxFD_DEFAULT_STYLE);

};

/// ファイル ドラッグ＆ドロップ
class L3basicFileDropTarget : public wxFileDropTarget
{
    L3basicFrame *frame;
public:
    L3basicFileDropTarget(L3basicFrame *parent);
    bool OnDropFiles(wxCoord x, wxCoord y ,const wxArrayString &filenames);
};

/// About dialog
class L3basicAbout : public wxDialog
{
public:
	L3basicAbout(wxWindow* parent, wxWindowID id);
};

#endif /* _L3S1BASIC_H_ */

