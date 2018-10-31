#include "Express.h"

/* --------------------- UnaryExpr ----------------- */
UnaryExpr::UnaryExpr(Operator_Type op, Expr *expr, SymbolTable *table)
    : Expr(expr->getType(), table),  oper(op), right(expr)
{
}


/* --------------------- BinaryExpr ----------------- */
BinaryExpr::BinaryExpr(Operator_Type ope, Expr *l, Expr *r, SymbolTable *table)
    : Expr(l->getType(), table), oper(ope), left(l), right(r)
{
    /*TODO: type check*/
}

/* --------------------- AssignExpr ----------------- */
AssignExpr::AssignExpr(Expr *l, Expr *r, SymbolTable *table)
    : Expr(l->getType(), table), right(r)
{
    if(INSTANCE_OF(l, AssignAbleExpr))
        left = dynamic_cast<AssignAbleExpr*>(l);
    else
        EmitError("Left Value is not assignable!");
}


/* --------------------- LiteralExpr ----------------- */
LiteralExpr::LiteralExpr(Expr_Type et, int val, SymbolTable *table)
    : Expr(et, table), value(val)
{
    /*TODO: type check? when adding floating type*/
}


/* --------------------- LiteralExpr ----------------- */
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


/* --------------------- GenCode ----------------- */
void UnaryExpr::gencode(FILE *f) const
{
    //TODO:
}
