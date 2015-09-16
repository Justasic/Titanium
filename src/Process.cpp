#include "Process.h"
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include "Socket.h"
#include "MySQL.h"
#include "tinyformat.h"
#include <cstring>
#include <vector>

#include "binn.h"

typedef struct {
	// Socket information
	socket_t c;
	time_t timeout;
	size_t totalpackets;
	size_t receivedpackets;
	bool bursting;

	// The data
	void *data;
	size_t len;
} whatever_t;

typedef struct
{
		uint8_t packet;
		uint32_t packetno;
		uint8_t data[512 - sizeof(uint8_t) - sizeof(uint32_t)];
} packet_t;

enum
{
    DATABURST = 1,
    ENDBURST,
    TIMEOUT,
    INFO,
    INFOPACKS
};


std::vector<whatever_t*> clients;

void ProcessUDP(Socket *s, socket_t client, void *buf, size_t len)
{
	if (len > 512)
	{
		printf("WARNING: Invalid packet size: %ld, rejecting packet.\n", len);
		return;
	}

	for (auto it = clients.begin(), it_end = clients.end(); it != it_end;)
	{
		printf("Iterating client!\n");
		whatever_t *w = *it;
		if (w->c.in.sin_port == client.in.sin_port)
		{
			packet_t pak;
			memset(&pak, 0, sizeof(packet_t));
			// Copy the packet.
			memcpy(&pak, buf, len);

			// Is this the start of a burst?
			if (pak.packet == DATABURST)
			{
				w->bursting = true;
				return;
			}

			// So it isnt a burst, but are we bursting?
			if (!w->bursting)
			{
				printf("Received packet %d but we're not bursting?? ignoring packet.\n", pak.packet);
				return;
			}

			switch (pak.packet)
			{
				case TIMEOUT:
					w->timeout = pak.packetno;
					return;
				case INFOPACKS:
					w->totalpackets = pak.packetno;
					return;
				case INFO:
				{
					// Calculate the size of the data. possibility: datalen < sizeof(data);
					size_t datalen = len - sizeof(uint8_t) - sizeof(uint32_t);
					void *newdata = realloc(w->data, w->len + datalen);
					if (!newdata)
					{
						fprintf(stderr, "Failed to allocate %ld bytes of memory: %s\n", w->len + datalen, strerror(errno));
						return;
					}
					w->data = newdata;
					// Copy the data
					memcpy(((uint8_t*)w->data) + w->len, pak.data, datalen);
					w->len += datalen;
					receivedpackets++;
					return;
				}
				case ENDBURST:
				{
					if (w->receivedpackets == w->totalpackets)
						break; // continue running the function.
					else
					{
						printf("Received invalid number of data packets: %ld (expected %ld)\n", w->receivedpackets, w->totalpackets);
						clients.erase(it);
						free(w->data);
						delete w;
						return;
					}
				}
				default:
					printf("Invalid packet %d, rejecting...\n", pak.packet);
					return;
			}

			// Unserialize the packet data, then process.
			information_t *info = UnserializeData(w->data, w->len);

			// Process the info structure, this submits it to the database among other things.
			ProcessInformation(info, w->timeout);

			// Remove ourselves from the vector.
			clients.erase(it);
			free(w->data);
			delete w;

			return;
		}
		++it;
	}

	whatever_t *we = new whatever_t;
	memset(we, 0, sizeof(whatever_t));

	memcpy(&we->c, &client, sizeof(socket_t));
	memcpy(&we->info, buf, len);
 	we->len = len;
	clients.push_back(we);

}
