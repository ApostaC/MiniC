#ifndef _SYMBOLS_HH_
#define _SYMBOLS_HH_

#include "common.h"

class Symbol;
class FuncSymbol;
class VarSymbol;
class SymbolTable;
class SymbolCounter;

class Symbol
{
    protected:
        int lineno = 0; /* definition line */
        std::string ident = "";
        Expr_Type etype = GARBAGE_TYPE;
    public:
        inline bool operator<(const Symbol & r) const
        {
            return ident < r.ident;
        }

        virtual ~Symbol() = default;
        Symbol(const std::string &n, Expr_Type et) : ident(n), etype(et) {this->lineno = yylineno;}
        void setlineno(int ln){lineno = ln;}
        int getlineno(){return lineno;}
        Expr_Type getExprType(){return etype;}
        virtual std::string getIdentifier(){return ident;}
        virtual std::string gencode() = 0;
        virtual void print(std::ostream &o);
};

class FuncSymbol : public Symbol
{
    protected:
        bool withDef;
        std::vector<Expr_Type> *args;
    public:
        FuncSymbol(const std::string &, Expr_Type et, 
                std::vector<Expr_Type> *params, bool wd);
        size_t getParamNum();
        std::vector<Expr_Type> *getParamType();
        virtual std::string gencode() override;
        virtual void print(std::ostream & o) override;
        virtual ~FuncSymbol() = default;
};


class VarSymbol : public Symbol
{
    protected:
        int id;
        Code_Type ctype;
        bool _isGlobal;
    public:
        VarSymbol(const std::string &, Expr_Type t, 
                int _i, Code_Type ct, bool isG);
        bool isGlobal();
        virtual std::string gencode() override;
        virtual void print(std::ostream & o) override;
        virtual ~VarSymbol() = default;
};

class ArraySymbol : public VarSymbol
{
    protected:
        size_t len;
    public:
        size_t getLength()const{return len;}
        virtual std::string gencode() override;
        virtual void print(std::ostream &o) override;
};

class SymbolTable
{
    protected:
        int depth; /* depth = 1 means root (global) sym table */
        SymbolTable *parent;
        std::map<std::string, Symbol*> symbols;
        SymbolCounter &counter;
    public:
        SymbolTable(SymbolTable *p, SymbolCounter &c);

        FuncSymbol *getFunctionSymbol(const std::string &ident);
        VarSymbol *getVariableSymbol(const std::string &ident);
        ArraySymbol *getArraySymbol(const std::string &ident);

        /* return false if there is duplicates in this scope */
        bool insertFunctionSymbol(const std::string &ident, Expr_Type et,
                std::vector<Expr_Type> *params, bool withDef);
        bool insertVariableSymbol(const std::string &ident, Expr_Type et,
                Code_Type ct);
        bool insertArraySymbol(const std::string &ident, Expr_Type et,
                Code_Type ct, size_t len);
        
        SymbolTable *getParent();
        void print(std::ostream &o);
        virtual ~SymbolTable();

    public:
        std::string errorHint(Symbol *); /* used when symbol not match */
        std::string errorNotMatch(Symbol *sym, const std::string &hint);
        std::string errorNotFound(const std::string &name);
        std::string errorFoundDup(Symbol *);

};

class SymbolCounter
{
    private:
        std::map<Code_Type, int> counter;
    public:
        SymbolCounter(){counter.clear();}
        int nextID(Code_Type ct);
        void clear();
        void print(std::ostream &o);
        void cleanFunctionParam();

        static std::string Genname(Code_Type ct, int id);
};

#endif
