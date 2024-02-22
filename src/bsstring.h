/// @file bsstring.h
///
/// @brief 8bit binary string
///
/// @note based on wxString
///

#ifndef BSSTRING_H
#define BSSTRING_H

#include <wx/wx.h>
#include <wx/string.h>

/// BinString is a convenience accessing class for binary string
class BinString : public wxString
{
public:
	BinString();
	BinString(const wxString& src);
	BinString(const BinString& src);
	BinString(const wxUint8 *src, size_t len);
	BinString(wxUint8 src);
	BinString(const char *src, size_t len);
	BinString(char src);
	void Set(const BinString& src);
	void Set(const wxUint8 *src, size_t len);
	void Set(wxUint8 src);
	wxString &Append(const BinString& src);
	wxString &Append(const wxUint8 *src, size_t len);
	wxString &Append(wxUint8 src);

	wxUint8 At(size_t index) const;
};

#endif // BSSTRING_H

