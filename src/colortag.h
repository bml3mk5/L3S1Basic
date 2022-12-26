/// @file colortag.h
///
/// @brief テキスト色
///

#ifndef MYCOLORTAG_H
#define MYCOLORTAG_H

#include "common.h"
#include <wx/string.h>
#include <wx/colour.h>

/// @brief テキスト色を保持する
typedef struct st_color_tag {
	wxUint8 start;
	wxUint8 red;
	wxUint8 green;
	wxUint8 blue;
} color_tag_t;

/// @brief テキスト色 ID
enum enColorTagIDs {
	COLOR_TAG_COMMENT = 2,
	COLOR_TAG_DATALINE = 3,
	COLOR_TAG_LINENUMBER = 11,
	COLOR_TAG_QUOTED = 16,
	COLOR_TAG_STATEMENT = 21,
	COLOR_TAG_COLON = 24,
	COLOR_TAG_COUNT = 26
};

/// @brief テキスト色にアクセスするクラス
class MyColorTag
{
private:
	static const color_tag_t cColorTags[COLOR_TAG_COUNT];
	color_tag_t mColorTags[COLOR_TAG_COUNT];
public:
	MyColorTag();
	~MyColorTag();

	void Clear();
	void Set(int id, wxUint8 r, wxUint8 g, wxUint8 b);
	void Set(int id, const wxColour &col);
	bool Get(int id, wxUint8 *r, wxUint8 *g, wxUint8 *b) const;
	bool Get(int id, wxColour &col) const;
	bool GetDefault(int id, wxColour &col) const;
	color_tag_t *Get(int id);
	void SetFromHTMLColor(int id, const wxString &val);
	bool GetFromHTMLColor(int id, wxString &val) const;
};

extern MyColorTag gColorTag;

#endif /* MYCOLORTAG_H */
