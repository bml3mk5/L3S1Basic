/// @file pssymbol.cpp
///
/// @brief 解析文字列格納
///

#include "pssymbol.h"
#include "wx/arrimpl.cpp"

//////////////////////////////////////////////////////////////////////

PsSymbol::PsSymbol()
{
	mType = 0;
}

PsSymbol::~PsSymbol()
{
}

/// クリア
void PsSymbol::Empty()
{
	mType = 0;
	mAscStr.Empty();
	mBinStr.Empty();
}

/// アスキー文字列が空白か
bool PsSymbol::AscStrIsEmpty() const
{
	return mAscStr.IsEmpty();
}

/// バイナリ文字列が空白か
bool PsSymbol::BinStrIsEmpty() const
{
	return mBinStr.IsEmpty();
}

/// セット
void PsSymbol::Set(const BinString &ascStr, const BinString &binStr)
{
	mAscStr = ascStr;
	mBinStr = binStr;
}

void PsSymbol::SetAscStr(const BinString &str)
{
	mAscStr = str;
}

const BinString &PsSymbol::GetAscStr() const
{
	return mAscStr;
}

BinString &PsSymbol::GetSetAscStr()
{
	return mAscStr;
}

size_t PsSymbol::AscStrLen() const
{
	return mAscStr.Len();
}

void PsSymbol::SetBinStr(const BinString &str)
{
	mBinStr = str;
}

const BinString &PsSymbol::GetBinStr() const
{
	return mBinStr;
}

BinString &PsSymbol::GetSetBinStr()
{
	return mBinStr;
}

size_t PsSymbol::BinStrLen() const
{
	return mBinStr.Len();
}

void PsSymbol::Append(const BinString &ascStr, const BinString &binStr)
{
	mAscStr.Append(ascStr);
	mBinStr.Append(binStr);
}

#if 0
void PsSymbol::Append(wxUniCharRef ascCh, wxUniCharRef binCh)
{
	mAscStr.Append(ascCh);
	mBinStr.Append(binCh);
}
#endif

void PsSymbol::Append(const wxUint8 *ascStr, size_t ascLen, const wxUint8 *binStr, size_t binLen)
{
	mAscStr.Append(ascStr, ascLen);
	mBinStr.Append(binStr, binLen);
}

void PsSymbol::Append(wxUint8 ascCh, wxUint8 binCh)
{
	mAscStr.Append(ascCh);
	mBinStr.Append(binCh);
}

void PsSymbol::AppendAscStr(const BinString &str)
{
	mAscStr.Append(str);
}

#if 0
void PsSymbol::AppendAscStr(wxUniCharRef ch)
{
	mAscStr.Append(ch);
}
#endif

void PsSymbol::AppendBinStr(const BinString &str)
{
	mBinStr.Append(str);
}

#if 0
void PsSymbol::AppendBinStr(wxUniCharRef ch)
{
	mBinStr.Append(ch);
}
#endif

/// 形式を指定
void PsSymbol::SetType(wxUint32 type)
{
	mType = type;
}

/// 形式を返す
wxUint32 PsSymbol::GetType() const
{
	return mType;
}

wxUint32 PsSymbol::SetBitType(wxUint32 type)
{
	mType |= type;
	return mType;
}

wxUint32 PsSymbol::ClearBitType(wxUint32 type)
{
	mType &= ~type;
	return mType;
}

//////////////////////////////////////////////////////////////////////
/// 解析文字列のArray
WX_DEFINE_OBJARRAY(PsSymbolArray);

#if 0
void PsSymbolSentence::Add(const PsSymbol &symbol)
{
	PsSymbolArray::Add(symbol);
}

void PsSymbolSentence::Add(PsSymbol::enTypes type, const wxString &str)
{
	PsSymbolArray::Add(new PsSymbol(type, str));
}
#endif

/// 解析文字列を追加 ただし空文字なら追加しない
/// @note Add an item unless is empty.
void PsSymbolSentence::Add(const PsSymbol &item, size_t nInsert)
{
	if (item.AscStrIsEmpty() && item.BinStrIsEmpty()) return;
	PsSymbolArray::Add(item, nInsert);
}

/// 文字列をすべて合わせた文字列を返す
wxString PsSymbolSentence::JoinAscStr() const
{
	wxString str;
	for(size_t i=0; i<Count(); i++) {
		str += Item(i).GetAscStr();
	}
	return str;
}

/// 文字列をすべて合わせた文字列を返す
wxString PsSymbolSentence::JoinBinStr() const
{
	wxString str;
	for(size_t i=0; i<Count(); i++) {
		str += Item(i).GetBinStr();
	}
	return str;
}

/// 文字列をすべて合わせた長さを返す
size_t PsSymbolSentence::BinStrLen() const
{
	size_t len = 0;
	for(size_t i=0; i<Count(); i++) {
		len += Item(i).BinStrLen();
	}
	return len;
}

/// 文字列をすべて合わせた文字列を返す
/// 通常はバイナリ側ただしCHAR_NUMBERの場合はアスキー側
wxString PsSymbolSentence::JoinSelectedStr() const
{
	wxString str;
	for(size_t i=0; i<Count(); i++) {
		if (Item(i).GetType() & PsSymbol::CHAR_NUMBER) {
			str += Item(i).GetAscStr();
		} else {
			str += Item(i).GetBinStr();
		}
	}
	return str;
}

/// 文字列をすべて合わせた長さを返す
/// 通常はバイナリ側ただしCHAR_NUMBERの場合はアスキー側
size_t PsSymbolSentence::SelectedStrLen() const
{
	size_t len = 0;
	for(size_t i=0; i<Count(); i++) {
		if (Item(i).GetType() & PsSymbol::CHAR_NUMBER) {
			len += Item(i).AscStrLen();
		} else {
			len += Item(i).BinStrLen();
		}
	}
	return len;
}

//////////////////////////////////////////////////////////////////////
/// 解析文字列の集合Array
WX_DEFINE_OBJARRAY(PsSymbolSentenceArray);
