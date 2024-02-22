/// @file pssymbol.h
///
/// @brief 解析文字列格納
///
#ifndef PSSYMBOL_H
#define PSSYMBOL_H

#include "common.h"
#include <wx/string.h>
#include <wx/dynarray.h>
#include "bsstring.h"

//////////////////////////////////////////////////////////////////////
/// 解析文字列格納
class PsSymbol
{
public:
	enum enTypes {
		// bit0 - bit7 are same as Parse::enParseAttrs
//		QUOTED_AREA			= 0x0001,
//		DATA_AREA			= 0x0002,
//		ALLDATA_AREA		= 0x0004,
//		COMMENT_AREA		= 0x0008,
//		SENTENCE_AREA		= 0x0010,
//		VARIABLE_AREA		= 0x0020,
//		STATEMENT_AREA		= 0x0040,
		
		HOME_LINE_NUMBER	= 0x0100,
		LINE_NUMBER			= 0x0200,
		CHAR_NUMBER			= 0x0800,
		COLON				= 0x1000,
		HEXSTRING			= 0x4000,
		OCTSTRING			= 0x8000,
	};

protected:
	wxUint32 mType;
	BinString mAscStr;
	BinString mBinStr;

public:
	PsSymbol();
	~PsSymbol();

	void Empty();
	bool AscStrIsEmpty() const;
	bool BinStrIsEmpty() const;

	void Set(const BinString &ascStr, const BinString &binStr);

	void SetAscStr(const BinString &str);
	const BinString &GetAscStr() const;
	BinString &GetSetAscStr();
	size_t AscStrLen() const;
	void SetBinStr(const BinString &str);
	const BinString &GetBinStr() const;
	BinString &GetSetBinStr();
	size_t BinStrLen() const;

	void Append(const BinString &ascStr, const BinString &binStr);
//	void Append(wxUniCharRef ascCh, wxUniCharRef binCh);
	void Append(const wxUint8 *ascStr, size_t ascLen, const wxUint8 *binStr, size_t binLen);
	void Append(wxUint8 ascCh, wxUint8 binCh);

	void AppendAscStr(const BinString &str);
//	void AppendAscStr(wxUniCharRef ch);
	void AppendBinStr(const BinString &str);
//	void AppendBinStr(wxUniCharRef ch);

	void SetType(wxUint32 type);
	wxUint32 GetType() const;
	wxUint32 SetBitType(wxUint32 type);
	wxUint32 ClearBitType(wxUint32 type);
};

//////////////////////////////////////////////////////////////////////
/// 解析文字列のArray
WX_DECLARE_OBJARRAY(PsSymbol, PsSymbolArray);

//////////////////////////////////////////////////////////////////////
/// 解析文字列の集合 文
class PsSymbolSentence : public PsSymbolArray
{
public:
//	void Add(const PsSymbol &symbol);
//	void Add(PsSymbol::enTypes type, const wxString &str);
	void Add(const PsSymbol &item, size_t nInsert = 1);
	wxString JoinAscStr() const;
	wxString JoinBinStr() const;
	size_t BinStrLen() const;
	wxString JoinSelectedStr() const;
	size_t SelectedStrLen() const;
};

//////////////////////////////////////////////////////////////////////
/// 解析文字列の集合Array
WX_DECLARE_OBJARRAY(PsSymbolSentence, PsSymbolSentenceArray);

//////////////////////////////////////////////////////////////////////
/// 解析文字列の集合の行
class PsSymbolChapter : public PsSymbolSentenceArray
{
};

#endif /* PSSYMBOL_H */
