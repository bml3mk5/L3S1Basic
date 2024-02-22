/// @file fileinfo.h
///
/// @brief ファイル情報
///
#ifndef FILEINFO_H
#define FILEINFO_H

#include "common.h"
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dynarray.h>
#include <wx/wfstream.h>
#include "bsstring.h"
#include "bsstream.h"

/// ファイルタイプ
enum enFileTypes {
	psBinary      = 0x00000000,
	psAscii       = 0x00000001,
	psUTF8        = 0x00000002,
	psUTF8BOM     = 0x00000004,
	psExtendBasic = 0x00000008,
	psEncrypted   = 0x00000010,
	psTapeImage   = 0x00000020,
	psDiskImage   = 0x00000040,
};

/// ファイルタイプ情報
class PsFileType
{
protected:
	int			flags;
	int         machine_type;
	wxString	basic_type;
	wxString	char_type;
	BinString	internal_name;
public:
	PsFileType();
	PsFileType(const PsFileType &new_type);
	PsFileType &operator=(const PsFileType &src);
	virtual ~PsFileType() {}

	virtual void SetType(const PsFileType &val);
	virtual PsFileType &GetType();
	virtual const PsFileType &GetType() const;

	virtual void SetTypeFlag(int flag_type, bool val);
	virtual bool GetTypeFlag(int flag_type);
	virtual void SetMachineType(int val);
	virtual int  GetMachineType() { return machine_type; }
	virtual void SetBasicType(const wxString &val, bool extended);
	virtual wxString &GetBasicType() { return basic_type; }
	virtual void SetMachineAndBasicType(int machine_type, const wxString &basic_type, bool extended);
	virtual void SetCharType(const wxString &val);
	virtual wxString &GetCharType() { return char_type; }
	virtual void SetInternalName(const BinString &val);
	virtual void SetInternalName(const wxUint8 *val, size_t size);
	virtual BinString &GetInternalName() { return internal_name; }
};

/// ファイル情報
class PsFileInfo
{
protected:
	wxFile		file;
	wxFileName	file_name;
public:
	PsFileInfo();
	PsFileInfo(const PsFileInfo &src);
	PsFileInfo &operator=(const PsFileInfo &src);
	virtual ~PsFileInfo();

	virtual bool Close();
	virtual bool IsOpened() const;
	virtual bool Open(const wxString &filename, wxFile::OpenMode mode=wxFile::read, int access=wxS_DEFAULT);
	virtual wxFile &GetFile();

	virtual void Assign(const wxString &fullpath, wxPathFormat format=wxPATH_NATIVE);
	virtual wxString GetFileFullPath() const;
	virtual wxString GetFileName() const;

	virtual bool IsSameFile(const wxFileName &dst) const;
};

/// ファイルタイプ＋バッファ
class PsFileData : public PsFileType
{
private:
	wxArrayString datas;
public:
	PsFileData();
	PsFileData(const PsFileData &new_type);
	PsFileData &operator=(const PsFileData &src);
	~PsFileData() {}

	wxArrayString &GetData();

	size_t Add(const wxString &str, size_t copies=1);
	size_t Add(const char *str, size_t len);
	size_t Add(const wxUint8 *str, size_t len);
	void Empty();
	size_t GetCount() const;
	wxString &operator[](size_t nIndex);
};

class PsFileOutput;
/// ファイルタイプ＋入力ストリーム抽象クラス
class PsFileInput : public PsFileType, public wxInputStream
{
protected:
	size_t start_pos;

public:
	PsFileInput();
	PsFileInput(const PsFileType &type);
	PsFileInput(const PsFileInput &src);
	PsFileInput(const PsFileOutput &src);
	PsFileInput &operator=(const PsFileInput &src);
	virtual ~PsFileInput() {}

	virtual size_t Read(const wxUint8 *buffer, size_t size) = 0;
	virtual wxInputStream &Read(void *buffer, size_t size);
	virtual PsFileInput &Read(PsFileOutput &src) = 0;
	virtual bool IsOpened() const = 0;
	virtual wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart) = 0;
	virtual void SeekStartPos() = 0;
	virtual void SeekStartPos(size_t pos) = 0;

protected:
//	virtual size_t OnSysRead(void *, size_t);
};

/// ファイルタイプ＋出力ストリーム抽象クラス
class PsFileOutput : public PsFileType, public wxOutputStream
{
public:
	PsFileOutput();
	PsFileOutput(const PsFileOutput &src);
	PsFileOutput &operator=(const PsFileOutput &src);
	virtual ~PsFileOutput() {}

	virtual size_t Write(const wxUint8 *buffer, size_t size) = 0;
	virtual wxOutputStream &Write(const void *buffer, size_t size);
	virtual size_t Write(const wxString &str) = 0;
	virtual size_t WriteUTF8(const wxString &str) = 0;
	virtual PsFileOutput &Write(PsFileInput &src) = 0;
	virtual bool IsOpened() const = 0;
	virtual wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart) = 0;

	virtual void Clear() = 0;
};

class PsFileStrOutput;
/// ファイルタイプ＋入力文字列ストリーム
class PsFileStrInput : public PsFileInput, public BinStringInputStream
{
public:
	PsFileStrInput();
	PsFileStrInput(const wxString &src);
	PsFileStrInput(const PsFileStrInput &src);
	PsFileStrInput(const PsFileStrOutput &src);
	PsFileStrInput &operator=(const PsFileStrInput &src);
	~PsFileStrInput() {}

	bool Eof() const;
	void Clear();
	bool IsOpened() const;

	wxFileOffset GetLength() const;
	size_t Read(const wxUint8 *buffer, size_t size);
	wxInputStream &Read(void *buffer, size_t size);
	PsFileStrInput &Read(PsFileOutput &src);
	wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart);
	void SeekStartPos();
	void SeekStartPos(size_t pos);

protected:
	size_t OnSysRead(void *buffer, size_t size);
};

/// ファイルタイプ＋出力文字列ストリーム
class PsFileStrOutput : public PsFileOutput, public BinStringOutputStream
{
public:
	PsFileStrOutput();
	PsFileStrOutput(const PsFileStrOutput &src);
	PsFileStrOutput &operator=(const PsFileStrOutput &src);
	~PsFileStrOutput() {}

	void Clear();
	bool IsOpened() const;

	size_t Write(const wxUint8 *buffer, size_t size);
	wxOutputStream &Write(const void *buffer, size_t size);
	size_t Write(const wxString &str);
	size_t WriteUTF8(const wxString &str);
	PsFileStrOutput &Write(PsFileInput &src);
	wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart);
};

/// ファイルタイプ＋入力ファイルストリーム
class PsFileFsInput : public PsFileInput, public wxFileInputStream
{
public:
	PsFileFsInput(wxFile &src);
	PsFileFsInput(PsFileInfo &src);
	~PsFileFsInput() {}

	bool Eof() const;
	bool IsOpened() const;

	size_t Read(const wxUint8 *buffer, size_t size);
	wxInputStream &Read(void *buffer, size_t size);
	PsFileFsInput &Read(PsFileOutput &src);
	wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart);
	void SeekStartPos();
	void SeekStartPos(size_t pos);

protected:
	size_t OnSysRead(void *buffer, size_t size);

private:
	// cannot copy
	PsFileFsInput(const PsFileFsInput &) {}
	PsFileFsInput &operator=(const PsFileFsInput &) { return *this; }
//	PsFileFsInput &Read(PsFileFsOutput &src) { return *this; }
};

/// ファイルタイプ＋出力ファイルストリーム
class PsFileFsOutput : public PsFileOutput, public wxFileOutputStream
{
public:
	PsFileFsOutput();
	PsFileFsOutput(wxFile &src);
	~PsFileFsOutput() {}

	void Clear();
	bool IsOpened() const;

	size_t Write(const wxUint8 *buffer, size_t size);
	wxOutputStream &Write(const void *buffer, size_t size);
	size_t Write(const wxString &str);
	size_t WriteUTF8(const wxString &str);
	PsFileFsOutput &Write(PsFileInput &src);
	wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode=wxFromStart);

private:
	// cannot copy
	PsFileFsOutput(const PsFileStrOutput &) {}
	PsFileFsOutput &operator=(const PsFileFsOutput &) { return *this; }
};


/// 入力ファイル情報
class PsFileInputInfo : public PsFileInfo, public PsFileStrInput
{
private:
	// cannot copy
	PsFileInputInfo(const PsFileInputInfo &) {}
public:
	PsFileInputInfo();
	PsFileInputInfo &operator=(const PsFileInputInfo &);
	~PsFileInputInfo();
	bool Exist() const;
	bool IsOpened() const;
//	PsFileStrInput *GetData();
	void SetData(PsFileStrInput &src);
	void ClearData();
};

/// 出力ファイル情報
class PsFileOutputInfo : public PsFileInfo, public PsFileType
{
private:
	// cannot copy
	PsFileOutputInfo(const PsFileOutputInfo &) {}
	PsFileOutputInfo &operator=(const PsFileOutputInfo &) { return *this; }
public:
	PsFileOutputInfo();
	~PsFileOutputInfo();
};

#endif /* FILEINFO_H */
