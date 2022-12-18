/// @file parseparam.h
///
/// @brief パーサー用パラメータ
///
#ifndef _PARSEPARAM_H_
#define _PARSEPARAM_H_

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

public:
	ParseParam() {
		mEofTextRead = false;
		mEofBinary = false;
		mEofAscii = false;
		mNewLineAscii = 0;
		mNewLineUtf8 = 1;
		mIncludeBOM = false;
		mStartAddr = 2;
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
	}
	virtual void SetEofTextRead(bool val) { mEofTextRead = val; }
	virtual void SetEofBinary(bool val) { mEofBinary = val; }
	virtual void SetEofAscii(bool val) { mEofAscii = val; }
	virtual void SetNewLineAscii(int val) { mNewLineAscii = val; }
	virtual void SetNewLineUtf8(int val) { mNewLineUtf8 = val; }
	virtual void SetIncludeBOM(bool val) { mIncludeBOM = val; }
	virtual void SetStartAddr(int val) { mStartAddr = val; }

	virtual ParseParam &GetParam() { return *this; }
	virtual bool GetEofTextRead() const { return mEofTextRead; }
	virtual bool GetEofBinary() const { return mEofBinary; }
	virtual bool GetEofAscii() const { return mEofAscii; }
	virtual int GetNewLineAscii() const { return mNewLineAscii; }
	virtual int GetNewLineUtf8() const { return mNewLineUtf8; }
	virtual bool GetIncludeBOM() const { return mIncludeBOM; }
	virtual int GetStartAddr() const { return mStartAddr; }
};

#endif /* _PARSEPARAM_H_ */
