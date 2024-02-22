/// @file parseparam.h
///
/// @brief パーサー用パラメータ
///
#ifndef PARSEPARAM_H
#define PARSEPARAM_H

#include "basicspecs.h"

enum enNewLineType {
	CR = 0,
	LF = 1,
	CRLF = 2
};

/// パーサー用共通パラメータ
class ParseParam
{
protected:
	bool mEofTextRead;	///< EOFコードで読み込み終了
	bool mEofBinary;	///< EOFコード入れるか(バイナリ)
	bool mEofAscii;		///< EOFコード入れるか(アスキー)
	int mNewLineAscii;	///< 改行コード番号(アスキー)
	int mNewLineUtf8;	///< 改行コード番号(UTF-8)
	bool mIncludeBOM;	///< BOMコード入れるか
	int  mStartAddr;	///< スタートアドレス番号
//	int  mStartAddrCount;	///< スタートアドレスの数

public:
	ParseParam() {
		mEofTextRead = false;
		mEofBinary = false;
		mEofAscii = false;
		mNewLineAscii = CR; //eDefaultNewLineAscii;
		mNewLineUtf8 = LF;
		mIncludeBOM = false;
		mStartAddr = 0; //eDefaultStartAddr;
//		mStartAddrCount = 0;
	}
	ParseParam(int newLineAscii, int startAddr) {
		mEofTextRead = false;
		mEofBinary = false;
		mEofAscii = false;
		mNewLineAscii = newLineAscii;
		mNewLineUtf8 = LF;
		mIncludeBOM = false;
		mStartAddr = startAddr;
//		mStartAddrCount = startAddrCount;
	}
	virtual ~ParseParam() {}

	virtual void SetParam(const ParseParam &src) {
		mEofTextRead = src.mEofTextRead;
		mEofBinary = src.mEofBinary;
		mEofAscii = src.mEofAscii;
		mNewLineAscii = src.mNewLineAscii;
		mNewLineUtf8 = src.mNewLineUtf8;
		mIncludeBOM = src.mIncludeBOM;
		mStartAddr = src.mStartAddr;
//		mStartAddrCount = src.mStartAddrCount;
	}
	virtual void SetEofTextRead(bool val) { mEofTextRead = val; }
	virtual void SetEofBinary(bool val) { mEofBinary = val; }
	virtual void SetEofAscii(bool val) { mEofAscii = val; }
	virtual void SetNewLineAscii(int val) { mNewLineAscii = val; }
	virtual void SetNewLineUtf8(int val) { mNewLineUtf8 = val; }
	virtual void SetIncludeBOM(bool val) { mIncludeBOM = val; }
	virtual void SetStartAddr(int val) { mStartAddr = val; }
//	virtual void SetStartAddrCount(int val) { mStartAddrCount = val; }

	virtual ParseParam &GetParam() { return *this; }
	virtual bool GetEofTextRead() const { return mEofTextRead; }
	virtual bool GetEofBinary() const { return mEofBinary; }
	virtual bool GetEofAscii() const { return mEofAscii; }
	virtual int GetNewLineAscii() const { return mNewLineAscii; }
	virtual int GetNewLineUtf8() const { return mNewLineUtf8; }
	virtual bool GetIncludeBOM() const { return mIncludeBOM; }
	virtual int GetStartAddr() const { return mStartAddr; }
//	virtual int GetStartAddrCount() const { return mStartAddrCount; }
};

#endif /* _PARSEPARAM_H_ */
