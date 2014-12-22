%{
        #include "parser.hpp"
      /*  int lineno = 0;*/
        int commentstart = 0;
        #pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-register"
	typedef yy::Parser::token token;

	#define YY_DECL int yylex(yy::Parser::semantic_type *yylval, \
		yy::Parser::location_type *yylloc, Config *ctx)

	#define YY_USER_ACTION yylloc->columns(yyleng);
%}


%option noyywrap
%option nounput
%option noinput
%option yylineno
 //%option case-insensitive

%x COMMENT

%%

%{
	// start where previous token ended.
	yylloc->step();
%}

 /* the tokens */
(?i:bind)             { return token::BIND; } /* IP/interface to bind to */
(?i:port)             { return token::PORT; } /* port to listen on */
(?i:username)         { return token::USERNAME; } /* Mysql username */
(?i:password)         { return token::PASSWORD; } /* MySQL password */
(?i:database)         { return token::DATABASE; } /* MySQL database */
(?i:host)             { return token::HOST; }     /* MySQL host */
(?i:pidfile)          { return token::PIDFILE; }  /* PID file location */
(?i:daemonize)        { return token::DAEMONIZE; }/* whether to fork to the background */
(?i:mysql)            { return token::MYSQL; }
(?i:module)           { return token::MODULE; }
(?i:server)           { return token::SERVER; }
(?i:retries)          { return token::RETRIES; }

 /* Ignore white space */
[ \t\n]               { }
[0-9]+                { yylval->ival = atoi(yytext); return token::CINT; }
\"[^\"\n]*[\"\n]      { yytext[yyleng - 1] = 0; yylval->sval = yytext + 1; return token::STR; }
true|yes              { yylval->bval = true; return token::BOOL; }
false|no              { yylval->bval = false; return token::BOOL; }
"/*"                  { BEGIN(COMMENT); commentstart = yylineno; }
<COMMENT>"*/"         { BEGIN(INITIAL); }
<COMMENT>([^*]|\n)+|. { }
<COMMENT><<EOF>>      { printf("line %d: Unterminated comment\n", commentstart+1); return 0; }
"//".*\n              { /* Ignore comments */ }
.                     { return yytext[0]; }

%%

#pragma clang diagnostic pop
