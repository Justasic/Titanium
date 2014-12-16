%{
        #include "parser.hpp"
      /*  int lineno = 0;*/
        int commentstart = 0;
%}


%option noyywrap
%option nounput
%option noinput
%option yylineno
%option c++

%x COMMENT

%%

 /* the tokens */
bind             { return BIND; } /* IP/interface to bind to */
port             { return PORT; } /* port to listen on */
username         { return USERNAME; } /* Mysql username */
password         { return PASSWORD; } /* MySQL password */
database         { return DATABASE; } /* MySQL database */
host             { return HOST; }     /* MySQL host */
pidfile          { return PIDFILE; }  /* PID file location */
daemonize        { return DAEMONIZE; }/* whether to fork to the background */
mysql            { return MYSQL; }
module           { return MODULE; }
server           { return SERVER; }
retries          { return RETRIES; }

 /*       { return MYSQLHOST; }
mysql_user       { return MYSQLUSER; }
fixpath          { return FIXPATH; }
user             { return USER; }
group            { return GROUP; }
readtimeout      { return READTIMEOUT; }
listen           { return LISTEN; }
name             { return NAME; }
path             { return PATH; }
modulesearchpath { return MODSEARCHPATH; }*/

 /* Ignore white space */
[ \t]                 { }
\n                    { /*++lineno;*/ }
[0-9]+                { yylval.ival = atoi(yytext); return CINT; }
\"[^\"\n]*[\"\n]      { yytext[yyleng - 1] = 0; yylval.sval = yytext + 1; return STR; }
true|yes              { yylval.bval = 1; return BOOL; }
false|no              { yylval.bval = 0; return BOOL; }
"/*"                  { BEGIN(COMMENT); commentstart = yylineno; }
<COMMENT>"*/"         { BEGIN(INITIAL); }
<COMMENT>([^*]|\n)+|. { /*++lineno;*/ }
<COMMENT><<EOF>>      { printf("line %d: Unterminated comment\n", commentstart+1); return 0; }
"//".*\n              { /* Ignore comments */ /*++lineno;*/ }
.                     { return yytext[0]; }

%%

