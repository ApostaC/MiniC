#include "Express.h"

std::map<Operator_Type, std::string> type2str {
    {OADD, "+"}, {OSUB, "-"}, {OMUL, "*"}, {ODIV, "/"}, {OMOD, "%"},
    {OGT, ">"}, {OLT, "<"}, {OEQ, "=="}, {ONE, "!="}, {OGE, ">="}, {OLE, "<="},
    {OAND, "&&"}, {OOR, "||"}, {OINC, "+"}, {ODEC, "-"}, {ONOT, "!"}
};

std::map<Expr_Type, int> type2len{
    {INT_TYPE, 4}, {BOOL_TYPE, 2}, {VOID_TYPE, 0}
};
/* --------------------- UnaryExpr ----------------- */
UnaryExpr::UnaryExpr(Operator_Type op, Expr *expr, SymbolTable *table)
    : Expr(expr->getType(), table),  oper(op), right(expr)
{
}


/* --------------------- BinaryExpr ----------------- */
BinaryExpr::BinaryExpr(Operator_Type ope, Expr *l, Expr *r, SymbolTable *table)
    : Expr(l->getType(), table), oper(ope), left(l), right(r)
{
    if(l->getType() == Expr_Type::VOID_TYPE 
            || r->getType() == Expr_Type::VOID_TYPE)
        EmitError("Expression argument with void type found!");
    if(l->getType() == Expr_Type::INT_TYPE 
            || r->getType() == Expr_Type::INT_TYPE)
        this->etype = Expr_Type::INT_TYPE;
}

/* --------------------- SelfExpr ----------------- */
SelfExpr::SelfExpr(Operator_Type op, VarSymbol *var, SymbolTable *table)
    : Expr(var->getExprType(), table), oper(op), sym(var)
{
    if(!isInt())
        EmitError("Self operator can only used on int variables");
    if(INSTANCE_OF(var, ArraySymbol))
        EmitError("Cannot use " + type2str[op] + " on array");
    switch(op)
    {
        case OINC:
        case ODEC:
            break;
        default:
            EmitError("Invalid usage of an operator, only ++ or -- is permitted");
    }
}

SelfExpr::SelfExpr(Operator_Type op, const std::string &id, SymbolTable *table)
    : SelfExpr(op, table->getVariableSymbol(id), table)
{
}

/* --------------------- AssignExpr ----------------- */
AssignExpr::AssignExpr(Expr *l, Expr *r, SymbolTable *table)
    : Expr(l->getType(), table), right(r)
{
    /* TYPE CHECK */
    if(r->isVoid())
        EmitError("Cannot assign a void value to others");
    if(l->isBool() && r->isInt())
        EmitWarning("Type conversion with narrowing: int -> bool");

    if(INSTANCE_OF(l, AssignAbleExpr))
        left = dynamic_cast<AssignAbleExpr*>(l);
    else
        EmitError("Left Value is not assignable!");

    if(INSTANCE_OF(l, ArrayExpr))
        dynamic_cast<ArrayExpr*>(l)->setPosition(true); // array on left
}

AssignExpr::AssignExpr(const std::string &name, Expr *r, SymbolTable *t)
    : Expr(r->getType(), t), left(NULL), right(r)
{
    /* TODO: typecheck! */

//    if(INSTANCE_OF(r, FuncSymbol))
//        EmitError("Cannot assign function to other var");

    auto sym = t->getVariableSymbol(name);
    if(INSTANCE_OF(sym, ArraySymbol))
    {
        EmitError("Left Value: " BOLD_KCYN "(array type)" KNRM
                " is not assignable");
    }
    else if (INSTANCE_OF(sym, VarSymbol))
    {
        /* it's normal var */
        if(INSTANCE_OF(r, ArraySymbol))
            EmitError("Cannot assign an array to a normal var");
        else this->left = new IdentExpr(sym, t);
    }
    else
        EmitError("Left Value is not assignable!");
}


/* --------------------- LiteralExpr ----------------- */
LiteralExpr::LiteralExpr(Expr_Type et, int val, SymbolTable *table)
    : Expr(et, table), value(val)
{
    if(et == Expr_Type::BOOL_TYPE && value)
        value = 1;      /* change truthy value to 1 */
}


/* --------------------- FuncExpr ----------------- */
FuncExpr::FuncExpr(FuncSymbol *f, std::vector<Expr*> *ps, SymbolTable *t)
    : Expr(f->getExprType(), t), func(f), params(ps)
{
    if(ps->size() != f->getParamNum()) 
        EmitError("No Matching function call to " + f->getIdentifier() 
                + ": Unmatched parameter count!");

    auto expectType = f->getParamType();
    for(size_t i = 0; i < ps->size(); ++i)
    {
        auto param = (*ps)[i];
        if(param->getType() != expectType->at(i))
            EmitError("No Matching function call to " + f->getIdentifier() 
                    + ": Unmatched parameter type for parameter " + std::to_string(i+1));
    }
}

/* --------------------- IdentExpr ----------------- */
IdentExpr::IdentExpr(VarSymbol *v, SymbolTable *t)
    : AssignAbleExpr(v->getExprType(), t), var(v)
{
}


/* --------------------- ArrayExpr ----------------- */
ArrayExpr::ArrayExpr(ArraySymbol *a, Expr *off, SymbolTable *t)
    : AssignAbleExpr(a->getExprType(), t), arr(a), offset(off), _isLeft(false)
{
    /* compile time range check */
    if(INSTANCE_OF(off, LiteralExpr))
    {
        int val = dynamic_cast<LiteralExpr*>(off)->getValue();
        if(val < 0)
            EmitWarning("Negative array subscription offset", this->lineno);
        else if(a->getLength() <= (size_t)val)
            EmitWarning("Array subscription index may be larger than length", this->lineno);
    }
}

/* --------------------- GenCode ----------------- */

std::string UnaryExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating UnaryExpr code"<<std::endl;
    auto rightn = this->right->gencode(f);
    /* get a temp symbol to store the result */
    Symbol *tempSym = this->table->generateNextTempVar(this->getType());
    std::string ret = tempSym->gencode();
    Emit(f, tempSym->gendecl() + "\n");
    Emit(f, ret + " = " + type2str[this->oper] + rightn + "\n");
    delete tempSym;
    return ret;
    //TODO:
}

std::string BinaryExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating BinaryExpr code"<<std::endl;
    auto leftn = this->left->gencode(f);
    auto rightn = this->right->gencode(f);
    /* get a temp symbol to store the result */
    Symbol *tempSym = this->table->generateNextTempVar(this->getType());
    std::string ret = tempSym->gencode();
    Emit(f, tempSym->gendecl() + "\n");
    Emit(f, ret + " = " +  leftn + " " + type2str[this->oper] + " "  + rightn + "\n");
    delete tempSym;
    return ret;
    //TODO:
}

std::string AssignExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating Assign expression code"<<std::endl;
    std::string leftn = left->gencode(f);
    std::string rightn = right->gencode(f);
    Emit(f, leftn + " = " + rightn + "\n");
    return "";
    //TODO
}

std::string LiteralExpr::gencode(FILE *f) const
{
    /* for a literal, we dont emit a asm decl
     * only return it's literal value as it's name
     */ 
    if(debug)
        std::cerr<<"//Generating Literal code"<<std::endl;
    return std::to_string(this->value);
    //TODO
}

std::string FuncExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating FuncExpr code"<<std::endl;
    /* get a temp name and emit the decl*/
    VarSymbol *tempSym = this->table->generateNextTempVar(this->getType());
    Emit(f, tempSym->gendecl() + "\n");
    std::string ret{tempSym->gencode()};
    /* set the param in asm and call the function */
    std::vector<std::string> param_name;
    for(auto pexpr : *(this->params))
        param_name.push_back(pexpr->gencode(f));
    for(auto name : param_name)
        Emit(f, "param " + name + "\n");
    Emit(f, ret + " = call " + func->gencode() + "\n");
    delete tempSym;
    return ret;
    //TODO
}

std::string SelfExpr::gencode(FILE *f) const
{
    std::string ret = this->sym->gencode();
    Emit(f, ret + " = " + ret + " " + type2str[oper] + " 1\n");
    return ret;
}

std::string IdentExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating IdentExpr code"<<std::endl;
    /* a declared var has already had its code
     * at decl_stmt, so don't emit here
     * Only return the asm name of the identifier
     */
    return var->gencode();
}

std::string ArrayExpr::gencode(FILE *f) const
{
    if(debug)
        std::cerr<<"//Generating ArrayExpr code"<<std::endl;
    /* emit the offset expression first */
    /* offname will be like 't32' for expression and '15' for literal 15 */
    /* generate a simple binary expr to cal and store the offset value */
    LiteralExpr lit(Expr_Type::INT_TYPE, type2len[this->getType()], this->table);
    BinaryExpr be(Operator_Type::OMUL, &lit, this->offset, this->table);

    auto offname = be.gencode(f);//ae.gencode(f);//offset->gencode(f);    
    //std::cerr<<"Got name: "<<offname<<std::endl;
    /* use a new symbol to store the 'size*offset' */
    if(this->_isLeft)
    {
        std::string emit{};
        emit += arr->gencode() + " [" + offname + "]";   
        /* ArrExpr as left val don't need temp var to store its value */
        return emit;
    }
    else 
    {
        /* generate a new temp value to store the array expr */
        /* var t3; t3 = T2[3] */
        VarSymbol *tempSym = this->table->generateNextTempVar(this->getType());
        std::string ret{tempSym->gencode()};
        Emit(f, tempSym->gendecl() + "\n");
        Emit(f, ret + " = " + arr->gencode() + " [" + offname + "]\n");
        delete tempSym;
        return ret;
        /* TODO: unfinished here! */
    }
}
