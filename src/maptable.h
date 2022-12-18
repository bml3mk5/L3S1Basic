/// @file maptable.h
///
/// @brief マッピングテーブル
///
#ifndef _MAPTABLE_H_
#define _MAPTABLE_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>

/// マッピングテーブルItem
class CodeMapItem
{
private:
	wxUint8  code[4];
	size_t   code_length;
	wxString str;
	wxString str_upper;
	wxString attr;
	wxUint8 *bytes;
	size_t   bytes_length;
	int      flags;
public:
	CodeMapItem();
	CodeMapItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, int new_flags = 0);
	~CodeMapItem();
	CodeMapItem &operator=(const CodeMapItem &item);
	/// codeが一致するか
	size_t CmpCode(const wxUint8 *code_name);
	/// codeの一部が一致するか(前方一致)
	size_t FindCode(const wxUint8 *code_name);
	/// strが一致するか
	bool CmpStr(const wxString &str_name);
	/// strに指定した文字列が含まれるか(前方一致)
	size_t FindStr(const wxString &str_name, bool case_insensitive = false);
	/// bytesに指定したバイト列が含まれるか(前方一致)
	size_t FindBytes(const wxUint8 *bytes_name);
	/// attrに指定した文字列が含まれるか
	bool FindAttr(const wxString &attr_name);

	/// codeを返す
	const wxUint8 *GetCode() { return code; }
	/// codeの長さを返す
	size_t GetCodeLength() { return code_length; }
	/// strを返す
	const wxString &GetStr() { return str; }
	/// bytesを返す
	const wxUint8 *GetBytes() { return bytes; }
	/// bytesの長さを返す
	size_t GetBytesLength() { return bytes_length; }
	/// flagsを返す
	int GetFlags() { return flags; }
};

/// マッピングテーブルItemArray
WX_DECLARE_OBJARRAY(CodeMapItem, CodeMapItems);

/// マッピングテーブルSection
class CodeMapSection
{
private:
	wxString     name;
	int          type;
	CodeMapItems items;
public:
	CodeMapSection(const wxString &new_name, int new_type);
	bool CmpSection(const wxString &section_name);
	/// アイテムを追加
	void AddItem(const CodeMapItem *new_item);
	/// アイテムを追加
	void AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, int new_flags = 0);
	/// アイテムを探す(code)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByCode(const wxUint8 *code, const wxString &attr = wxEmptyString, bool matching = true);
	/// アイテムを探す(str)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByStr(const wxString &str, bool case_insensitive = false, const wxString &attr = wxEmptyString, bool matching = true);
	/// アイテムを探す(bytes)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByBytes(const wxUint8 *bytes, const wxString &attr = wxEmptyString, bool matching = true);
	/// セクション名
	const wxString &GetName() const { return name; }
	/// セクション種類番号
	int GetType() const { return type; }
};

/// マッピングテーブルSectionArray
WX_DECLARE_OBJARRAY(CodeMapSection, CodeMapSections);

/// マッピングテーブル本体
class CodeMapTable
{
private:
	CodeMapSections sections;
	CodeMapSection *current_section;
public:
	CodeMapTable();
	/// セクションを追加
	void AddSection(const wxString &section_name, int type_number);
	/// セクションを探す
	bool FindSection(const wxString &section_name);
	bool FindSectionByType(int type_number);
	/// 現在のセクションを返す
	CodeMapSection *GetCurrentSection();
	/// セクション名を返す
	void GetAllSectionNames(wxArrayString &section_names);
	const wxString &GetSectionName(size_t index) const;
	/// アイテムを追加
	void AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, int new_flags = 0);
	/// アイテムを探す(code)(前方一致 & 最長一致)
	CodeMapItem *FindByCode(const wxUint8 *code, const wxString &attr = wxEmptyString, bool matching = true);
	/// アイテムを探す(str)(前方一致 & 最長一致)
	CodeMapItem *FindByStr(const wxString &str, bool case_insensitive = false, const wxString &attr = wxEmptyString, bool matching = true);
	/// アイテムを探す(bytes)(前方一致 & 最長一致)
	CodeMapItem *FindByBytes(const wxUint8 *bytes, const wxString &attr = wxEmptyString, bool matching = true);
	/// 全セクションでアイテムを探す(code)(前方一致 & 最長一致)
	CodeMapItem *FindByCodeInAllSections(const wxUint8 *code, const wxString &attr = wxEmptyString, bool matching = true);
	/// 全セクションでアイテムを探す(str)(前方一致 & 最長一致)
	CodeMapItem *FindByStrInAllSections(const wxString &str, bool case_insensitive = false, const wxString &attr = wxEmptyString, bool matching = true);
};

#endif /* _MAPTABLE_H_ */
