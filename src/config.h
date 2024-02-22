/// @file config.h
///
/// @brief 設定ファイル入出力
///
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/fileconf.h>
#include "parseparam.h"
#include "colortag.h"

enum enMachines {
	eL3S1Basic = 0,
	eMSXBasic,
	eMachineCount
};

#define MAX_RECENT_FILES 20

#define USE_RECENT_PATH_EACH_MACHINE 1

/// 設定ファイルサブ
class ConfigParam : public ParseParam
{
protected:
#ifdef USE_RECENT_PATH_EACH_MACHINE
	wxString mFilePath;
#endif
	wxString mFontName;
	int      mFontSize;

	bool	 mAddSpaceAfterColon;

	bool	 mLoaded;

	void SetDefaultFontName();

	MyColorTag mColorTag;

public:
	ConfigParam();
	~ConfigParam();

	void Load(wxFileConfig *ini, const wxString &section_name);
	void Save(wxFileConfig *ini, const wxString &section_name);
	void Delete(wxFileConfig *ini, const wxString &section_name);

#ifdef USE_RECENT_PATH_EACH_MACHINE
	void SetFilePath(const wxString &val);
	wxString &GetFilePath() { return mFilePath; }
	const wxString &GetFilePath() const { return mFilePath; }
#endif
	void SetFontName(const wxString &val) { mFontName = val; }
	wxString &GetFontName() { return mFontName; }
	void SetFontSize(int val) { mFontSize = val; }
	int GetFontSize() { return mFontSize; }
	void EnableAddSpaceAfterColon(bool val) { mAddSpaceAfterColon = val; }
	bool EnableAddSpaceAfterColon() const { return mAddSpaceAfterColon; }
	MyColorTag &GetColorTag() { return mColorTag; }
	const MyColorTag &GetColorTag() const { return mColorTag; }
	void SetColorTag(const MyColorTag &val) { mColorTag = val; }

	bool IsLoaded() const { return mLoaded; }
};

/// 設定ファイル入出力
class Config
{
private:
	wxString ini_file;

	wxString	mLanguage;			///< 言語

	enMachines	mCurrentMachine;	///< 処理中の機種

	ConfigParam mParam[eMachineCount];

#ifndef USE_RECENT_PATH_EACH_MACHINE
	wxString	  mRecentFilePath;
#endif
	wxArrayString mRecentFiles;

	static const char *cSection[eMachineCount];

public:
	Config();
	~Config();
	void SetFileName(const wxString &file);
	void Load(const wxString &file);
	void Load();
	void Save();

	/// @name properties
	//@{
	const wxString &GetLanguage() const { return mLanguage; }
	void SetLanguage(const wxString &val) { mLanguage = val; }
	enMachines GetCurrentMachine() const { return mCurrentMachine; }
	void SetCurrentMachine(enMachines val) { mCurrentMachine = val; }
	ConfigParam &GetParam(int idx) { return mParam[idx]; }
	ConfigParam &GetCurrentParam();
	void SetRecentFilePath(const wxString &val);
	const wxString &GetRecentFilePath() const;
	void AddRecentFile(const wxString &val, bool update_path, bool is_append);
	wxString &GetRecentFile(int idx = 0);
	void GetRecentFiles(wxArrayString &vals);
	int GetRecentFileCount() const;
	//@}
};

extern Config gConfig;

#endif /* _CONFIG_H_ */
