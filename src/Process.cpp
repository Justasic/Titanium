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

				printf("%lu.%.6lu second difference\n\n", tm.tv_sec - info->tm.tv_sec, tm.tv_usec - info->tm.tv_usec);

				// Update the MySQL database with this big nasty query.
				try {
					// INSERT INTO table (id, name, age) VALUES(1, "A", 19) ON DUPLICATE KEY UPDATE name=VALUES(name), age=VALUES(age)
					extern MySQL *ms;
					std::string query;
					std::string os = ms->Escape(tfm::format("%s %s", info->u.sysname, info->u.release));

					// We gotta do it the old fashioned way. Find the id by our hostname then insert based on that.
					MySQL_Result res = ms->Query(tfm::format("SELECT id FROM systems WHERE hostname='%s'", ms->Escape(info->u.nodename)));
					int id = 0;
					if (!res.rows.empty())
					{
						id = strtol(res.rows[0][0], NULL, 10);
						if (errno == ERANGE)
							id = 0;
					}
					printf("ID: %d\n", id);

					if (id == 0)
					{
						query = tfm::format("INSERT INTO systems (uptime, processes, hostname, architecture, os) VALUES(%d, %d, '%s', '%s', '%s')",
							info->s.uptime, info->s.procs, ms->Escape(info->u.nodename), ms->Escape(info->u.machine), os);
					}
					else
					{
						// Update the database entry.
						query = tfm::format("INSERT INTO systems (id, uptime, processes, hostname, architecture, os)"
						" VALUES(%d, %d, %d, '%s', '%s', '%s') ON DUPLICATE KEY UPDATE uptime=VALUES(uptime), processes=VALUES(processes),"
						" architecture=VALUES(architecture), os=VALUES(os)",
							id, info->s.uptime, info->s.procs, ms->Escape(info->u.nodename), ms->Escape(info->u.machine), os);
					}

					tfm::printf("Running query: \"%s\"\n", query);
					// Actually run the query
					ms->Query(query);
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
