#include "MySQL.h"
#include "tinyformat.h"
#include "misc.h"


MySQL::MySQL(const std::string &host, const std::string &user, const std::string &pass, const std::string &db) :
	user(user), pass(pass), host(host), db(db)
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
	// Form our object
	MySQL_Result res;

	// If we fail to connect, just return an empty query.
	if (!this->con)
		if (!this->CheckConnection())
			return res;

	// Run the query
	if (mysql_query(this->con, query.c_str()))
		throw MySQLException(tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con)));

	// Store the query result
	MYSQL_RES *result = mysql_store_result(this->con);
	if (result == NULL)
		throw MySQLException(tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con)));

	// Get total columns/fields w/e
	res.fields = mysql_num_fields(result);

	// Loop through the MySQL objects and create the array for the query result
	MYSQL_ROW row;
	int cnt = 0;
	while ((row = mysql_fetch_row(result)))
		res.rows[cnt++] = row;

	mysql_free_result(result);

	return res;
}

bool MySQL::DoConnection()
{
	// If mysql_real_connect returns NULL then we have failed to connect, return true or false or whatever
	return mysql_real_connect(this->con, this->host.c_str(), this->user.c_str(),
				  this->pass.c_str(), this->db.c_str(), 0, NULL, 0) != NULL;
}

bool MySQL::CheckConnection()
{
	dprintf("CheckConnection()...\n");
	if (!this->con)
		goto tryconnect;

	// Ping the server, if it doesn't reply then start reconnecting.
	if (mysql_ping(this->con) != 0)
	{
tryconnect:
		// Retry 5 times to connect to the server, return true if we do
		for (int i = 0; i < 5; ++i)
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
	mysql_real_escape_string(this->con, tmp, str.c_str(), str.length());
	std::string retStr(tmp);
	delete [] tmp;
	return retStr;
}
