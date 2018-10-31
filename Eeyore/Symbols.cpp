#include "Symbols.h"

/* ------------------- FuncSymbol ------------------*/
FuncSymbol::FuncSymbol(const std::string &n, Expr_Type et,
        std::vector<Expr_Type> *params, bool wd) 
    : Symbol(n, et), withDef(wd), args(params) {}

std::string FuncSymbol::gencode()
{
    return "f_" + this->ident;
}

std::vector<Expr_Type> *FuncSymbol::getParamType()
{
    return this->args;
}

size_t FuncSymbol::getParamNum()
{
    return this->args->size();
}

/* ------------------- VarSymbol ------------------*/
VarSymbol::VarSymbol(const std::string &n, Expr_Type t,
        int _i, 
        Code_Type ct, 
        bool isG)
    : Symbol(n, t), id(_i), 
    ctype(ct), 
    _isGlobal(isG) {}

bool VarSymbol::isGlobal() { return this->_isGlobal; }

std::string VarSymbol::gencode()
{
    std::string ret{};
    ret += this->ctype;
    ret.append(std::to_string(id));
    return ret;
}

/* ------------------- SymbolTable ------------------*/
SymbolTable::SymbolTable(SymbolTable *table, SymbolCounter &c)
    : parent(table), counter(c)
{
    /**
     * When finding symbols, we go along the symboltables chain
     * Therefore, there is no need for the symbol inheritance
     * 
     * When creating a compount_stmt, table is the out-scope table
     * When creating a function, table is Global var table
     * Global var table's parent is NULL/nullptr
     */
    if(table)
    {
        this->depth = table->depth + 1;
    }
    else depth = 1;
}

FuncSymbol *SymbolTable::getFunctionSymbol(const std::string &ident)
{
    Symbol *sym = NULL;
    if(symbols.count(ident)) 
    {
        sym = symbols[ident];
        if(INSTANCE_OF(sym, FuncSymbol))
            return dynamic_cast<FuncSymbol*>(sym);
        else 
        {
            errorNotMatch(sym, "callable");
            return NULL;
        }
    }
    else if(parent)
    {
        sym = parent->getFunctionSymbol(ident);
        /* if error occurs in parent, it will return NULL 
         * and the error will NOT be missed
         * so don't print error here
         */
        if(sym)
            return dynamic_cast<FuncSymbol*>(sym);
    }
    errorNotFound(ident);
    return NULL;
}

VarSymbol *SymbolTable::getVariableSymbol(const std::string &ident)
{
    Symbol *sym = NULL;
    if(symbols.count(ident)) 
    {
        sym = symbols[ident];
        if(INSTANCE_OF(sym, VarSymbol))
            return dynamic_cast<VarSymbol*>(sym);
        else 
        {
            errorNotMatch(sym, "a variable");
            return NULL;
        }
    }
    else if(parent)
    {
        sym = parent->getFunctionSymbol(ident);
        /* if error occurs in parent, it will return NULL 
         * and the error will NOT be missed
         * so don't print error here
         */
        if(sym)
            return dynamic_cast<VarSymbol*>(sym);
    }
    errorNotFound(ident);
    return NULL;
}

ArraySymbol *SymbolTable::getArraySymbol(const std::string &ident)
{
    if(symbols.count(ident))
    {
        auto sym = symbols[ident];
        if(INSTANCE_OF(sym, ArraySymbol))
            return dynamic_cast<ArraySymbol*>(sym);
        else
        {
            errorNotMatch(sym, "an array");
            return NULL;
        }
    }
    else if(parent)
    {
        auto sym = parent->getArraySymbol(ident);
        if(sym)
            return sym;
    }
    return NULL;
}

bool SymbolTable::insertFunctionSymbol(const std::string &ident, Expr_Type et,
        std::vector<Expr_Type> *params, bool withDef)
{
    /* redeclaration of a function is ok */
    /* while redifinition of a function is not ok */
    /* However, redifinition only happens in root */
    /* symbol table, so there is no need for sear-*/
    /* ching along the prototype chain */
    if(withDef && symbols.count(ident))
    {
        errorFoundDup(symbols[ident]);
        return false;
    }
    /* if current scope is not global but found a */
    /* function with definition in the scope, then*/
    /* error occured */
    if(withDef && this->depth!=1)
    {
        EmitError("Cannot define a function inside a function!");        
    }
    return symbols.insert({ident, 
            new FuncSymbol(ident, et, params, withDef)}).second;
}

bool SymbolTable::insertVariableSymbol(const std::string &ident, Expr_Type et,
        Code_Type ct)
{
    if(symbols.count(ident))
        errorFoundDup(symbols[ident]);
    bool isglb = this->depth == 1;
    int id = this->counter.nextID(ct);
    return symbols.insert({ident,
            new VarSymbol(ident, et, id, ct, isglb)}).second;
}

SymbolTable::~SymbolTable()
{
    for(auto ent : symbols)
        delete ent.second;
}

std::string SymbolTable::errorHint(Symbol *sym)
{
    int line = sym->getlineno();
    std::string error {"\n"};
    if(line > 1)
        error += BOLD_KGRN "Note: " KNRM + sym->getIdentifier() + 
            " is defined at line: " + std::to_string(line);
    return error;

}

std::string SymbolTable::errorNotMatch(Symbol *sym, const std::string &hint)
{
    /* report error */
    std::string error = "Symbol " BOLD_KCYN + sym->getIdentifier() 
        + KNRM " is not " + hint ;
    error += errorHint(sym);
    EmitError(error);
    return error;
}

std::string SymbolTable::errorNotFound(const std::string &name)
{
    std::string error = "Symbol " BOLD_KRED + name + KNRM " is not found!";
    EmitError(error);
    return error;
}

std::string SymbolTable::errorFoundDup(Symbol *sym)
{
    std::string error = "Redefinition of the symbol " BOLD_KRED +
        sym->getIdentifier() + KNRM;
    error += errorHint(sym);
    EmitError(error);
    return error;
}

SymbolTable *SymbolTable::getParent()
{
    return this->parent;
}
/* ------------------- SymbolCounter ------------------*/
int SymbolCounter::nextID(Code_Type ct)
{
    auto a = counter[ct];
    counter[ct] += 1;
    return a;
}

std::string SymbolCounter::Genname(Code_Type ct, int id)
{
    std::string ret{};
    ret += ct;
    ret += std::to_string(id);
    return ret;
}

void SymbolCounter::clear()
{
    for(auto & ent : counter) ent.second = 0;
}

void SymbolCounter::cleanFunctionParam()
{
    this->counter[Code_Type::p] = 0;
}

/* -------------------- Print Functions -------------------- */
void Symbol::print(std::ostream &o)
{
    o<<"Symbol Name: " KGRN << this->ident << KNRM <<
        " defined at line " KCYN << this->lineno << KNRM <<std::endl;
}

void FuncSymbol::print(std::ostream &o)
{
    Symbol::print(o);
    o<<"\tSymbol Type: " KBLU "Function Symbol" KNRM << std::endl <<
        "\tReturn Type: " KMAG << (char)this->etype << KNRM <<std::endl <<
        "\tArgs: " KYEL << this->getParamNum() << KNRM ": ";
    for(auto e : *args) o<<(char)e;
    o<<std::endl;
}

void VarSymbol::print(std::ostream &o)
{
    Symbol::print(o);
    std::string glb{};
    if(this->_isGlobal) glb = "Global ";
    o<<"\tSymbol Type: " KBLU << glb << "Variable" KNRM << std::endl <<
        "\tType: " KMAG << (char)this->etype << KNRM << std::endl <<
        "\tAsm Type: " KYEL << (char)this->ctype << KNRM << 
        "; Asm Name: " KYEL << this->gencode() << KNRM << std::endl;
}


void ArraySymbol::print(std::ostream &o)
{
    Symbol::print(o);
    std::string glb{};
    if(this->_isGlobal) glb = "Global ";
    o<<"\tSymbol Type: " KBLU << glb << "Array [" KRED << this->len 
        << KBLU "]" KNRM << std::endl <<
        "\tType: " KMAG << (char)this->etype << KNRM << std::endl <<
        "\tAsm Name: " KYEL << this->gencode() << KNRM << std::endl;
}

void SymbolTable::print(std::ostream &o)
{
    o<<KGRN "======================== SYMBOL TABLE ========================" KNRM <<std::endl <<
        "Depth: " << this->depth << "; Size: " << symbols.size() <<std::endl;
    o<<KRED "  ------------- SYMBOLS START ------------ " KNRM << std::endl;
    for(auto ent : symbols)
        ent.second->print(o);
    o<<KRED "  ------------- SYMBOLS END ------------ " KNRM << std::endl;
    o<<KGRN "======================== TABLE END ========================" KNRM <<std::endl;
}

void SymbolCounter::print(std::ostream &o)
{
    o<<"---------- Symbol Counter Info ----------"<<std::endl;
    for(auto ent:counter)
        o<<(char)ent.first<<" : "<<ent.second << std::endl;
    o<<"---------- Symbol Counter End ----------"<<std::endl;

}
