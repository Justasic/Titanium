#pragma once
#include <cstdio>
#include <string>


// libfcgi-dev includes
#define NO_FCGI_DEFINES 1
//#include <fcgio.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>

class Request
{
	FCGX_Request *request;
public:
	Request(FCGX_Request *request);

	~Request();

	// Write to the datta stream as plain text
	void Write(const char *str, ...);

	// Write binary data to the data stream
	void Write(void*, size_t len);

	std::string GetParam(const std::string &str);

	void SetStatus(int st);
};
