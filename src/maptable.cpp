/// @file maptable.cpp
///
/// @brief マッピングテーブル
///
#include "maptable.h"
#include "wx/arrimpl.cpp"

/// マッピングテーブルItem
CodeMapItem::CodeMapItem() {
	memset(code, 0, sizeof(code));
	code_length = 0;
	str.Empty();
	str_upper.Empty();
	attr.Empty();
	bytes = NULL;
	bytes_length = 0;
	flags = 0;
}
CodeMapItem::CodeMapItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, int new_flags) {
	memset(code, 0, sizeof(code));
	if (new_code_len > sizeof(code)) new_code_len = sizeof(code);
	if (new_code && new_code_len > 0) memcpy(code, new_code, new_code_len);
	code_length = new_code_len;

	str = new_str;
	str_upper = new_str.Upper();
	attr = new_attr;

	if (str.Length() > 0) {
		wxCharBuffer buf = str.mb_str(wxConvUTF8);
		bytes_length = strlen(buf);
		bytes = new wxUint8[bytes_length+1];
		memset((void *)bytes, 0, bytes_length+1);
		memcpy((void *)bytes, (const void *)buf, bytes_length);
	} else {
		bytes = NULL;
		bytes_length = 0;
	}
	flags = new_flags;
}
CodeMapItem::~CodeMapItem() {
	delete[] bytes;
}
CodeMapItem &CodeMapItem::operator=(const CodeMapItem &item) {
	if (item.code && item.code_length > 0) memcpy(code, item.code, sizeof(code));
	else memset(code, 0, sizeof(code));
	code_length = item.code_length;
	str = item.str;
	str_upper = item.str.Upper();
	attr = item.attr;
	// duplicate
	bytes_length = strlen((const char *)item.bytes);
	if (bytes_length > 0) {
		bytes = new wxUint8[bytes_length+1];
		memset((void *)bytes, 0, bytes_length+1);
		memcpy((void *)bytes, (const void *)item.bytes, bytes_length);
	} else {
		bytes = NULL;
	}
	flags = item.flags;

	return *this;
}
/// codeが一致するか
/// @return 一致したバイト数
size_t CodeMapItem::CmpCode(const wxUint8 *code_name) {
	size_t match = 0;
	if (flags == 1) {
		// SJIS -> UTF-8
		wxCSConv conv(wxFONTENCODING_CP932);
		if (code_name[0] >= 0xa0 && code_name[0] <= 0xdf) {
			// 半角カナ
			wxString nstr((const char *)code_name, conv, 1);
			if (nstr.Length() > 0) {
				str = nstr;
				code_length = 1;
				match = 1;
			}
		} else if (code_name[0] >= 0x80) {
			// 2byte
			wxString nstr((const char *)code_name, conv, 2);
			if (nstr.Length() > 0) {
				str = nstr;
				code_length = 2;
				match = 2;
			}
		}

	} else {
		if (memcmp(code_name, code, code_length) == 0) {
			// match
			match = code_length;
		}
	}
	return match;
}
/// codeの一部が一致するか(前方一致)
/// @return 一致したバイト数
size_t CodeMapItem::FindCode(const wxUint8 *code_name) {
	size_t match = 0;
	for(size_t len = code_length; len > 0; len--) {
		if (memcmp(code_name, code, len) == 0) {
			// match
			match = len;
			break;
		}
	}
	return match;
}
/// strが一致するか
bool CodeMapItem::CmpStr(const wxString &str_name) {
	if (str == str_name) return true;
	else return false;
}
/// strに指定した文字列が含まれるか(前方一致)
/// @return 一致した文字数
size_t CodeMapItem::FindStr(const wxString &str_name, bool case_insensitive) {
	int pos = 0;
	if (case_insensitive) {
		pos = str_name.Find(str_upper);
	} else {
		pos = str_name.Find(str);
	}
	if (pos == 0) return str.Len();
	else return 0;
}
/// bytesに指定したバイト列が含まれるか(前方一致)
/// @return 一致したバイト数
size_t CodeMapItem::FindBytes(const wxUint8 *bytes_name) {
	if (flags == 1) {
		// bytes UTF-8 -> SJIS
		// ASCIIコードではないバイト数
		size_t len = 0;
		for( ;bytes_name[len] >= 0x80; len++) {}
		if (len > 0) {
			wxString nstr((const char *)bytes_name, wxConvUTF8, len);
			wxCSConv conv(wxFONTENCODING_CP932);
			wxCharBuffer cbuf(nstr.mb_str(conv));
			if (cbuf.length() > 0) {
				bytes_length = cbuf.length();
				delete [] bytes;
				bytes = new wxUint8[bytes_length + 1];
				memcpy(bytes, cbuf, bytes_length);
				bytes[bytes_length]=0;
				code_length = len;
				return bytes_length;
			}
		}
	} else {
		// bytesが変換できるか
		char *pos = strstr((char *)bytes_name, (const char *)bytes);
		if (pos != NULL && pos == (char *)bytes_name) return bytes_length;
	}
	return 0;
}
/// attrに指定した文字列が含まれるか
bool CodeMapItem::FindAttr(const wxString &attr_name) {
	if (attr.Find(attr_name) != wxNOT_FOUND) return true;
	else return false;
}

/// マッピングテーブルItemArray
WX_DEFINE_OBJARRAY(CodeMapItems);

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
void CodeMapSection::AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, int new_flags) {
	CodeMapItem *new_item = new CodeMapItem(new_code, new_code_len, new_str, new_attr, new_flags);
	AddItem(new_item);
}
/// アイテムを探す(code)(前方一致 & 最長一致)
CodeMapItem *CodeMapSection::FindByCode(const wxUint8 *code, const wxString &attr, bool matching) {
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
			if (attr.IsEmpty() != true) {
				bool match = item->FindAttr(attr);
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
CodeMapItem *CodeMapSection::FindByStr(const wxString &str, bool case_insensitive, const wxString &attr, bool matching) {
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
			if (attr.IsEmpty() != true) {
				bool match = item->FindAttr(attr);
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
CodeMapItem *CodeMapSection::FindByBytes(const wxUint8 *bytes, const wxString &attr, bool matching) {
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
			if (attr.IsEmpty() != true) {
				bool match = item->FindAttr(attr);
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

/// マッピングテーブルSectionArray
WX_DEFINE_OBJARRAY(CodeMapSections);

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
void CodeMapTable::AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr, int new_flags) {
	if (current_section != NULL) current_section->AddItem(new_code, new_code_len, new_str, new_attr, new_flags);
}
/// アイテムを探す(code)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByCode(const wxUint8 *code, const wxString &attr, bool matching) {
	CodeMapItem *item = NULL;
	if (current_section != NULL) item = current_section->FindByCode(code, attr, matching);
	return item;
}
/// アイテムを探す(str)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByStr(const wxString &str, bool case_insensitive, const wxString &attr, bool matching) {
	CodeMapItem *item = NULL;
	wxString n_str;
	if (case_insensitive) n_str = str.Upper(); else n_str = str;
	if (current_section != NULL) item = current_section->FindByStr(n_str, case_insensitive, attr, matching);
	return item;
}
/// アイテムを探す(bytes)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByBytes(const wxUint8 *bytes, const wxString &attr, bool matching) {
	CodeMapItem *item = NULL;
	if (current_section != NULL) item = current_section->FindByBytes(bytes, attr, matching);
	return item;
}
/// 全セクションでアイテムを探す(code)
CodeMapItem *CodeMapTable::FindByCodeInAllSections(const wxUint8 *code, const wxString &attr, bool matching) {
	CodeMapItem *item = NULL;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		item = sections.Item(i).FindByCode(code, attr, matching);
		if (item != NULL) break;
	}
	return item;
}
/// 全セクションでアイテムを探す(str)(前方一致 & 最長一致)
CodeMapItem *CodeMapTable::FindByStrInAllSections(const wxString &str, bool case_insensitive, const wxString &attr, bool matching) {
	CodeMapItem *item = NULL;
	wxString n_str;
	if (case_insensitive) n_str = str.Upper(); else n_str = str;
	size_t i;
	for (i = 0; i < sections.GetCount(); i++) {
		item = sections.Item(i).FindByStr(str, case_insensitive, attr, matching);
		if (item != NULL) break;
	}
	return item;
}
