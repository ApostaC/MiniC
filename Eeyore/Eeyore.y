%{
#include <iostream>
#include "common.h"
#include "Symbols.h"
#include "Express.h"

SymbolCounter global_counter;
SymbolTable *currentTable = new SymbolTable(NULL, global_counter);

void NewScope();
void EndScope();
class FuncSymbol;
%}

%union
{
    int i;
    char *s;
    bool b;
    Expr_Type type;
    Expr* expr;
    Stmt* stmt;
    FuncSymbol *func;
    std::vector<Expr_Type>* typeList;
    std::vector<Stmt*>* stmtList;
    std::vector<Expr*>* exprList;
};

%token <s> IDENTIFIER
%token IF ELSE WHILE RETURN
%token <i> INT_LITERAL 
%token <b> BOOL_LITERAL
%token LE GE EQ NE '<' '>'
%token OR AND
%token <type> TOKINT TOKBOOL TOKVOID

%type <type> type param
%type <expr> expr literal
%type <stmt> stmt var_decl var_defn func_decl compound_stmt return_stmt while_stmt if_stmt func_defn function_stmt
%type <func> func_head
%type <typeList> param_list params
%type <stmtList> stmt_list decl_list
%type <exprList> arg_list args


%right '='
%left OR
%left AND
%nonassoc LE '<' '>' GE EQ NE
%left '+' '-'
%left '*' '/' '%'
%right UNIARY_OP

%%

program     :   decl_list       {printf(KGRN "-- COMPILE FINISHED !!! --\n" KNRM);}
            ;

decl_list   :   decl_list decl
            |   decl
            ;

decl        :   func_decl
            |   func_defn
            |   var_decl
            |   var_defn
            ;

var_decl    :   type IDENTIFIER ';'
                { 
                    /*TODO:Stmt object*/ currentTable->insertVariableSymbol($2, $1, Code_Type::T);
                }
            ;

var_defn    :   type IDENTIFIER '=' literal ';'
                { 
                    /*TODO:Stmt object*/ currentTable->insertVariableSymbol($2, $1, Code_Type::T);
                }

            |   type IDENTIFIER '[' INT_LITERAL ']' '=' '{' int_list '}' ';'
                { 
                    /*TODO:Stmt object*/ currentTable->insertVariableSymbol($2, $1, Code_Type::T);
                }

int_list    :
            |   int_list ',' INT_LITERAL
            |   INT_LITERAL
            ;

func_head   :   type IDENTIFIER     { NewScope(); yydebug("Start new scope! (func)");}
                '(' params ')'      { $$ = new FuncSymbol($2, $1, $5, false); }

func_decl   :   func_head ';'
                {
                    currentTable->getParent()->insertFunctionSymbol($1->getIdentifier(), $1->getExprType(), $1->getParamType(), false);
                    EndScope(); yydebug("scope escaped (func-decl)");
                    global_counter.cleanFunctionParam();
                }
            ;

func_defn   :   func_head 
                { currentTable->getParent()->insertFunctionSymbol($1->getIdentifier(), $1->getExprType(), $1->getParamType(), true);}
                function_stmt
                { /*TODO: stmt*/
                    EndScope(); yydebug("scope escaped! (func-defn)");
                    global_counter.cleanFunctionParam();
                }
            ;

params      :   { $$ = new std::vector<Expr_Type>(); }
            |   param_list { $$ = $1; }
            ;

param_list  :   param_list ',' param    { $$ = $1; $$->push_back($3); }
            |   param                   { $$ = new std::vector<Expr_Type>(); $$->push_back($1); }
            ;

param       :   type IDENTIFIER         { $$ = $1; currentTable->insertVariableSymbol($2, $1, Code_Type::p); yydebug("Add param variable to SymbolTable");}
            ;

stmt_list   :   stmt_list stmt
            |   stmt

stmt        :   expr_stmt
            |   compound_stmt
            |   if_stmt
            |   while_stmt
            |   return_stmt
            |   decl_stmt   
            ;

decl_stmt   :   var_decl    {
                    /*TODO: add Stmt object for all directions here!*/
                    /*TODO: EmptyStmt for var_decl and func_decl */           
                }
            |   var_defn
            |   func_decl
            ;

function_stmt:  '{' stmt_list {/*TODO: stmt here */} '}'
            ;

compound_stmt:  '{' { NewScope(); yydebug("new scope established! (compound)"); }
                stmt_list 
                '}' { /*TODO: stmt here*/ EndScope(); yydebug("Scope escaped! (compound)");}
             ;

expr_stmt   :   expr ';'
            |   ';'
            ;

while_stmt  :   WHILE '(' expr ')' stmt
            ;

return_stmt :   RETURN expr ';'
            /*  disable empty return stmt 
            |   RETURN ';'
        */
            ;

expr        :   IDENTIFIER '=' expr 
                {
                    /*TODO: deal with initialize params for ALL of Expr object*/
                    $$ = new AssignExpr();
                }
            |   expr OR expr            { $$ = new BinaryExpr(); }
            |   expr AND expr           { $$ = new BinaryExpr(); }
            |   expr EQ expr            { $$ = new BinaryExpr(); }
            |   expr NE expr            { $$ = new BinaryExpr(); }
            |   expr LE expr            { $$ = new BinaryExpr(); }
            |   expr GE expr            { $$ = new BinaryExpr(); }
            |   expr '<' expr           { $$ = new BinaryExpr(); }
            |   expr '>' expr           { $$ = new BinaryExpr(); }
            |   expr '+' expr           { $$ = new BinaryExpr(); }
            |   expr '-' expr           { $$ = new BinaryExpr(); }
            |   expr '*' expr           { $$ = new BinaryExpr(); }
            |   expr '/' expr           { $$ = new BinaryExpr(); }
            |   expr '%' expr           { $$ = new BinaryExpr(); }
        /* NO UNARY OPERATORS HERE CURRENTLY
            |
            |
            |
        */
            |   '(' expr ')'            { $$ = $2; }
            |   IDENTIFIER '[' expr ']' { $$ = new ArrayExpr(); }
            |   IDENTIFIER              { $$ = new IdentExpr(); }
            |   IDENTIFIER '(' args ')'   /* function call */
                { $$ = new FuncExpr(); }
            |   literal                 { $$ = $1; }
            ;

args        :               {$$ = new std::vector<Expr*>();}
            |   arg_list    {$$ = $1;}
            ;

arg_list    :   arg_list ',' expr   { $$ = $1; $$->push_back($3); }
            |   expr                { $$ = new std::vector<Expr*>(); $$->push_back($1); }
            ;


if_stmt     :   IF '(' expr ')' stmt
            |   IF '(' expr ')' stmt ELSE stmt 
            ;

literal     :   INT_LITERAL         /* only provide int literals currently */
                { $$ = new LiteralExpr();}
            ;

type        :   TOKINT              /* only provide int type currently */
                { $$ = $1; }

%%

void yyerror(const char *s)
{
    printf("Line: %d -- %s\n", yylineno, s);
}

void yydebug(const char *s)
{
    printf("(DEBUG) Line: %d -- %s\n", yylineno, s);
}

void EmitError(const std::string &s)
{
    yyerror(s.c_str());
    exit(-1);
}
