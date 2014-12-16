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
#include "Request.h"

std::atomic_bool quit;

ThreadHandler *threads;
MySQL *ms;
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
		r.Write("<title>Example Test thingy</title>");
		r.Write("</head>");

		r.Write("<h2>%s thread %d</h2>", r.GetParam("QUERY_STRING"), ThreadHandler::GetThreadID());

		r.Write("<p>%s<br/>", r.GetParam("REMOTE_ADDR"));
		r.Write("%s<br/>", r.GetParam("REQUEST_URI"));
		r.Write("%s<br/>", r.GetParam("SERVER_PROTOCOL"));
		r.Write("%s<br/>", r.GetParam("REQUEST_METHOD"));
		r.Write("%s<br/>", r.GetParam("REMOTE_PORT"));
		r.Write("%s<br/>", r.GetParam("SCRIPT_NAME"));
		r.Write("</p>");
		r.Write("<br /><br /><br /><h2>MySQL Query:</h2><br />");

		printf("Running MySQL Query...\n");
		// Test Query
		MySQL_Result mr = ms->Query("SELECT * from " + ms->Escape("testtbl"));

		printf("%lu rows with %d columns\n", mr.rows.size(), mr.fields);

		for (auto it : mr.rows)
		{
			for (int i = 0; i < mr.fields; ++i)
				r.Write("%s ", it.second[i] ? it.second[i] : "(NULL)");
			r.Write("<br/>");
		}

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
	ThreadHandler th;
	th.Initialize();
	threads = &th;

	FCGX_Init();
	int sock_fd = FCGX_OpenSocket(":3000", 1024);
	printf("Opened socket fd: %d\n", sock_fd);

	// Initialize MySQL
	MySQL m("localhost", "root", "", "test");
	ms = &m;

	for (unsigned int i = 0; i < (th.totalConcurrentThreads * 2) / 2; ++i)
		th.AddQueue(OpenListener, sock_fd);

	printf("Submitting jobs...\n");
	th.Submit();


	printf("Idling main thread.\n");
	while(!quit)
	{
		sleep(5);
		ms->CheckConnection();
	}

	printf("Shutting down.\n");
	th.Shutdown();

	return EXIT_SUCCESS;
}
