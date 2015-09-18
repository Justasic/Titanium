#include <cstdio>
#include <string>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <future>
#include <cerrno>
#include <atomic>

// Our thread engine.
#include "ThreadEngine.h"
#include "EventDispatcher.h"
#include "MySQL.h"
#include "Config.h"
#include "Request.h"
#include "multiplexer.h"
#include "misc.h"

std::atomic_bool quit;

ThreadHandler *threads;
MySQL *ms;
Config *c;
EventDispatcher OnRequest;


void OpenListener(int sock_fd)
{
	printf("Listener spawned for socket %d in thread %d\n", sock_fd, ThreadHandler::GetThreadID());
	FCGX_Request request;
	memset(&request, 0, sizeof(FCGX_Request));

	FCGX_InitRequest(&request, sock_fd, 0);

	// Idle infinitely and accept requests.
	while(FCGX_Accept_r(&request) == 0)
	{
		printf("Thread %d handling request\n", ThreadHandler::GetThreadID());
		Request r(&request);

		// Set the status to 200 OK
		r.SetStatus(200);

		tfm::printf("Request URI: %s\n", r.GetParam("REQUEST_URI"));

		// WARNING: This is nasty and should be handled by nginx.
		// don't use this XD
		if (r.GetParam("REQUEST_URI") == "/base.css")
		{
			// we're assuming we're running from the build directory.
			FILE *f = fopen("../static/css/base.css", "r");
			if (!f)
			{
				// 404 the thing.
				r.SetStatus(404);
				goto finish;
			}

			r.Write("Content-Type: text/css\r\n\r\n");

			uint8_t *buf = new uint8_t[1024];

			fseek(f, 0, SEEK_END);
			size_t size = ftell(f);
			rewind(f);

			// Write the data to the FastCGI stream.
			while(size > 0)
			{
				size_t read = fread(buf, 1, 1024, f);
				r.WriteData(buf, read);
				size -= read;
			}

			delete[] buf;
			// Finish the request.
			goto finish;
		}

		// Form the HTTP header enough, nginx will fill in the rest.
		r.Write("Content-Type: text/html\r\n\r\n");

		// Send our message
		r.Write("<html><head>");
		r.Write("<link rel=\"stylesheet\" type=\"text/css\" href=\"/base.css\"></link>");
		r.Write("<script src=\"//ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js\"></script>");
		r.Write("<link rel=\"stylesheet\" type=\"text/css\" href=\"//netdna.bootstrapcdn.com/bootstrap/3.1.1/css/bootstrap.min.css\"></link>");
		r.Write("<script src=\"//netdna.bootstrapcdn.com/bootstrap/3.1.1/js/bootstrap.min.js\"></script>");
		r.Write("<title>Titanium Home</title>");
		r.Write("</head>");
		r.Write("<body>");

		// Basic welcome.
		r.Write("<h2>Welcome to Titanium!</h2>");

// 		r.Write("<h2>%s thread %d</h2>", r.GetParam("QUERY_STRING"), ThreadHandler::GetThreadID());

// 		r.Write("<p>%s<br/>", r.GetParam("REMOTE_ADDR"));
// 		r.Write("%s<br/>", r.GetParam("REQUEST_URI"));
// 		r.Write("%s<br/>", r.GetParam("SERVER_PROTOCOL"));
// 		r.Write("%s<br/>", r.GetParam("REQUEST_METHOD"));
// 		r.Write("%s<br/>", r.GetParam("REMOTE_PORT"));
// 		r.Write("%s<br/>", r.GetParam("SCRIPT_NAME"));
// 		r.Write("</p>");
// 		r.Write("<br /><br /><br /><h2>MySQL Query:</h2><br />");

		// Dump a table out.
		r.Write("<table id=\"SysmonTable\"> \
		<thead> \
		<tr> \
			<th>Hostname</th> \
			<th>Processes</th> \
			<th>Uptime</th> \
			<th>Architecture</th> \
			<th>Kernel</th> \
			<th>Last Updated</th> \
			<th>Down since</th> \
		</tr> \
		</thead>");

		// Run query for hosts
		try {
			MySQL_Result mr = ms->Query("SELECT * from " + ms->Escape("systems"));
			for (auto it : mr.rows)
			{
				time_t utime = strtol(it.second[1].c_str(), NULL, 10);
				std::string uptime = Duration(utime);

				// Get the last time it was updated.
				time_t lastupdate = 0;
				try {
					//MySQL_Result res = ms->Query(tfm::format("SELECT UNIX_TIMESTAMP(lastupdate) from systems where id='%s'", it.second[0]));
					MySQL_Result res = ms->Query("SELECT UNIX_TIMESTAMP(lastupdate) from systems where id='%s'", it.second[0]);
					if (!res.rows.empty())
						lastupdate = strtol(res.rows[0][0].c_str(), NULL, 10);
				} catch (const MySQLException &e)
				{
					printf("MySQL error: %s\n", e.what());
				}

				printf("Last Updated: %lu\n", lastupdate);

				// make sure we mark the status of the item first.
				time_t timediff = time(NULL) - lastupdate;
				printf("Time difference: %lu\n", timediff);
				std::string classstr = "up";

				// if the time difference is too great,
				// mark it as being down or as it has yet to
				// receive contact.
				if (timediff > 60) // we've been down for a whole minute? fuck.
					classstr = "down";
				else if (timediff > 30) // we're down for 30 seconds, we could've just missed a packet.
					classstr = "recent";

				r.Write("<tr class=\"%s clickable\" id=\"row%s\" data-toggle=\"collapse\" data-target=\"#accordion%s\">",
								classstr, it.second[0], it.second[0]);

				r.Write("<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td>", it.second[3], it.second[2], uptime, it.second[4],
					it.second[5], it.second[6], classstr == "up" ? "Never" : Duration(timediff));

				r.Write("</tr>");

				r.Write("<tr><td colspan=\"7\"><div id=\"accordion%s\" class=\"collapse\">", it.second[0]);

				r.Write("<div class=\"left\">");
				r.Write("Hostname: %s<br>Process Count: %s<br>Uptime: %s", it.second[3], it.second[2], uptime);
				r.Write("</div>");

				r.Write("<div class=\"center\">");
				r.Write("This is test text");
				r.Write("</div>");

				r.Write("<div class=\"right\">");
				r.Write("Architecture: %s<br>Kernel: %s<br>Last Updated: %s", it.second[4], it.second[5], Duration(timediff));
				r.Write("</div>");

				r.Write("</div></td></tr>");

			}
		}
		catch(const MySQLException &e)
		{
			printf("MySQL error: %s\n", e.what());
			r.Write("<p>MySQL error: %s</p><br/>", e.what());
		}

		r.Write("</table>");

		// JavaScript
		r.Write("<script type=\"text/javascript\">");
		r.Write("</script>");

		r.Write("</body></html>");
finish:

		FCGX_Finish_r(&request);
	}

	printf("Exiting listener thread %d\n", ThreadHandler::GetThreadID());

	FCGX_Free(&request, sock_fd);
}

int main(int argc, char **argv)
{
	std::vector<std::string> args(argv, argv+argc);
	quit = false;

	// Parse our config before anything
	Config conf(GetCurrentDirectory() + "titanium.conf");
	c = &conf;

	printf("Config:\n");
	if (c->Parse() != 0) // Already printed an error.
		return EXIT_FAILURE;

	ThreadHandler th;
	th.Initialize();
	threads = &th;

	// Initialize the multiplexer for our sockets.
	InitializeMultiplexer();

	// Initialize the FastCGI library
	FCGX_Init();
	// Formulate the string from the config.
	std::stringstream val;
	val << c->bind << ":" << c->port;
	// Initialize a new FastCGI socket.
	std::cout << "Opening FastCGI socket: " << val.str() << std::endl;
	int sock_fd = FCGX_OpenSocket(val.str().c_str(), 1024);
	printf("Opened socket fd: %d\n", sock_fd);

	// Initialize MySQL
	MySQL m(c->hostname, c->username, c->password, c->database, c->mysqlport);
	ms = &m;

	// Start listening threads.
	for (unsigned int i = 0; i < (th.totalConcurrentThreads * 2) / 2; ++i)
		th.AddQueue(OpenListener, sock_fd);

	printf("Submitting jobs...\n");
	th.Submit();


	// Iterate over all the sockets we must bind to for UDP listening
	bool bound = false;
	for (auto it : c->listenblocks)
	{
		if (!Socket::CreateSocket(it->bind, it->port))
			continue; // We already printed an error, just keep going.
		else
			bound = true;
	}

	// We failed to bind to any of the ports, no point in keeping the program running.
	if (!bound)
	{
		fprintf(stderr, "Failed to bind to any listen interfaces, please adjust your configuration and try again.\n");
		return EXIT_FAILURE;
	}

	printf("Idling main thread.\n");
	while(!quit)
	{
		ms->CheckConnection();
		ProcessSockets();
	}

	printf("Shutting down.\n");
	th.Shutdown();

	ShutdownMultiplexer();

	return EXIT_SUCCESS;
}
