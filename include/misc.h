#pragma once
#include <cstdlib>
#include <cstdarg>

#ifndef NDEBUG
# define dprintf(...) printf(__VA_ARGS__)
#else
# define dprintf(...)
#endif

extern char *SizeReduce(size_t size);

extern int vasprintf(char **str, const char *fmt, va_list args);
