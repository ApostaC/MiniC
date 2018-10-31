#ifndef _EXPR_HH_
#define _EXPR_HH_

#include <string>
#include "common.h"
#include "Symbols.h"

class Expr;
class UnaryExpr;
class BinaryExpr;
class AssignExpr;
class LiteralExpr;
class FuncExpr;

class AssignAbleExpr;
class IdentExpr;
class ArrayExpr;

enum Operator_Type {
        OADD, OSUB, OMUL, ODIV, OMOD, 
        OGT, OLT, OEQ, ONE, OGE, OLE 
};

std::map<Operator_Type, std::string> type2str {
    {OADD, "+"}, {OSUB, "-"}, {OMUL, "*"}, {ODIV, "/"}, {OMOD, "%"},
    {OGT, ">"}, {OLT, "<"}, {OEQ, "=="}, {ONE, "!="}, {OGE, ">="}, {OLE, "<="}
};

class Expr
{
    protected:
        int lineno = 0;
        Expr_Type etype;
        SymbolTable *table;
        //std::string name;
    public:
        Expr(Expr_Type et, SymbolTable *st) : etype(et), table(st) { lineno = yylineno;}
        Expr_Type getType() const {return etype;}
        virtual void gencode(FILE *f) const = 0;// now debugging for stmt
        virtual ~Expr() = default;
};

class UnaryExpr : public Expr
{
    protected:
        Operator_Type oper;
        Expr *right;
    public:
        UnaryExpr(Operator_Type oper, Expr *expr, SymbolTable *st);
        virtual void gencode(FILE *f) const;
};

class BinaryExpr : public Expr
{
    protected:
        Operator_Type oper;
        Expr *left, *right;
    public:
        BinaryExpr(Operator_Type ope, Expr *l, Expr *r, SymbolTable *st);
        virtual void gencode(FILE *f)const;
};

class AssignExpr : public Expr
{
    protected:
        AssignAbleExpr *left;
        Expr *right;
    public:
        AssignExpr(Expr *l, Expr *r, SymbolTable *st);
        virtual void gencode(FILE *f) const;
};

class LiteralExpr : public Expr
{
    protected:
        int value;
    public:
        LiteralExpr(Expr_Type et, int value, SymbolTable *table);
        virtual void gencode(FILE *f) const;
};


class FuncExpr : public Expr
{
    protected:
        FuncSymbol *func;
        std::vector<Expr *> *params;
    public:
        FuncExpr(FuncSymbol *, std::vector<Expr *>*, SymbolTable *);
};

class AssignAbleExpr : public Expr
{
    /* This class does not contain anything */
    /* It's only for mark an Expr assignable*/
};

class IdentExpr : public AssignAbleExpr
{
};
class ArrayExpr : public AssignAbleExpr
{
};


#endif
