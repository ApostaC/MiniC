#include <algorithm>
#include "RegAlloc.h"

namespace res
{

/*===============================================*/
/*                  StackFrame                   */
/*===============================================*/
int StackFrame::AllocateVar(res::EeyoreVariable *var)
{
    int pos = -1;
    if(content.count(*var) == 0)
    {
        pos = _allocate(1);
        content[*var] = {var, pos};
    }
    else
    {
        pos = content[*var].offset;
        fprintf(stderr,"%s Already in the stack!\n",
                var->getName().c_str());
    }
    
    return pos;
}

int StackFrame::AllocateArr(res::EeyoreVariable *var, int size)
{
    int pos = -1;
    if(content.count(*var) == 0)
    {
        pos = _allocate(size/4);
        content[*var] = {var, pos};
    }
    else
    {
        pos = content[*var].offset;
        fprintf(stderr,"%s Already in the stack!\n",
                var->getName().c_str());
    }
    return pos;
}

int StackFrame::GetRegAddr(int rid)
{
    if(rid > 0) return rid - 1;
    else throw std::runtime_error("Invalid reg on stack: " + globalRegs[rid]);
}

int StackFrame::GetVarAddr(res::EeyoreVariable *var)
{
    if(content.count(*var) == 0)
    {
        fprintf(stderr, "%s dosen't in the Stack!\n",
                var->getName().c_str());
        return -1;
    }
    return content[*var].offset;
}

void StackFrame::gencode_spillin(FILE *f, int offset, RegID rid, RegID offreg)
{
    if(offreg > 0)
    {
        Emit(f, "loadaddr " + std::to_string(offset) + " t6\n"); //base addr
        Emit(f, "t6 = " + globalRegs[offreg] + "\n");   //base + offset
        Emit(f, "t6 [0] = " + globalRegs[rid] + "\n");
        return;
    }
    else
    {
        std::string code = "store " + globalRegs[rid] + " " +
            std::to_string(offset) + "\n";
        Emit(f, code);
    }
}

void StackFrame::gencode_spillout(FILE *f, int offset, RegID rid)
{
    std::string code = "load " + std::to_string(offset) + " " +
        globalRegs[rid] + "\n";
    Emit(f, code);
}

void StackFrame::gencode_lea(FILE *f, int offset, RegID rid)
{
    std::string code = "loadaddr " + std::to_string(offset) + " " +
        globalRegs[rid] + "\n";
    Emit(f, code);
}

/*===============================================*/
/*                  RegAllocer                   */
/*===============================================*/

RegID RegPool::AllocReg(int preference)
{
    unsigned i=0;
    if(preference != -1)
    {
        auto st = regStatus[preference];
        if(st == USED)
            throw std::runtime_error("Preference of " +
                    globalRegs[preference] + " cannot be satisified!");
        else 
        {
            i = preference;
            goto FIN;
        }
    }
    if(used == TOTAL_REG) 
        throw std::runtime_error("Not enough free regs!");
    for(i=0;i<regStatus.size();i++)
        if(regStatus[i] == FREE) break;
FIN:
    regStatus[i] = USED;
    used++;
    return i;
}

RegAllocer::AllocResult_t RegAllocer::Alloc()
{
    AllocResult_t ret;
    using Var = res::EeyoreVariable;
    std::map<Var, RegID> registers;

    /* Get <Liveinterval, var> vector and sort it*/
    std::vector<LiveInterval> intervals;
    {
        std::map<Var, LiveInterval> tempmap;
        for(auto ent : liveness)
        {
            for(auto var : ent.second)
            {
                if(tempmap.count(var) == 0)
                {
                    auto pvar = GlobalPool.GetVar(var.getName());
                    tempmap[var] = { ent.first, ent.first, pvar};
                }
                auto &intv = tempmap[var];
                intv.end = ent.first; //update the end
            }
        }
        for(auto ent : tempmap)
        {
            intervals.push_back(ent.second);
        }
    }
    std::sort(intervals.begin(), intervals.end(), 
            [](auto l, auto r){return l.start<r.start;});

    Active_t active;
    /* Definition of Expire and Spill */
    auto ExpireOldInterval = [&](LiveInterval &i)
    {
        for(auto j : active)
        {
            if(j.end >= i.start) return;
            else 
            {
                /* get reg and free it */
                auto var = j.var;
                auto rid = registers[*var];
                this->pool.FreeReg(rid);
                active.erase(j);    //erase from active set
                registers.erase(*var);
                /* TODO: update return */
            }
        }
    };
    auto SpillAtInterval = [&](LiveInterval &i)
    {
        auto pspill = active.begin();
        auto spill = *pspill;
        if(spill.end > i.end) // spill out spill
        {
            registers[*i.var] = registers[*spill.var];
            _gen_spill_msg(i.start, spill.var);
            active.erase(pspill);
            //registers.erase(*spill.var);
            registers[*spill.var] = -1; // MARK IT ON THE STACK
            active.insert(i);
        }
        else // spill out i
        {
            _gen_spill_msg(i.start, i.var);
        }
    };

    /* check for time in func */
    std::set<int> intervalBegins;
    for(auto i : intervals) intervalBegins.insert(i.start);
    auto updateRet = [&](int time)
    {
        auto &target = ret[time];
        for(auto ent : registers)
        {
            RegID reg = ent.second;
            Var *pvar = GlobalPool.GetVar(ent.first.getName());
            target.insert({pvar, reg});
        }
    };

    for(auto ent : liveness)
    {
        int time = ent.first;
        if(intervalBegins.count(time) == 0)
        {
            updateRet(time);
        }
        else
        {
            for(auto intev : intervals)
            {
                if(intev.start == time)
                {
                    //fprintf(stderr,KRED "here! free regs:%d\n" KNRM, pool.FreeCount());
                    pool.dbg_print();
                    ExpireOldInterval(intev);
                    if(pool.FreeCount() == 0)
                        SpillAtInterval(intev);
                    else
                    {
                        int rid = -2;
                        if(intev.var->getName()[0] == 'p') //param
                            rid = pool.AllocReg(
                                    intev.var->getName()[1] - '0' 
                                    + pool.getRID("a0")
                                    );
                        else rid = pool.AllocReg();
                        active.insert(intev);
                        registers[*intev.var] = rid;
                    }
                }
            }
            updateRet(time);
        }
    } 
    return ret;
}

} // namespace res
