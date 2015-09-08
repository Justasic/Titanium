#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


///////////////////////////////////////////////////
// Function: SizeReduce
//
// description:
// Returns the highest human-readable byte size
// possible from the input number.
// NOTE: Pointers returns by SizeReduce should be free()'d
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

///////////////////////////////////////////////////
// Function: ReadEntireFile
//
// description:
// Read an entire file into a memory buffer.
// Useful for single-liners in the procfs.
char *ReadEntireFile(const char *filepath, size_t *size)
{
	FILE *f = fopen(filepath, "r");

	if(!f)
		return NULL;

	// Go to the end of the file
	fseek(f, 0, SEEK_END);
	// Get length, then go to the top
	size_t length = ftell(f);
	rewind(f);

	// Create new memory buffer and read the entire file into the buffer
	char *data = malloc(length);
	size_t sz = fread(data, 1, length, f);

	// Make sure we got all the file we want.
	if (sz != length)
	{
		// If no, deallocate and return null.
		free(data);
		data = NULL;
	}
	else
	{
		// Copy file size data if we want it.
		if (size)
			*size = sz;
	}

	// Clean up and return.
	close(f);
	return data;
}
