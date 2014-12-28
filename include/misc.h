#pragma once
#include <cstdlib>
#include <cstdarg>
#include "tinyformat.h"

#ifndef NDEBUG
# define dprintf(...) tfm::printf(__VA_ARGS__)
# define dfprintf(...) tfm::fprintf(__VA_ARGS__)
#else
# define dprintf(...)
#endif

extern char *SizeReduce(size_t size);
extern std::string GetCurrentDirectory();
extern std::string Duration(time_t t);
