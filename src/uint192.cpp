/** @file uint192.cpp

 @brief 192ビット数値

*/

#include "uint192.h"
#include <string.h>

#define UINT64_SIZE_BITS	64

UINT192::UINT192()
{
	memset(value.u.d, 0, sizeof(t_uint192));
}

UINT192::UINT192(const UINT192 &src)
{
	memcpy(value.u.d, src.value.u.d, sizeof(t_uint192));
}

UINT192::UINT192(unsigned int val)
{
	memset(value.u.d, 0, sizeof(t_uint192));
	value.u.d[0] = val;
}

void UINT192::Set(unsigned int val)
{
	memset(value.u.d, 0, sizeof(t_uint192));
	value.u.d[0] = val;
}

UINT192::UINT192(const unsigned int *vals)
{
	memcpy(value.u.d, vals, sizeof(t_uint192));
}

void UINT192::Set(const unsigned int *vals)
{
	memcpy(value.u.d, vals, sizeof(t_uint192));
}

UINT192::UINT192(const unsigned char *vals, int size, bool bigendian)
{
	memset(value.u.d, 0, sizeof(t_uint192));
	if (bigendian) {
		for(int i=0; i<size && i<UINT192_MAX_BYTE; i++) {
			value.u.b[size-i-1] = vals[i];
		}
	} else {
		for(int i=0; i<size && i<UINT192_MAX_BYTE; i++) {
			value.u.b[i] = vals[i];
		}
	}
}

void UINT192::Set(const unsigned char *vals, int size, bool bigendian)
{
	memset(value.u.d, 0, sizeof(t_uint192));
	if (bigendian) {
		for(int i=0; i<size && i<UINT192_MAX_BYTE; i++) {
			value.u.b[size-i-1] = vals[i];
		}
	} else {
		for(int i=0; i<size && i<UINT192_MAX_BYTE; i++) {
			value.u.b[i] = vals[i];
		}
	}
}

UINT192::~UINT192()
{
}

UINT192 &UINT192::operator=(const UINT192 &src)
{
	memcpy(value.u.d, src.value.u.d, sizeof(t_uint192));
	return *this;
}

UINT192 &UINT192::operator=(unsigned int src)
{
	memset(value.u.d, 0, sizeof(t_uint192));
	value.u.d[0] = src;
	return *this;
}

/// たし算
UINT192 &UINT192::Add(const UINT192 &src)
{
	unsigned int carry = 0;
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		unsigned long long prev = value.u.d[i];
		value.u.d[i] += (src.value.u.d[i] + carry);
		if (prev > value.u.d[i]) {
			// over flow
			carry = 1;
		} else {
			carry = 0;
		}
	}
	return *this;
}

UINT192 &UINT192::operator+=(const UINT192 &src)
{
	return Add(src);
}

UINT192 &UINT192::Add(unsigned int src)
{
	unsigned int carry = 0;
	unsigned long long prev = value.u.d[0];
	value.u.d[0] += src;
	if (prev > value.u.d[0]) {
		// over flow
		carry = 1;
	}
	for(int i=1; i<UINT192_MAX_INT64; i++) {
		prev = value.u.d[i];
		value.u.d[i] += carry;
		if (prev > value.u.d[i]) {
			// over flow
			carry = 1;
		} else {
			carry = 0;
		}
	}
	return *this;
}

/// 引き算
UINT192 &UINT192::Sub(const UINT192 &src)
{
	unsigned int carry = 0;
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		unsigned long long prev = value.u.d[i];
		value.u.d[i] -= (src.value.u.d[i] + carry);
		if (prev < value.u.d[i]) {
			// under flow
			carry = 1;
		} else {
			carry = 0;
		}
	}
	return *this;
}

UINT192 &UINT192::operator-=(const UINT192 &src)
{
	return Sub(src);
}

/// AND
UINT192 &UINT192::And(const UINT192 &src)
{
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		value.u.d[i] &= src.value.u.d[i];
	}
	return *this;
}

/// OR
UINT192 &UINT192::Or(const UINT192 &src)
{
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		value.u.d[i] |= src.value.u.d[i];
	}
	return *this;
}

/// NOT
UINT192 &UINT192::Not()
{
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		value.u.d[i] = ~value.u.d[i];
	}
	return *this;
}

UINT192 &UINT192::Shift(int size)
{
	if (size > 0) {
		return LShift(size);
	} else if (size < 0) {
		return RShift(-size);
	}
	return *this;
}

UINT192 &UINT192::LShift(unsigned int size)
{
	if (size == 0) return *this;

	int s=(size % UINT64_SIZE_BITS);

	t_uint192 tmp;
	memset(tmp.u.d, 0, sizeof(t_uint192));
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		int j=i + (size / UINT64_SIZE_BITS);
		if (0 <= j && j < UINT192_MAX_INT64) {
			tmp.u.d[j] |= (value.u.d[i] << s);
		}
		if (s != 0) {
			j++;
			if (0 <= j && j < UINT192_MAX_INT64) {
				tmp.u.d[j] |= (value.u.d[i] >> (UINT64_SIZE_BITS-s));
			}
		}
	}
	memcpy(value.u.d, tmp.u.d, sizeof(t_uint192));
	return *this;
}

UINT192 &UINT192::RShift(unsigned int size)
{
	if (size == 0) return *this;

	int s=(size % UINT64_SIZE_BITS);

	t_uint192 tmp;
	memset(tmp.u.d, 0, sizeof(t_uint192));
	for(int i=0; i<UINT192_MAX_INT64; i++) {
		int j=i - (size / UINT64_SIZE_BITS);
		if (0 <= j && j < UINT192_MAX_INT64) {
			tmp.u.d[j] |= (value.u.d[i] >> s);
		}
		if (s != 0) {
			j--;
			if (0 <= j && j < UINT192_MAX_INT64) {
				tmp.u.d[j] |= (value.u.d[i] << (UINT64_SIZE_BITS-s));
			}
		}
	}
	memcpy(value.u.d, tmp.u.d, sizeof(t_uint192));
	return *this;
}

/// 掛け算
UINT192 &UINT192::Mul(const UINT192 &src)
{
	UINT192 tmp;
	for(int i=0; i<UINT192_MAX_BITS; i++) {
		int di=(i / UINT64_SIZE_BITS);
		int dd=(i % UINT64_SIZE_BITS);
		if (src.value.u.d[di] & ((unsigned long long)1 << dd)) {
			UINT192 sh(*this);
			sh.LShift(i);
			tmp.Add(sh);
		}
	}
	*this = tmp;
	return *this;
}

UINT192 &UINT192::operator*=(const UINT192 &src)
{
	return Mul(src);
}

/// 10倍
void UINT192::Mul10()
{
	UINT192 tmp;
	UINT192 sh(*this);
	sh.LShift(1);
	tmp.Add(sh);
	sh.LShift(2);
	tmp.Add(sh);
	*this = tmp;
}

bool UINT192::operator==(const UINT192 &src)
{
	return (memcmp(value.u.d, src.value.u.d, sizeof(t_uint192)) == 0);
}

bool UINT192::operator==(unsigned int src)
{
	bool rc = false;
	if (value.u.d[0] == src) {
		rc = true;
		for(int i=1; i<UINT192_MAX_INT64; i++) {
			if (value.u.d[i] != 0) {
				rc = false;
				break;
			}
		}
	}
	return rc;
}

bool UINT192::operator>(const UINT192 &src)
{
	bool rc = false;
	for(int i=(UINT192_MAX_INT64-1); i>=0; i--) {
		if (value.u.d[i] > src.value.u.d[i]) {
			rc = true;
			break;
		} else if (value.u.d[i] < src.value.u.d[i]) {
			rc = false;
			break;
		}
	}
	return rc;
}

bool UINT192::operator<(const UINT192 &src)
{
	bool rc = false;
	for(int i=(UINT192_MAX_INT64-1); i>=0; i--) {
		if (value.u.d[i] < src.value.u.d[i]) {
			rc = true;
			break;
		} else if (value.u.d[i] > src.value.u.d[i]) {
			rc = false;
			break;
		}
	}
	return rc;
}

unsigned char UINT192::GetByte(unsigned int pos)
{
	if (pos < UINT192_MAX_BYTE) {
		return value.u.b[pos];
	} else {
		return value.u.b[0];
	}
}

void UINT192::SetByte(unsigned int pos, unsigned char val)
{
	if (pos < UINT192_MAX_BYTE) {
		value.u.b[pos] = val;
	} else {
		value.u.b[0] = val;
	}
}

unsigned int UINT192::Digits() const
{
	unsigned int d = 0;
	for(int i=(UINT192_MAX_INT64-1); i>=0; i--) {
		if (value.u.d[i] != 0) {
			unsigned long long v = value.u.d[i];
			for(int j=0; j<UINT64_SIZE_BITS; j++) {
				if (v & ((unsigned long long)1 << (UINT64_SIZE_BITS-1))) break;
				v = (v << 1);
				d++;
			}
			break;
		}
		d += UINT64_SIZE_BITS;
	}
	return (UINT192_MAX_BITS-d);
}

/// 特定の位置を四捨五入
void UINT192::RoundBit(unsigned int digit)
{
	UINT192 m(1), c;
	m.LShift(digit);
	c = m;
	c.And(*this);
	if (!(c == 0)) {
		this->Add(c);
	}
	m.Sub(1);
	m.Not();
	this->And(m);
}

/// 特定の位置を切り捨て
void UINT192::RoundDownBit(unsigned int digit)
{
	UINT192 m(1);
	m.LShift(digit);
	m.Sub(1);
	m.Not();
	this->And(m);
}

/// 特定の位置を切り上げ
void UINT192::RoundUpBit(unsigned int digit)
{
	UINT192 m(1);
	m.LShift(digit);
	this->Add(m);
	m.Sub(1);
	m.Not();
	this->And(m);
}
