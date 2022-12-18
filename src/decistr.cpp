/** @file decistr.cpp

 @brief 10進文字列

*/

#include "decistr.h"
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning( disable : 4819 )
#pragma warning( disable : 4995 )
#pragma warning( disable : 4996 )
#endif
#include <stdio.h>

DeciStr::DeciStr()
{
	memset(decistr, 0, sizeof(decistr));
	decipos = 0;
}

DeciStr::DeciStr(const DeciStr &src)
{
	memcpy(decistr, src.decistr, sizeof(decistr));
	decipos = src.decipos;
}

DeciStr::DeciStr(const char *src)
{
	memset(decistr, 0, sizeof(decistr));
	this->Set(src);
}

DeciStr::DeciStr(const char *src, int len)
{
	memset(decistr, 0, sizeof(decistr));
	this->Set(src, len);
}

DeciStr::~DeciStr()
{
}

/// 指定位置に文字を入れる
bool DeciStr::Set(int digit, char val)
{
	if (digit < 0 || digit >= DECISTR_MAX_BYTES) return false;
	decistr[digit] = val;
	return true;
}

/// 文字列を入れる
bool DeciStr::Set(const char *vals)
{
	int len = (int)strlen(vals);
	return Set(vals, len);
}

/// 文字列を入れる
bool DeciStr::Set(const char *vals, int len)
{
	len = (len <= DECISTR_MAX_BYTES ? len : DECISTR_MAX_BYTES);
	memcpy(decistr, vals, len);
	decipos = len;
	return true;
}

/// 指定桁数文字を入れる
bool DeciStr::Padding(int digit, char val)
{
	if (digit < 0) digit = 0;
	if (digit >= DECISTR_MAX_BYTES) digit = DECISTR_MAX_BYTES;
	for(int pos = 0; pos < digit; pos++) {
		if (decistr[pos] == '\0') {
			decistr[pos] = val;
			decipos = pos;
		}
	}
	return true;
}

/// 指定桁数に削る
bool DeciStr::Shrink(int digit)
{
	if (digit < 0) digit = 0;
	if (digit >= DECISTR_MAX_BYTES) digit = DECISTR_MAX_BYTES;
	for(int pos = digit; pos < DECISTR_MAX_BYTES; pos++) {
		decistr[pos] = '\0';
	}
	decipos = digit;
	return true;
}

/// 最終桁に数値を入れる
bool DeciStr::Push(char val)
{
	if (decipos >= DECISTR_MAX_BYTES) return false;
	decistr[decipos++] = val;
	return true;
}

/// 最終桁に数値を入れる
bool DeciStr::PushInteger(int val)
{
	if (decipos >= DECISTR_MAX_BYTES) return false;
	char str[32];
	sprintf(str, "%c%02d", val >= 0 ? '+' : '-', val >= 0 ? val : -val);
	int len = (int)strlen(str);
	if (decipos+len >= DECISTR_MAX_BYTES) return false;
	strncat(&decistr[decipos], str, len);
	decipos += len;
	return true;
}

/// 長さ
int DeciStr::Length() const
{
	return decipos;
}

/// 指定位置以降の文字列を返す
const char *DeciStr::GetStr(int digit)
{
	return &decistr[digit];
}

/// 指定位置の文字を返す
char DeciStr::Get(int digit) const
{
	return decistr[digit];
}

/// 指定位置から左シフト
void DeciStr::LShift(int digit, int size)
{
	int pos = digit;
	for(; pos<decipos && (pos+size)<DECISTR_MAX_BYTES; pos++) {
		decistr[pos] = decistr[pos+size];
	}
	// 右側は'\0'padding
	for(; pos<decipos; pos++) {
		decistr[pos] = '\0';
	}
	if (decipos > digit) decipos -= size;
	if (decipos < 0) decipos = 0;
}

/// 指定位置から右シフト
void DeciStr::RShift(int digit, int size)
{
	int pos = decipos + size - 1;
	if (pos >= DECISTR_MAX_BYTES) pos = DECISTR_MAX_BYTES - 1;
	for(; pos>=digit && pos>=(digit+size); pos--) {
		decistr[pos] = decistr[pos-size];
	}
	// 左側は0padding
	for(; pos>=digit; pos--) {
		decistr[pos] = '0';
	}
	if (decipos >= digit) decipos += size;
	if (decipos >= DECISTR_MAX_BYTES) decipos = DECISTR_MAX_BYTES;
}

/// クリア
void DeciStr::Clear()
{
	memset(decistr, 0, sizeof(decistr));
	decistr[0]='0';
	decipos=1;
}

/// 最終桁を四捨五入
void DeciStr::Round()
{
	Round(decipos-1);
}

/// 指定桁を四捨五入
void DeciStr::Round(int digit)
{
	char c = (decistr[digit] & 0xf);
	if (c >= 5) {
		for(int i=(digit-1); i>=0; i--) {
			if (decistr[i] == '.') continue;
			decistr[i]++;
			if ((decistr[i] & 0xf) < 10) {
				break;
			}
			decistr[i] &= ~0xf;
		}
	}
	// 四捨五入した位置以下を0にする
	for(int i=digit; i<decipos; i++) {
		if (decistr[i] == '.') continue;
		decistr[i] &= ~0xf;
	}
}

/// 2倍する
void DeciStr::Mul2()
{
	int carry = 0;
	for(int i=(decipos-1); i>=0; i--) {
		int n = (decistr[i] & 0xf);
		n *= 2;
		n += carry;
		if (n > 9) {
			n -= 10;
			carry = 1;
		} else {
			carry = 0;
		}
		decistr[i] = (n | 0x30);
	}
}

/// 0か
bool DeciStr::IsZero()
{
	bool rc = true;
	for(int i=0; i<decipos; i++) {
		if (decistr[i] != '0' && decistr[i] != '.') {
			rc = false;
			break;
		}
	}
	return rc;
}

/// 小数点以下の0を削除
void DeciStr::Trim()
{
	// 0trim
	for(int i=decipos-1; i>=0; i--) {
		if (decistr[i] != '0') break;
		decistr[i] = '\0';
		decipos--;
	}
	if (decipos == 0 || decistr[decipos-1] == '.') {
		Push('0');
	}
}

/// 指定文字のある位置
/// @return >=0 位置 -1 なし
int DeciStr::Find(char val) const
{
	int pos = -1;
	const char *p = strchr(decistr, val);
	if (p != NULL) {
		pos = (int)(p - decistr);
	}
	return pos;
}

/// 指定位置からの数値をint型で返す
int DeciStr::ToInteger(int digit)
{
	int val = 0;
	sscanf(&decistr[digit], "%d", &val);
	return val;

}
