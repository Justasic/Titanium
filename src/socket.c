#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "socket.h"
#include "serialize.h"

int fd = 0;
uint32_t timeout = 60;
extern int port;
extern char *ipaddress;

#define MIN(a,b) (((a)<(b))?(a):(b))

// Simple type information so we can handle IPv4 and IPv6 easily
union
{
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
		struct sockaddr sa;
} saddr;

void InitializeSocket(void)
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

void ShutdownSocket(void)
{
		close(fd);
}

// Make a datapacket struct.
typedef struct __attribute__((packed))
{
		uint8_t packet;
		uint32_t packetno;
		uint8_t data[512 - sizeof(uint8_t) - sizeof(uint32_t)];
} datapack_t;

typedef struct __attribute__((packed))
{
		uint8_t packet;
		uint32_t data;
} packet_t;

static void SendDataPackets(void *data, size_t len)
{
		// Send the UDP datagrams.
		const uint8_t *ptr = (uint8_t*)data;

		printf("%ld bytes of data\n", len);

		uint32_t packetno = 0;
		uint32_t totalpackets = len < 512 ? 1 : len / 512;

		// Send our INFOPACKS packet
		printf("Sending %d (INFOPACKS) packet with a total of %d (%d) packets to send\n", INFOPACKS, totalpackets, htonl(totalpackets));
		packet_t pak = { INFOPACKS, htonl(totalpackets) };
		sendto(fd, &pak, sizeof(packet_t), 0, &saddr.sa, sizeof(saddr.sa));

		// Start transmitting data.
		while (len > 0)
		{
				// Fill out the struct
				datapack_t dat;
				memset(&dat, 0, sizeof(datapack_t));
				dat.packetno = htonl(packetno);
				dat.packet = INFO;

				// Who is the smallest?
				size_t clen = MIN(len, sizeof(dat.data));

				// Copy the data
				memcpy(dat.data, data, clen);

				// Update the pointer and remaining length
				ptr += clen;
				len -= clen;

				// Send the data packet
				sendto(fd, &dat, sizeof(datapack_t), 0, &saddr.sa, sizeof(saddr.sa));

				// Increment the packet number.
				packetno++;
		}
}

void SendDataBurst(information_t *info)
{
		printf("Sending %d (DATABURST) packet\n", DATABURST);
		// Send the BEGINBURST packet
		uint8_t begin = DATABURST;
		sendto(fd, &begin, sizeof(begin), 0, &saddr.sa, sizeof(saddr.sa));

		// Send the timeout packet so the server knows when to expect our next message
		printf("Sending %d (TIMEOUT) packet\n", TIMEOUT);
		packet_t pak = { TIMEOUT, htonl(timeout) };
		sendto(fd, &pak, sizeof(packet_t), 0, &saddr.sa, sizeof(saddr.sa));

		// Serialize the information_t struct to a data block we can just send
		// in 512-byte chunks like a file.
		printf("Serialize and send\n");
		binn *map = SerializeData(info);

		// Now transmit the data to the server.
		printf("Datalength is %d\n", binn_size(map));
		SendDataPackets(binn_ptr(map), binn_size(map));

		// Send ENDBURST packet
		printf("Sending %d (ENDBURST) packet\n", ENDBURST);
		uint8_t end = ENDBURST;
		sendto(fd, &end, sizeof(end), 0, &saddr.sa, sizeof(saddr.sa));

		// Delete the serialized data.
		// TODO: Does this memleak because we have nested maps and lists?
		binn_free(map);
}
