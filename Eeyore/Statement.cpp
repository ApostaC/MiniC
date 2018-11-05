#include "Statement.h"

#ifndef DBG_FUNC
#define DBG_FUNC(name)  \
    if(debug) std::cerr<<KRED "//In " <<name<< KNRM <<std::endl;
#endif

LabelCounter _lcnter{"l", 0};

static void modify_etype(Expr_Type &in, Expr_Type newt)
{
    if(newt == Expr_Type::INT_TYPE)
        in = newt;

    if(newt == Expr_Type::BOOL_TYPE || newt == Expr_Type::VOID_TYPE)
        if(in == Expr_Type::GARBAGE_TYPE)
            in = newt;
}

/* ----------------------- compile time check --------------------- */

ArrdefnStmt::ArrdefnStmt(ArraySymbol *a, std::vector<Expr*> *i, SymbolTable *t) 
        : ArrdeclStmt(a), init(i), table(t) 
{
    if(init->size() > arr->getLength())
        EmitWarning("Too many value in initializer list! Expected " +
                std::to_string(arr->getLength()) + " But got " + std::to_string(init->size())
                + "; Some value will be ignored ");
    else if (init->size() < arr->getLength())
        EmitWarning("Not enough value in initializer list! Expected " +
                std::to_string(arr->getLength()) + " But got " + std::to_string(init->size()));
}

/* ------------------------- gencode ------------------------- */
void IfStmt::gencode(FILE *f) const
{
    DBG_FUNC("IfStmt");
    /* if cnd goto label1
     *  else stmt...
     * label1:
     *  then stmt
     * normal stmt
     */
    std::string label = this->lbcnt.nextLabel();
    std::string cndname = cnd->gencode(f);
    if(this->elseStmt)
    {
        Emit(f, "if " + cndname + " != 0 goto " + label + "\n"); 
        elseStmt->gencode(f);
        Emit(f, label + ":\n");
        thenStmt->gencode(f);
    }
    /* if not cnd goto label 1
     *  then stmt
     * label1:
     *  normal stmt
     */
    else
    {
        Emit(f, "if " + cndname + " == 0 goto " + label + "\n");
        thenStmt->gencode(f);
        Emit(f, label + ":\n");
    }
}

void WhileStmt::gencode(FILE *f) const
{
    DBG_FUNC("WhileStmt");
    /*label1:
     * if not cond goto label2
     *  while stmt
     *  goto label1
     *label 2
     * normal stmt
     */

    std::string label1 = this->lbcnt.nextLabel();
    std::string label2 = this->lbcnt.nextLabel();

    Emit(f, label1 + ":\n");
    std::string cndname = cnd->gencode(f);
    Emit(f, "if " + cndname + " == 0 goto " + label2 + "\n");
    this->body->gencode(f);
    Emit(f, "goto " + label1 + "\n");
    Emit(f, label2 + ":\n");
}

void FuncBodyStmt::gencode(FILE *f) const
{
    DBG_FUNC("FuncBodyStmt");
    CompoundStmt::gencode(f);
}

void ReturnStmt::gencode(FILE *f) const
{
    DBG_FUNC("ReturnStmt");
    std::string name = this->ret->gencode(f);
    Emit(f, "return " + name + "\n");
}

void VardefnStmt::gencode(FILE *f) const
{
    DBG_FUNC("VardefnStmt");
    /* call father's gencode first */
    /* then treat as assign expr */
    VardeclStmt::gencode(f);
    AssignExpr ae(sym->getIdentifier(), init, table);
    ae.gencode(f);
}

void ArrdefnStmt::gencode(FILE *f) const
{
    DBG_FUNC("ArrdefnStmt");
    /* call father's gencode first */
    /* then treat as a lot of assign expr */   
    ArrdeclStmt::gencode(f);
    /* Do length check */
    
    auto size = std::min(init->size(), arr->getLength());
    for(size_t i = 0; i < size; ++i)
    {
        LiteralExpr off(Expr_Type::INT_TYPE, i, table);
        ArrayExpr ae(arr, &off, table);
        AssignExpr(&ae, init->at(i), this->table).gencode(f);
    }
                
}
