%{
#include <iostream>
#include "common.h"
#include "Symbols.h"
#include "Eeyore.tab.hpp"

#define printf(x) printf(x)     // MASK FOR PRINTF

void yyerror(const char *);
void ignoreComments();
%}

DIGIT [0-9]
CHARA [A-Za-z]
NUM -?[1-9][0-9]*|0

%s COMMENT

%%

 /* Comments */
\/\/ {BEGIN COMMENT; printf("Enter Comment State\n");}
<COMMENT>. {}
<COMMENT>\n {BEGIN 0; printf("COMMENT end\n");}
"/*"			{ignoreComments(); } /* slash star style comments  */

 /* Single Char Tokens */
[\(\)\{\}\;\,\+\-\*\/\%\<\>\=\!] 	{return yytext[0];}

 /* Double Char Tokens */
\<\= 			{return LE;}
\>\=			{return GE;}
\=\= 			{return EQ;}
\!\= 			{return NE;}
\|\| 			{return OR;}
\&\& 			{return AND;}

 /* Reserved Words */
int                     {yylval.type = Expr_Type::INT_TYPE; return TOKINT;}
void                    {yylval.type = Expr_Type::VOID_TYPE; return TOKVOID;}
bool                    {yylval.type = Expr_Type::BOOL_TYPE; return TOKBOOL;}

if                      {return IF;}
else                    {return ELSE;}
while                   {return WHILE;}
return                  {return RETURN;}

 /* Literals */
true                    {yylval.i = 1; return BOOL_LITERAL;}
false                   {yylval.i = 0; return BOOL_LITERAL;}
{NUM}                   {yylval.i = atoi(yytext); return INT_LITERAL;}

 /* Identifier */
[a-zA-Z_][a-zA-Z_0-9]*  {yylval.s = strdup(yytext); return IDENTIFIER; }

 /* Eat Whitespace */
[ \t\r]+                {;} 
[\n]			{yylineno++;}

.               {char buf[100] = "Unrecognized Token:     "; buf[20] = yytext[0]; yyerror(buf);}


%%

#undef printf
void ignoreComments()
{
    register char c;
    while(1)
    {
        while((c = getchar()) != '*' && c!=EOF);
        if(c == '*')
        {
            while((c = getchar()) == '*');
            if(c == '/') break;
        }
        if(c == EOF)
        {
            yyerror("Found EOF in comment");
            break;
        }
    }
}

int yywrap()
{
    yylineno = 1;
    return 1;
}

