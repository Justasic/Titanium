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
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include "socket.h"

// Global variable on whether or not we're quitting
char quit = 0;
int verbose = 0, port = 2970;
char *pidfile = NULL;

void HandleSignals(int sig)
{
	printf("Received signal %d\n", sig);
	quit = 1;
	// Ignore the signal, we'll quit gracefully.
	signal(sig, SIG_IGN);
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
	signal(SIGPIPE, SIG_IGN);

	// Parse command line arguments so we know what is going where.
	ParseCommandLineArguments(argc, argv);

	// Acquire our UDP socket.
	InitializeSocket();

	// Our main idle loop
	while (!quit)
	{
		// Collect the information and send it!
		SendDataBurst(GetSystemInformation());

		// We will make this adjustable eventually.
		printf("Idling for 15 seconds\n");
		sleep(5);
	}

	// Clean up.
	ShutdownSocket();
	free(pidfile);

	printf("Exiting!\n");

	return EXIT_SUCCESS;
}
