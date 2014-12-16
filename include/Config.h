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

class Config
{
public:
	Config();
	~Config();

	// Parse the config (again)
	void Parse();

	// Are we daemonizing?
	bool daemonize;

	// PID file path
	std::string pidfile;

	// MySQL authentication and connection information
	std::string username;
	std::string password;
	std::string database;
	std::string hostname;

	// Port and interface to bind to
	std::string bind;
	short int port;
};
