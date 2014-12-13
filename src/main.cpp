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

// Mysql
#include <mysql/my_global.h>
#include <mysql/mysql.h>

// Our thread engine.
#include "ThreadEngine.h"

#include "Request.h"

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
		Request r(&request);

		// Set the status to 200 OK
		r.SetStatus(200);

		// Form the HTTP header enough, nginx will fill in the rest.
		r.Write("Content-Type: text/html\r\n\r\n");

		// Send our message
		r.Write("<h2>%s thread %d</h2>", r.GetParam("QUERY_STRING").c_str(), ThreadHandler::GetThreadID());

		r.Write("<p>%s<br/>", r.GetParam("REMOTE_ADDR").c_str());
		r.Write("%s<br/>", r.GetParam("REQUEST_URI").c_str());
		r.Write("%s<br/>", r.GetParam("SERVER_PROTOCOL").c_str());
		r.Write("%s<br/>", r.GetParam("REQUEST_METHOD").c_str());
		r.Write("%s<br/>", r.GetParam("REMOTE_PORT").c_str());
		r.Write("%s<br/>", r.GetParam("SCRIPT_NAME").c_str());
		r.Write("</p>");

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
	printf("MySQL client version: %s\n", mysql_get_client_info());

	MYSQL *con = mysql_init(NULL);
	if (!con)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		return EXIT_FAILURE;
	}

	if (!mysql_real_connect(con, "localhost", "root", "root_pswd", NULL, 0, NULL, 0))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < 10; ++i)
		th.AddQueue(OpenListener, sock_fd);

	printf("Submitting jobs...\n");
	th.Submit();


	printf("Idling main thread.\n");
	while(!quit)
		sleep(1);

	printf("Shutting down.\n");
	th.Shutdown();

	mysql_close(con);

	return EXIT_SUCCESS;
}
