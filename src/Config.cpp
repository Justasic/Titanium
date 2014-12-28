#include "parser.hpp"
#include "Config.h"
#include "misc.h"


// Needed to interact with flex/bison
extern FILE *yyin;
// extern int yyparse();
// extern void yylex_destroy();
extern int yylineno;

Config::Config(const std::string &filepath) : filepath(filepath)
{
	this->p = new yy::Parser(this);
	this->readtimeout = 1;
}

Config::~Config()
{
	delete this->p;
}

// Parse the config (again)
int Config::Parse()
{
	// Call the EventDispatcher class here to announce we're about to parse a file.
	//ConfigEvents.CallVoidEvent("OnPreParse", this, this->filepath);

    char *path = realpath(this->filepath.c_str(), nullptr);
    if (!path)
    {
        fprintf(stderr, "Failed to resolve path \"%s\": %s\n", this->filepath.c_str(), strerror(errno));
        return -1;
    }

    dprintf("Reading config from \"%s\"\n", path);

	yyin = fopen(path, "r");
	if (!yyin)
	{
		fprintf(stderr, "Failed to open file \"%s\"\n", path);
		return -1;
	}
	int ret = this->p->parse();
	fclose(yyin);
	yyin = nullptr;
    free(path);
	return ret;
}

extern "C" void yyerror(const char *s)
{
	std::cerr << "Lexical error at " << yylineno << ": " << s << std::endl;
}

// Define the error reporting function
namespace yy {
	void Parser::error(location const &loc, const std::string &str)
	{
		std::cerr << "Parse error at " << loc << ": " << str << std::endl;
	}
};
