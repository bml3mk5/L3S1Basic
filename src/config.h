/// @file config.h
///
/// @brief 設定ファイル入出力
///
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "common.h"
#include <wx/wx.h>
#include "parseparam.h"

#define MAX_RECENT_FILES 20

/// 設定ファイル入出力
class Config : public ParseParam
{
private:
	wxString ini_file;

	wxString mFilePath;
	wxString mFontName;
	int      mFontSize;
	wxArrayString mRecentFiles;

	bool	 mAddSpaceAfterColon;

	wxString	mLanguage;			///< 言語

	void SetDefaultFontName();
public:
	Config();
	~Config();
	void SetFileName(const wxString &file);
	void Load(const wxString &file);
	void Load();
	void Save();

	/// @name properties
	//@{
	void SetFilePath(const wxString &val);
	void SetFontName(const wxString &val) { mFontName = val; }
	void SetFontSize(int val) { mFontSize = val; }
	wxString &GetFilePath() { return mFilePath; }
	wxString &GetFontName() { return mFontName; }
	void DoesAddSpaceAfterColon(bool val) { mAddSpaceAfterColon = val; }
	const wxString &GetLanguage() const { return mLanguage; }
	int GetFontSize() { return mFontSize; }
	void AddRecentFile(const wxString &val);
	wxString &GetRecentFile();
	void GetRecentFiles(wxArrayString &vals);
	bool DoesAddSpaceAfterColon() const { return mAddSpaceAfterColon; }
	void SetLanguage(const wxString &val) { mLanguage = val; }
	//@}
};

extern Config gConfig;

#endif /* _CONFIG_H_ */
