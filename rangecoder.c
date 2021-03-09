#include "rangecoder.h"

_Static_assert(sizeof(void *) == 4, "sizeof(void *) ought to be 4");
_Static_assert(sizeof(unsigned long long) == 8, "sizeof(unsigned long long) ought to be 8");

void shift_left(uint64_u *dst, int length)
{
	dst->hi = dst->lo >> (32 - length) | dst->hi << length;
	dst->lo <<= length;
}
