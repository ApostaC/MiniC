#include <iostream>
#include "common.h"
#include "Symbols.h"
#include "Statement.h"

extern int yyparse();
extern SymbolCounter global_counter;
extern SymbolTable *currentTable;

std::vector<SymbolTable*> tables = {currentTable};

void NewScope();
void EndScope();
void CleanUp();
void Generate(FILE *f, std::vector<Stmt*> *list);

int debug;
int main(int argc, char *argv[])
{
    debug = 0;
    auto ret = yyparse();
    //printf("ret = %d\n", ret);

    //for(auto table : tables)
    //    table->print(std::cout);
    //global_counter.print(std::cout);
    //CleanUp();
}

void NewScope()
{
    currentTable = new SymbolTable(currentTable, global_counter);
    tables.push_back(currentTable);
}

void EndScope()
{
    auto outdatedScope = currentTable;
    if(!currentTable->getParent())
        yyerror("Unknown: Try to escape to a unexisted scope");
    currentTable = currentTable->getParent();
    /* cannot delete the outdatedScope because the symbols will 
     * still be used after the scope is expired */
    //delete outdatedScope;
}

void CleanUp()
{
    for(auto table : tables)
        delete table;
}

void Generate(FILE *f, std::vector<Stmt*> *list)
{
    for(auto stmt : *list)
        stmt->gencode(f);
    //Some clean Up?
}
