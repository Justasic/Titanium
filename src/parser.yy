%language "C++"
%defines
%locations
%error-verbose

%define parser_class_name { Parser }

// Add our custom Config class to the parser
// so we can use it. Forward declare it because
// it cannot be included in the definitions file.
%parse-param { Config *ctx }
%lex-param   { Config *ctx }

// This is a %code requires because it will allow
// us to include the header file which provides
// the definitions for the above %parse-param parameters.
%code requires {
	#include "Config.h"
}

// Require bison 2.3 or newer
%require "2.3"

// Use newer C++ code
%skeleton "lalr1.cc"

%{
	extern "C" void yyerror(const char *s);

	// we include our own definitions file simply to forward declare
	// this function.
	#include "parser.hpp"
	extern int yylex(yy::Parser::semantic_type *yylval, yy::Parser::location_type *yylloc,
			Config *ctx);

	// Temporary for debug reasons.
	#include "tinyformat.h"
%}

%union
{
	int ival;
	char *sval;
	bool bval;
}

%token <ival> CINT
%token <sval> STR
%token <bval> BOOL

%token BIND
%token PORT
%token USERNAME
%token PASSWORD
%token DATABASE
%token HOST
%token PIDFILE
%token DAEMONIZE
%token MYSQL
%token MODULE
%token NAME
%token PATH
%token SERVER
%token RETRIES


/*%token SERVER
%token BIND
%token PORT
%token DIRECTORY
%token USER
%token GROUP
%token READTIMEOUT
%token LISTEN
%token FIXPATH
%token MODSEARCHPATH*/

%start conf

%%

conf: | conf conf_items;

conf_items: server_entry | MySQL_entry | module_entry;

module_entry: MODULE
{
/*	conf_module_t *m = nmalloc(sizeof(conf_module_t));
	m->path = NULL;
	m->name = NULL;
	curmod = m;

	if (!config)
	{
		config = nmalloc(sizeof(config_t));
		config->daemonize = -1;
		config->readtimeout = 5;
		config->fixpath = 1;
		vec_init(&config->listenblocks);
		vec_init(&config->moduleblocks);
	}

	vec_push(&config->moduleblocks, m);*/
}
'{' module_items '}';

MySQL_entry: MYSQL
{
	// We already have a Config object, nothing to concern ourselves with.
}
'{' MySQL_items '}';

server_entry: SERVER
{
/*	config = nmalloc(sizeof(config_t));
	// Defaults
	config->daemonize = 1;
	config->readtimeout = 5;
	config->fixpath = 1;
	vec_init(&config->listenblocks);
	vec_init(&config->moduleblocks);*/
}
'{' server_items '}';

server_items: | server_item server_items;
server_item: server_daemonize | server_pidfile | server_port | server_bind;

MySQL_items: | MySQL_item MySQL_items;
MySQL_item: MySQL_host | MySQL_port | MySQL_username | MySQL_database | MySQL_password | MySQL_retrytimes;

module_items: | module_item module_items;
module_item: module_path | module_name;




module_path: PATH '=' STR ';'
{
/* 	curmod->path = strdup(yylval.sval); */
};

module_name: NAME '=' STR ';'
{
/* 	curmod->name = strdup(yylval.sval); */
};






MySQL_host: HOST '=' STR ';'
{
	ctx->hostname = yyla.value.sval;
	tfm::printf(" MySQL Hostname: %s\n", ctx->hostname);
};

MySQL_username: USERNAME '=' STR ';'
{
	ctx->username = yyla.value.sval;
	tfm::printf(" MySQL Username: %s\n", ctx->username);
};

MySQL_password: PASSWORD '=' STR ';'
{
	ctx->password = yyla.value.sval;
	tfm::printf(" MySQL Password: %s\n", ctx->password);
};

MySQL_database: DATABASE '=' STR ';'
{
	ctx->database = yyla.value.sval;
	tfm::printf(" MySQL Database: %s\n", ctx->database);
};

MySQL_port: PORT '=' CINT ';'
{
	ctx->mysqlport = yyla.value.ival;
	tfm::printf(" MySQL Port: %d\n", ctx->mysqlport);
};

MySQL_retrytimes: RETRIES '=' CINT ';'
{
/* 	curblock->port = yylval.ival; */
};






/*server_directory: DIRECTORY '=' STR ';'
{
	config->directory = strdup(yylval.sval);
	if (!config->directory)
	{
		fprintf(stderr, "Failed to parse config: %s\n", strerror(errno));
		exit(1);
	}
};*/

/*server_user: USER '=' STR ';'
{
	config->user = strdup(yylval.sval);
	if (!config->user)
	{
		fprintf(stderr, "Failed to parse config: %s\n", strerror(errno));
		exit(1);
	}
};

server_group: GROUP '=' STR ';'
{
	config->group = strdup(yylval.sval);
	if (!config->group)
	{
		fprintf(stderr, "Failed to parse config: %s\n", strerror(errno));
		exit(1);
	}
};

server_module_search_path: MODSEARCHPATH '=' STR ';'
{
	config->modsearchpath = strdup(yylval.sval);
};*/

server_daemonize: DAEMONIZE '=' BOOL ';'
{
	ctx->daemonize = yyla.value.bval;
	tfm::printf(" Daemonize: %s\n", ctx->daemonize);
};

server_pidfile: PIDFILE '=' STR ';'
{
	ctx->pidfile = yyla.value.sval;
	tfm::printf(" PIDFile: %s\n", ctx->pidfile);
};

server_port: PORT '=' CINT ';'
{
	tfm::printf(" Server port: %d\n", yyla.value.ival);
	ctx->port = yyla.value.ival;
};

server_bind: BIND '=' STR ';'
{
	ctx->bind = yyla.value.sval;
	tfm::printf(" Server bind: %s\n", ctx->bind);
};

/*server_readtimeout: READTIMEOUT '=' CINT ';'
{
	config->readtimeout = yylval.ival;
};*/

/*server_fixpath: FIXPATH '=' BOOL ';'
{
	config->fixpath = yylval.bval;
};*/

