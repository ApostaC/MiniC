#ifndef _STATEMENT_HH_
#define _STATEMENT_HH_

#include "common.h"
#include "Symbols.h"
#include "Express.h"

#define DBG_FUNC(name)  \
    if(debug) std::cerr<<KRED "//In " <<name<< KNRM <<std::endl;

class Stmt;
class NullStmt;

class ExprStmt;
class IfStmt;
class WhileStmt;
class ReturnStmt;
class CompoundStmt;
class FuncBodyStmt;

class VardeclStmt;
class VardefnStmt;
class ArrdeclStmt;
class ArrdefnStmt;
class FuncdefnStmt;

class LabelCounter;
extern LabelCounter _lcnter;

/**
 * @function: gencode(file)
 * generate the asm code to the file
 *
 * @function retCheck(expected_type)
 * check if the stmt given the return value of expected type
 * in all return path
 * default returns false
 * @returns true if all path have return value no matter type match
 *          , otherwise false
 */
class Stmt
{
    protected:
        //Expr_Type etype;
        int lineno;
        LabelCounter &lbcnt;
    public:
        Stmt() : lbcnt(_lcnter) {lineno = yylineno;}
        virtual void gencode(FILE *f) const = 0;
        virtual bool retCheck(Expr_Type ext) { return false; }
        virtual ~Stmt() = default;
};

class NullStmt : public Stmt
{
    public:
        NullStmt() = default;
        /* do nothing at gencode */
        virtual void gencode(FILE *f) const override {}
};

class ExprStmt : public Stmt
{
    protected:
        Expr *expr;
    public:
        ExprStmt(Expr *e) : expr(e) {}
        virtual void gencode(FILE *f) const override { DBG_FUNC("ExprStmt"); expr->gencode(f); }
};

class IfStmt : public Stmt
{
    protected:
        Expr *cnd;
        Stmt *thenStmt;
        Stmt *elseStmt;
    public:
        /* elseStmt = NULL --> no else */
        IfStmt(Expr *e, Stmt *t, Stmt *es = NULL)
            : cnd(e), thenStmt(t), elseStmt(es) {}
        virtual void gencode(FILE *f) const override;
        virtual bool retCheck(Expr_Type) override;
};

class WhileStmt : public Stmt
{
    protected:
        Expr *cnd;
        Stmt *body;
    public:
        WhileStmt(Expr *e, Stmt *b) : cnd(e), body(b) {}
        virtual void gencode(FILE *f) const override;
        virtual bool retCheck(Expr_Type) override;
};

class ReturnStmt : public Stmt
{
    protected:
        Expr *ret;
    public:
        /* if ret = NULL --> return (void) */
        ReturnStmt(Expr *e = NULL) : ret(e){}
        virtual void gencode(FILE *f) const override;
        virtual bool retCheck(Expr_Type) override;
};

class CompoundStmt : public Stmt
{
    protected:
        std::vector<Stmt*> *stmts;
    public:
        CompoundStmt(std::vector<Stmt*> *ss) : stmts(ss) {}
        virtual void gencode(FILE *f) const override 
        {
            DBG_FUNC("CompoundStmt");
            for(auto stmt : *stmts) stmt->gencode(f);
        }
        virtual bool retCheck(Expr_Type) override;
};

class FuncBodyStmt : public CompoundStmt
{
    protected:
        /* if stmts is NULL then it's a empty function body */
        std::vector<Stmt*> *stmts;
    public:
        FuncBodyStmt(std::vector<Stmt*> *ss) : CompoundStmt(ss) {}
        virtual void gencode(FILE *f) const override;
        virtual bool retCheck(Expr_Type) override;
};

class VardeclStmt : public Stmt
{
    protected:
        VarSymbol *sym;
    public:
        VardeclStmt(VarSymbol *v) : sym(v) {}
        virtual void gencode(FILE *f) const override
        {
            DBG_FUNC("VardeclStmt");
            Emit(f, sym->gendecl() + "\n");
        }
};

class VardefnStmt : public VardeclStmt
{
    protected:
        Expr *init;
        SymbolTable *table;
    public:
        VardefnStmt(VarSymbol *v, Expr *i, SymbolTable *t) 
            : VardeclStmt(v), init(i), table(t) {}
        virtual void gencode(FILE *f) const override;
};

class ArrdeclStmt : public Stmt
{
    protected:
        ArraySymbol * arr;
    public:
        ArrdeclStmt(ArraySymbol *a) : arr(a) {}
        virtual void gencode(FILE *f) const override
        {
            DBG_FUNC("ArrdeclStmt");
            Emit(f, arr->gendecl() + "\n");
        }
};

class ArrdefnStmt : public ArrdeclStmt
{
    protected:
        std::vector<Expr*> *init;
        SymbolTable *table;
    public:
        ArrdefnStmt(ArraySymbol *a, std::vector<Expr*> *i, SymbolTable *t);
            //: ArrdeclStmt(a), init(i), table(t) {}
        virtual void gencode(FILE *f) const override;
};

class FuncdefnStmt : public Stmt
{
    protected:
        FuncSymbol *func;
        FuncBodyStmt *bd;
    public:
        FuncdefnStmt(FuncSymbol *f, Stmt *b) : func(f) 
        {
            if(INSTANCE_OF(b, FuncBodyStmt))
                bd = dynamic_cast<FuncBodyStmt*>(b);
            else
                EmitError("Unknown Error: FuncdefnStmt.NO_FUNC_BODY");
            bd->retCheck(func->getExprType());
        }
        virtual void gencode(FILE *f) const override
        {
            DBG_FUNC("FuncdefnStmt");
            Emit(f, func->gendecl() + "\n");
            bd->gencode(f);
            Emit(f, "end " + func->gencode() + "\n");
        }
};


class LabelCounter
{
    private:
        int cnt;
        const std::string prefix;
    public:
        LabelCounter(const std::string &pre, int start) 
            : cnt(start), prefix(pre) {}
        std::string nextLabel() {return prefix + std::to_string(cnt++);}
        int getCount() { return cnt; }
};

#undef DBG_FUNC
#endif
