#ifndef _REGALLOC_HH_
#define _REGALLOC_HH_


#include <vector>
#include <stdexcept>
#include <map>
#include "common.h"
#include "Resources.h"

namespace res
{
class RegPool;
class RegAllocer; // read a Liveness graph, get the time-alloc table
class StackFrame; // A function's StackFrame

/* helper classes */
struct VarInfo
{
    res::EeyoreVariable *var = NULL;
    RegID reg = -1;
    bool operator<(const VarInfo &r) const
    {
        return *(var) < *(r.var);
    }
};

class StackFrame
{
    /* layout:
     * | regs: 26 * 4 | params | vars |
     */
    private:
        struct VarPos
        {
            res::EeyoreVariable *var;
            int offset;
            bool operator<(const VarPos &r) const 
            {
                return *(this->var) < *(r.var);
            }
        };

    private:
        static const int REGS = 26;
        int slotcnt = 0;
        int slotused = 0;
        std::map<res::EeyoreVariable, VarPos> content;

    private:
        int _allocate(int slots){ 
            slotused += slots; return slotused-slots+1;
            if(slotused > slotcnt) slotcnt = slotused;
        }
    public:
        StackFrame()
        {
            slotcnt  = REGS;
            slotused = REGS;
        }
        void ExpandSize(size_t size){slotcnt += size/4;}
        int GetSlots(){return this->slotcnt;}
        int SlotCount(){return slotcnt;}
        int AllocateVar(res::EeyoreVariable *var); // return the address of position
        int AllocateArr(res::EeyoreVariable *var, int size); // size should div by 4
        int GetVarAddr(res::EeyoreVariable *var);   // return the address
        int GetRegAddr(int rid);

    public:
        /* API for stack operation code gen */
        /**
         * offset: var's offset, got from GetVarAddr(var);
         * offreg: the offset to deal with array 
         */
        void gencode_spillin(FILE *f, int offset, RegID rid, RegID offreg = 0);
        void gencode_spillout(FILE *f, int offset, RegID rid);
        void gencode_lea(FILE *f, int offset, RegID rid);
};

class RegPool
{
    private:
        static const int FREE = 0;
        static const int USED = 1;
        static const int TOTAL_REG = 27;
        std::vector<int> regStatus;
        int used;
        std::map<std::string, int> reg2id;
    private:
        void _init(int state = FREE)
        {
            for(int i=0;i<TOTAL_REG;i++)
                regStatus.push_back(state);
            for(auto ent : globalRegs) reg2id[ent.second] = ent.first;
        }
    public:
        RegPool()
        {
            /* available regs are a0 - a7, s0-s10 */
            /* t0-t5 are free, but doesn't use here */
            _init();
            regStatus[0] = USED;
            for(int i=12; i<=18; i++) // @see Resources.cpp to see the REGID
                regStatus[i] = USED;
            used = 0;
            for(int i=0;i<TOTAL_REG;i++)
                if(regStatus[i] == USED) used++;
        }

        RegPool(const std::vector<int> &freeRegs)
        {
            _init(USED);
            used = TOTAL_REG;
            for(auto freereg : freeRegs)
            {
                regStatus[freereg] = FREE;
                used--;
            }

        }

        RegID AllocReg(int preference = -1);
        int getRID(const std::string &name){return reg2id[name];}

        void FreeReg(RegID r)
        {
            if(regStatus[r] == FREE)
                fprintf(stderr, "Warning, double free on REG %d: %s\n",
                        r, globalRegs[r].c_str());
            regStatus[r] = FREE;
            used--;
        }

        int FreeCount()
        {
            return TOTAL_REG - used;
        }

        void dbg_print()
        {
            if(!global_debug_flag) return;
            std::string out[2] = {"FREE","USED"};
            for(int i=0;i<TOTAL_REG;i++)
                if(regStatus[i] == USED)
                    fprintf(stderr,KRED "(%s, %s); ", 
                            globalRegs[i].c_str(), out[regStatus[i]].c_str());
            fprintf(stderr, "\n" KNRM);
        }
};

class RegAllocer
{
    private:
        using _TIME_T = int;
        struct LiveInterval
        {
            _TIME_T start;
            _TIME_T end;
            res::EeyoreVariable *var;
            bool operator<(const LiveInterval &r) const
            {
                return this->end<r.end;
            }
        };

        struct SpillInfo
        {
            _TIME_T line;
            res::EeyoreVariable *var;
            bool operator<(const SpillInfo&r) const
            {
                return this->line<r.line;
            }

        };

        using Active_t = std::set<LiveInterval>;
    private:
        RegPool pool;
        Liveness &liveness;
        StackFrame &stack;
        std::multiset<SpillInfo> spillPos; // position to spill, used in Function::gencode
        std::set<RegID> usedRegs;

    public:
        using VarInfoList = std::set<VarInfo>;
        using AllocResult_t = std::map<int, VarInfoList>;

    private:
        void _gen_spill_msg(int line, res::EeyoreVariable *var)
        {
            SpillInfo info{line, var};
            spillPos.insert(info);
        }

    public:
        RegAllocer(Liveness &l, StackFrame &stk) 
            : pool(), liveness(l), stack(stk)
        {
        }

        /* TODO: HERE! */
        AllocResult_t Alloc();

        decltype(spillPos.equal_range({1,NULL}))
            GetSpillInfo(int line) 
            { return spillPos.equal_range({line, NULL});}

        const std::set<RegID> &GetUsedRegs() const
        {
            return usedRegs;
        }
};


}   // namespace res
#endif
