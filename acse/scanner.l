/***************************************************************************

                                    Scanner 

***************************************************************************/
%{
/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * scanner.l
 * Formal Languages & Compilers Machine, 2007/2008
 */

#include <string.h>
#include "expressions.h"
#include "utils.h"
#include "list.h"
#include "parser.tab.h"

/* Variables declared in the lexer for error tracking */
extern int line_num;
extern int num_error;

/* extern declaration of function yyerror */
extern void yyerror(const char*);

%}

%option noyywrap
%x comment

/******************************************************************************
 * DEFINITIONS
 ******************************************************************************/
DIGIT    [0-9]
ID       [a-zA-Z_][a-zA-Z0-9_]*

/******************************************************************************
 * RULES
 ******************************************************************************/
%%

"\r\n"            { ++line_num; }
"\n"              { ++line_num; }

[ \t\f\v]+        { /* Ignore whitespace. */ }

"//"[^\n]*        { /* ignore comment lines */ }
"/*"              BEGIN(comment);

<comment>[^*\n]*
<comment>[^*\n]*\n      { ++line_num; }
<comment>"*"+[^*/\n]*   
<comment>"*"+[^*/\n]*\n { ++line_num; }
<comment>"*"+"/"        BEGIN(INITIAL);

"{"               { return LBRACE; }
"}"               { return RBRACE; }
"["               { return LSQUARE; }
"]"               { return RSQUARE; }
"("               { return LPAR; }
")"               { return RPAR; }
";"               { return SEMI; }
"+"               { return PLUS; }
"-"               { return MINUS; }
"*"               { return MUL_OP; }
"/"               { return DIV_OP; }
"&"               { return AND_OP; }
"|"               { return OR_OP; }
"!"               { return NOT_OP; }
"="               { return ASSIGN; }
"<"               { return LT; }
">"               { return GT; }
"<<"              { return SHL_OP; }
">>"              { return SHR_OP; }
"=="              { return EQ; }
"!="              { return NOTEQ; }
"<="              { return LTEQ; }
">="              { return GTEQ; }
"&&"              { return ANDAND; }
"||"              { return OROR; }
","               { return COMMA; }

"do"              { return DO; }
"else"            { return ELSE; }
"if"              { return IF; }
"int"             { yylval.integer = INTEGER_TYPE; return TYPE; }
"while"           { return WHILE; }
"return"          { return RETURN; }
"read"            { return READ; }
"write"           { return WRITE; }

{ID}              { yylval.string = strdup(yytext); return IDENTIFIER; }
{DIGIT}+          { yylval.integer = atoi(yytext);
                    return NUMBER; }

.                 { yyerror("unexpected token");
                    num_error++;
                    return -1; /* invalid token */ }
<INITIAL><<EOF>>  { return EOF_TOK; }