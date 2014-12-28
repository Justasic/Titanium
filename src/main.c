#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>

// Some typedefs to make things easier
typedef struct sysinfo sysinfo_t;
typedef struct utsname utsname_t;

// Global variable on whether or not we're quitting
char quit = 0;
int verbose = 0, port = 2970;
char *ipaddress = NULL, *pidfile = NULL;

typedef struct information_s {
	sysinfo_t s;
	utsname_t u;
	time_t timestamp;
} information_t;

void HandleSignals(int sig)
{
	printf("Received signal %d\n", sig);
	quit = 1;
	// Ignore the signal, we'll quit gracefully.
	signal(sig, SIG_IGN);
}

void CollectInformation(information_t *info)
{
	// Get some basic system information right now
	if (sysinfo(&info->s) != 0)
	{
fail:
		fprintf(stderr, "Failed to get system information: %s\n", strerror(errno));
		return;
	}

	// Print some basics.
	printf("%ld seconds uptime\n", time(NULL) - info->s.uptime);
	printf("%lu %lu %lu load times\n", info->s.loads[0], info->s.loads[1], info->s.loads[2]);
	printf("%d processes running\n", info->s.procs);
	// 	printf("")

	// TODO: Memory info and CPU percentage

	// Get hostname and other info
	if (uname(&info->u) != 0)
		goto fail;

	printf("Hostname: %s\n", info->u.nodename);
	printf("Architecture: %s\n", info->u.machine);
	printf("OS: %s %s\n", info->u.sysname, info->u.release);

	info->timestamp = time(NULL);

	// Try to get release information
	FILE *f = fopen("/etc/lsb-release", "r");
	if (!f && errno == EEXIST)
	{
		// skip.
	}
	else if (!f)
	{
		fprintf(stderr, "Failed to open file /etc/lsb-release: %s\n", strerror(errno));
		return;
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
}

// We will send our info over this function. You write all info you must send to
// the data pointer and give the length of the memory block to len. This function
// will write the entirety of the memory block in chunks to the Titanium server
// via 512-byte packets.
void SendInformation(void *data, size_t len)
{
	int fd = 0;
	// Simple type information so we can handle IPv4 and IPv6 easily
	union
	{
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
		struct sockaddr sa;
	} saddr;

	// are we IPv4 or IPv6? TODO: We will deal with hostname resolution later.
	saddr.sa.sa_family = strstr(ipaddress, ":") != NULL ? AF_INET6 : AF_INET;

	// Set the port
	*(saddr.sa.sa_family == AF_INET ? &saddr.in.sin_port : &saddr.in6.sin6_port) = htons(port);

	// Convert the IP address to binary so we can use it.
	switch (inet_pton(saddr.sa.sa_family, ipaddress, (saddr.sa.sa_family == AF_INET ? &saddr.in.sin_addr : &saddr.in6.sin6_addr)))
	{
		case 1: // Success.
			break;
		case 0:
			fprintf(stderr, "Invalid %s address: %s\n",
				saddr.sa.sa_family == AF_INET ? "IPv4" : "IPv6", ipaddress);
		default:
			perror("inet_pton");
			return;
	}

	// Create the UDP socket
	if ((fd = socket(saddr.sa.sa_family, SOCK_DGRAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		return;
	}

	// Send the UDP datagrams.
	const uint8_t *ptr = (uint8_t*)data;
	while (len > 0)
	{
		// Send a 512-byte block
		if (len > 512)
		{
			sendto(fd, ptr, 512, 0, (struct sockaddr *)&saddr.sa, sizeof(saddr.sa));
			ptr += 512;
			len -= 512;
		}
		else    // we're under 512-bytes now, send the remainder.
			len -= sendto(fd, ptr, len, 0, (struct sockaddr *)&saddr.sa, sizeof(saddr.sa));

	}

	// Close the socket.
	close(fd);
}

void WritePIDFile(const char *file)
{
	FILE *f = fopen(file, "w");
	fprintf(f, "%d\n", getpid());
	fflush(f);
	fclose(f);
}

void ParseCommandLineArguments(int argc, char **argv)
{
	static struct option long_options[] =
	{
		/* These options set a flag. */
		{"verbose", no_argument,       &verbose, 1},
		/* These options don’t set a flag.
		 *   We distinguish them by their indices. */

		{"nofork",      no_argument,       0, 'f'},
		{"help",        no_argument,       0, 'h'},
		{"connect",     required_argument, 0, 'c'},
		{"port",        required_argument, 0, 'p'},
		{"pidfile",     required_argument, 0, 0  },
		{0, 0, 0, 0}
	};

	int c = 0, option_index = 0;

	while((c = getopt_long (argc, argv, "fhc:p:", long_options, &option_index)) != -1)
	{

		switch (c)
		{
			case 0:
				if (!strcasecmp(long_options[option_index].name, "verbose"))
					printf("verbose flag here.\n");
				else
				{
					pidfile = strdup(optarg);
					WritePIDFile(optarg);
					printf("PID file: %s\n", optarg);
				}
				break;
			case 'c':
				printf("Sending statistical info to %s\n", optarg);
				ipaddress = strdup(optarg);
				break;
			case 'p':
				printf("Sending statical info to port %s\n", optarg);
				port = atoi(optarg);
				break;
			case 'f':
				printf("No forking requested\n");
				break;
			case 'h':
			case '?': fail:
				/* getopt_long already printed an error message. */
				fprintf(stderr, "OVERVIEW: Titanium statistical information collector\n\n");
				fprintf(stderr, "USAGE: Titanium [options]\n\n");
				fprintf(stderr, "OPTIONS:\n");
				fprintf(stderr, " -f, --nofork               Do not fork to the background\n");
				fprintf(stderr, " -h, --help                 Print this message\n");
				fprintf(stderr, " -c, --connect <ipaddress>  IP address or hostname to send information to\n");
				fprintf(stderr, " -p, --port <port>          Port of the host to send information to\n");
				fprintf(stderr, " --pidfile <location>       PID file location\n");
				fprintf(stderr, " --verbose                  Print verbose debug information to the terminal\n");
				exit(EXIT_FAILURE);
				break;
			default:
				abort();
		}
	}

	if (!ipaddress)
	{
		fprintf(stderr, "--connect requires an IP address or hostname\n");
		goto fail;
	}
}

// Entry Point.
int main (int argc, char **argv)
{
	signal(SIGINT, HandleSignals);
	signal(SIGTERM, HandleSignals);

	// Parse command line arguments so we know what is going where.
	ParseCommandLineArguments(argc, argv);

	if (sizeof(information_t) > 512)
		printf("information_t is greater than 512 bytes! %ld\n", sizeof(information_t));

	// Our main idle loop
	while (!quit)
	{
		information_t info;
		CollectInformation(&info);
		SendInformation(&info, sizeof(information_t));

		// We will make this adjustable eventually.
		printf("Idling for 15 seconds\n");
		sleep(5);
	}

	// Clean up.
	free(ipaddress);
	free(pidfile);

	printf("Exiting!\n");

	return EXIT_SUCCESS;
}
