#pragma once
#include <cstdio>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>

#include "tinyformat.h"


// Mysql
#include <mysql/my_global.h>
#include <mysql/mysql.h>

typedef struct MySQL_Result_s {
	int fields;
	std::map<int, std::vector<std::string>> rows;
} MySQL_Result;

class MySQLException : public std::exception
{
	const char *str;
public:
	MySQLException(const std::string &mstr)
	{
		// we're crashing, I honestly have bigger issues than memleaks right now.
		this->str = strdup(mstr.c_str());
	}

	~MySQLException()
	{
		// albeit this is very bad, I won't concern myself with it right now.
		delete this->str;
	}

	const char *what() const noexcept
	{
		return this->str;
	}
};

class MySQL
{
	// MySQL connection object
	MYSQL *con;

	// As much as I hate storing these, we must to reconnect on timeouts.
	std::string user, pass, host, db;

	// MySQL server's port.
	short int port;

	// Used to connect to the database
	bool DoConnection();

	// Mutex so we can be thread-safe
	std::mutex mtx;
public:

	MySQL(const std::string &host, const std::string &user, const std::string &pass, const std::string &db, short);
	~MySQL();

	// Run a query
	MySQL_Result Query(const std::string &query);

	// Check the connection to the database.
	bool CheckConnection();

	// Escape a string
	std::string Escape(const std::string &str);


	// variadic functions which automatically escape args
	// passed to them. We lose our ability to do types and
	// fancy formatting but oh well.
	template<typename... Args>
	MySQL_Result Query(const std::string &query, const Args&... args)
	{
		// Stringify then escape each argument and pass it through the formatter
		return this->Query(tfm::format(query.c_str(), this->Escape(stringify(args))...));
	}

	template<typename... Args>
	std::string Escape(const std::string &str, const Args&... args)
	{
		return tfm::format(str.c_str(), this->Escape(stringify(args))...);
	}

private:

	// private implementation of stringify
	template<typename T>
	std::string stringify(const T& t)
	{
		std::stringstream ss;
		ss << t;
		return ss.str();
	}
};
