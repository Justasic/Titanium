#pragma once
#include <cstdio>
#include <string>
#include "tinyformat.h"


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

	inline void Write(const std::string &str)
	{
		this->WriteData(str.c_str(), str.length());
	}

	// Write to the data stream as plain text
	template<typename... Args>
	void Write(const std::string &str, const Args&... args)
	{
		std::string tmp = tfm::format(str.c_str(), args...);
		this->WriteData(tmp.c_str(), tmp.length());
	}

	// Write binary data to the data stream
	template<typename ptr>
	void WriteData(ptr *data, size_t len)
	{
		if (!data || !len)
			return;

		// Write to the FastCGI data stream
		FCGX_PutStr(reinterpret_cast<const char *>(data), len, this->request->out);
	}



	std::string GetParam(const std::string &str);

	void SetStatus(int st);
};
