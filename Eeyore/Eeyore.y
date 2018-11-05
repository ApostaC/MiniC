%{
#include <iostream>
#include "common.h"
#include "Symbols.h"
#include "Express.h"
#include "Statement.h"


SymbolCounter global_counter;
SymbolTable *currentTable = new SymbolTable(NULL, global_counter);

void NewScope();
void EndScope();
void Generate(FILE *f, std::vector<Stmt*> *list);
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
%type <expr> expr literal array_expr
%type <stmt> stmt var_decl var_defn func_decl compound_stmt return_stmt while_stmt if_stmt func_defn function_stmt decl decl_stmt expr_stmt
%type <func> func_head
%type <typeList> param_list params
%type <stmtList> stmt_list decl_list
%type <exprList> arg_list args expr_list


%nonassoc NO_ELSE
%nonassoc ELSE

%right '='
%left OR
%left AND
%nonassoc LE '<' '>' GE EQ NE
%left '+' '-'
%left '*' '/' '%'
%right MINUS INC DEC NOT

%%

program     :   decl_list       {
                                    yydebug(KGRN "-- COMPILE FINISHED !!! --\n" KNRM); 
                                    Generate(stdout, $1);
                                }
            ;

decl_list   :   decl_list decl  { $$->push_back($2); }
            |   decl            { $$ = new std::vector<Stmt*>(); $$->push_back($1);}
            ;

decl        :   func_decl
            |   func_defn
            |   var_decl
            |   var_defn
            ;

var_decl    :   type IDENTIFIER ';'
                { 
                    currentTable->insertVariableSymbol($2, $1, Code_Type::T);
                    $$ = new VardeclStmt(currentTable->getVariableSymbol($2));
                }
            |   type IDENTIFIER '[' INT_LITERAL ']' ';'
                {
                    currentTable->insertArraySymbol($2, $1, Code_Type::T, $4);
                    $$ = new ArrdeclStmt(currentTable->getArraySymbol($2));
                }
            ;

var_defn    :   type IDENTIFIER '=' expr ';'
                { 
                    currentTable->insertVariableSymbol($2, $1, Code_Type::T);
                    $$ = new VardefnStmt(currentTable->getVariableSymbol($2), $4, currentTable);
                }

            |   type IDENTIFIER '[' INT_LITERAL ']' '=' '{' expr_list '}' ';'
                { 
                    currentTable->insertArraySymbol($2, $1, Code_Type::T, $4);
                    $$ = new ArrdefnStmt(currentTable->getArraySymbol($2), $8, currentTable);
                }

expr_list   :   { $$ = new std::vector<Expr *>(); }
            |   expr_list ',' expr  { $$ = $1; $$->push_back($3); }
            |   expr                { $$ = new std::vector<Expr *>(); $$->push_back($1); }
            ;

func_head   :   type IDENTIFIER     { NewScope(); yydebug("Start new scope! (func)");}
                '(' params ')'      { $$ = new FuncSymbol($2, $1, $5, false); }

func_decl   :   func_head ';'
                {
                    currentTable->getParent()->insertFunctionSymbol($1->getIdentifier(), $1->getExprType(), $1->getParamType(), false);
                    EndScope(); yydebug("scope escaped (func-decl)");
                    global_counter.cleanFunctionParam();
                    $$ = new NullStmt();
                }
            ;

func_defn   :   func_head 
                { 
                    currentTable->getParent()->insertFunctionSymbol($1->getIdentifier(), $1->getExprType(), $1->getParamType(), true);
                }
                function_stmt
                { 
                    EndScope(); yydebug("scope escaped! (func-defn)");
                    global_counter.cleanFunctionParam();
                    $$ = new FuncdefnStmt(currentTable->getFunctionSymbol($1->getIdentifier()),$3);
                }
            ;

params      :                           { $$ = new std::vector<Expr_Type>(); }
            |   param_list              { $$ = $1; }
            ;

param_list  :   param_list ',' param    { $$ = $1; $$->push_back($3); }
            |   param                   { $$ = new std::vector<Expr_Type>(); $$->push_back($1); }
            ;

param       :   type IDENTIFIER         
                { $$ = $1; currentTable->insertVariableSymbol($2, $1, Code_Type::p); yydebug("Add param variable to SymbolTable");}
            |   type IDENTIFIER '[' INT_LITERAL ']'
                { $$ = $1; currentTable->insertArraySymbol($2, $1, Code_Type::p, $4); yydebug("Add param variable to SymbolTable");}
            ;

stmt_list   :   stmt_list stmt          { $$ = $1; $$->push_back($2); }
            |   stmt                    { $$ = new std::vector<Stmt*>(); $$->push_back($1);}

stmt        :   expr_stmt               { $$ = $1; }
            |   compound_stmt           { $$ = $1; }
            |   if_stmt                 { $$ = $1; }
            |   while_stmt              { $$ = $1; }
            |   return_stmt             { $$ = $1; }
            |   decl_stmt               { $$ = $1; }
            ;

decl_stmt   :   var_decl                { $$ = $1; }
            |   var_defn                { $$ = $1; }
            |   func_decl               { $$ = $1; }
            ;

function_stmt:  '{' stmt_list '}'       { $$ = new FuncBodyStmt($2);}
             |  '{' '}'                 { $$ = new FuncBodyStmt(NULL);}
            ;

compound_stmt:  '{'                     { NewScope(); yydebug("new scope established! (compound)"); }
                stmt_list '}'
                { 
                    EndScope(); yydebug("Scope escaped! (compound)");
                    $$ = new CompoundStmt($3);
                }
             |  '{' '}' { $$ = new NullStmt();}
             ;

expr_stmt   :   expr ';' /*{ printf(KRED " GEN CODE: \n");$1->gencode(stdout);printf(KNRM); }*/
                { $$ = new ExprStmt($1); }
            |   ';'                     { $$ = new NullStmt(); }
            ;

while_stmt  :   WHILE '(' expr ')' stmt { $$ = new WhileStmt($3, $5);}
            ;

return_stmt :   RETURN expr ';'         { $$ = new ReturnStmt($2); }
            /*  disable empty return stmt 
            |   RETURN ';'
        */
            ;

expr        :   IDENTIFIER '=' expr 
                {
                    $$ = new AssignExpr($1, $3, currentTable);
                }
            |   array_expr '=' expr     { $$ = new AssignExpr($1, $3, currentTable); }
            |   expr OR expr            { $$ = new BinaryExpr(Operator_Type::OOR, $1, $3, currentTable); }
            |   expr AND expr           { $$ = new BinaryExpr(Operator_Type::OAND, $1, $3, currentTable); }
            |   expr EQ expr            { $$ = new BinaryExpr(Operator_Type::OEQ, $1, $3, currentTable); }
            |   expr NE expr            { $$ = new BinaryExpr(Operator_Type::ONE, $1, $3, currentTable); }
            |   expr LE expr            { $$ = new BinaryExpr(Operator_Type::OLE, $1, $3, currentTable); }
            |   expr GE expr            { $$ = new BinaryExpr(Operator_Type::OGE, $1, $3, currentTable); }
            |   expr '<' expr           { $$ = new BinaryExpr(Operator_Type::OLT, $1, $3, currentTable); }
            |   expr '>' expr           { $$ = new BinaryExpr(Operator_Type::OGT, $1, $3, currentTable); }
            |   expr '+' expr           { $$ = new BinaryExpr(Operator_Type::OADD, $1, $3, currentTable); }
            |   expr '-' expr           { $$ = new BinaryExpr(Operator_Type::OSUB, $1, $3, currentTable); }
            |   expr '*' expr           { $$ = new BinaryExpr(Operator_Type::OMUL, $1, $3, currentTable); }
            |   expr '/' expr           { $$ = new BinaryExpr(Operator_Type::ODIV, $1, $3, currentTable); }
            |   expr '%' expr           { $$ = new BinaryExpr(Operator_Type::OMOD, $1, $3, currentTable); }
            |   '-' expr %prec MINUS    { $$ = new UnaryExpr(Operator_Type::OSUB, $2, currentTable); }
            |   '!' expr %prec NOT      { $$ = new UnaryExpr(Operator_Type::ONOT, $2, currentTable); }
            |   INC IDENTIFIER          { $$ = new SelfExpr(Operator_Type::OINC, $2, currentTable); }
            |   DEC IDENTIFIER          { $$ = new SelfExpr(Operator_Type::ODEC, $2, currentTable); }
        /* NO UNARY OPERATORS HERE CURRENTLY
            |
            |
            |
        */
            |   '(' expr ')'            { $$ = $2; }
            |   array_expr { $$ = $1; }
            |   IDENTIFIER '(' args ')'   /* function call */
                { $$ = new FuncExpr(currentTable->getFunctionSymbol($1), $3, currentTable); }
            |   IDENTIFIER              { $$ = new IdentExpr(currentTable->getVariableSymbol($1), currentTable); }
            |   literal                 { $$ = $1; }
            ;

array_expr  :   IDENTIFIER '[' expr ']'
                { $$ = new ArrayExpr(currentTable->getArraySymbol($1), $3, currentTable); }
            ;

args        :                           {$$ = new std::vector<Expr*>();}
            |   arg_list                {$$ = $1;}
            ;

arg_list    :   arg_list ',' expr       { $$ = $1; $$->push_back($3); }
            |   expr                    { $$ = new std::vector<Expr*>(); $$->push_back($1); }
            ;


if_stmt     :   IF '(' expr ')' stmt %prec NO_ELSE 
                { $$ = new IfStmt($3, $5); }
            |   IF '(' expr ')' stmt ELSE stmt 
                { $$ = new IfStmt($3, $5, $7);}
            ;

literal     :   INT_LITERAL         /* only provide int literals currently */
                { $$ = new LiteralExpr(Expr_Type::INT_TYPE, $1, currentTable);}
            |   BOOL_LITERAL
                { $$ = new LiteralExpr(Expr_Type::BOOL_TYPE, $1, currentTable);}
            ;

type        :   TOKINT              /* only provide int type currently */
                { $$ = $1; }
            |   TOKBOOL
                { $$ = $1; }
            |   TOKVOID
                { $$ = $1; }

%%

void yyerror(const char *s)
{
    fprintf(stderr, "Line: %d -- %s\n", yylineno, s);
}

void yydebug(const char *s)
{
    if(debug)
        fprintf(stderr, "(DEBUG) Line: %d -- %s\n", yylineno, s);
}

void EmitError(const std::string &s)
{
    yyerror(s.c_str());
    exit(-1);
}

void EmitWarning(const std::string &s, int lineno)
{
    if(!lineno) lineno = yylineno;
    fprintf(stderr, "Line: %d -- " BOLD_KYEL "Warning: " KNRM "%s\n", lineno, s.c_str());
}

void Emit(FILE *f, const std::string &s)
{
    fprintf(f, "%s", s.c_str());
}

