/** @file uint192.h

 @brief 192ビット数値

*/

#include <stdio.h>

#ifndef _UINT192_H_
#define _UINT192_H_

#define UINT192_MAX_INT64 3
#define UINT192_MAX_BYTE  24
#define UINT192_MAX_BITS  192

/// 192ビットunsigned int
class UINT192
{
private:
	typedef struct st_uint192 {
		union un_uint192 {
			unsigned long long d[UINT192_MAX_INT64];
			unsigned char      b[UINT192_MAX_BYTE];
		} u;
	} t_uint192;

	t_uint192 value;
public:
	UINT192();
	UINT192(const UINT192 &src);
	UINT192(unsigned int val);
	UINT192(const unsigned int *vals);
	UINT192(const unsigned char *vals, int size, bool bigendian = false);
	~UINT192();

	void Set(unsigned int val);
	void Set(const unsigned int *vals);
	void Set(const unsigned char *vals, int size, bool bigendian = false);

	UINT192 &operator=(const UINT192 &src);
	UINT192 &operator=(unsigned int src);

	UINT192 &Add(const UINT192 &src);
	UINT192 &operator+=(const UINT192 &src);
	UINT192 &Add(unsigned int src);

	UINT192 &Sub(const UINT192 &src);
	UINT192 &operator-=(const UINT192 &src);

	UINT192 &And(const UINT192 &src);
	UINT192 &Or(const UINT192 &src);
	UINT192 &Not();

	UINT192 &Shift(int size);
	UINT192 &LShift(unsigned int size);
	UINT192 &RShift(unsigned int size);

	UINT192 &Mul(const UINT192 &src);
	UINT192 &Mul(unsigned char src);
	UINT192 &operator*=(const UINT192 &src);

	void Mul10();

	bool operator==(const UINT192 &src);
	bool operator==(unsigned int src);
	bool operator>(const UINT192 &src);
	bool operator<(const UINT192 &src);

	unsigned char GetByte(unsigned int pos);
	void SetByte(unsigned int pos, unsigned char val);

	unsigned int Digits() const;

	void RoundBit(unsigned int digit);
	void RoundDownBit(unsigned int digit);
	void RoundUpBit(unsigned int digit);
};






#endif /* _UINT192_H_ */
