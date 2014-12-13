#pragma once
#include <cstdlib>
#include <cstdarg>

extern char *SizeReduce(size_t size);

extern int vasprintf(char **str, const char *fmt, va_list args);
