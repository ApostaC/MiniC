#ifndef _COMMON_HH_
#define _COMMON_HH_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>

#define INSTANCE_OF(value, Type) Type * __val = dynamic_cast<Type*>(value)

class Expr;
class Stmt;
class SymbolTable;
class SymbolCounter;

enum Expr_Type { BOOL_TYPE='B', INT_TYPE='I', ARRAY_TYPE='A', VOID_TYPE='V', GARBAGE_TYPE='G'};
enum Code_Type { T = 'T', t = 't', p = 'p'};

extern int yylineno;
extern int debug;



int yylex();
int yyparse(void);
void yyerror(const char* msg);
void yydebug(const char* msg);
void EmitError(const std::string &msg);
extern int currentLabel;
extern SymbolTable* currentSymbolTable;
extern Expr_Type currentReturnType;



extern int getLabel();
extern void emit(FILE* f, char* op);
extern void emit(FILE* f, char* op, int value);
extern void emit(FILE* f, char* op, const char* ident);
extern void emitLabel(FILE* f, int label);
extern void emitGlobal(FILE* f, std::string ident, Expr_Type type);


/* escape code for screen 256 */
#define KNRM  "\x1B[0;0m"
#define KRED  "\x1B[0;31m"
#define KGRN  "\x1B[0;32m"
#define KYEL  "\x1B[0;33m"
#define KBLU  "\x1B[0;34m"
#define KMAG  "\x1B[0;35m"
#define KCYN  "\x1B[0;36m"
#define KWHT  "\x1B[0;37m"
#define BOLD_KNRM  "\x1B[1;30m"
#define BOLD_KRED  "\x1B[1;31m"
#define BOLD_KGRN  "\x1B[1;32m"
#define BOLD_KYEL  "\x1B[1;33m"
#define BOLD_KBLU  "\x1B[1;34m"
#define BOLD_KMAG  "\x1B[1;35m"
#define BOLD_KCYN  "\x1B[1;36m"
#define BOLD_KWHT  "\x1B[1;37m"

#endif
