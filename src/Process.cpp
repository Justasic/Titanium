#include "Process.h"
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include "Socket.h"
#include <cstring>
#include <vector>

typedef struct information_s {
	struct sysinfo s;
	struct utsname u;
	struct timeval tm;
} information_t;

typedef struct {
	socket_t c;
	information_t info;
	size_t len;
} whatever_t;

std::vector<whatever_t*> clients;

void ProcessUDP(Socket *s, socket_t client, void *buf, size_t len)
{
	for (auto it = clients.begin(), it_end = clients.end(); it != it_end;)
	{
		printf("Iterating client!\n");
		whatever_t *w = *it;
		if (w->c.in.sin_port == client.in.sin_port)
		{
			// Copy the remainder
			memcpy(((uint8_t*)&w->info) + w->len, buf, len);
			w->len += len;
			// Are we completely filled? we should only have sent 2 packets so far
			// so it sould be full.
			if (w->len == sizeof(information_t))
			{
				// Print our info and return.
				information_t *info = &w->info;

				printf("\n%ld seconds uptime\n", info->tm.tv_sec - info->s.uptime);
				printf("%lu %lu %lu load times\n", info->s.loads[0], info->s.loads[1], info->s.loads[2]);
				printf("%d processes running\n", info->s.procs);

				printf("Hostname: %s\n", info->u.nodename);
				printf("Architecture: %s\n", info->u.machine);
				printf("OS: %s %s\n", info->u.sysname, info->u.release);

				struct timeval tm;
				gettimeofday(&tm, NULL);

				printf("%lu.%.6lu second difference\n\n", tm.tv_sec - info->tm.tv_sec, tm.tv_usec - info->tm.tv_usec);

				// Remove ourselves from the vector.
				clients.erase(it);
				delete w;

				return;
			}
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
