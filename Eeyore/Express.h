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
        OGT, OLT, OEQ, ONE, OGE, OLE,
        OAND, OOR
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

        /* Gencode: output something to the file and return the name of the 
         * temporary variable of this expression */
        virtual std::string gencode(FILE *f) const = 0;

        virtual ~Expr() = default;
};

class UnaryExpr : public Expr
{
    protected:
        Operator_Type oper;
        Expr *right;
    public:
        UnaryExpr(Operator_Type oper, Expr *expr, SymbolTable *st);
        virtual std::string gencode(FILE *f) const;
};

class BinaryExpr : public Expr
{
    protected:
        Operator_Type oper;
        Expr *left, *right;
    public:
        BinaryExpr(Operator_Type ope, Expr *l, Expr *r, SymbolTable *st);
        virtual std::string gencode(FILE *f)const;
};

class AssignExpr : public Expr
{
    protected:
        AssignAbleExpr *left;
        Expr *right;
    public:
        AssignExpr(const std::string &id, Expr *, SymbolTable*);
        AssignExpr(Expr *l, Expr *r, SymbolTable *st);
        virtual std::string gencode(FILE *f) const;
};

class LiteralExpr : public Expr
{
    protected:
        int value;
    public:
        int getValue()const {return this->value;}
        LiteralExpr(Expr_Type et, int value, SymbolTable *table);
        virtual std::string gencode(FILE *f) const;
};


class FuncExpr : public Expr
{
    protected:
        FuncSymbol *func;
        std::vector<Expr *> *params;
    public:
        FuncExpr(FuncSymbol *, std::vector<Expr *>*, SymbolTable *);
        virtual std::string gencode(FILE *f) const;
};

class AssignAbleExpr : public Expr
{
    /* This class does not contain anything */
    /* It's only for mark an Expr assignable*/
    public:
        AssignAbleExpr(Expr_Type et, SymbolTable *t) : Expr(et, t){}
};

class IdentExpr : public AssignAbleExpr
{
    protected:
        VarSymbol *var;
    public:
        IdentExpr(VarSymbol *, SymbolTable *);
        virtual std::string gencode(FILE *f) const;
};

class ArrayExpr : public AssignAbleExpr
{
    protected:
        ArraySymbol *arr;
        Expr *offset;
        bool _isLeft;    /* different position have different action */
    public:
        ArrayExpr(ArraySymbol *, Expr *offset, SymbolTable *);
        void setPosition(bool isl){_isLeft = isl;}
        bool isLeft(){return _isLeft;}
        virtual std::string gencode(FILE *f) const;
};


#endif
