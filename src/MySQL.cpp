#include "MySQL.h"
#include "tinyformat.h"
#include "misc.h"
#include "Config.h"


MySQL::MySQL(const std::string &host, const std::string &user, const std::string &pass, const std::string &db, short p = 0) :
	user(user), pass(pass), host(host), db(db), port(p)
{
	printf("MySQL client version: %s\n", mysql_get_client_info());

	// Create the MySQL object
	this->con = mysql_init(NULL);

	if (!con)
		throw MySQLException("Failed to create a mysql object!");

	// Authenticate to the MySQL database
	if (!this->DoConnection())
		throw MySQLException(tfm::format("%s (%d)", mysql_error(this->con), mysql_errno(this->con)));

	printf("Connected to %s: %lu\n", host.c_str(), mysql_thread_id(this->con));
}

MySQL::~MySQL()
{
	mysql_close(con);
}

MySQL_Result MySQL::Query(const std::string &query)
{
	this->mtx.lock();
	// Form our object
	MySQL_Result res;
	MYSQL_RES *result = nullptr;
	MYSQL_ROW row;
	int cnt = 0;

	// If we fail to connect, just return an empty query.
	if (!this->con)
		if (!this->CheckConnection())
			goto exit;

	// Run the query
	if (mysql_query(this->con, query.c_str()))
	{
		std::string msg = tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con));
		this->mtx.unlock();
		throw MySQLException(msg);
	}

	// Store the query result
	result = mysql_store_result(this->con);
	if (result == NULL && mysql_errno(this->con) == 0)
		goto exit;
	else if (result == NULL)
	{
		std::string msg = tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con));
		this->mtx.unlock();
		throw MySQLException(msg);
	}

	// Get total columns/fields w/e
	res.fields = mysql_num_fields(result);

	// Loop through the MySQL objects and create the array for the query result
	while ((row = mysql_fetch_row(result)))
		res.rows[cnt++] = row;

	mysql_free_result(result);

exit:

	this->mtx.unlock();

	return res;
}

bool MySQL::DoConnection()
{
	// If mysql_real_connect returns NULL then we have failed to connect, return true or false or whatever
	this->mtx.lock();
	bool status = mysql_real_connect(this->con, this->host.c_str(), this->user.c_str(),
									 this->pass.c_str(), this->db.c_str(), this->port, NULL, 0) != NULL;
	this->mtx.unlock();
	return status;
}

bool MySQL::CheckConnection()
{
	dprintf("CheckConnection()...\n");
	int status = -1;
	if (!this->con)
		goto tryconnect;

	this->mtx.lock();
	status = mysql_ping(this->con);
	this->mtx.unlock();

	// Ping the server, if it doesn't reply then start reconnecting.
	if (status != 0)
	{
tryconnect:
		// Retry 5 times to connect to the server, return true if we do
		for (int i = 0; i < c->mysqlretries; ++i)
			if (this->DoConnection())
				return true;
		// otherwise we fall through and failed to connect 5 times so return false.
		return false;
	}

	return true;
}

std::string MySQL::Escape(const std::string &str)
{
	char *tmp = new char[str.length() * 2 + 1];
	this->mtx.lock();
	mysql_real_escape_string(this->con, tmp, str.c_str(), str.length());
	this->mtx.unlock();
	std::string retStr(tmp);
	delete [] tmp;
	return retStr;
}
