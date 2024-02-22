/// @file maptable.h
///
/// @brief マッピングテーブル
///
#ifndef _MAPTABLE_H_
#define _MAPTABLE_H_

#include "common.h"
#include <wx/wx.h>
#include <wx/dynarray.h>

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルItem
class CodeMapItem
{
public:
	enum enMapItemAttrs {
		ATTR_DATA				= 0x0001,
		ATTR_ALLDATA			= 0x0002,
		ATTR_COMMENT			= 0x0004,
		ATTR_COLON				= 0x0008,
		ATTR_LINENUMBER			= 0x0010,
		ATTR_CHARNUMBER			= 0x0020,
		ATTR_CONTLINENUMBER		= 0x0040,
		ATTR_ONELINENUMBER		= 0x0080,
		ATTR_CONTONELINENUMBER	= 0x0100,
		ATTR_INNERSENTENCE		= 0x0200,
		ATTR_NOSTATEMENT		= 0x0400,
		ATTR_CONTSTATEMENT		= 0x0800,

		ATTR_HIGHER				= 0x1000,	// "<"
		ATTR_LOWER				= 0x2000,	// ">"
		ATTR_HEXSTRING			= 0x4000,
		ATTR_OCTSTRING			= 0x8000,
	};
private:
	wxUint8  m_code[4];
	size_t   m_code_length;
	wxString m_str;
	wxString m_str_upper;
	wxUint32 m_attr;
	wxUint32 m_attr2;
	wxUint8 *m_bytes;
	size_t   m_bytes_length;
	int      m_flags;

	wxUint32 ConvAttr(const wxString &attr);
public:
	CodeMapItem();
	CodeMapItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, const wxString &new_attr2 = wxEmptyString, int new_flags = 0);
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
//	/// attrに指定した文字列が含まれるか
//	bool FindAttr(int attr);
//	/// attr2に指定した文字列が含まれるか
//	bool FindAttr2(int attr);

	/// codeを返す
	const wxUint8 *GetCode() const { return m_code; }
	/// codeの長さを返す
	size_t GetCodeLength() const { return m_code_length; }
	/// strを返す
	const wxString &GetStr() const { return m_str; }
	/// bytesを返す
	const wxUint8 *GetBytes() const { return m_bytes; }
	/// bytesの長さを返す
	size_t GetBytesLength() const { return m_bytes_length; }
	/// attrを返す
	wxUint32 GetAttr() const { return m_attr; }
	/// attr2を返す
	wxUint32 GetAttr2() const { return m_attr2; }
	/// flagsを返す
	int GetFlags() const { return m_flags; }
};

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルItemArray
WX_DECLARE_OBJARRAY(CodeMapItem, CodeMapItems);

//////////////////////////////////////////////////////////////////////
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
	void AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, const wxString &new_attr2 = wxEmptyString, int new_flags = 0);
	/// アイテムを探す(code)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByCode(const wxUint8 *code, int attr = -1, bool matching = true);
	/// アイテムを探す(str)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByStr(const wxString &str, bool case_insensitive = false, int attr = -1, bool matching = true);
	/// アイテムを探す(bytes)(前方一致 & 最長一致)
	/// attrを指定した場合、attrが含まれるものを絞り込みこむ。
	/// matching=falseにするとattrが含まれないものを絞り込む。
	CodeMapItem *FindByBytes(const wxUint8 *bytes, int attr = -1, bool matching = true);
	/// セクション名
	const wxString &GetName() const { return name; }
	/// セクション種類番号
	int GetType() const { return type; }
};

//////////////////////////////////////////////////////////////////////
/// マッピングテーブルSectionArray
WX_DECLARE_OBJARRAY(CodeMapSection, CodeMapSections);

//////////////////////////////////////////////////////////////////////
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
	void AddItem(const wxUint8 *new_code, size_t new_code_len, const wxString &new_str, const wxString &new_attr = wxEmptyString, const wxString &new_attr2 = wxEmptyString, int new_flags = 0);
	/// アイテムを探す(code)(前方一致 & 最長一致)
	CodeMapItem *FindByCode(const wxUint8 *code, int attr = -1, bool matching = true);
	/// アイテムを探す(str)(前方一致 & 最長一致)
	CodeMapItem *FindByStr(const wxString &str, bool case_insensitive = false, int attr = -1, bool matching = true);
	/// アイテムを探す(bytes)(前方一致 & 最長一致)
	CodeMapItem *FindByBytes(const wxUint8 *bytes, int attr = -1, bool matching = true);
	/// 全セクションでアイテムを探す(code)(前方一致 & 最長一致)
	CodeMapItem *FindByCodeInAllSections(const wxUint8 *code, int attr = -1, bool matching = true);
	/// 全セクションでアイテムを探す(str)(前方一致 & 最長一致)
	CodeMapItem *FindByStrInAllSections(const wxString &str, bool case_insensitive = false, int attr = -1, bool matching = true);
};

#endif /* _MAPTABLE_H_ */
