#include "parser.hpp"
#include "Config.h"


// Needed to interact with flex/bison
extern FILE *yyin;
// extern int yyparse();
// extern void yylex_destroy();
extern int yylineno;

Config::Config(const std::string &filepath) : filepath(filepath)
{
	this->p = new yy::Parser(this);
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
	yyin = fopen(this->filepath.c_str(), "r");
	if (!yyin)
	{
		fprintf(stderr, "Failed to open file \"%s\"\n", this->filepath.c_str());
		return -1;
	}
	int ret = this->p->parse();
	fclose(yyin);
	yyin = nullptr;
	return ret;
}

extern "C" void yyerror(const char *s)
{
	std::cerr << "Parser Error at " << yylineno << ": " << s << std::endl;
}

// Define the error reporting function
namespace yy {
	void Parser::error(location const &loc, const std::string &str)
	{
		std::cerr << "error at " << loc << ": " << str << std::endl;
	}
};
