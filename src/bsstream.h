/// @file bsstream.h
///
/// @brief 8bit binary string stream
///
/// @note based on wxStringInputStream / wxStringOutputStream
///

#ifndef _BSSTREAM_H_
#define _BSSTREAM_H_

#include <wx/wx.h>
#include <wx/stream.h>

/// BinStringInputStream is a stream reading from the given (fixed size) string
class BinStringInputStream : public wxInputStream
{
public:
    BinStringInputStream();
    BinStringInputStream(const wxString& s);
    BinStringInputStream(const BinStringInputStream& s);

	BinStringInputStream &operator=(const BinStringInputStream &s);

    virtual void Clear();
    virtual wxFileOffset GetLength() const;
    virtual bool IsSeekable() const { return true; }

protected:
    virtual wxFileOffset OnSysSeek(wxFileOffset ofs, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;
    virtual size_t OnSysRead(void *buffer, size_t size);

private:
    // the string that was passed in the ctor
    wxString m_str;

    // length of the buffer we're reading from
    size_t m_len;

    // position in the stream in bytes, *not* in chars
    size_t m_pos;

};

/// BinStringOutputStream writes data to the given string, expanding it as needed
class BinStringOutputStream : public wxOutputStream
{
public:
    BinStringOutputStream();
	BinStringOutputStream(const BinStringOutputStream &s);

	BinStringOutputStream &operator=(const BinStringOutputStream &s);

    // get the string containing current output
    const wxString& GetString() const { return m_str; }

    virtual bool IsSeekable() const { return true; }

	virtual bool Close();

protected:
    virtual wxFileOffset OnSysTell() const;
    virtual size_t OnSysWrite(const void *buffer, size_t size);

private:
    // internal string, not used if caller provided his own string
    wxString m_str;

    // position in the stream in bytes, *not* in chars
    size_t m_pos;
};

#endif // _BSSTREAM_H_

