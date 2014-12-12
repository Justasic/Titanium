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

// libfcgi-dev includes
#define NO_FCGI_DEFINES 1
//#include <fcgio.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>

// Our thread engine.
#include "ThreadEngine.h"

std::atomic_bool quit;

ThreadHandler *threads;

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
		const char *qstr = FCGX_GetParam("QUERY_STRING", request.envp);

		// 200 OK
		FCGX_SetExitStatus(200, request.out);

		// Form the HTML header enough, nginx will fill in the rest.
		FCGX_FPrintF(request.out, "Content-Type: text/html\r\n\r\n");

		// Send our message
		FCGX_FPrintF(request.out, "<h2>%s thread %d</h2>", qstr, ThreadHandler::GetThreadID());

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

	for (int i = 0; i < 10; ++i)
		th.AddQueue(OpenListener, sock_fd);

	printf("Submitting jobs...\n");
	th.Submit();


	printf("Idling main thread.\n");
	while(!quit)
		sleep(1);

	printf("Shutting down.\n");
	th.Shutdown();

	return EXIT_FAILURE;
}
