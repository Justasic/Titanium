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

		// Call an event, this will later be used by plugins
		// once they register with the handler.
		//OnRequest.CallVoidEvent("REQUEST", r, r.GetParam("SCRIPT_NAME"));

		// Form the HTTP header enough, nginx will fill in the rest.
		r.Write("Content-Type: text/html\r\n\r\n");

		// Send our message

		r.Write("<html><head>");
		r.Write("<title>Titanium Home</title>");
		r.Write("</head>");
		r.Write("<body>");

		// Basic welcome.
		r.Write("<center><h1>Welcome to Titanium!</h1></center>");

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
		r.Write("<table> \
		<tr> \
			<th>Hostname</th> \
			<th>Processes</th> \
			<th>Uptime</th> \
			<th>Architecture</th> \
			<th>Kernel</th> \
			<th>Last Updated</th> \
		</tr>");
//
// 		printf("Running MySQL Query...\n");
// 		// Test Query
		try {
			MySQL_Result mr = ms->Query("SELECT * from " + ms->Escape("systems"));
			for (auto it : mr.rows)
			{

				r.Write("<tr>");

				time_t utime = strtol(it.second[1], NULL, 10);
// 				tfm::pr
// 				time_t snaptime = strtol()
				std::string uptime = Duration(utime);

				r.Write("<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td> \
				<td>%s</td>", it.second[3], it.second[2], uptime, it.second[4],
					it.second[5], it.second[6]);

				r.Write("</tr>");

// 				for (int i = 0; i < mr.fields; ++i)
// 					r.Write(" ", it.second[i] ? it.second[i] : "(NULL)");
// 				r.Write("<br/>");
			}
// 			printf("%lu rows with %d columns\n", mr.rows.size(), mr.fields);
		}
		catch(const MySQLException &e)
		{
			printf("MySQL error: %s\n", e.what());
			r.Write("<p>MySQL error: %s</p><br/>", e.what());
		}


		r.Write("</table></body>");

		r.Write("</html>");

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
