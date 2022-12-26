/// @file maptable.cpp
///
/// @brief マッピングテーブル
///
#include "maptable.h"
#include "wx/arrimpl.cpp"

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルItem
CodeMapItem::CodeMapItem() {
	memset(m_code, 0, sizeof(m_code));
	m_code_length = 0;
	m_str.Empty();
	m_str_upper.Empty();
	m_attr = 0;
	m_attr2 = 0;
	m_bytes = NULL;
	m_bytes_length = 0;
	m_flags = 0;
}
CodeMapItem::CodeMapItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, const wxString &new_attr2, int new_flags) {
	memset(m_code, 0, sizeof(m_code));
	if (new_code_len > sizeof(m_code)) new_code_len = sizeof(m_code);
	if (new_code && new_code_len > 0) memcpy(m_code, new_code, new_code_len);
	m_code_length = new_code_len;

	m_str = new_str;
	m_str_upper = new_str.Upper();
	m_attr = ConvAttr(new_attr);
	m_attr2 = ConvAttr(new_attr2);

	if (m_str.Length() > 0) {
		wxCharBuffer buf = m_str.mb_str(wxConvUTF8);
		m_bytes_length = strlen(buf);
		m_bytes = new wxUint8[m_bytes_length+1];
		memset((void *)m_bytes, 0, m_bytes_length+1);
		memcpy((void *)m_bytes, (const void *)buf, m_bytes_length);
	} else {
		m_bytes = NULL;
		m_bytes_length = 0;
	}
	m_flags = new_flags;
}
CodeMapItem::~CodeMapItem() {
	delete[] m_bytes;
}
CodeMapItem &CodeMapItem::operator=(const CodeMapItem &item) {
	if (item.m_code && item.m_code_length > 0) memcpy(m_code, item.m_code, sizeof(m_code));
	else memset(m_code, 0, sizeof(m_code));
	m_code_length = item.m_code_length;
	m_str = item.m_str;
	m_str_upper = item.m_str.Upper();
	m_attr = item.m_attr;
	m_attr2 = item.m_attr2;
	// duplicate
	m_bytes_length = strlen((const char *)item.m_bytes);
	if (m_bytes_length > 0) {
		m_bytes = new wxUint8[m_bytes_length+1];
		memset((void *)m_bytes, 0, m_bytes_length+1);
		memcpy((void *)m_bytes, (const void *)item.m_bytes, m_bytes_length);
	} else {
		m_bytes = NULL;
	}
	m_flags = item.m_flags;

	return *this;
}
/// 属性文字列をフラグに変換
wxUint32 CodeMapItem::ConvAttr(const wxString &attr)
{
	const struct {
		const char *str;
		wxUint32 pos;
	} lists[] = {
		{ "contonelinenumber", ATTR_CONTONELINENUMBER },
		{ "contlinenumber", ATTR_CONTLINENUMBER },
		{ "onelinenumber", ATTR_ONELINENUMBER },
		{ "linenumber", ATTR_LINENUMBER },
		{ "hexstring", ATTR_HEXSTRING },
		{ "octstring", ATTR_OCTSTRING },
		{ "comment", ATTR_COMMENT },
		{ "alldata", ATTR_ALLDATA },
		{ "colon", ATTR_COLON },
		{ "data", ATTR_DATA },
		{ "<", ATTR_HIGHER },
		{ ">", ATTR_LOWER },
		{ NULL, 0 }
	};
	wxString nstr = attr;
	wxUint32 val = 0;
	for(int i=0; lists[i].str; i++) {
		if (nstr.Find(lists[i].str) >= 0) {
			val |= lists[i].pos;
			nstr.Replace(lists[i].str, "");
		}
	}
	return val;
}
/// codeが一致するか
/// @return 一致したバイト数
size_t CodeMapItem::CmpCode(const wxUint8 *code_name) {
	size_t match = 0;
	if (m_flags == 1) {
		// SJIS -> UTF-8
		wxCSConv conv(wxFONTENCODING_CP932);
		if (code_name[0] >= 0xa0 && code_name[0] <= 0xdf) {
			// 半角カナ
			wxString nstr((const char *)code_name, conv, 1);
			if (nstr.Length() > 0) {
				m_str = nstr;
				m_code_length = 1;
				match = 1;
			}
		} else if (code_name[0] >= 0x80) {
			// 2byte
			wxString nstr((const char *)code_name, conv, 2);
			if (nstr.Length() > 0) {
				m_str = nstr;
				m_code_length = 2;
				match = 2;
			}
		}

	} else {
		if (memcmp(code_name, m_code, m_code_length) == 0) {
			// match
			match = m_code_length;
		}
	}
	return match;
}
/// codeの一部が一致するか(前方一致)
/// @return 一致したバイト数
size_t CodeMapItem::FindCode(const wxUint8 *code_name) {
	size_t match = 0;
	for(size_t len = m_code_length; len > 0; len--) {
		if (memcmp(code_name, m_code, len) == 0) {
			// match
			match = len;
			break;
		}
	}
	return match;
}
/// strが一致するか
bool CodeMapItem::CmpStr(const wxString &str_name) {
	if (m_str == str_name) return true;
	else return false;
}
/// strに指定した文字列が含まれるか(前方一致)
/// @return 一致した文字数
size_t CodeMapItem::FindStr(const wxString &str_name, bool case_insensitive) {
	bool match = false;
	if (case_insensitive) {
		match = str_name.StartsWith(m_str_upper);
	} else {
		match = str_name.StartsWith(m_str);
	}
	if (match) return m_str.Len();
	else return 0;
}
/// bytesに指定したバイト列が含まれるか(前方一致)
/// @return 一致したバイト数
size_t CodeMapItem::FindBytes(const wxUint8 *bytes_name) {
	if (m_flags == 1) {
		// bytes UTF-8 -> SJIS
		// ASCIIコードではないバイト数
		size_t len = 0;
		for( ;bytes_name[len] >= 0x80; len++) {}
		if (len > 0) {
			wxString nstr((const char *)bytes_name, wxConvUTF8, len);
			wxCSConv conv(wxFONTENCODING_CP932);
			wxCharBuffer cbuf(nstr.mb_str(conv));
			if (cbuf.length() > 0) {
				m_bytes_length = cbuf.length();
				delete [] m_bytes;
				m_bytes = new wxUint8[m_bytes_length + 1];
				memcpy(m_bytes, cbuf, m_bytes_length);
				m_bytes[m_bytes_length]=0;
				m_code_length = len;
				return m_bytes_length;
			}
		}
	} else {
		// bytesが変換できるか
		char *pos = strstr((char *)bytes_name, (const char *)m_bytes);
		if (pos != NULL && pos == (char *)bytes_name) return m_bytes_length;
	}
	return 0;
}
#if 0
/// attrに指定した文字列が含まれるか
/// @param[in] attr enMapItemAttrsの値
bool CodeMapItem::FindAttr(int attr) {
	if (m_attr & (wxUint32)attr) return true;
	else return false;
}
/// attr2に指定した文字列が含まれるか
/// @param[in] attr enMapItemAttrsの値
bool CodeMapItem::FindAttr2(int attr) {
	if (m_attr2 & (wxUint32)attr) return true;
	else return false;
}
#endif

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルItemArray
WX_DEFINE_OBJARRAY(CodeMapItems);

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルSection
CodeMapSection::CodeMapSection(const wxString &new_name, int new_type) {
	name = new_name;
	type = new_type;
	items.Empty();
}
bool CodeMapSection::CmpSection(const wxString &section_name) {
	if (name == section_name) return true;
	else return false;
}
/// アイテムを追加
void CodeMapSection::AddItem(const CodeMapItem *new_item) {
	items.Add(new_item);
}
void CodeMapSection::AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, const wxString &new_attr2, int new_flags) {
	CodeMapItem *new_item = new CodeMapItem(new_code, new_code_len, new_str, new_attr, new_attr2, new_flags);
	AddItem(new_item);
}
/// アイテムを探す(code)(前方一致 & 最長一致)
CodeMapItem *CodeMapSection::FindByCode(const wxUint8 *code, int attr, bool matching) {
	CodeMapItem *item = NULL;
	CodeMapItem *item_max = NULL;
	size_t i;
	size_t len = 0;
	size_t len_max = 0;
	bool find = false;
	for (i = 0; i < items.GetCount(); i++) {
		find = false;
		item = &(items.Item(i));
		len = item->CmpCode(code);
		if (len > len_max) {
			find = true;
			if (attr >= 0) {
				bool match = (item->GetAttr() & (wxUint32)attr) != 0;
				if (match != matching) {
					find = false;
				}
			}
			if (find) {
				len_max = len;
				item_max = item;
			}
		}
	}
	return item_max;
}
/// アイテムを探す(str)(前方一致 & 最長一致)
CodeMapItem *CodeMapSection::FindByStr(const wxString &str, bool case_insensitive, int attr, bool matching) {
	CodeMapItem *item = NULL;
	CodeMapItem *item_max = NULL;
	size_t i;
	size_t len = 0;
	size_t len_max = 0;
	bool find = false;
	for (i = 0; i < items.GetCount(); i++) {
		find = false;
		item = &(items.Item(i));
		len = item->FindStr(str, case_insensitive);
		if (len > len_max) {
			find = true;
			if (attr >= 0) {
				bool match = (item->GetAttr() & (wxUint32)attr) != 0;
				if (match != matching) {
					find = false;
				}
			}
			if (find) {
				len_max = len;
				item_max = item;
			}
		}
	}
	return item_max;
}
/// アイテムを探す(bytes)(前方一致 & 最長一致)
CodeMapItem *CodeMapSection::FindByBytes(const wxUint8 *bytes, int attr, bool matching) {
	CodeMapItem *item = NULL;
	CodeMapItem *item_max = NULL;
	size_t i;
	size_t len = 0;
	size_t len_max = 0;
	bool find = false;
	for (i = 0; i < items.GetCount(); i++) {
		find = false;
		item = &(items.Item(i));
		len = item->FindBytes(bytes);
		if (len > len_max) {
			find = true;
			if (attr >= 0) {
				bool match = (item->GetAttr() & (wxUint32)attr) != 0;
				if (match != matching) {
					find = false;
				}
			}
			if (find) {
				len_max = len;
				item_max = item;
			}
		}
	}
	return item_max;
}

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルSectionArray
WX_DEFINE_OBJARRAY(CodeMapSections);

//////////////////////////////////////////////////////////////////////
/// マッピングテーブル本体
CodeMapTable::CodeMapTable() {
	sections.Empty();
	current_section = NULL;
}
/// セクションを追加
void CodeMapTable::AddSection(const wxString &section_name, int type_number) {
	// 重複を避ける
	if (!FindSection(section_name)) {
		CodeMapSection new_section(section_name, type_number);
		sections.Add(new_section);
		current_section = &(sections.Last());
	}
}
/// セクションを探す
bool CodeMapTable::FindSection(const wxString &section_name) {
	bool find = false;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		if (sections.Item(i).CmpSection(section_name)) {
			find = true;
			break;
		}
	}
	if (find) {
		current_section = &(sections.Item(i));
	}
	return find;
}
/// 種類番号でセクションを探す
bool CodeMapTable::FindSectionByType(int type_number) {
	bool find = false;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		if (sections.Item(i).GetType() == type_number) {
			find = true;
			break;
		}
	}
	if (find) {
		current_section = &(sections.Item(i));
	}
	return find;
}
/// 現在のセクションを返す
CodeMapSection *CodeMapTable::GetCurrentSection() {
	return current_section;
}
/// セクション名を返す
void CodeMapTable::GetAllSectionNames(wxArrayString &section_names) {
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		section_names.Add(sections.Item(i).GetName());
	}
	return;
}
const wxString &CodeMapTable::GetSectionName(size_t index) const {
	return sections.Item(index).GetName();
}

/// アイテムを追加
void CodeMapTable::AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, const wxString &new_attr2, int new_flags) {
	if (current_section != NULL) current_section->AddItem(new_code, new_code_len, new_str, new_attr, new_attr2, new_flags);
}
/// アイテムを探す(code)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByCode(const wxUint8 *code, int attr, bool matching) {
	CodeMapItem *item = NULL;
	if (current_section != NULL) item = current_section->FindByCode(code, attr, matching);
	return item;
}
/// アイテムを探す(str)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByStr(const wxString &str, bool case_insensitive, int attr, bool matching) {
	CodeMapItem *item = NULL;
	wxString n_str;
	if (case_insensitive) n_str = str.Upper(); else n_str = str;
	if (current_section != NULL) item = current_section->FindByStr(n_str, case_insensitive, attr, matching);
	return item;
}
/// アイテムを探す(bytes)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByBytes(const wxUint8 *bytes, int attr, bool matching) {
	CodeMapItem *item = NULL;
	if (current_section != NULL) item = current_section->FindByBytes(bytes, attr, matching);
	return item;
}
/// 全セクションでアイテムを探す(code)
CodeMapItem *CodeMapTable::FindByCodeInAllSections(const wxUint8 *code, int attr, bool matching) {
	CodeMapItem *item = NULL;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		item = sections.Item(i).FindByCode(code, attr, matching);
		if (item != NULL) break;
	}
	return item;
}
/// 全セクションでアイテムを探す(str)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByStrInAllSections(const wxString &str, bool case_insensitive, int attr, bool matching) {
	CodeMapItem *item = NULL;
//	wxString n_str;
//	if (case_insensitive) n_str = str.Upper(); else n_str = str;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		item = sections.Item(i).FindByStr(str, case_insensitive, attr, matching);
		if (item != NULL) break;
	}
	return item;
}
