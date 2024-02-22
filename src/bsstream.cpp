/// @file bsstream.cpp
///
/// @brief 8bit string stream
///
/// @note based on wxStringInputStream / wxStringOutputStream
///

#include "bsstream.h"

// ============================================================================
// BinStringInputStream implementation

BinStringInputStream::BinStringInputStream()
    : m_str(), m_len(0)
{
    m_pos = 0;
}
BinStringInputStream::BinStringInputStream(const wxString& s)
    : m_str(s), m_len(s.length())
{
    m_pos = 0;
}
// copy string but position is not.
BinStringInputStream::BinStringInputStream(const BinStringInputStream& s)
	: m_str(s.m_str), m_len(s.m_len)
{
	m_pos = 0;
}

BinStringInputStream &BinStringInputStream::operator=(const BinStringInputStream &s)
{
	m_str = s.m_str;
	m_len = s.m_len;
	m_pos = 0;
	return *this;
}

void BinStringInputStream::Clear()
{
	m_str.Clear();
	m_len = 0;
	m_pos = 0;
}

wxFileOffset BinStringInputStream::GetLength() const
{
    return m_len;
}

wxFileOffset BinStringInputStream::OnSysSeek(wxFileOffset ofs, wxSeekMode mode)
{
    switch ( mode )
    {
        case wxFromStart:
            // nothing to do, ofs already ok
            break;

        case wxFromEnd:
            ofs += m_len;
            break;

        case wxFromCurrent:
            ofs += m_pos;
            break;

        default:
            wxFAIL_MSG( wxT("invalid seek mode") );
            return wxInvalidOffset;
    }

    if ( ofs < 0 || ofs > static_cast<wxFileOffset>(m_len) )
        return wxInvalidOffset;

	m_pos = ofs;

    return ofs;
}

wxFileOffset BinStringInputStream::OnSysTell() const
{
    return static_cast<wxFileOffset>(m_pos);
}

// ----------------------------------------------------------------------------

size_t BinStringInputStream::OnSysRead(void *buffer, size_t size)
{
    const size_t sizeMax = m_len - m_pos;

    if ( size >= sizeMax )
    {
        if ( sizeMax == 0 )
        {
            m_lasterror = wxSTREAM_EOF;
            return 0;
        }

        size = sizeMax;
    }

    memcpy(buffer, m_str.Mid(m_pos).To8BitData(), size);
    m_pos += size;

    return size;
}

// ============================================================================
// BinStringOutputStream implementation

BinStringOutputStream::BinStringOutputStream()
{
    m_pos = m_str.length();
}
BinStringOutputStream::BinStringOutputStream(const BinStringOutputStream &s)
	: m_str(s.m_str)
{
    m_pos = m_str.length();
}

BinStringOutputStream &BinStringOutputStream::operator=(const BinStringOutputStream &s)
{
	m_str = s.m_str;
    m_pos = m_str.length();
	return *this;
}

bool BinStringOutputStream::Close()
{
	m_str.Clear();
	return true;
}

// ----------------------------------------------------------------------------

wxFileOffset BinStringOutputStream::OnSysTell() const
{
    return static_cast<wxFileOffset>(m_pos);
}

size_t BinStringOutputStream::OnSysWrite(const void *buffer, size_t size)
{
	wxString str = wxString::From8BitData((const char *)buffer, size);

    // no recoding necessary
    m_str += str;

    // update position
    m_pos += size;

    // return number of bytes actually written
    return size;
}

