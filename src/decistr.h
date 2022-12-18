/** @file decistr.h

 @brief 10進文字列

*/

#ifndef _DECISTR_H_
#define _DECISTR_H_

#include <string.h>

#define DECISTR_MAX_BYTES 128

/// 1バイトを1桁とする10進文字列
class DeciStr
{
private:
	char decistr[DECISTR_MAX_BYTES+4];
	int  decipos;
public:
	DeciStr();
	DeciStr(const DeciStr &src);
	DeciStr(const char *src);
	DeciStr(const char *src, int len);
	~DeciStr();

	bool Set(int digit, char val);
	bool Set(const char *vals);
	bool Set(const char *vals, int len);
	bool Padding(int digit, char val);
	bool Shrink(int digit);
	bool Push(char val);
	bool PushInteger(int val);
	int  Length() const;
	const char *GetStr(int digit);
	char Get(int digit) const;

	void LShift(int digit, int size);
	void RShift(int digit, int size);

	void Clear();

	void Round();
	void Round(int digit);

	void Mul2();

	bool IsZero();

	void Trim();

	int Find(char val) const;

	int ToInteger(int digit);

};



#endif
