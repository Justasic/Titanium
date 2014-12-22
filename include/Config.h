#pragma once
#include <map>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <cstdint>

#include "EventDispatcher.h"

// Forward declare for the private parser variable
namespace yy {
	class Parser;
};

class Config
{
	const std::string filepath;
	// The Bison++ parser class
	yy::Parser *p;
public:
	Config(const std::string &fp);
	~Config();

	// Parse the config (again)
	int Parse();

	// Are we daemonizing?
	bool daemonize;

	// PID file path
	std::string pidfile;

	// MySQL authentication and connection information
	std::string username;
	std::string password;
	std::string database;
	std::string hostname;
	short int mysqlport;

	// Port and interface to bind to
	std::string bind;
	short int port;

	// Call an event.
	EventDispatcher ConfigEvents;
};
