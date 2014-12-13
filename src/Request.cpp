#include "Request.h"
#include "misc.h"


Request::Request(FCGX_Request *request) : request(request)
{
}

Request::~Request()
{
}

// Write to the datta stream as plain text
void Request::Write(const char *str, ...)
{
	va_list va;
	char *writeable = nullptr;
	va_start(va, str);
	vasprintf(&writeable, str, va);
	va_end(va);
	FCGX_FPrintF(this->request->out, "%s", writeable);
	free(writeable);
}

// Write binary data to the data stream
void Request::Write(void *data, size_t len)
{
	if (!data || !len)
		return;

	FCGX_PutStr(reinterpret_cast<const char *>(data), len, this->request->out);
}

std::string Request::GetParam(const std::string &str)
{
	return FCGX_GetParam(str.c_str(), this->request->envp);
}

void Request::SetStatus(int st)
{
	FCGX_SetExitStatus(st, this->request->out);
}
