#define _POSIX_C_SOURCE 201412L // This is the year+month of writing this line.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <errno.h>

typedef struct sysinfo sysinfo_t;
typedef struct utsname utsname_t;

// Entry Point.
int main (int argc, char **argv)
{
	printf("Hello!\n");

	// Get some basic system information right now
	sysinfo_t s;
	if (sysinfo(&s) != 0)
	{
fail:
		fprintf(stderr, "Failed to get system information: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	// Print some basics.
	printf("%ld seconds uptime\n", time(NULL) - s.uptime);
	printf("%lu %lu %lu load times\n", s.loads[0], s.loads[1], s.loads[2]);
	printf("%d processes running\n", s.procs);
// 	printf("")

	// Get hostname and other info
	utsname_t u;
	if (uname(&u) != 0)
		goto fail;

	printf("Hostname: %s\n", u.nodename);
	printf("Architecture: %s\n", u.machine);
	printf("OS: %s %s\n", u.sysname, u.release);

	// Try to get release information
	FILE *f = fopen("/etc/lsb-release", "r");
	if (!f && errno == EEXIST)
	{
		// skip.
	}
	if (!f)
	{
		fprintf(stderr, "Failed to open file /etc/lsb-release: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		// get length of file
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		rewind(f);

		// Read entire file into memory then parse it.
		char *file = malloc(len);
		fread(file, sizeof(char), len, f);
		fclose(f);
		printf("DEBUG: file: \"%s\"\n", file);

		free(file);
	}

	return EXIT_SUCCESS;
}
