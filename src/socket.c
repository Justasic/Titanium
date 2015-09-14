#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "socket.h"

int fd = 0;

// Simple type information so we can handle IPv4 and IPv6 easily
union
{
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
		struct sockaddr sa;
} saddr;

void InitializeSocket()
{
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
}

void ShutdownSocket()
{
		close(fd);
}

static void SendDataPacket(void *data, size_t len)
{
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
}

void SendDataBurst(information_t *info)
{
		// Send the BEGINBURST packet

		// Send the data packets.

		// Send ENDBURST packet
}
