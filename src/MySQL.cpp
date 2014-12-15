#include "MySQL.h"
#include "tinyformat.h"



MySQL::MySQL(const std::string &host, const std::string &user, const std::string &pass, const std::string &db)
{
	printf("MySQL client version: %s\n", mysql_get_client_info());

	// Create the MySQL object
	this->con = mysql_init(NULL);

	if (!con)
		throw MySQLException(tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con)));

	// Authenticate to the MySQL database
	if (!mysql_real_connect(this->con, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), 0, NULL, 0))
		throw MySQLException(tfm::format("%s (%d)\n", mysql_error(this->con), mysql_errno(this->con)));
}

MySQL::~MySQL()
{
	mysql_close(con);
}

MySQL_Result MySQL::Query(const std::string &query)
{
	// Form our object
	MySQL_Result res;

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
	{
		res.rows[cnt] = row;
// 		for(int i = 0; i < num_fields; i++)
// 		{
// 			printf("%s ", row[i] ? row[i] : "NULL");
// 		}
// 		printf("\n");
	}

	mysql_free_result(result);

	return res;
}
