/// @file l3float.cpp
///
/// @brief レベル3の実数内部表現形式
///
///

#include "l3float.h"

/// 実数からUINT192形式にする
/// @param[in] vals 実数
/// @param[in] vals_size バイト数
/// @param[out] cin 整数部
/// @param[out] cpo 小数部
void L3Float::RealStrToUint192(const unsigned char *vals, int vals_size, UINT192 &cin, UINT192 &cpo)
{
	// 指数部
	int e2 = vals[0];

	// 0のときは"0"
	if (e2 == 0) {
		return;
	}
	e2 = e2 - 129;
	// 小数部 1.xxxx -> 0.1xxxx にする
	e2 = e2 + 1;

	// 小数部 0.1xxxx
	cpo.Set(vals, vals_size, true);
	cpo.SetByte(vals_size-2, cpo.GetByte(vals_size-2) | 0x80);
	// 数値部分を最上位ビットに左シフト
	cpo.LShift(UINT192_MAX_BITS-(vals_size-1)*8);
	if (e2 > 0) {
		// 小数部を削除
		cpo.LShift(e2);
		cpo.RShift(8);
	} else {
		// 小数点を移動
		cpo.RShift(8-e2);
	}

	// 整数部
	cin.Set(vals, vals_size, true);
	if (e2 > 0) {
		cin.SetByte(vals_size-2, cin.GetByte(vals_size-2) | 0x80);
		cin.SetByte(vals_size-1, 0);
		// 小数部を削除
		cin.Shift(e2-(vals_size-1)*8);
	} else {
		// 整数部0
		cin = 0;
	}
}

/// UINT192形式から実数にする
/// @param[in,out] cin 整数部
/// @param[in,out] cpo 小数部
/// @param[out] vals 実数
/// @param[in] vals_size バイト数
/// @return 1:オーバーフロー 2:アンダーフロー
int L3Float::Uint192ToRealStr(UINT192 &cin, UINT192 &cpo, unsigned char *vals, int vals_size)
{
	int ex2 = 0;
	int rc = 0;
	if (!(cin == 0 && cpo == 0)) {
		// 整数部が1になるように小数点を移動する
		unsigned int di = cin.Digits();
		if (di > 1) {
			// 整数部が1より大きい
			ex2 = (di - 1);
			UINT192 c(cin);
			c.LShift(UINT192_MAX_BITS+1-di);
			cpo.RShift(di-1);
			cpo.Add(c);
			cin = 0;
		} else if (di == 0) {
			// 整数部0
			// 小数部
			di = cpo.Digits();
			ex2 = (di-UINT192_MAX_BITS-1);
			cpo.LShift(UINT192_MAX_BITS+1-di);
		}
		cpo.RShift(1);	// 最上位ビットは符号なのでシフト

		// 指数部は129たす
		ex2 += 129;

		if (ex2 > 255) {
			// オーバフロー
			rc = 1;
			ex2 = 255;
		} else if (ex2 < 1) {
			// アンダーフロー
			rc = 2;
			ex2 = 1;
		}
	} else {
		// ゼロ
		ex2 = 0;
	}

	// 四捨五入
	cpo.RoundBit((UINT192_MAX_BYTE-vals_size)*8+7);

	// 内部形式
	vals[0] = (ex2 & 0xff);
	for(int i=1; i<vals_size; i++) {
		vals[i] = cpo.GetByte(UINT192_MAX_BYTE-i);
	}

	return rc;
}

/// 10進を指数表記
bool L3Float::conv_to_deciexpstr(DeciStr &decs, int limit_digit)
{
	int ex = 0;
	int pos = decs.Find('.');
	int len = decs.Length();

	if (pos < 0) ex = len - 1;
	else         ex = pos - 1;

	if (len + (pos < 0 ? 0 : -1) <= limit_digit) {
		// 指数表記しない
		if (decs.Length() == 0) {
			decs.Set("0");
		}
		if (limit_digit > 6) {
			// 倍精度の場合
			decs.Push('#');
		} else {
			// 単精度の場合
			if (pos < 0) {
				// 小数点なしなら整数型と区別するため符号追加
				decs.Push('!');
			}
		}
		return false;
	}

	// 0でない桁
	for(int i=0; i<len; i++) {
		if (decs.Get(i) == '.') continue;
		if (decs.Get(i) != '0') break;
		ex--;
	}

	int digit=0;
	DeciStr decexs;
	for(int i=0; i<len; i++) {
		if (decs.Get(i) == '.') continue;
		if (decs.Get(i) != '0' || digit > 0) digit++;
		if (digit > 0) {
			decexs.Push(decs.Get(i));
			if (digit==1) decexs.Push('.');
		}
	}
	// 0trim
	decexs.Trim();
	char exchr = 'E';
	if (limit_digit > 6) exchr = 'D';

	decexs.Push(exchr);
	decexs.PushInteger(ex);

	decs = decexs;

	return true;
}

/// 2進→10進文字列に変換
/// @param[in,out] cin 整数部
/// @param[in,out] cpo 小数部
/// @param[in] limit_digit 有効桁数
/// @param[out] decs 10進文字列
void L3Float::Uint192ToDeciStr(UINT192 &cin, UINT192 &cpo, int limit_digit, DeciStr &decs)
{
	char c1 = 0;
	bool over = true;
	int limit = 0;
//	int decs_pos = 0;

	// 整数部を10進表示
	if (!(cin == 0)) {
		for(int k=38; k>=0; k--) {
			c1=0;
			if (limit <= limit_digit) {
				UINT192 de(1);
				for(int kk=0; kk<k; kk++) {
					de.Mul10();
				}
				for(; c1<10; c1++) {
					if (de > cin) {
						break;
					}
					cin.Sub(de);
				}
			}
			if (c1 != 0) over = false;
			if (!over) {
				decs.Push(c1 | 0x30);
				limit++;
				if (limit == (limit_digit + 1)) {
					decs.Round();
				}
			}
		}
	}

	// 小数部を10進表示
	if (!(cpo == 0)) {
		UINT192 msk;
		msk.Sub(1);
		msk.RShift(8);

		decs.Push('.');
		over = true;
		for(int i=0; i<64; i++) {
			cpo.Mul10();
			c1 = cpo.GetByte(UINT192_MAX_BYTE-1);
			cpo.And(msk);
			decs.Push(c1 | 0x30);
			if (c1 != 0) over = false;
			if (!over) {
				limit++;
				if (limit == (limit_digit + 1)) {
					// 四捨五入して終り
					decs.Round();
					break;
				}
			}
			if (cpo == 0) break;
		}
		decs.Trim();
	}
	
	conv_to_deciexpstr(decs, limit_digit);
}

/// 10進文字列→2進に変換
/// @param[in] decistr 10進文字列
/// @param[in] decistr_len 10進文字列の長さ
/// @param[out] cin 整数部
/// @param[out] cpo 小数部
/// @return 1:単精度 2:倍精度
int L3Float::DeciStrToUint192(const char *decistr, int decistr_len, UINT192 &cin, UINT192 &cpo)
{
	int pos = -1;
	int ex = 0;
	int type = 0;	// 1:単精度 2:倍精度
	DeciStr decs(decistr, decistr_len);

	/// 指数表記か
	pos = decs.Find('E');
	if (pos >= 0) {
		// 単精度
		type = 1;
		ex = decs.ToInteger(pos+1);
		decs.Shrink(pos);
	}
	pos = decs.Find('D');
	if (pos >= 0) {
		// 倍精度
		type = 2;
		ex = decs.ToInteger(pos+1);
		decs.Shrink(pos);
	}
	// 実数表記
	pos = decs.Find('#');
	if (pos >= 0) {
		// 倍精度
		type = 2;
		decs.Shrink(pos);
	} 
	pos = decs.Find('!');
	if (pos >= 0) {
		// 単精度
		type = 1;
		decs.Shrink(pos);
	}

	// 小数点
	int sft = 0;
	pos = decs.Find('.');
	if (pos < 0) {
		pos = decs.Length();
	}
	// 小数点消す
	decs.LShift(pos, 1);

	// 桁数で決める
	if (type == 0) {
		if (decs.Length() > 6) {
			type = 2;
		} else {
			type = 1;
		}
	}

	// 数値０
	if (decs.IsZero()) {
		return type;
	}

	DeciStr dpo(decs);
	DeciStr din(decs);

	// 指数の数だけ足す
	sft = pos + ex;	
	// 小数点以下をシフト
	if (sft > 0) {
		dpo.LShift(0, sft-1);
	} else {
		dpo.RShift(0, 1-sft);
	}
	// 0バイト目に'.'があるのでこれを消す
	dpo.Set(0, '0');

	// 整数部
	sft = pos + ex;	
	if (sft < 0) {
		// 0
		din.Clear();
	} else {
		din.Padding(sft, '0');
		din.Shrink(sft);
	}

	// 小数点以下を2進数にする(192ビット)
	if (!dpo.IsZero()) {
		for(int i=0; i<192; i++) {
			cpo.LShift(1);
			dpo.Mul2();
			if (dpo.Get(0) == '1') {
				cpo.Add(1);
			}
			dpo.Set(0,'0');
		}
		cpo.LShift(UINT192_MAX_BITS-192);
	}

	// 整数部を2進数にする(40ケタ)
	if (!din.IsZero()) {
		int d_digits = 40;
		din.RShift(0, d_digits - din.Length());
		for(int i=0; i<d_digits; i++) {
			if (din.Get(i) != '0') {
				UINT192 c((unsigned int)(din.Get(i) & 0xf));
				for(int j=0; j<(d_digits-1-i); j++) {
					c.Mul10();
				}
				cin.Add(c);
			}
		}
	}
	return type;
}
