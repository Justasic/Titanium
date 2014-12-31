#include "Process.h"
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include "Socket.h"
#include "MySQL.h"
#include "tinyformat.h"
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

				printf("\n%ld seconds uptime\n", info->s.uptime);
				printf("%lu %lu %lu load times\n", info->s.loads[0], info->s.loads[1], info->s.loads[2]);
				printf("%d processes running\n", info->s.procs);

				printf("Hostname: %s\n", info->u.nodename);
				printf("Architecture: %s\n", info->u.machine);
				printf("OS: %s %s\n", info->u.sysname, info->u.release);

				if (std::string(info->u.nodename).empty())
				{
						printf("This host is corrupt, ignoring.\n");
						goto end;
				}

				struct timeval tm;
				gettimeofday(&tm, NULL);

				printf("%lu.%.6lu second difference\n\n", info->tm.tv_sec - tm.tv_sec, info->tm.tv_usec - tm.tv_usec);

				// Update the MySQL database with this big nasty query.
				try {
					extern MySQL *ms;
					std::string os = ms->Escape("%s %s", info->u.sysname, info->u.release);

					// We gotta do it the old fashioned way. Find the id by our hostname then insert based on that.
					MySQL_Result res = ms->Query("SELECT id FROM systems WHERE hostname='%s'", info->u.nodename);
					int id = 0;
					if (!res.rows.empty())
					{
						id = strtol(res.rows[0][0].c_str(), NULL, 10);
						if (errno == ERANGE)
							id = 0;
					}

					if (id == 0)
						ms->Query("INSERT INTO systems (uptime, processes, hostname, architecture, os) VALUES(%d, %d, '%s', '%s', '%s')",
							info->s.uptime, info->s.procs, info->u.nodename, info->u.machine, os);
					else
						ms->Query("UPDATE systems SET uptime=%d, processes=%d, hostname='%s', architecture='%s', os='%s' WHERE id=%d",
										   info->s.uptime, info->s.procs, info->u.nodename, info->u.machine, os, id);
				} catch (const MySQLException &e)
				{
					printf("MySQL error: %s\n", e.what());
				}
end:

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
