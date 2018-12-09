#ifndef _RESOURCE_HH_
#define _RESOURCE_HH_
#include <string>
#include <vector>
#include <set>
#include <map>

namespace res
{

class RightVariable;    /* imm-value or eeyore-var */
class ImmediateVal;     /* imm-value */
class EeyoreVariable;   /* eeyore-var */
class VarPool;          /* Pool for eeyore-var */
//class Register;         /* Tigger reg */
using Register = std::string;
class TiggerVariable;   /* Tigger glb var */
class GlobalVarMgr;
//class ResourceManager;  /* resource mgr */
//class GlobalManager;    /* res mgr for glb tigger var */
//class FunctionManager;  /* res mgr for local vars */

extern VarPool GlobalPool;
extern GlobalVarMgr globalVars;
extern std::map<int, Register> globalRegs;

using VarList = std::set<res::EeyoreVariable>;
using Liveness = std::map<int, VarList>;
using RegID = int;

class RightVariable
{
    private:
    public:
        /* NOTHING HERE ? */
        virtual std::string getName() const = 0;
        virtual bool operator<(const RightVariable &r) const 
        {
            return this->getName() < r.getName();
        }
    public:
        static RightVariable *GenRvar(const std::string &n);
};

class EeyoreVariable : public RightVariable
{
    private:
        std::string name;
        EeyoreVariable(const std::string &n) : name(n){}
        bool isarr = false;
    public:
        virtual std::string getName()const override {return name;} 
        static EeyoreVariable *GenRvar(const std::string &n);
        void setArray(){isarr = true;}
        bool isArray(){return isarr;}
        friend class VarPool;
};

class ImmediateVal : public RightVariable
{
    private:
        int value;
    public:
        ImmediateVal(int va){ value = va; }
        virtual std::string getName()const override {return std::to_string(value); }
        int getval(){return value;}

};
using RVariable = RightVariable;

/* all var in the pool are EeyoreVar, NO imm-val */
class VarPool
{
    private:
        std::map<std::string, res::EeyoreVariable*> pool;
    public:
        void InsertNewVar(res::EeyoreVariable *var)
        {
            pool[var->getName()] = var;
        }

        /**
         * GetVar(varname)
         * if var doesn't exist, create one in the pool and return it
         */
        res::EeyoreVariable *GetVar(const std::string &vname)   
        {
            if(!pool.count(vname)) 
                InsertNewVar(new res::EeyoreVariable(vname));
            return pool[vname];
        }
};



class TiggerVariable /* global variable for tigger */
{
    private:
        std::string name;
        size_t size;
        bool isArr;
    public:
        TiggerVariable(const std::string &n, size_t s, bool isa) 
            : name(n), size(s), isArr(isa) {}
        std::string getName()const {return name;}
        size_t getSize()const {return size;}
        bool isArray(){return isArr;}
};

class GlobalVarMgr
{
    /* GlobalVarmgr got a special register: t6 */
    private:
        std::map<std::string, res::TiggerVariable*> pool;
        int varcnt = 0;
    private:
        std::string _genname()
        {
            return "v" + std::to_string(varcnt);
        }
    public:
        bool isGlobalVar(res::EeyoreVariable *var);

        /**
         * return the tigger global var name of a eeyore var
         */
        std::string GetVarName(res::EeyoreVariable *var);

        /* ABI FOR GLB VAR/ARR DEFINITION */
        void AllocGlobalVar(FILE *f,res::EeyoreVariable *var, int initval = 0);
        void AllocGlobalArr(FILE *f,res::EeyoreVariable *var, int size);
        /**
         * if var is variable, reg have it's value
         * if var is array,    reg have it's address
         */
        void gencode_loadvar(FILE *f, res::EeyoreVariable *var, const std::string &regname);
        void gencode_storevar(FILE *f, res::EeyoreVariable *var, int offreg, int valreg);

        /**
         * return the number of global variables
         */
        size_t getGlobalVarcnt() {return pool.size();} 

        std::vector<res::TiggerVariable*> AllTVars();
        std::vector<res::EeyoreVariable*> AllEVars();
};

//class ResourceManager
//{
//    /* TODO: resource management */
//    protected:
//        enum  Position_t        {STK = 0, REG = 1} ;
//        using Offset_t          = int;
//        using Position          = std::map<Position_t, Offset_t>;
//        using Var               = EeyoreVariable;
//        using VarPositionTable  = std::map<Var, Position>;
//    protected:
//        ResourceManager *parent; // for finding global variables
//        VarPositionTable vpt;
//        FILE *f;
//        
//    public:
//        ResourceManager(ResourceManager *p) : parent(p){f = stdout;}
//        virtual std::string Load(Var *v) = 0;
//        virtual std::string Lea(Var *v) = 0;
//        virtual void Store(Var *v) = 0;
//        virtual void ToFile(FILE *ff){f=ff;};
//};
//
//class GlobalManager : public ResourceManager
//{
//    /* TODO: resource management for global variables */
//    /* NO REG HERE! */
//    protected:
//        int glbvarcnt = 0;
//        std::map<Var, TiggerVariable> glbmap;
//    public:
//        GlobalManager() : ResourceManager(NULL)
//        {
//            glbvarcnt = 0;
//        }
//        virtual std::string Load(Var *v) override;
//        virtual std::string Lea(Var *v) override;
//        virtual void Store(Var *v) override;
//        virtual void AllocateArr(Var *v, size_t size);  // allocate for global array
//        virtual void AllocateVar(Var *v, size_t size, int initval = 0);  // allocate for global var
//};
//
//class FunctionManager : public ResourceManager
//{
//    /* TODO: resource management for local variables and registers */
//    /* TODO: manage a0-a7 when use "setparam" */
//    /* TODO: calculate the stack size */
//    /* stack structure:
//     * | params | data |
//     */
//    protected:
//        size_t stksize;
//        size_t stkoff;
//
//    public:
//        FunctionManager(ResourceManager *parent)
//            : ResourceManager(parent)
//        {
//        }
//        void StoreParams();
//        virtual std::string Load(Var *v) override;
//        virtual std::string Lea(Var *v) override;
//        virtual void Store(Var *v) override;
//};


} //namespace res

#endif
