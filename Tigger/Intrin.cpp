#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include "common.h"
#include "Intrin.h"


#define DBG_PRINT(name) \
    if(global_debug_flag)   \
        std::cerr<<"Debug: "<<name<<std::endl;  \

#define DBP_FUNC \
    virtual void dbg_print() const override

namespace intr
{
/* Intrin's derived class definition */

int Intrin::getPrevLineno() const 
{
    Intrin *prev = f->getPrevIntrin(this);
    if(prev) return prev->getline();
    else return 0;
    //return f->getPrevIntrin(this)->getline();
}

class Ilabel : public Intrin
{
    public:
        /* LABEL: */
        std::string name;
    public:
        std::string getname(){return name;}
        Ilabel(std::vector<std::string> &n, int lineno, Function *m) 
            : Intrin(lineno,m), name(n[0]) {}
        virtual std::string gencode(FILE *f) const override 
        { 
            fprintf(f, "%s\n", name.c_str());
            return "";
        }
        DBP_FUNC{DBG_PRINT("Ilabel: " + name)}
};

class Ivreturn : public Intrin
{
    public:
    public:
        Ivreturn(std::vector<std::string> &, int lineno, Function *m) : Intrin(lineno, m) {}
        virtual std::string gencode(FILE *f) const override
        {
            this->f->EndFun(f);
            fprintf(f, "return // Ivreturn\n");
            return "";
        }
        DBP_FUNC{DBG_PRINT("Ivreturn")}
};

class Ijmp : public Intrin
{
    public:
        /* ee&ti: goto label */
        std::string label;
    public:
        Ijmp(std::vector<std::string> &vec, int linno, Function *m)
            : Intrin(linno, m), label(vec[1]) {}
        virtual std::string gencode(FILE *f) const override
        {
            fprintf(f, "goto %s\n", label.c_str());
            return "";
        }
        DBP_FUNC{DBG_PRINT("Ijmp to: " + label)}
};

class Ivardef : public Intrin
{
    public:
        /* ee: var varname */
        res::EeyoreVariable *var;
    public:
        Ivardef(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            var = res::EeyoreVariable::GenRvar(vec[1]);
            this->getdef().insert(*var);
        }
        virtual std::string gencode(FILE *f) const override
        {
            /* TODO -- : gencode ? */
            return "";
        }
        DBP_FUNC{
            DBG_PRINT("Ivardef: " + var->getName());
        }
};

class Iendfun : public Intrin
{
    public:
        /* ee: end f_main */
        std::string name;
    public:
        Iendfun(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m), name(vec[1]) {}
        virtual std::string gencode(FILE *f) const override
        {
            /* TODO -- : nothing here ? */
            Emit(f, "end " + name + "\n");
            return "";
        }
        DBP_FUNC{DBG_PRINT("Iendfun" + name)}
};

class Iparam : public Intrin
{
    public:
        /* ee: param t3 */
        res::RVariable *var;   
    public:
        Iparam(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m), var(res::RVariable::GenRvar(vec[1]))
        {
            if(INSTANCE_OF(var, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)var);
        }

        virtual std::string gencode(FILE *f) const override
        {
            // number should come from resource manager
            auto preg = this->f->PrepareParam(f);
            auto vreg = this->f->getReg(f, var, this->getPrevLineno()); //result from last line
            Emit(f, preg + " = " + vreg + "\n");
            return "";
        } 
        DBP_FUNC{
            DBG_PRINT("Iparam: " + var->getName());
        }
};

class Ireturn : public Intrin
{
    public:
        /* ee: return t3 */
        res::RVariable *var;
    public: 
        Ireturn(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m), var(res::RVariable::GenRvar(vec[1]))
        {
            if(INSTANCE_OF(var, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)var);
        }
        virtual std::string gencode(FILE *f)const override
        {
            /* TODO: OPTIMIZATION FOR RETURN A CONTSANT HERE! */ 
            auto reg = this->f->getReg(f, var, getPrevLineno());
            Emit(f, "a0 = " + reg + " // Ireturn\n");
            this->f->EndFun(f);
            Emit(f, "return // Ireturn\n");
            return "";
        }
        DBP_FUNC{
            DBG_PRINT("Ireturn: " + var->getName());
        }
};

class Ifuncst : public Intrin
{
    public:
        std::string name;
        std::string paramcnt;
    public:
        Ifuncst(const std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(0, m), name(vec[0]), paramcnt(vec[1])
        {
            // mark all start function's lineno to 0
            // for getPrevIntrin or something like that
        }
        DBP_FUNC{
            DBG_PRINT("Ifuncst: "+ name + " with " + paramcnt);
        }

        virtual std::string gencode(FILE *f) const override
        {
            auto size = this->f->getStack().GetSlots();
            Emit(f, name + " " + paramcnt + " [" + std::to_string(size) +"]\n");
            this->f->StartFun(f);
            return "";
        }
};

class Icallvfun : public Intrin
{
    public:
        std::string funcname;
    public:
        Icallvfun(std::vector<std::string> &fn, int lineno, Function *m)
            : Intrin(lineno, m), funcname(fn[1])
        {
        }
        DBP_FUNC{
            DBG_PRINT("Icallvfunc: " + funcname);
        }

        virtual std::string gencode(FILE *f) const override
        {
            this->f->PrepareCalling(f, funcname);
            fprintf(f,"call %s\n", funcname.c_str());
            this->f->EndCalling(f, funcname);
            /* restore a0:19 */
            auto offset = this->f->getStack().GetRegAddr(19);
            this->f->getStack().gencode_spillout(f, offset, 19);
            return "";
        }
};

class Iassign : public Intrin
{
    public:
        /* lvar = rvar */
        res::EeyoreVariable *lvar;
        res::RVariable *rvar;
    public:
        Iassign(const std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            rvar = res::RVariable::GenRvar(vec[2]);
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            this->getdef().insert(*lvar);
            using namespace res;
            if(INSTANCE_OF(rvar, EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar);
        }
        DBP_FUNC{
            DBG_PRINT("Iassign: " + lvar->getName() + " <- " + rvar->getName());
        }

        virtual std::string gencode(FILE *f) const override
        {
            auto lreg = this->f->getReg(f, lvar, lineno);
            std::string rreg;
            /* OPTIMIZATION FOR CONSTANT, do not use reg */
            if(INSTANCE_OF(rvar, res::ImmediateVal))
                rreg = rvar->getName();
            else
                rreg = this->f->getReg(f, rvar, getPrevLineno());

            if(lreg != rreg)
                Emit(f, lreg + " = " + rreg + " // Iassign\n");
            if(lreg[0] == 't' || res::globalVars.isGlobalVar(lvar)) // temp reg
            {
                res::RegPool temp;
                this->f->BackToMemory(f, lvar, temp.getRID(lreg));
            }
            return "";
        }
};

class Iarrdec : public Intrin
{
    public:
        res::EeyoreVariable *var;
        int len;
    public:
        Iarrdec(std::vector<std::string> &name, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            var = res::EeyoreVariable::GenRvar(name[2]);
            var->setArray();
            len = std::stoi(name[1]);
            this->getdef().insert(*var);
        }
        DBP_FUNC{
            DBG_PRINT("Iarrdec: " + var->getName() + " of size " + std::to_string(len));
        }
        virtual std::string gencode(FILE *f) const override
        {
            /* TODO -- : resmgr? */
            return "";
        }
        int getlen(){return len;}
};

class Icallfunc : public Intrin
{
    public:
        std::string fname;
        res::EeyoreVariable *lvar;
    public:
        Icallfunc(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m), fname(vec[3])
        {
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            this->getdef().insert(*lvar);   
        }
        DBP_FUNC{
            DBG_PRINT("Icallfunc: " + fname + " -> " + lvar->getName());
        }

        virtual std::string gencode(FILE *f) const override
        {
            this->f->PrepareCalling(f, fname);
            fprintf(f, "call %s\n", fname.c_str());
            auto lreg = this->f->getReg(f, lvar, lineno);
            this->f->EndCalling(f, fname);
            Emit(f, lreg + " = a0 // Icallfunc\n");
            /* restore a0:19 */
            auto offset = this->f->getStack().GetRegAddr(19);
            this->f->getStack().gencode_spillout(f, offset, 19);
            return "";
        }

        Intrin *toCallVFunc()
        {
            std::vector<std::string> vec{"call",this->fname};
            auto ret = new Icallvfun(vec, this->getline(), this->f);
            ret->setcode("call " + this->fname);
            return ret;
        }
};

class Iunaryexpr : public Intrin
{
    public:
        /* ee: lvar = OP1 rvar
         * ti: reg = OP1 reg*/
        res::EeyoreVariable *lvar;
        std::string op;
        res::RVariable *rvar;
    public:
        Iunaryexpr(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            op = vec[2];
            rvar = res::EeyoreVariable::GenRvar(vec[3]);
            this->getdef().insert(*lvar);
            if(INSTANCE_OF(rvar, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar);
        }
        DBP_FUNC{
            DBG_PRINT("Iunaryexpr: " + lvar->getName() + " <- " + op + rvar->getName());
        }
        virtual std::string gencode(FILE *f) const override
        {

            auto lreg = this->f->getReg(f, lvar, lineno);
            std::string rreg;
            rreg = this->f->getReg(f, rvar, getPrevLineno());
            Emit(f, lreg + " = " + op + " " + rreg + " // Iunaryexpr\n");
            if(lreg[0] == 't' || res::globalVars.isGlobalVar(lvar))
            {
                res::RegPool pool;
                this->f->BackToMemory(f, lvar, pool.getRID(lreg));
            }
            return "";
        }
};

class Iaassv : public Intrin
{
    public:
        /* arr [3] = rval */
        res::EeyoreVariable *lvar;
        res::RVariable *offset;
        res::RightVariable *rvar;
    public:
        Iaassv(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            vec[1].pop_back();
            offset = res::RightVariable::GenRvar(vec[1].substr(1));
            rvar = res::RVariable::GenRvar(vec[3]);
            //this->getdef().insert(*lvar);
            this->getuse().insert(*lvar); // we cannot tell whether value of this
                                          // array object will be used in the
                                          // future
            if(INSTANCE_OF(rvar, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar);
            if(INSTANCE_OF(offset, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)offset);
        }
        DBP_FUNC{
            DBG_PRINT("Iaassv: " + lvar->getName()+"["+offset->getName()+"] <- " + rvar->getName());
        }
        virtual std::string gencode(FILE *f) const override
        {
            /* TODO: use resmgr to get 1 or 2 reg */
            auto lreg = this->f->getReg(f, lvar, lineno),
                 rreg = this->f->getReg(f, rvar, getPrevLineno()),
                 oreg = this->f->getReg(f, offset, getPrevLineno());
            res::RegPool pool;
            this->f->BackToMemory(f, lvar, pool.getRID(rreg), pool.getRID(oreg));
            Emit(f, "       // ^ Iaassv\n");
            return "";
        }
};

class Ivassa : public Intrin
{
    public:
        /* lvar = arr [3] */
        /* lvar = rvar [offset] */
        res::EeyoreVariable *lvar;
        res::RVariable *offset;
        res::EeyoreVariable *rvar;
    public:
        Ivassa(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            vec[3].pop_back();  // pop last ']'
            offset = res::RVariable::GenRvar(vec[3].substr(1));
            rvar = res::EeyoreVariable::GenRvar(vec[2]);
            this->getdef().insert(*lvar);
            this->getuse().insert(*rvar);
            if(INSTANCE_OF(offset, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)offset);
        }
        DBP_FUNC{
            DBG_PRINT("Ivassa: " + lvar->getName() + " <- " + rvar->getName() + "[" +
                   offset->getName() +"]");
        }
        
        virtual std::string gencode(FILE *f) const override
        {
            auto lreg = this->f->getReg(f, lvar, lineno),
                 rreg = this->f->getReg(f, rvar, getPrevLineno()),
                 oreg = this->f->getReg(f, offset, getPrevLineno());
            /* load the address into t6 */
            Emit(f, lreg + " = " + rreg + " + " + oreg + " // Ivassa\n");
            Emit(f, lreg + " = " + lreg + " [0] " + " // Ivassa\n");
            if(lreg[0] == 't' || res::globalVars.isGlobalVar(lvar))
            {
                res::RegPool pool;
                this->f->BackToMemory(f, lvar, pool.getRID(lreg));
            }
            return "";
        }
};

class Ibinexpr : public Intrin
{
    public:
        /* lvar = rvar OP2 rvar
         * reg = reg OP2 reg
         * if 2 rvar are all imm-val, we can calculate the result immediately
         * else move the imm-val (if exist) into s11
         */
        res::EeyoreVariable *lvar;
        res::RVariable *rvar1, *rvar2;
        std::string op;
    public:
        Ibinexpr(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            lvar = res::EeyoreVariable::GenRvar(vec[0]);
            rvar1 = res::RVariable::GenRvar(vec[2]);
            rvar2 = res::RVariable::GenRvar(vec[4]);
            op = vec[3];
            this->getdef().insert(*lvar);
            if(INSTANCE_OF(rvar1, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar1);
            if(INSTANCE_OF(rvar2, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar2);
        }
        DBP_FUNC{
            DBG_PRINT("Ibinexpr: " + lvar->getName() + " <- " + rvar1->getName() + op + rvar2->getName());
        }

        virtual std::string gencode(FILE *f) const override
        {
            auto lreg = this->f->getReg(f, lvar, this->lineno);
            std::string r1, r2;
            if(INSTANCE_OF(rvar1, res::ImmediateVal))
            {
                if(INSTANCE_OF(rvar2, res::ImmediateVal))
                {
                    auto v1 = (res::ImmediateVal*)rvar1,
                         v2 = (res::ImmediateVal*)rvar2;
                    auto val = 0;
                    switch(this->op[0])
                    {
                        case '+': val = v1->getval() + v2->getval(); break;
                        case '-': val = v1->getval() - v2->getval(); break;
                        case '*': val = v1->getval() * v2->getval(); break;
                        case '/': val = v1->getval() / v2->getval(); break;
                        case '%': val = v1->getval() % v2->getval(); break;
                        case '<': val = v1->getval() < v2->getval(); break;
                        case '>': val = v1->getval() > v2->getval(); break;
                        case '&': val = v1->getval()&& v2->getval(); break;
                        case '|': val = v1->getval()|| v2->getval(); break;
                        case '!': val = v1->getval()!= v2->getval(); break;
                        case '=': val = v1->getval()== v2->getval(); break;
                        default:
                                  throw std::runtime_error("Unknown Not Found: " + this->op);
                    }
                    Emit(f, lreg + " = " + std::to_string(val));
                    goto BACK;
                }
            }
            r1 = this->f->getReg(f, rvar1, this->getPrevLineno());
            r2 = this->f->getReg(f, rvar2, this->getPrevLineno());
            Emit(f, lreg + " = " + r1 + " " + this->op + " " + 
                    r2 + " // Ibinexpr\n");
BACK:
            if(lreg[0] == 't' || res::globalVars.isGlobalVar(lvar))
            {
                res::RegPool pool;
                this->f->BackToMemory(f, lvar, pool.getRID(lreg));
            }
            return "";
        }
};

class Icjmp : public Intrin
{
    public:
        /* if rvar1 LOP rvar2 goto LABEL */
        res::RightVariable *rvar1, *rvar2;
        std::string op;
        std::string label;
    public:
        Icjmp(std::vector<std::string> &vec, int lineno, Function *m)
            : Intrin(lineno, m)
        {
            rvar1 = res::RVariable::GenRvar(vec[1]);
            rvar2 = res::RVariable::GenRvar(vec[3]);
            label = vec[5];
            op = vec[2];
            if(INSTANCE_OF(rvar1, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar1);
            if(INSTANCE_OF(rvar2, res::EeyoreVariable))
                this->getuse().insert(*(res::EeyoreVariable*)rvar2);
        }
        DBP_FUNC{
            DBG_PRINT("Icjmp: if " + rvar1->getName() + op + rvar2->getName() + " then goto " + label); 
        }

        virtual std::string gencode(FILE *f) const override
        {
            auto reg1 = this->f->getReg(f, rvar1, this->getPrevLineno());
            auto reg2 = this->f->getReg(f, rvar2, this->getPrevLineno());
            Emit(f, "if " + reg1 + " " + op + " " + reg2 + " goto " + 
                    label +  " // Icjmp\n");
            return "";
        }
};

/* ====================================================== */
/*                    Intrin Manager                      */
/* ====================================================== */

Function::Function(IntrinManager *pmg)
{
    this->pmgr = pmg;
    std::vector<int> freereg{13,14,15,16,17,18};   
    this->tempPool = res::RegPool(freereg);
    inited = false;
    tempParamCount = 0;
    paramcnt = -1;
}

void Function::AddIntrin(Intrin *i)
{
    if(inited)
        throw std::runtime_error("Add intrin after Initialized function!");
    intrins[i->getline()] = i;
}

void Function::Initialize()
{
    /*TODO: calculate the size and set the stk */
    if(inited) return;
    int size = 0;
    for(auto in : intrins)
    {
        if(INSTANCE_OF(in.second, Ivardef))
            size += 4;
        if(INSTANCE_OF(in.second, Iarrdec))
            size += ((Iarrdec*)(in.second))->getlen();
    }
    this->stk.ExpandSize(size);
    this->stk.ExpandSize(res::globalVars.getGlobalVarcnt() * 4);
    /* record local variables into stack */
    for(auto in : intrins)
    {
        if(INSTANCE_OF(in.second, Ivardef))
            this->stk.AllocateVar(((Ivardef*)in.second)->var);
        if(INSTANCE_OF(in.second, Iarrdec))
        {
            Iarrdec *vin = (Iarrdec *)in.second;
            this->stk.AllocateArr(vin->var, vin->len);
        }
    }
    for(auto gevar : res::globalVars.AllEVars())
        this->stk.AllocateVar(gevar);
    inited = true;
}

void Function::InitRes(res::Liveness &liveness)
{
    /* CODE ANALYSIS from liveness */
    /* REG ALLOCATION */
    pallocer = new res::RegAllocer(liveness, this->stk);
    this->allocresult = pallocer->Alloc();
    this->usedReg = pallocer->GetUsedRegs();
    if(global_debug_flag)
    {
        std::cerr<<"Liveness result:========================== "<<std::endl;
        for(auto ent : liveness)
        {
            std::cerr<<"Line: "<<ent.first<<": ";
            for(auto var : ent.second)
                std::cerr<<var.getName()<<" ";
            std::cerr<<std::endl;
        }
        std::cerr<<"Alloc result:=========================== "<<std::endl;
        for(auto ent : allocresult)
        {
            std::cerr<<"Line: "<<ent.first<<": ";
            for(auto varinfo : ent.second)
            {
                std::cerr<<"("<<varinfo.var->getName()<<", "
                    <<res::globalRegs[varinfo.reg]<<") ";
            }
            auto sinfo = pallocer->GetSpillInfo(ent.first);
            if(std::distance(sinfo.first, sinfo.second) > 0)
            {
                std::cerr<<" [spill: ";
                std::for_each(sinfo.first, sinfo.second,
                        [](auto &spi){
                            std::cerr<<spi.var->getName()<<" ";
                        });
                std::cerr<<"]";
            }
            std::cerr<<std::endl;
        }
    }
}

void Function::gencode(FILE *f)//, res::Liveness &liveness)
{
    //Initialize();
    /* debug info ! */
    /* TRANSLATION */
    res::VarList valid_live;
    for(auto ent : this->intrins)
    {
        /* gen code for current line */
        Emit(f, "   // v line: " + std::to_string(ent.first) + ": " + 
               ent.second->getcode() + "\n");
        ent.second->gencode(f);

        /* after this line's execution, should move new vars
         * into regs if needed
         */
        auto aled = allocresult[ent.first];
        for(auto &var : aled)
        {
            if(valid_live.count(*var.var) == 0)
            {
                valid_live.insert(*var.var);
                LoadVariable(f, var.var, var.reg);
            }
        }

        /* before next intrin gencode, we find the spill info */
        auto sinfo = pallocer->GetSpillInfo(ent.first + 1);
        std::for_each(sinfo.first, sinfo.second,
                [this, &aled, f](auto &spillinfo){
                    auto var = spillinfo.var;
                    res::VarInfo info{var, -1};
                    info = *aled.find(info);
                    auto rid = info.reg;
                    this->BackToStack(f, var, rid);
                    Emit(f,"    // ^ spill var\n");
                });

    }
}

/**
 * return NULL if no prev intrin is found
 * This situation is rare, it will only happen when some initializing code
 * from global scope has been transferred to f_main
 */
Intrin *Function::getPrevIntrin(const Intrin *in)
{
    if(this->intrins.count(in->getline()) == 0)
        throw std::runtime_error("The intrin of line: "
               + std::to_string(in->getline()) + " is not in this function");
    auto it = this->intrins.find(in->getline());
    if(it == this->intrins.begin())
        return NULL;
    else --it;
    return it->second;
}

std::string Function::getReg(FILE *f, res::RightVariable *rvar, int lineno)
{
    /* TODO: compelete here!*/
    static int constreg = 0;
    static std::string constregs[2] = {"s11", "t5"};
    static int stkreg = 0;
    static std::string stkregs[5] = {"t0", "t1", "t2", "t3", "t4"};
    if(INSTANCE_OF(rvar, res::ImmediateVal)) // use "s11" and "t5" as const reg
    {
        res::ImmediateVal *val = (res::ImmediateVal *)rvar;
        std::string ret = constregs[constreg];
        Emit(f, ret + " = x0 + " + std::to_string(val->getval()) + "\n");
        constreg = (constreg + 1) % 2;
        return ret;
    }

    std::string ret;
    res::EeyoreVariable *evar = (res::EeyoreVariable*)rvar;
    auto &ares = this->allocresult[lineno];

    //auto &pres = this->allocresult[intrins[lineno]->getPrevLineno()];
    res::VarInfo info {evar, -2};
    if(ares.count(info) == 0)
        throw std::runtime_error("Variable " + evar->getName() 
                + " is not alive, and should be optimized out! At line: " + std::to_string(lineno));
    info = *ares.find(info);
    if(info.reg < -1) 
        throw std::runtime_error("Invalid register id: " + std::to_string(info.reg));
    else if (info.reg == -1) // on the stack, store into temp reg
    {
        ret = stkregs[stkreg];
        stkreg = (stkreg + 1) % 5;
        auto offset = this->stk.GetVarAddr(info.var);
        this->stk.gencode_spillout(f, offset, this->tempPool.getRID(ret));
    }
    else ret = res::globalRegs[info.reg];

    /*
       if(pres.count(info) == 0) // need to move the var into reg
       {
       if(res::globalVars.isGlobalVar(evar))
       res::globalVars.gencode_loadvar(f, evar, ret);
       else
       {
       }
       }
       */
    return ret;
}

void Function::LoadVariable(FILE *f, res::EeyoreVariable *var, int reg)
{
    if(res::globalVars.isGlobalVar(var))
    {
        res::globalVars.gencode_loadvar(f, var, res::globalRegs[reg]);
    }
    else
    {
        auto offset = this->stk.GetVarAddr(var);
        if(var->isArray())
            this->stk.gencode_lea(f, offset, reg);
    }
}

std::string Function::PrepareParam(FILE *f)
{
    /* a0: 19; a7: 26 */
    auto ret = res::globalRegs[tempParamCount + 19];
    _savereg(f, tempParamCount + 19);
    tempParamCount ++;
    return ret;
}

void Function::BackToStack(FILE *f, res::EeyoreVariable *e, int reg, int offreg)
{
    auto offset = this->stk.GetVarAddr(e);
    this->stk.gencode_spillin(f, offset, reg);
}

void Function::BackToGlobal(FILE *f, res::EeyoreVariable *var, int reg, int offreg)
{
    res::globalVars.gencode_storevar(f, var, offreg, reg);
}

void Function::BackToMemory(FILE *f, res::EeyoreVariable *var, int reg, int offreg)
{
    if(res::globalVars.isGlobalVar(var))
        BackToGlobal(f, var, reg, offreg);
    else BackToStack(f, var, reg, offreg);
}

void Function::StartFun(FILE *f) //save s0 - s11 (save params?)
{
    /* s0: 1; s11: 12 */
    //for(int i = 1; i<= 12; i++) _savereg(f, i);
    //for(int i = 19; i<=26; i++) _savereg(f, i);
    for(auto regid : usedReg)
    {
        if(regid >= 1 && regid <= 12) _savereg(f, regid);
    }
}

void Function::EndFun(FILE *f)
{
    //for(int i = 1; i <= 12; i++) _restorereg(f, i);
    for(auto regid : usedReg)
    {
        if(regid >= 1 && regid <= 12) _restorereg(f, regid);
    }
}

void Function::PrepareCalling(FILE *f, const std::string &fn) // save t0 - t5, a1 - a7
{
    /* t0:13; t5:18 ; a0:19*/
    //for(int i = 19 + tempParamCount; i < 27; i++)
    //    _savereg(f, i); //save a0 - a7
    for(auto regid : usedReg)
    {
        if(regid >= 19 + tempParamCount && regid < 27) _savereg(f, regid);
    }
}

void Function::EndCalling(FILE *f, const std::string &fn)
{
    /* restore t0-t5, a1 - a? */
    //for(int i = 13; i<=18; i++) _restorereg(f, i);
    //for(int i = 1; i<8; i++) _restorereg(f, i+19); //restore a1 - a7
    tempParamCount = 0;
    for(auto regid : usedReg)
    {
        if(regid > 19 && regid < 27) _restorereg(f, regid);
    }
}

bool Function::Optimize(res::Liveness &liveness)
{
    std::vector<int> removelist;
    bool opted = false;
    for(auto &ent : this->intrins)
    {
        auto line = ent.first;
        auto in = ent.second;
        auto livevars = liveness[in->getline()];
        if(INSTANCE_OF(in, Iassign))
        {
            Iassign *vin = (Iassign*)in;
            if(livevars.count(*vin->lvar) == 0)
            {
                removelist.push_back(line);
            }
        }
        if(INSTANCE_OF(in, Icallfunc))
        {
            Icallfunc *vin = (Icallfunc*)in;
            if(livevars.count(*vin->lvar) == 0)
            {
                ent.second = vin->toCallVFunc();
                opted = true;
            }
        }
        if(INSTANCE_OF(in, Iunaryexpr))
        {
            Iunaryexpr *vin = (Iunaryexpr*)in;
            if(livevars.count(*vin->lvar) == 0)
            {
                removelist.push_back(line);
            }
        }
        if(INSTANCE_OF(in, Ivassa))
        {
            Ivassa *vin = (Ivassa*)in;
            if(livevars.count(*vin->lvar) == 0)
            {
                removelist.push_back(line);
            }
        }
        if(INSTANCE_OF(in, Ibinexpr))
        {
            Ibinexpr *vin = (Ibinexpr*)in;
            if(livevars.count(*vin->lvar) == 0)
            {
                removelist.push_back(line);
            }
        }
    }   

    for(auto line : removelist)
    {
        opted = true;
        this->intrins.erase(line);
    }
    return opted;
}


const std::string IntrinManager::GLOBAL_SCOPE_NAME = "f__init";
const std::string IntrinManager::GLOBAL_RESMGR_NAME = "global";
int IntrinManager::SearchLabel(const std::string &label)
{
    int val = labels[label + ":"];
    if(val == 0) throw std::runtime_error("Label: "+label+" Not Found!");
    return val + 1;     /* return the next intrin position*/
}

std::vector<Intrin*> IntrinManager::Succ(IntrinSet &func, int lineno)
{
    std::vector<Intrin*> vec;
    if(!func.count(lineno))
        return vec;
    auto in = func[lineno];
    auto next = func.find(lineno);
    ++next; // now next is real next
    int line, line1, line2;
    if(INSTANCE_OF(in, Ivreturn))goto RETURN;
    if(INSTANCE_OF(in, Ireturn)) goto RETURN;
    if(INSTANCE_OF(in, Ijmp))
    {
        Ijmp * vin = (Ijmp*)in;
        line = this->SearchLabel(vin->label);
        if(func.count(line)) vec.push_back(func[line]);
        goto RETURN;
    }
    if(INSTANCE_OF(in, Ifuncst)) goto RETURN;
    if(INSTANCE_OF(in, Iendfun)) goto RETURN;
    if(INSTANCE_OF(in, Icjmp))
    {
        Icjmp *vin = (Icjmp*) in;
        line1 = this->SearchLabel(vin->label);
        if(func.count(line1)) vec.push_back(func[line1]);
        if(next!=func.end())
            line2 = next->second->getline();//in->getline() + 1;
        if(func.count(line2)) vec.push_back(func[line2]);
        goto RETURN;
    }

    /* normal case */
    if(next!=func.end())
        line = next->second->getline();//in->getline() + 1;
    if(func.count(line)) vec.push_back(func[line]);



RETURN:
    if(global_debug_flag)
    {
        in->dbg_print();
        for(auto v : vec)
        {
            std::cerr<<"\t NEXT: ";
            v->dbg_print();
        }
    }
    return vec;
}

void IntrinManager::NewScope(const std::string &scopename)
{
    DBG_PRINT("New Scope: " + scopename);
    curr_func = scopename;
    funcs[curr_func] = new Function(this);
}

void IntrinManager::EndScope()
{
    DBG_PRINT("Scope End!");
    curr_func = GLOBAL_SCOPE_NAME;
}

void IntrinManager::AddIntrin(const std::string &line)
{
#define ADDI(type) _addintrin(new type(vec, lines, getfunc(curr_func)))
    this->input.push_back(line);
    std::vector<std::string> vec = split(line, ' ');
    switch(vec.size())
    {
        case 1:
            if(vec[0] == "return")
                ADDI(Ivreturn);
            else if (vec[0].back() == ':')
            {
                this->labels[vec[0]] = this->lines;
                ADDI(Ilabel);
            }
            else goto ERROR;
            break;

        case 2:
            if (vec[0] == "goto")
                ADDI(Ijmp);
            else if (vec[0] == "var")
                ADDI(Ivardef);
            else if (vec[0] == "end")
            {
                ADDI(Iendfun);
                EndScope();
            }
            else if (vec[0] == "param")
                ADDI(Iparam);
            else if (vec[0] == "return")
                ADDI(Ireturn);
            else if (vec[0].substr(0,2) == "f_")
            {
                NewScope(vec[0]);
                Ifuncst *in = (Ifuncst*)ADDI(Ifuncst);
                auto pmc = in->paramcnt;
                pmc.pop_back();
                funcs[curr_func]->paramcnt = std::stoi(pmc.substr(1));
            }
            else if (vec[0] == "call")
                ADDI(Icallvfun);
            else goto ERROR;
            break;

        case 3:
            if (vec[1] == "=")
                ADDI(Iassign);
            else if (vec[0] == "var")
                ADDI(Iarrdec);
            else goto ERROR;
            break;

        case 4:
            if (vec[2] == "call")
                ADDI(Icallfunc);
            else if (vec[1] == "=" && vec[3][0] == '[')
                ADDI(Ivassa);
            else if (vec[1] == "=")
                ADDI(Iunaryexpr);
            else if (vec[2] == "=")
                ADDI(Iaassv);
            else goto ERROR;
            break;

        case 6:
            if(vec[0] == "if")
                ADDI(Icjmp);
            else goto ERROR;
            break;

        case 5:
            if (vec[1] == "=")
                ADDI(Ibinexpr);
            else goto ERROR;
            break;

        default:
ERROR:
            throw std::runtime_error("Invalid Grammar at line: "+std::to_string(lines)+": "
                    +line+"; Len: "+std::to_string(vec.size()));
    };

#undef ADDI
}

void IntrinManager::gencode(FILE *f)
{
    std::cerr<<"Gencode start!"<<std::endl;
    /* special for f__init */
    auto &finit = *this->funcs[GLOBAL_SCOPE_NAME];
    for(auto entin : finit.getIntrins())
    {
        auto in = entin.second;
        if(INSTANCE_OF(in, Ivardef))
        {
            Ivardef *vin = (Ivardef *)in;
            res::globalVars.AllocGlobalVar(f, vin->var, 0);
        }
        else if(INSTANCE_OF(in, Iarrdec))
        {
            Iarrdec *vin = (Iarrdec *)in;
            res::globalVars.AllocGlobalArr(f, vin->var, vin->len);
        }
        else
        {
            //throw std::runtime_error("Invalid intrin in f__init!");
            /* TODO: move these intrin into f_main may be a good choice? */
            auto fmain = this->getfunc("f_main");
            fmain->AddIntrin(in);
            in->TransferFunction(fmain);
        }

    }
    this->funcs.erase(GLOBAL_SCOPE_NAME);

    for(auto funcent : this->funcs)
    {
        DBG_PRINT("parsing " + funcent.first);
        for(auto in : funcent.second->getIntrins()) 
            this->Succ(funcent.second->getIntrins(), in.first);
        funcent.second->Initialize();
        bool opted = false;
        res::Liveness liveness;
        do
        {
            liveness = LivenessAnalysis(*this, funcent.first);
            opted = funcent.second->Optimize(liveness);
        }while(opted);
        funcent.second->InitRes(liveness);

        /* debug! */
        if(global_debug_flag)
        {
            std::cerr<<"After Optimize: "<<std::endl;
            //global_debug_flag = 1;
            for(auto in : funcent.second->getIntrins())
            {
                std::cerr<<"Line: "<<in.first<<": "<<in.second->getcode()<<" -- ";
                in.second->dbg_print();
            }
            //global_debug_flag = 0;
        }

        funcent.second->gencode(f);
    }
    /* TODO: main gencode function here! */
}


/* Functions */
Liveness LivenessAnalysis(IntrinManager &m, const std::string &funcname)
{
    /* data structures */
    using VarSet = std::set<res::EeyoreVariable>;
    auto sameset = [](VarSet &l, VarSet &r) // is two VarSet the same ?
    {
        if(l.size()!=r.size()) return false;
        for(auto i : l) if(!r.count(i)) return false;
        return true;
    };

    struct IntrinNode
    {
        Intrin * i;
        VarSet in;
        VarSet out;
        IntrinNode(Intrin *in) : i(in) {}
        IntrinNode() : i(NULL) {}
    };

    Liveness ret;
    auto *function = m.getfunc(funcname);
    auto &func = function->getIntrins();
    /* initialize graph */
    std::map<int, IntrinNode> graph;
    for(auto ent : func)
        graph[ent.first] = IntrinNode(ent.second);

    bool finishflag = false;
    while(!finishflag)
    {
        finishflag = true;
        for(auto ent : func)    //evern intrin
        {
            auto &node = graph[ent.first];
            // formula 1: node.in = node.use + (node.out - node.def)
            VarSet in1(node.out);
            for(auto def : node.i->getdef()) in1.erase(def);
            for(auto use : node.i->getuse()) in1.insert(use);
            if(!sameset(node.in, in1)) finishflag=false;
            node.in.swap(in1);      // update

            // formula 2: node.out = addUp(succ.in foreach succ in node.succ)
            VarSet out1;
            auto succs = m.Succ(func, node.i->getline());
            for(auto succ : succs)
            {
                auto succnode = graph[succ->getline()];
                out1.insert(succnode.in.begin(), succnode.in.end());
            }
            if(!sameset(node.out, out1)) finishflag=false;
            node.out.swap(out1);
        }
    }

    for(auto line : func)
    {
        auto &node = graph[line.first];
        ret[line.first].insert(node.out.begin(), node.out.end());
    }

    return ret;
}

#undef DBG_PRINT
}   // namespace intr
