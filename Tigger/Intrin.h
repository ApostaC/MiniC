#ifndef _INTRIN_HH_
#define _INTRIN_HH_

#include <set>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include "Resources.h"
#include "RegAlloc.h"


namespace intr
{
class Intrin;
class IntrinManager;

/* sub intrin class */
/**
 * Intrin type:
 *  label:      len = 1, LABEL:
 *  vreturn:    len = 1, return 
 *  jmp:        len = 2, goto LABEL
 *  vardef:     len = 2, 'var' varmame
 *  endfun:     len = 2, end funname
 *  param:      len = 2, param rvar
 *  return:     len = 2, return rvar
 *  funcst:     len = 2, fname '['num']'
 *  callvfun:   len = 2, call funcname
 *--assign:     len = 3, lvar = rvar    OPTIMIZABLE
 *  arrdec:     len = 3, 'var' INT varname
 *  callfunc:   len = 4, lvar = call funcname     OPTIMIZABLE
 *--unaryexpr:  len = 4, lvar = OP1 rvar    OPTIMIZABLE
 *--aassv:      len = 4, arr '['num']' = rval
 *--vassa:      len = 4, lval = arr '['num']'    OPTIMIZABLE
 *--binexpr:    len = 5, lvar = rvar OP2 rvar    OPTIMIZABLE
 *  cjmp:       len = 6, 'if' rvar LOP rvar 'goto' LABEL
 */
class Ilabel;
class Ivreturn;
class Ijmp;
class Ivardef;
class Iendfun;
class Iparam;
class Ireturn;
class Ifuncst;
class Icallvfun;
class Iassign;
class Iarrdec;
class Icallfunc;
class Iunaryexpr;
class Iaassv;
class Ivassa;
class Ibinexpr;
class Icjmp;

class Function;


class Intrin
{
    protected:
        int lineno;
        std::string code;
        std::set<res::EeyoreVariable> def;
        std::set<res::EeyoreVariable> use;
        Function *f;

    public:
        Intrin(int line, Function *m) : lineno(line), f(m) {}
        virtual int getline() const {return lineno;}
        virtual std::set<res::EeyoreVariable> &getdef(){return def;}
        virtual std::set<res::EeyoreVariable> &getuse(){return use;}
        virtual std::string gencode(FILE *f) const = 0;

        virtual int getPrevLineno() const;
        virtual void dbg_print() const = 0;
        virtual void setcode(const std::string &c)
        {this->code = c;}
        virtual std::string getcode() { return code;}
        virtual void TransferFunction(Function *newf){f=newf;}
};

class Function
{
    private:
        using IntrinSet = std::map<int, Intrin*>;
        IntrinSet intrins;
        IntrinManager *pmgr;
        res::RegPool tempPool;
        res::StackFrame stk;
        bool inited = false;
    private:
        res::RegAllocer *pallocer = NULL;
        res::RegAllocer::AllocResult_t allocresult;
        std::set<res::RegID> usedReg;
    public:
        int paramcnt = -1;
    public:
        Function(IntrinManager *);
        void AddIntrin(Intrin *);
        void Initialize(); // call after finishing AddIntrin
        void InitRes(res::Liveness &liveness); // call after Initialize
        int GetSize(){Initialize(); return this->stk.GetSlots();}
        IntrinSet &getIntrins(){return intrins;}
        bool Optimize(res::Liveness &liveness);
        res::StackFrame &getStack(){return stk;}
        ~Function(){delete pallocer;}

    private:
        int tempParamCount = 0;
        void _savereg(FILE *f, int rid)
        {
            auto offset = this->stk.GetRegAddr(rid);
            this->stk.gencode_spillin(f, offset, rid);
            //Emit(f,"    // ^ _savereg\n");
        }
        void _restorereg(FILE *f, int rid)
        {
            auto offset = this->stk.GetRegAddr(rid);
            this->stk.gencode_spillout(f, offset, rid);
            //Emit(f,"    // ^ _restore reg\n");
        }
    public:
        Intrin *getPrevIntrin(const Intrin *in);
        /* ABI for codegen */
        void gencode(FILE *f);//, res::Liveness &liveness);
        std::string getReg(FILE *f, res::RightVariable *rvar, int lineno);
        std::string PrepareParam(FILE *f); // return the next free "a?" reg
        void StartFun(FILE *f);     // save callee saved regs
        void PrepareCalling(FILE *f, const std::string &); // store the regs into stack
        void EndCalling(FILE *f, const std::string &);   // restore the regs from stack
        void EndFun(FILE *f);       // restore callee saved regs
        void BackToStack(FILE *f, res::EeyoreVariable *lvar, int reg, int offreg = 0);
        void BackToGlobal(FILE *f, res::EeyoreVariable *gvar, int reg, int offreg = 0);
        void BackToMemory(FILE *f, res::EeyoreVariable *var, int reg, int offreg = 0);
        void LoadVariable(FILE *f, res::EeyoreVariable *var, int reg);
};

/**
 * IntrinManager: give a line of input and parse a intrin
 */
class IntrinManager
{
    private:
        static const std::string GLOBAL_RESMGR_NAME;// = "global";
        static const std::string GLOBAL_SCOPE_NAME;// = "f__init";
    private:
        std::vector<std::string> input; //used for debug;
        int lines = 1;
        using IntrinSet = std::map<int, Intrin*>;
        IntrinSet intrins;

        /* function management */
        std::map<std::string, Function*> funcs;
        std::string curr_func;

        /* resmgr */
        /* label searching */
        std::map<std::string, int> labels;  /* lineno of label itself */
        
    private:
        Intrin *_addintrin(Intrin *in)
        {
            in->setcode(input.back());
            intrins[this->lines] = in;
            funcs[curr_func]->AddIntrin(in); // insert into function
            ++lines;
            return in;
        }
    public:
        IntrinManager() : lines(1)
        {
            this->lines = 1;
            auto &s = GLOBAL_RESMGR_NAME;
            curr_func = GLOBAL_SCOPE_NAME;
            funcs[curr_func] = new Function(this);
        }
        /**
         * add intrin (update labels here)
         * search for label
         * give an intrin's succ
         * set correct ResourceManager
         */
        std::string getInputLine(int lineno)
        {return input[lineno - 1];}
        void AddIntrin(const std::string &line);
        int SearchLabel(const std::string &label); //param have no ':', return the next intrin
        std::vector<Intrin*> Succ(IntrinSet &, int lineno);
        void NewScope(const std::string &scopename);
        void EndScope();

        void gencode(FILE *f);

        std::vector<std::string> getFuncNames()
        {
            std::vector<std::string> ret;
            for(auto func : this->funcs) ret.push_back(func.first);
            return ret;
        }
        Function *getfunc(const std::string &f) { 
            if(this->funcs.count(f) == 0)
                throw std::runtime_error("No such function");
            return this->funcs[f]; 
        }
};

/* Functions */
using Liveness = res::Liveness;
Liveness LivenessAnalysis(IntrinManager &m, const std::string &funcname);
} //namespace intr


#endif
