#pragma once
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <cstring>

// Mysql
#include <mysql/my_global.h>
#include <mysql/mysql.h>

// map of objs   id           columns  row data
//typedef std::map<int, std::pair<int, MYSQL_ROW>> MySQL_Result;

typedef struct MySQL_Result_s {
	int fields;
	std::map<int, MYSQL_ROW> rows;
} MySQL_Result;

class MySQLException : public std::exception
{
	std::string string;
public:
	MySQLException(const std::string &str) : string(str)
	{
	}

	const char *what() const noexcept
	{
		return this->string.c_str();
	}
};

class MySQL
{
	// MySQL connection object
	MYSQL *con;
public:

	MySQL(const std::string &host, const std::string &user, const std::string &pass, const std::string &db);
	~MySQL();

	MySQL_Result Query(const std::string &query);
};
