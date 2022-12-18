/// @file l3float.h
///
/// @brief レベル3の実数内部表現形式
///
///

#ifndef _L3FLOAT_H_
#define _L3FLOAT_H_

#include "uint192.h"
#include "decistr.h"

/// レベル3の実数内部表現形式を変換
class L3Float
{
private:
	/// 10進を指数表記
	static bool conv_to_deciexpstr(DeciStr &decs, int limit_digit);
public:
	L3Float() {};
	~L3Float() {};

	/// 実数からUINT192形式にする
	static void RealStrToUint192(const unsigned char *vals, int vals_size, UINT192 &cin, UINT192 &cpo);
	/// UINT192形式から実数にする
	static int Uint192ToRealStr(UINT192 &cin, UINT192 &cpo, unsigned char *vals, int vals_size);
	/// 2進→10進文字列に変換
	static void Uint192ToDeciStr(UINT192 &cin, UINT192 &cpo, int limit_digit, DeciStr &decs);
	/// 10進文字列→2進に変換
	static int DeciStrToUint192(const char *decistr, int decistr_len, UINT192 &cin, UINT192 &cpo);
};

#endif
