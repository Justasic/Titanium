#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include "tinyformat.h"


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

std::string Duration(time_t t)
{
	/* We first calculate everything */
	time_t years = t / 31536000;
	time_t days = (t / 86400) % 365;
	time_t hours = (t / 3600) % 24;
	time_t minutes = (t / 60) % 60;
	time_t seconds = (t) % 60;
	if (!days && !hours && !minutes)
		return tfm::format("%lu second%c", seconds, (seconds != 1 ? 's' : '\0'));
	else
	{
		bool need_comma = false;
		std::string buffer;
		if (years)
		{
			buffer = tfm::format("%lu year%c", years, (years != 1 ? 's' : '\0'));
			need_comma = true;
		}
		if (days)
		{
			buffer += need_comma ? ", " : "";
			buffer += tfm::format("%lu day%c", days, (days != 1 ? 's' : '\0'));
			need_comma = true;
		}
		if (hours)
		{
			buffer += need_comma ? ", " : "";
			buffer += tfm::format("%lu hour%c", hours, (hours != 1 ? 's' : '\0'));
			need_comma = true;
		}
		if (minutes)
		{
			buffer += need_comma ? ", " : "";
			buffer += tfm::format("%lu minute%c", minutes, (minutes != 1 ? 's' : '\0'));
		}
		return buffer;
	}
}
