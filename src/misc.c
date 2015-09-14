#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "misc.h"

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
	if (fseek(f, 0L, SEEK_END) == -1)
		perror("fseek");
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
	fclose(f);
	return data;
}

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
void *memrev(void *dest, const void *src, size_t n)
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

///////////////////////////////////////////////////
// Function: sgets
//
// description:
// Operates extacly like fgets except on a string
// buffer as input not a FILE pointer.
char *sgets(char *s, size_t sz, const char *input)
{
	if (!input || !s)
		return NULL;

	const char *orig = input;
	do
	{
		if (*input == '\n' || (input - orig) == sz)
			break;
		*s = *input;
		s++;
	} while (*input++);
	return s;
}

///////////////////////////////////////////////////
// Function: FreeSystemInformation
//
// description:
// This function simply deallocates all the allocated
// memory in the information_t structure so we don't
// memleak every time we send information to the server.
void FreeSystemInformation(information_t *info)
{
	// Free the various information
	if (info->Hostname)              free(info->Hostname);
	if (info->cpu_info.Architecture) free(info->cpu_info.Architecture);
	if (info->cpu_info.Model)        free(info->cpu_info.Model);
	if (info->kernel_info.Type)      free(info->kernel_info.Type);
	if (info->kernel_info.Version)   free(info->kernel_info.Version);
	if (info->kernel_info.Release)   free(info->kernel_info.Release);
	if (info->lsb_info.Version)      free(info->lsb_info.Version);
	if (info->lsb_info.Dist_id)      free(info->lsb_info.Dist_id);
	if (info->lsb_info.Release)      free(info->lsb_info.Release);
	if (info->lsb_info.Description)  free(info->lsb_info.Description);

	// Free the hard drives.
	for (hdd_info_t *iter = info->hdd_start, *nextiter; iter; iter = nextiter)
	{
		nextiter = iter->next;
		if (iter->Name) free(iter->Name);
		free(iter);
	}

	for (network_info_t *iter = info->net_start, *nextiter; iter; iter = nextiter)
	{
		nextiter = iter->next;
		if (iter->InterfaceName) free(iter->InterfaceName);
		free(iter);
	}

	// Finally free our struct.
	if (info) free(info);
}
