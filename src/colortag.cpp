/// @file colortag.cpp
///
/// @brief テキスト色
///

#include "colortag.h"

//

MyColorTag::MyColorTag()
{
	Clear();
}

MyColorTag::~MyColorTag()
{
}

// 0x41 - 0x5a
const color_tag_t MyColorTag::cColorTags[COLOR_TAG_COUNT] = {
	{ 0xff, 0, 0, 0 },	// a
	{ 0xff, 0, 0, 0 },	// b
	{ 1, 0, 128, 0 },	// comment
	{ 1, 128, 0, 128 },	// data
	{ 0, 0, 0, 0 },		// end
	{ 0xff, 0, 0, 0 },	// f
	{ 0xff, 0, 0, 0 },	// g
	{ 1, 0, 0, 0 },	// hexa/octa
	{ 0xff, 0, 0, 0 },	// i
	{ 0xff, 0, 0, 0 },	// j
	{ 0xff, 0, 0, 0 },	// k
	{ 1, 0, 128, 128 },	// line number
	{ 0xff, 0, 0, 0 },	// m
	{ 0xff, 0, 0, 0 },	// n
	{ 0xff, 0, 0, 0 },	// o
	{ 0xff, 0, 0, 0 },	// p
	{ 1, 192, 0, 0 },	// quote
	{ 0xff, 0, 0, 0 },	// r
	{ 1, 0, 0, 255 },	// statement
	{ 0xff, 0, 0, 0 },	// t
	{ 0xff, 0, 0, 0 },	// u
	{ 1, 128, 128, 0 },	// variable
	{ 0xff, 0, 0, 0 },	// w
	{ 0xff, 0, 0, 0 },	// x
	{ 1, 192, 192, 0 },	// y colon
	{ 0xff, 0, 0, 0 },	// z
};

void MyColorTag::Clear()
{
	for(int i=0; i<COLOR_TAG_COUNT; i++) {
		mColorTags[i] = cColorTags[i];
	}
}

void MyColorTag::Set(int id, wxUint8 r, wxUint8 g, wxUint8 b)
{
	if (id >= 0 && id < COLOR_TAG_COUNT) {
		mColorTags[id].red = r;
		mColorTags[id].green = g;
		mColorTags[id].blue = b;
	}
}

void MyColorTag::Set(int id, const wxColour &col)
{
	if (id >= 0 && id < COLOR_TAG_COUNT) {
		mColorTags[id].red = (wxUint8)col.Red();
		mColorTags[id].green = (wxUint8)col.Green();
		mColorTags[id].blue = (wxUint8)col.Blue();
	}
}

bool MyColorTag::Get(int id, wxUint8 *r, wxUint8 *g, wxUint8 *b) const
{
	if (id >= 0 && id < COLOR_TAG_COUNT && mColorTags[id].start == 1) {
		if (r) *r = mColorTags[id].red;
		if (g) *g = mColorTags[id].green;
		if (b) *b = mColorTags[id].blue;
		return true;
	}
	return false;
}

bool MyColorTag::Get(int id, wxColour &col) const
{
	if (id >= 0 && id < COLOR_TAG_COUNT && mColorTags[id].start == 1) {
		col.Set(mColorTags[id].red, mColorTags[id].green, mColorTags[id].blue);
		return true;
	}
	return false;
}

bool MyColorTag::GetDefault(int id, wxColour &col) const
{
	if (id >= 0 && id < COLOR_TAG_COUNT && mColorTags[id].start == 1) {
		col.Set(cColorTags[id].red, cColorTags[id].green, cColorTags[id].blue);
		return true;
	}
	return false;
}

color_tag_t *MyColorTag::Get(int id)
{
	return &mColorTags[id];
}

void MyColorTag::SetFromHTMLColor(int id, const wxString &val)
{
	if (id < 0 || id >= COLOR_TAG_COUNT) return;

	wxString nstr = val.Mid(1);
	unsigned long lval = 0;
	if (nstr.ToULong(&lval, 16)) {
		mColorTags[id].red = (lval >> 16) & 0xff;
		mColorTags[id].green = (lval >> 8) & 0xff;
		mColorTags[id].blue = (lval >> 0) & 0xff;
	}
}

bool MyColorTag::GetFromHTMLColor(int id, wxString &val) const
{
	if (id < 0 || id >= COLOR_TAG_COUNT || mColorTags[id].start != 1) return false;

	val = wxString::Format(wxT("#%02X%02X%02X")
		, mColorTags[id].red
		, mColorTags[id].green
		, mColorTags[id].blue
	);
	return true;
}
