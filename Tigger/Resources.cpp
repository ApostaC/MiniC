#include <stdexcept>
#include "common.h"
#include "Resources.h"
/* static variables */
namespace res
{
res::VarPool GlobalPool;
res::GlobalVarMgr globalVars;
std::map<RegID, res::Register> globalRegs{
    {-1, "ONSTACK!"},
    {0, "x0"},
        /* s reg: callee saved */
    {1, "s0"}, {2, "s1"}, {3, "s2"},
    {4, "s3"}, {5, "s4"}, {6, "s5"},
    {7, "s6"}, {8, "s7"}, {9, "s8"},
    {10, "s9"}, {11, "s10"}, {12, "s11"},
        /* t reg: caller saved */
    {13, "t0"}, {14, "t1"}, {15, "t2"},
    {16, "t3"}, {17, "t4"}, {18, "t5"}, 
        /* a reg: for parameters */
    {19, "a0"}, {20, "a1"}, {21, "a2"},
    {22, "a3"}, {23, "a4"}, {24, "a5"},
    {25, "a6"}, {26, "a7"}, 
};

/* functions */

RightVariable *RightVariable::GenRvar(const std::string &n)
{
    int value;
    try 
    {
        value = std::stoi(n);
        return new ImmediateVal(value);
    }catch(...)
    {
        return res::GlobalPool.GetVar(n);
    }
}

EeyoreVariable *EeyoreVariable::GenRvar(const std::string &n)
{
    return res::GlobalPool.GetVar(n);
}

/* GLBVAR Manager */
bool GlobalVarMgr::isGlobalVar(res::EeyoreVariable *var)
{
    if(pool.count(var->getName())) return true;
    return false;
}

std::string GlobalVarMgr::GetVarName(res::EeyoreVariable *var)
{
    if(!isGlobalVar(var))
    {
        fprintf(stderr, "Error: Cannot find global var of %s (forget Call ABI?)\n",
                var->getName().c_str());
        return "";
    }
    else return pool[var->getName()]->getName();
}

std::vector<res::EeyoreVariable*> GlobalVarMgr::AllEVars()
{
    std::vector<res::EeyoreVariable*> ret;
    for(auto &name : this->pool)
        ret.push_back(GlobalPool.GetVar(name.first));
    return ret;
}

void GlobalVarMgr::AllocGlobalArr(FILE *f, res::EeyoreVariable *var, int size)
{
    if(isGlobalVar(var))
        throw std::runtime_error("Cannot allocate twice for var: " + var->getName());
    TiggerVariable *tg = new TiggerVariable(_genname(), size, true);
    pool[var->getName()] = tg;
    Emit(f, tg->getName() + " = malloc " + std::to_string(size) + "\n");
    varcnt++;
}

void GlobalVarMgr::AllocGlobalVar(FILE *f, res::EeyoreVariable *var, int initval)
{
    if(isGlobalVar(var))
        throw std::runtime_error("Cannot allocate twice for var: " + var->getName());
    TiggerVariable *tg = new TiggerVariable(_genname(), 4, false);
    pool[var->getName()] = tg;
    Emit(f, tg->getName() + " = " + std::to_string(initval) + "\n");
    varcnt++;

}

void GlobalVarMgr::gencode_loadvar(FILE *f, res::EeyoreVariable *var, const std::string &regname)
{
    if(!isGlobalVar(var))
        throw std::runtime_error("Not a global var: " + var->getName());
    auto tg = pool[var->getName()];
    if(tg->isArray())
        Emit(f, "loadaddr ");
    else Emit(f, "load ");
    Emit(f, tg->getName() + " " + regname + "\n");
}

void GlobalVarMgr::gencode_storevar(FILE *f, res::EeyoreVariable *var, int offsetreg,
        int valreg)
{
    if(!isGlobalVar(var))
        throw std::runtime_error("Not a global var: " + var->getName());
    auto tg = pool[var->getName()];
    Emit(f, "loadaddr " + tg->getName() + " t6 // Store global\n");
    Emit(f, "t6 = t6 + " + globalRegs[offsetreg] + " // Store global\n");
    Emit(f, "t6 [0] = " + globalRegs[valreg] + " //store global\n");
    
}

} //namespace res;
