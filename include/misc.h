#pragma once
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include "tinyformat.h"

#ifndef NDEBUG
# define dprintf(...) tfm::printf(__VA_ARGS__)
# define dfprintf(...) tfm::fprintf(__VA_ARGS__)
#else
# define dprintf(...)
#endif

///////////////////////////////////////////////////
// Function: memrev
//
// description:
// Reverses the byte-order of the source buffer
// to the destination buffer then returns dest.
// Source and dest are the same memory blocks and
// therefore should be treated as the same.
// Useful for network byte-order reversing when
// communicating to other platforms with little-endian
// or big-endian byte-orders.
inline void *memrev(void *dest, const void *src, size_t n)
{
	// Iterators, s is beginning, e is end.
	unsigned char *s = (unsigned char*)dest, *e = ((unsigned char*)dest) + n - 1;

	// Copy to out buffer for our work
	memcpy(dest, src, n);

	// Iterate and reverse copy the bytes
	for (; s < e; ++s, --e)
	{
		unsigned char t = *s;
		*s = *e;
		*e = t;
	}

	// Return provided buffer
	return dest;
}

extern char *SizeReduce(size_t size);
extern std::string GetCurrentDirectory();
extern std::string Duration(time_t t);
