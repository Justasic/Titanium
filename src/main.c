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
char *pidfile = NULL, *ipaddress = NULL;

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
				#ifdef _DEBUG
				fprintf(stderr, "OVERVIEW: Titanium statistical information collector (debug version)\n\n");
				#else
				fprintf(stderr, "OVERVIEW: Titanium statistical information collector\n\n");
				#endif
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

#ifdef _DEBUG
void DumpSystemInformation(information_t *info)
{
	printf("================ DUMPING STATS =================\n");
	// General system info.
	printf("General system information:\n");
	printf("Current Time: %ld\n", info->CurrentTime);
	printf("StartTime: %ld\n", info->StartTime);
	printf("Loads: 5 %f, 15 %f, 30 %f\n", info->Loads[0], info->Loads[1], info->Loads[2]);
	printf("SecondsIdle: %f\n", info->SecondsIdle);
	printf("SecondsUptime: %f\n", info->SecondsUptime);
	printf("ProcessCount: %ld\n", info->ProcessCount);
	printf("RunningProcessCount: %ld\n", info->RunningProcessCount);
	printf("Zombies: %ld\n", info->Zombies);
	printf("UserCount: %ld\n", info->UserCount);
	printf("Hostname: %s\n\n", info->Hostname);

	// Processor information.
	printf("Processor information:\n");
	printf("Architecture: %s\n", info->cpu_info.Architecture);
	printf("Model: %s\n", info->cpu_info.Model);
	printf("Cores: %d\n", info->cpu_info.Cores);
	printf("PhysicalCores: %d\n", info->cpu_info.PhysicalCores);
	printf("CurrentSpeed: %f\n", info->cpu_info.CurrentSpeed);
	printf("CPUPercent: %d\n\n", info->cpu_info.CPUPercent);

	// Ram information
	printf("RAM information\n");
	printf("FreeRam: %lu bytes\n", info->memory_info.FreeRam);
	printf("UsedRam: %lu bytes\n", info->memory_info.UsedRam);
	printf("TotalRam: %lu bytes\n", info->memory_info.TotalRam);
	printf("AvalableRam: %lu bytes\n", info->memory_info.AvailRam);
	printf("SwapFree: %lu bytes\n", info->memory_info.SwapFree);
	printf("SwapTotal: %lu bytes\n\n", info->memory_info.SwapTotal);

	// HDD info
	printf("Hard disk information:\n");
	printf("(TODO)\n\n");
	for (hdd_info_t *iter = info->hdd_start; iter; iter = iter->next)
	{
		// TODO
	}

	// Network information
	printf("Network Information:\n");
	for (network_info_t *iter = info->net_start; iter; iter = iter->next)
	{
		printf("InterfaceName: %s\n", iter->InterfaceName);
		printf("IPv6Address: %s\n", iter->IPv6Address);
		printf("IPv4Address: %s\n", iter->IPv4Address);
		printf("MACAddress: %s\n", iter->MACAddress);
		printf("SubnetMask: %s\n", iter->SubnetMask);
		printf("Sent Byte Count: %lu\n", iter->TX);
		printf("Received Byte Count: %lu\n\n", iter->RX);
	}

	// Kernel information
	printf("Kernel Information:\n");
	printf("Type: %s\n", info->kernel_info.Type);
	printf("Version: %s\n", info->kernel_info.Version);
	printf("Release: %s\n", info->kernel_info.Release);
	printf("IsTainted: %d\n\n", info->kernel_info.IsTainted);

	// LSB (Distro) information
	printf("LSB (distro) information:\n");
	printf("Version: %s\n", info->lsb_info.Version);
	printf("Distro ID: %s\n", info->lsb_info.Dist_id);
	printf("Release: %s\n", info->lsb_info.Release);
	printf("Description: %s\n\n", info->lsb_info.Description);

	// ALL done.
	printf("================ END DUMPING STATS =================\n");
}
#endif

// Entry Point.
int main (int argc, char **argv)
{
	signal(SIGINT, HandleSignals);
	signal(SIGTERM, HandleSignals);
	signal(SIGPIPE, SIG_IGN);

	// Parse command line arguments so we know what is going where.
	ParseCommandLineArguments(argc, argv);

	#ifdef _DEBUG
	printf("Debug version\n");
	#endif

	// Acquire our UDP socket.
	InitializeSocket();

	//free(ReadEntireFile(__FILE__, NULL));

	// Our main idle loop
	while (!quit)
	{
		information_t *info = GetSystemInformation();
		#ifdef _DEBUG
		DumpSystemInformation(info);
		#endif
		// Collect the information and send it!
		SendDataBurst(info);

		// Free our data so we dont memleak.
		FreeSystemInformation(info);

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
