/// @file config.cpp
///
/// @brief 設定ファイル入出力
///
#include "config.h"
#include <wx/filename.h>
#include "colortag.h"

//////////////////////////////////////////////////////////////////////

const char *Config::cSection[] = {
	"L3S1BASIC",
	"MSXBASIC"
};

//////////////////////////////////////////////////////////////////////

ConfigParam::ConfigParam() : ParseParam()
{
	// default value
#ifdef USE_RECENT_PATH_EACH_MACHINE
	mFilePath = _T("");
#endif

	SetDefaultFontName();

	mAddSpaceAfterColon = false;

	mLoaded = false;
}

ConfigParam::~ConfigParam()
{
}

void ConfigParam::Load(wxFileConfig *ini, const wxString &section_name)
{
	int v;
	wxString s;
	bool b;
	wxString sec = section_name;
	if (!sec.IsEmpty()) sec += _T("/");
#ifdef USE_RECENT_PATH_EACH_MACHINE
	s = GetFilePath(); ini->Read(sec + _T("Path"), &s, s); SetFilePath(s);
#endif
	b = GetEofTextRead(); ini->Read(sec + _T("ReadUntilEof"), &b, b); SetEofTextRead(b);
	b = GetEofBinary(); ini->Read(sec + _T("EofBinary"), &b, b); SetEofBinary(b);
	v = GetNewLineAscii(); ini->Read(sec + _T("NewLineAscii"), &v, v); if(CR <= v && v <= CRLF) SetNewLineAscii(v);
	b = GetEofAscii(); ini->Read(sec + _T("EofAscii"), &b, b); SetEofAscii(b);
	v = GetNewLineUtf8(); ini->Read(sec + _T("NewLineUtf8"), &v, v); if(CR <= v && v <= CRLF) SetNewLineUtf8(v);
	b = GetIncludeBOM(); ini->Read(sec + _T("IncludeBOM"), &b, b); SetIncludeBOM(b);
	v = GetStartAddr(); ini->Read(sec + _T("StartAddr"), &v, v); if(0 <= v && v < 8) SetStartAddr(v);
	s = GetFontName(); ini->Read(sec + _T("FontName"), &s, s); SetFontName(s);
	v = GetFontSize(); ini->Read(sec + _T("FontSize"), &v, v); if(1 <= v && v < 100) SetFontSize(v);
	b = EnableAddSpaceAfterColon(); ini->Read(sec + _T("AddSpaceAfterColon"), &b, b); EnableAddSpaceAfterColon(b);
	for(int i=0; i<COLOR_TAG_COUNT; i++) {
		ini->Read(wxString::Format(sec + _T("Color%d"), i), &s);
		if (!s.IsEmpty() && s.Left(1) == wxT("#")) {
			mColorTag.SetFromHTMLColor(i, s);
		}
	}
	mLoaded = true;
}

void ConfigParam::Save(wxFileConfig *ini, const wxString &section_name)
{
	wxString s;
	wxString sec = section_name;
	if (!sec.IsEmpty()) sec += _T("/");
#ifdef USE_RECENT_PATH_EACH_MACHINE
	ini->Write(sec + _T("Path"), GetFilePath());
#endif
	ini->Write(sec + _T("ReadUntilEof"), GetEofTextRead());
	ini->Write(sec + _T("EofBinary"), GetEofBinary());
	ini->Write(sec + _T("NewLineAscii"), GetNewLineAscii());
	ini->Write(sec + _T("EofAscii"), GetEofAscii());
	ini->Write(sec + _T("NewLineUtf8"), GetNewLineUtf8());
	ini->Write(sec + _T("IncludeBOM"), GetIncludeBOM());
	ini->Write(sec + _T("StartAddr"), GetStartAddr());
	ini->Write(sec + _T("FontName"), GetFontName());
	ini->Write(sec + _T("FontSize"), GetFontSize());
	ini->Write(sec + _T("AddSpaceAfterColon"), EnableAddSpaceAfterColon());
	for(int i=0; i<COLOR_TAG_COUNT; i++) {
		if (mColorTag.GetFromHTMLColor(i, s)) {
			ini->Write(wxString::Format(sec + _T("Color%d"), i), s);
		}
	}
}

void ConfigParam::Delete(wxFileConfig *ini, const wxString &section_name)
{
	wxString s;
	wxString sec = section_name;
	if (!sec.IsEmpty()) sec += _T("/");
	ini->DeleteEntry(sec + _T("Path"), false);
	ini->DeleteEntry(sec + _T("ReadUntilEof"), false);
	ini->DeleteEntry(sec + _T("EofBinary"), false);
	ini->DeleteEntry(sec + _T("NewLineAscii"), false);
	ini->DeleteEntry(sec + _T("EofAscii"), false);
	ini->DeleteEntry(sec + _T("NewLineUtf8"), false);
	ini->DeleteEntry(sec + _T("IncludeBOM"), false);
	ini->DeleteEntry(sec + _T("StartAddr"), false);
	ini->DeleteEntry(sec + _T("FontName"), false);
	ini->DeleteEntry(sec + _T("FontSize"), false);
	ini->DeleteEntry(sec + _T("AddSpaceAfterColon"), false);
	for(int i=0; i<COLOR_TAG_COUNT; i++) {
		if (mColorTag.GetFromHTMLColor(i, s)) {
			ini->DeleteEntry(wxString::Format(sec + _T("Color%d"), i), false);
		}
	}
}

void ConfigParam::SetDefaultFontName()
{
	mFontName = _T("");
	mFontSize = 10;
}

#ifdef USE_RECENT_PATH_EACH_MACHINE
void ConfigParam::SetFilePath(const wxString &val)
{
	mFilePath = wxFileName::FileName(val).GetPath();
}
#endif

//////////////////////////////////////////////////////////////////////

Config::Config()
{
	ini_file = _T("");
	mCurrentMachine = eL3S1Basic;
	// default parameter
	mParam[eL3S1Basic].SetParam(ParseParam(0, 2));
	mParam[eMSXBasic].SetParam(ParseParam(2, 0));
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

	ConfigParam dummy;
	dummy.Load(ini, wxEmptyString);

	for(int n=0; n<eMachineCount; n++) {
		ConfigParam *p = &mParam[n];
		wxString sec(cSection[n]);
		if (!ini->HasGroup(sec)) {
			*p = dummy;
			continue;
		}
		p->Load(ini, sec);
	}
	ini->Read(_T("Language"), &mLanguage);
	wxString s = cSection[mCurrentMachine];
	ini->Read(_T("CurrentMachine"), &s, s);
	for(int i=0; i<eMachineCount; i++) {
		if (s == cSection[i]) {
			mCurrentMachine = (enMachines)i;
			break;
		}
	}
#ifndef USE_RECENT_PATH_EACH_MACHINE
	s = GetRecentFilePath(); ini->Read(_T("Path"), &s, s); SetRecentFilePath(s);
#endif

	s.Empty();
	for(int i=0; i<MAX_RECENT_FILES; i++) {
		ini->Read(wxString::Format(_T("Recent%d"), i), &s);
		if (!s.IsEmpty()) {
			AddRecentFile(s, false, true);
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

	ConfigParam dummy;
	dummy.Delete(ini, wxEmptyString);

	wxString s;
	for(int n=0; n<eMachineCount; n++) {
		ConfigParam *p = &mParam[n];
		wxString sec(cSection[n]);
		p->Save(ini, sec);
	}
	ini->Write(_T("Language"), mLanguage);
	ini->Write(_T("CurrentMachine"), cSection[mCurrentMachine]);
#ifndef USE_RECENT_PATH_EACH_MACHINE
	ini->Write(_T("Path"), GetRecentFilePath());
#endif
	for(int i=0,row=0; row<MAX_RECENT_FILES && i<GetRecentFileCount(); i++) {
		s = GetRecentFile(i);
		if (s.IsEmpty()) continue;
		ini->Write(wxString::Format(_T("Recent%d"), row), s);
		row++;
	}
	// write
	delete ini;
}

ConfigParam &Config::GetCurrentParam()
{
	return mParam[mCurrentMachine];
}

void Config::SetRecentFilePath(const wxString &val)
{
#ifdef USE_RECENT_PATH_EACH_MACHINE
	mParam[mCurrentMachine].SetFilePath(val);
#else
	mRecentFilePath = wxFileName::FileName(val).GetPath();
#endif
}

const wxString &Config::GetRecentFilePath() const
{
#ifdef USE_RECENT_PATH_EACH_MACHINE
	return mParam[mCurrentMachine].GetFilePath();
#else
	return mRecentFilePath;
#endif
}

void Config::AddRecentFile(const wxString &val, bool update_path, bool is_append)
{
	if (update_path) {
		SetRecentFilePath(val);
	}

	wxFileName fpath = wxFileName::FileName(val);
	// 同じファイルがあるか
	int pos = mRecentFiles.Index(fpath.GetFullPath());
	if (pos >= 0) {
		// 消す
		mRecentFiles.RemoveAt(pos);
	}
	// 追加
	if (is_append) {
		// append to the last of the list
		mRecentFiles.Add(fpath.GetFullPath());
	} else {
		// insert into the top of the list
		mRecentFiles.Insert(fpath.GetFullPath(), 0);
	}
	// 10を超える分は消す
	if (mRecentFiles.Count() > MAX_RECENT_FILES) {
		mRecentFiles.RemoveAt(MAX_RECENT_FILES);
	}
}

wxString &Config::GetRecentFile(int idx)
{
	return mRecentFiles[idx];
}

void Config::GetRecentFiles(wxArrayString &vals)
{
	vals = mRecentFiles;
}

int Config::GetRecentFileCount() const
{
	return (int)mRecentFiles.Count();
}

/// インスタンス
Config gConfig;
