#include <cstdio>
#include <cstdarg>
#include <cstdlib>


// Pointers returns by SizeReduce should be free()'d
char *SizeReduce(size_t size)
{
	static const char *sizes[] = {
		"B",
		"KB",
		"MB",
		"GB",
		"TB",
		"PB",
		"EB",
		"ZB",
		"YB"
	};

	unsigned long sz = 0;
	for (sz = 0; size > 1024; size >>= 10, sz++)
		;

	char *str = NULL;
	asprintf(&str, "%zu %s", size, sz >= (sizeof(sizes) / sizeof(*sizes)) ? "(Hello Future!)" : sizes[sz]);
	return str;
}


int vasprintf(char **str, const char *fmt, va_list args)
{
	int size = 0;
	va_list tmpa;
	// copy
	va_copy(tmpa, args);
	// apply variadic arguments to
	// sprintf with format to get size
	size = vsnprintf(NULL, size, fmt, tmpa);
	// toss args
	va_end(tmpa);
	// return -1 to be compliant if
	// size is less than 0
	if (size < 0)
		return -1;
	// alloc with size plus 1 for `\0'
	*str = (char*)malloc(size + 1);
	// return -1 to be compliant
	// if pointer is `NULL'
	if (!*str)
		return -1;
	// format string with original
	// variadic arguments and set new size
	return vsprintf(*str, fmt, args);
}
