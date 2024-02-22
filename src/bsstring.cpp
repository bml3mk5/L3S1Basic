/// @file bsstring.cpp
///
/// @brief 8bit string
///
/// @note based on wxString
///

#include "bsstring.h"

BinString::BinString()
	: wxString()
{
}
BinString::BinString(const wxString& src)
	: wxString(src)
{
}
BinString::BinString(const BinString& src)
	: wxString(src)
{
}
BinString::BinString(const wxUint8 *src, size_t len)
	: wxString(wxString::From8BitData((const char *)src, len))
{
}
BinString::BinString(wxUint8 src)
	: wxString(wxString::From8BitData((const char *)&src, 1))
{
}
BinString::BinString(const char *src, size_t len)
	: wxString(wxString::From8BitData(src, len))
{
}
BinString::BinString(char src)
	: wxString(wxString::From8BitData((const char *)&src, 1))
{
}
void BinString::Set(const BinString& src)
{
	wxString::operator=(src);
}
void BinString::Set(const wxUint8 *src, size_t len)
{
	wxString::operator=(wxString::From8BitData((const char *)src, len));
}
void BinString::Set(wxUint8 src)
{
	wxString::operator=(wxString::From8BitData((const char *)&src, 1));
}
wxString &BinString::Append(const BinString& src)
{
	return wxString::Append(src);
}
wxString &BinString::Append(const wxUint8 *src, size_t len)
{
	return wxString::Append(wxString::From8BitData((const char *)src, len));
}
wxString &BinString::Append(wxUint8 src)
{
	return wxString::Append(wxString::From8BitData((const char *)&src, 1));
}
wxUint8 BinString::At(size_t index) const
{
	return (wxUint8)To8BitData().operator[](index);
}
