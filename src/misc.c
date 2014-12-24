#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

/*
char *format_time(time_t time, const char *format)
{

	return buffer;
}*/
