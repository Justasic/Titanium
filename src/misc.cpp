#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <unistd.h>


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

std::string GetCurrentDirectory()
{
	// The posix 2001 standard states that getcwd will call malloc() to
	// allocate a string, this is great because I hate static buffers.
	char *cwd = getcwd(NULL, 0);
	std::string str = cwd;
	str += "/";
	free(cwd);

	return str;
}
