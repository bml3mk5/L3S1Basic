/// @file config.cpp
///
/// @brief 設定ファイル入出力
///
#include "config.h"
#include <wx/filename.h>
#include <wx/fileconf.h>

Config::Config() : ParseParam()
{
	ini_file = _T("");

	// default value
	mFilePath = _T("");

	SetDefaultFontName();
}

Config::~Config()
{
}

void Config::SetFileName(const wxString &file)
{
	ini_file = file;
}

void Config::Load()
{
	if (ini_file.IsEmpty()) return;

	// load ini file
	wxFileConfig *ini = new wxFileConfig(wxEmptyString,wxEmptyString,ini_file,wxEmptyString
		,wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);

	int v;
	ini->Read(_T("Path"), &mFilePath, mFilePath);
	ini->Read(_T("ReadUntilEof"), &mEofTextRead, mEofTextRead);
	ini->Read(_T("EofBinary"), &mEofBinary, mEofBinary);
	ini->Read(_T("NewLineAscii"), &v, mNewLineAscii); if(0 <= v && v <= 2) mNewLineAscii = v;
	ini->Read(_T("EofAscii"), &mEofAscii, mEofAscii);
	ini->Read(_T("NewLineUtf8"), &v, mNewLineUtf8); if(0 <= v && v <= 2) mNewLineUtf8 = v;
	ini->Read(_T("IncludeBOM"), &mIncludeBOM, mIncludeBOM);
	ini->Read(_T("StartAddr"), &v, mStartAddr); if(0 <= v && v <= 3) mStartAddr = v;
	ini->Read(_T("FontName"), &mFontName, mFontName);
	ini->Read(_T("FontSize"), &v, mFontSize); if(1 <= v && v < 100) mFontSize = v;
	for(int i=0; i<MAX_RECENT_FILES; i++) {
		wxString sval;
		ini->Read(wxString::Format(_T("Recent%d"), i), &sval);
		if (!sval.IsEmpty()) {
			mRecentFiles.Add(sval);
		}
	}
	delete ini;
}

void Config::Load(const wxString &file)
{
	SetFileName(file);
	Load();
}

void Config::Save()
{
	if (ini_file.IsEmpty()) return;

	// save ini file
	wxFileConfig *ini = new wxFileConfig(wxEmptyString,wxEmptyString,ini_file,wxEmptyString
		,wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);
	ini->Write(_T("Path"), mFilePath);
	ini->Write(_T("ReadUntilEof"), mEofTextRead);
	ini->Write(_T("EofBinary"), mEofBinary);
	ini->Write(_T("NewLineAscii"), mNewLineAscii);
	ini->Write(_T("EofAscii"), mEofAscii);
	ini->Write(_T("NewLineUtf8"), mNewLineUtf8);
	ini->Write(_T("IncludeBOM"), mIncludeBOM);
	ini->Write(_T("StartAddr"), mStartAddr);
	ini->Write(_T("FontName"), mFontName);
	ini->Write(_T("FontSize"), mFontSize);
	for(int i=0,row=0; row<MAX_RECENT_FILES && i<(int)mRecentFiles.Count(); i++) {
		wxString sval = mRecentFiles.Item(i);
		if (sval.IsEmpty()) continue;
		ini->Write(wxString::Format(_T("Recent%d"), row), sval);
		row++;
	}
	// write
	delete ini;
}

void Config::SetDefaultFontName()
{
	mFontName = _T("");
	mFontSize = 10;
}

void Config::SetFilePath(const wxString &val)
{
	mFilePath = wxFileName::FileName(val).GetPath(wxPATH_GET_SEPARATOR);
}

void Config::AddRecentFile(const wxString &val)
{
	wxFileName fpath = wxFileName::FileName(val);
	mFilePath = fpath.GetPath(wxPATH_GET_SEPARATOR);
	// 同じファイルがあるか
	int pos = mRecentFiles.Index(fpath.GetFullPath());
	if (pos >= 0) {
		// 消す
		mRecentFiles.RemoveAt(pos);
	}
	// 追加
	mRecentFiles.Insert(fpath.GetFullPath(), 0);
	// 10を超える分は消す
	if (mRecentFiles.Count() > MAX_RECENT_FILES) {
		mRecentFiles.RemoveAt(MAX_RECENT_FILES);
	}
}

wxString &Config::GetRecentFile()
{
	return mRecentFiles[0];
}

void Config::GetRecentFiles(wxArrayString &vals)
{
	vals = mRecentFiles;
}

