/// @file fileinfo.cpp
///
/// @brief ファイル情報
///
#include "fileinfo.h"

//
//
//
PsFileType::PsFileType() {
	flags = 0;
	machine_type = 0;
}
PsFileType::PsFileType(const PsFileType &new_type) {
	flags = new_type.flags;
	machine_type = new_type.machine_type;
	basic_type = new_type.basic_type;
	SetCharType(new_type.char_type);
	internal_name = new_type.internal_name;
}
PsFileType &PsFileType::operator=(const PsFileType &src) {
	flags = src.flags;
	machine_type = src.machine_type;
	basic_type = src.basic_type;
	SetCharType(src.char_type);
	internal_name = src.internal_name;
	return *this;
}
void PsFileType::SetType(const PsFileType &val) {
	*this = val;
}
PsFileType &PsFileType::GetType() {
	return *this;
}
const PsFileType &PsFileType::GetType() const {
	return *this;
}
void PsFileType::SetTypeFlag(int flag_type, bool val) {
	if (val) flags |= flag_type;
	else flags &= ~flag_type;
}
bool PsFileType::GetTypeFlag(int flag_type) {
	return (flags & flag_type) ? true : false;
}
void PsFileType::SetMachineType(int val) {
	machine_type = val;
}
void PsFileType::SetBasicType(const wxString &val, bool extended) {
	basic_type = val;
	SetTypeFlag(psExtendBasic, extended);
//	SetMachineType(basic_type, machine_type);
}
void PsFileType::SetMachineAndBasicType(int n_machine_type, const wxString &n_basic_type, bool n_extended) {
	SetMachineType(n_machine_type);
	SetBasicType(n_basic_type, n_extended);
}
void PsFileType::SetCharType(const wxString &val) {
	char_type = val;
}
void PsFileType::SetInternalName(const BinString &val) {
	internal_name = val.Left(8);
}
void PsFileType::SetInternalName(const wxUint8 *val, size_t size) {
	internal_name.Set(val, size);
}

//
//
//
PsFileInfo::PsFileInfo() {
}
PsFileInfo::PsFileInfo(const PsFileInfo &src) {
	file_name = src.file_name;
}
PsFileInfo &PsFileInfo::operator=(const PsFileInfo &src) {
	file_name = src.file_name;
	return *this;
}
PsFileInfo::~PsFileInfo() {
}
bool PsFileInfo::Close() {
	bool rc = file.Close();
//	if (rc) file_name.Clear();
	return rc;
}
bool PsFileInfo::IsOpened() const {
	return file.IsOpened();
}
bool PsFileInfo::Open(const wxString &filename, wxFile::OpenMode mode, int access) {
	bool rc = file.Open(filename, mode, access);
	if (rc) file_name.Assign(filename);
	return rc;
}
wxFile &PsFileInfo::GetFile() {
	return file;
}
void PsFileInfo::Assign(const wxString &fullpath, wxPathFormat format) {
	file_name.Assign(fullpath, format);
}
wxString PsFileInfo::GetFileFullPath() const {
	return file_name.GetFullPath();
}
wxString PsFileInfo::GetFileName() const {
	return file_name.GetName();
}
bool PsFileInfo::IsSameFile(const wxFileName &dst) const {
	return (file_name == dst);
}

//
//
//
PsFileData::PsFileData() : PsFileType() {
}
PsFileData::PsFileData(const PsFileData &new_type) : PsFileType(new_type) {
	datas = new_type.datas;
}
PsFileData &PsFileData::operator=(const PsFileData &src) {
	PsFileType::operator=(src);
	datas = src.datas;
	return *this; 
}
wxArrayString &PsFileData::GetData() {
	return datas;
}
size_t PsFileData::Add(const wxString &str, size_t copies) {
	return datas.Add(str, copies);
}
size_t PsFileData::Add(const char *str, size_t len) {
	return datas.Add(BinString(str, len));
}
size_t PsFileData::Add(const wxUint8 *str, size_t len) {
	return datas.Add(BinString(str, len));
}
void PsFileData::Empty() {
	datas.Empty();
}
size_t PsFileData::GetCount() const {
	return datas.GetCount();
}
wxString &PsFileData::operator[](size_t nIndex) {
	return datas[nIndex];
}

//
//
//
PsFileInput::PsFileInput()
	: PsFileType(), wxInputStream() {
	start_pos = 0;
}
PsFileInput::PsFileInput(const PsFileType &type) 
	: PsFileType(type), wxInputStream() {
	start_pos = 0;
}
PsFileInput::PsFileInput(const PsFileInput &src)
	: PsFileType(src), wxInputStream() {
	start_pos = src.start_pos;
}
PsFileInput::PsFileInput(const PsFileOutput &src)
	: PsFileType(src), wxInputStream() {
	start_pos = 0;
}
PsFileInput &PsFileInput::operator=(const PsFileInput &src) {
	PsFileType::operator=(src);
	start_pos = src.start_pos;
	return *this; 
}
wxInputStream &PsFileInput::Read(void *buffer, size_t size) {
	return wxInputStream::Read(buffer, size);
}
#if 0
size_t PsFileInput::Read(const wxUint8 *buffer, size_t size) {
	return wxInputStream::Read((void *)buffer, size).LastRead();
}
PsFileInput &PsFileInput::Read(PsFileOutput &src) {
	PsFileType::operator=(src);
	wxInputStream::Read(src);
	return *this; 
}
bool PsFileInput::IsOpened() const {
	return false;
}
wxFileOffset PsFileInput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return SeekI(pos, mode);
}
void PsFileInput::SeekStartPos() {
	SeekI(start_pos);
}
void PsFileInput::SeekStartPos(size_t pos) {
	start_pos = pos;
	SeekI(pos);
}
size_t PsFileInput::OnSysRead(void *, size_t) {
	return 0;
}
#endif

//
//
//
PsFileOutput::PsFileOutput()
	: PsFileType(), wxOutputStream() {
}
PsFileOutput::PsFileOutput(const PsFileOutput &src)
	: PsFileType(src), wxOutputStream() {
}
PsFileOutput &PsFileOutput::operator=(const PsFileOutput &src) {
	PsFileType::operator=(src);
	return *this; 
}
wxOutputStream &PsFileOutput::Write(const void *buffer, size_t size) {
	return wxOutputStream::Write(buffer, size);
}
#if 0
size_t PsFileOutput::Write(const wxUint8 *buffer, size_t size) {
	return wxOutputStream::Write((void *)buffer, size).LastWrite();
}
size_t PsFileOutput::Write(const wxString &str) {
	return wxOutputStream::Write(str.To8BitData(), str.Length()).LastWrite();
}
PsFileOutput &PsFileOutput::Write(PsFileInput &src) {
	PsFileType::operator=(src);
	wxOutputStream::Write(src);
	return *this;
}
bool PsFileOutput::IsOpened() const {
	return false;
}
wxFileOffset PsFileOutput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return SeekO(pos, mode);
}
void PsFileOutput::Clear() {
}
#endif

//
//
//
PsFileStrInput::PsFileStrInput()
	: PsFileInput(), BinStringInputStream() {
}
PsFileStrInput::PsFileStrInput(const wxString &src)
	: PsFileInput(), BinStringInputStream(src) {
}
PsFileStrInput::PsFileStrInput(const PsFileStrInput &src)
	: PsFileInput(src), BinStringInputStream(src) {
}
PsFileStrInput::PsFileStrInput(const PsFileStrOutput &src)
	: PsFileInput(src), BinStringInputStream(src.GetString()) {
}
PsFileStrInput &PsFileStrInput::operator=(const PsFileStrInput &src) {
	PsFileInput::operator=(src);
	BinStringInputStream::operator=(src);
	return *this; 
}
bool PsFileStrInput::Eof() const {
	return BinStringInputStream::Eof();
}
void PsFileStrInput::Clear() {
	BinStringInputStream::Clear();
}
bool PsFileStrInput::IsOpened() const {
	return true;
}
wxFileOffset PsFileStrInput::GetLength() const {
	return BinStringInputStream::GetLength();
}
size_t PsFileStrInput::Read(const wxUint8 *buffer, size_t size) {
	return BinStringInputStream::Read((void *)buffer, size).LastRead();
}
wxInputStream &PsFileStrInput::Read(void *buffer, size_t size) {
	return BinStringInputStream::Read((void *)buffer, size);
}
PsFileStrInput &PsFileStrInput::Read(PsFileOutput &src) {
	PsFileType::operator=(src);
	BinStringInputStream::Read(src);
	start_pos = 0;
	return *this; 
}
wxFileOffset PsFileStrInput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return BinStringInputStream::SeekI(pos, mode);
}
void PsFileStrInput::SeekStartPos() {
	BinStringInputStream::SeekI(start_pos);
}
void PsFileStrInput::SeekStartPos(size_t pos) {
	start_pos = pos;
	BinStringInputStream::SeekI(pos);
}
size_t PsFileStrInput::OnSysRead(void *buffer, size_t size) {
	return BinStringInputStream::OnSysRead(buffer, size);
}

//
//
//
PsFileStrOutput::PsFileStrOutput()
	: PsFileOutput(), BinStringOutputStream() {
}
PsFileStrOutput::PsFileStrOutput(const PsFileStrOutput &src)
	: PsFileOutput(src), BinStringOutputStream(src) {
}
PsFileStrOutput &PsFileStrOutput::operator=(const PsFileStrOutput &src) {
	PsFileOutput::operator=(src);
	BinStringOutputStream::operator=(src);
	return *this; 
}
void PsFileStrOutput::Clear() {
	BinStringOutputStream::Close();
}
bool PsFileStrOutput::IsOpened() const {
	return true;
}
size_t PsFileStrOutput::Write(const wxUint8 *buffer, size_t size) {
	return BinStringOutputStream::Write((const void *)buffer, size).LastWrite();
}
wxOutputStream &PsFileStrOutput::Write(const void *buffer, size_t size) {
	return BinStringOutputStream::Write(buffer, size);
}
size_t PsFileStrOutput::Write(const wxString &str) {
	wxScopedCharBuffer buf = str.To8BitData();
	size_t len = buf.length();
	return BinStringOutputStream::Write(buf, len).LastWrite();
}
size_t PsFileStrOutput::WriteUTF8(const wxString &str) {
	wxScopedCharBuffer buf = str.ToUTF8();
	size_t len = buf.length();
	return BinStringOutputStream::Write(buf, len).LastWrite();
}
PsFileStrOutput &PsFileStrOutput::Write(PsFileInput &src) {
	BinStringOutputStream::Write(src);
	return *this;
}
wxFileOffset PsFileStrOutput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return BinStringOutputStream::SeekO(pos, mode);
}

//
//
//
PsFileFsInput::PsFileFsInput(wxFile &src)
	: PsFileInput(), wxFileInputStream(src) {
}
PsFileFsInput::PsFileFsInput(PsFileInfo &src)
	: PsFileInput(), wxFileInputStream(src.GetFile()) {
}
bool PsFileFsInput::Eof() const {
	return wxFileInputStream::Eof();
}
bool PsFileFsInput::IsOpened() const {
	wxFile *file = wxFileInputStream::GetFile();
	return (file != NULL ? file->IsOpened() : false);
}
size_t PsFileFsInput::Read(const wxUint8 *buffer, size_t size) {
	return wxFileInputStream::Read((void *)buffer, size).LastRead();
}
wxInputStream &PsFileFsInput::Read(void *buffer, size_t size) {
	return wxFileInputStream::Read((void *)buffer, size);
}
PsFileFsInput &PsFileFsInput::Read(PsFileOutput &src) {
	PsFileType::operator=(src);
	wxFileInputStream::Read(src);
	return *this; 
}
wxFileOffset PsFileFsInput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return wxFileInputStream::SeekI(pos, mode);
}
void PsFileFsInput::SeekStartPos() {
	wxFileInputStream::SeekI(start_pos);
}
void PsFileFsInput::SeekStartPos(size_t pos) {
	start_pos = pos;
	wxFileInputStream::SeekI(pos);
}
size_t PsFileFsInput::OnSysRead(void *buffer, size_t size) {
	return wxFileInputStream::OnSysRead(buffer, size);
}

//
//
//
PsFileFsOutput::PsFileFsOutput()
	: PsFileOutput(), wxFileOutputStream() {
}
PsFileFsOutput::PsFileFsOutput(wxFile &src)
	: PsFileOutput(), wxFileOutputStream(src) {
}
void PsFileFsOutput::Clear() {
	wxFileOutputStream::SeekO(0);
}
bool PsFileFsOutput::IsOpened() const {
	wxFile *file = wxFileOutputStream::GetFile();
	return (file != NULL ? file->IsOpened() : false);
}
size_t PsFileFsOutput::Write(const wxUint8 *buffer, size_t size) {
	return wxFileOutputStream::Write((const void *)buffer, size).LastWrite();
}
wxOutputStream &PsFileFsOutput::Write(const void *buffer, size_t size) {
	return wxFileOutputStream::Write(buffer, size);
}
size_t PsFileFsOutput::Write(const wxString &str) {
	wxScopedCharBuffer buf = str.To8BitData();
	size_t len = buf.length();
	return wxFileOutputStream::Write(buf, len).LastWrite();
}
size_t PsFileFsOutput::WriteUTF8(const wxString &str) {
	wxScopedCharBuffer buf = str.ToUTF8();
	size_t len = buf.length();
	return wxFileOutputStream::Write(buf, len).LastWrite();
}
PsFileFsOutput &PsFileFsOutput::Write(PsFileInput &src) {
	wxFileOutputStream::Write(src);
	return *this;
}
wxFileOffset PsFileFsOutput::Seek(wxFileOffset pos, wxSeekMode mode) {
	return wxFileOutputStream::SeekO(pos, mode);
}

//
//
//
PsFileInputInfo::PsFileInputInfo()
	: PsFileInfo(), PsFileStrInput() {
}
PsFileInputInfo &PsFileInputInfo::operator=(const PsFileInputInfo &src) {
	PsFileInfo::operator=(src);
	PsFileStrInput::operator=(src);
	return *this; 
}
PsFileInputInfo::~PsFileInputInfo() {
}
bool PsFileInputInfo::Exist() const {
	return (GetLength() > 0);
}
bool PsFileInputInfo::IsOpened() const {
	return file.IsOpened();
}
//PsFileStrInput *PsFileInputInfo::GetData() {
//	return data;
//}
void PsFileInputInfo::SetData(PsFileStrInput &src) {
	PsFileStrInput::operator=(src);
}
void PsFileInputInfo::ClearData() {
	PsFileStrInput::Clear();
}

//
//
//
PsFileOutputInfo::PsFileOutputInfo()
	: PsFileInfo(), PsFileType() {
}
PsFileOutputInfo::~PsFileOutputInfo() {
}
