#include <iostream>
#include <map>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

/* SOME DEFINITIONS */
using List = std::set<std::string>;
using Dict = std::map<std::string, std::string>;
using std::to_string;

/* GLOBAL VARIABLES */
int stk = 0;        // global stack size
int label_cnt = 0;  // label counter
const std::string lbl_pref = ".Lbl"; // Label prefix

List reg_names = {"x0"};
Dict expr_ops = {{"+", "add"}, {"-", "sub"},
                 {"*", "mul"}, {"/", "div"}, {"%", "rem"},
                 {"<", "slt"}, {">", "sgt"}},
     cond_ops = {{"||","or" }, {"!=","xor"}, {"==", "sub"}},
     goto_ops = {{"<", "blt"}, {">", "bgt"}, {"!=", "bne"},
                 {"==","beq"}, {"<=","ble"}, {">=", "bge"}},
     rimm_ops = {{"+", "add"}, {"<", "slti"}};

/* INITIALIZE */
void Initialize()
{
    const static std::map<std::string, int> v
    {
        {"t", 6}, {"a", 7}, {"s", 12}
    };
    for(auto ent : v)
        for(int i=0;i<=ent.second;i++)
        {
            reg_names.insert(ent.first+to_string(i));
        }
}

/* HELPERS */
std::vector<std::string> split(const std::string &, char);
void Emit(const std::string &str);
void Emit(FILE *f, const std::string &str);
void gencode(const std::vector<std::string> &);

std::vector<std::string> split(const std::string &s, char c)
{
    std::stringstream sin;
    std::vector<std::string> ret;
    if(s.length() == 0) return ret;
    sin.str(s); 
    std::string part;
    while(std::getline(sin, part, c))
    {
        if(part.length())
            ret.emplace_back(std::move(part));
    }
    return ret;
}

void Emit(const std::string &str)
{
    Emit(stdout, str);
}

void Emit(FILE *f, const std::string &str)
{
    fprintf(f, "%s\n", str.c_str());
}

inline std::string newLabel()
{
    label_cnt += 1;
    return lbl_pref + to_string(label_cnt);
}

inline bool isNum(const std::string &s)
{
    try{ auto num = std::stoll(s);  }
    catch(...){ return false;  }
    return true;
}
inline bool Contains(const std::string &src, const std::string &tgt)
{
    return src.find(tgt) != std::string::npos;
}
inline bool Contains(const List &l, const std::string &elem)
{
    return l.count(elem) > 0;
}
inline bool Contains(const Dict &d, const std::string &elem)
{
    return d.count(elem) > 0;
}

void gencode(const std::vector<std::string> &line)
{
    const int len = line.size();
    if(len == 0) return;

    /* f_name [pcnt] [size] */
    if(len == 3 && Contains(line[0], "f_"))
    {
        int size = std::stoi(line[2].substr(0,line[2].length() - 1).substr(1));
        //std::cerr<<"Size = "<<size<<std::endl;
        auto fname = line[0].substr(2);
        Emit("\t.text");
        Emit("\t.align\t2");
        Emit("\t.global\t" + fname);
        Emit("\t.type\t" + fname + ", @function");
        Emit(fname + ":");
        stk = (((size + 2) >> 2) + 1) << 4;
        Emit("\tadd\tsp,sp,-" + to_string(stk));
        Emit("\tsd\tra," + to_string(stk - 8) + "(sp)");
        return;
    }

    /* end f_name */
    if(len == 2 && line[0] == "end")
    {
        auto name = line[1].substr(2);
        Emit("\t.size\t" + name + ", .-" + name);
        return;
    }

    /* v1 = malloc 12345 */
    if(len == 4 && line[2] == "malloc")
    {
        auto size = to_string(std::stoi(line[3]) * 4);
        Emit("\t.comm\t" + line[0] + "," + size + ",4");
        return;
    }

    /* v0 = 0 */
    if(len == 3 && !Contains(reg_names, line[0]) && isNum(line[2]) && line[1] == "=")
    {
        auto &vname = line[0];
        Emit("\t.global\t" + vname);
        Emit("\t.section\t.sdata");
        Emit("\t.align\t2");
        Emit("\t.type\t" + vname + ", @object");
        Emit("\t.size\t" + vname + ", 4");
        Emit(vname + ":");
        Emit("\t.word\t" + line[2]);
        return;
    }

    /* reg = integer */
    if(len == 3 && Contains(reg_names, line[0]) && isNum(line[2]))
    {
        Emit("\tli\t" + line[0] + ", " + line[2]);
        return;
    }

    /* reg = reg +/< integer */
    if(len == 5 && Contains(reg_names, line[0]) && Contains(reg_names, line[2])
            && isNum(line[4]) && Contains(rimm_ops, line[3]))
    {
        Emit("\t" + rimm_ops[line[3]] + "\t" + line[0] + "," + line[2] + "," + line[4]);
        return;
    }

    /* reg1 = reg2 op reg3 */
    if(len == 5 && Contains(reg_names, line[0]) 
            && Contains(reg_names, line[2]) && Contains(reg_names, line[4])
            && Contains(expr_ops, line[3]))
    {
        Emit("\t" + expr_ops[line[3]] + "\t" + line[0] + "," + line[2] + "," + line[4]);
        return;
    }

    /* reg1 = reg2 condop reg3 */
    if(len == 5 && Contains(reg_names, line[0]) 
            && Contains(reg_names, line[2]) && Contains(reg_names, line[4])
            && Contains(cond_ops, line[3]))
    {
        Emit("\t" + cond_ops[line[3]] + "\t" + line[0] + "," + line[2] + "," + line[4]);
        std::string intrin = "snez";
        if(line[3] == "==") intrin = "seqz";
        Emit("\t" + intrin + "\t" + line[0] + "," + line[0]);
        return;
    }

    /* reg1 = reg2 && reg3 */
    if(len == 5 && Contains(reg_names, line[0]) 
            && Contains(reg_names, line[2]) && Contains(reg_names, line[4])
            && line[3] == "&&") // need jmp
    {
        auto lbl0 = newLabel();
        auto lbl1 = newLabel();
        Emit("\tbeqz\t" + line[2] + ", " + lbl0);
        Emit("\tbeqz\t" + line[4] + ", " + lbl0);
        Emit("\tli\t" + line[0] + ",1");
        Emit("\tj\t" + lbl1);
        Emit(lbl0 + ":");
        Emit("\tli\t" + line[0] + ",0");
        Emit(lbl1 + ":");
        return;
    }

    /* reg1 = reg2 */
    if(len == 3 && Contains(reg_names, line[0]) && Contains(reg_names, line[2]))
    {
        Emit("\tmv\t" + line[0] + "," + line[2]);
        return;
    }

    /* reg [1] = reg */
    if(len == 4 && line[2] == "=" && line[1].back() == ']')
    {
        auto snum = line[1].substr(0, line[1].length() - 1).substr(1);
        Emit("\tsw\t" + line[3] + "," + snum + "(" + line[0] + ")");
        return;
    }

    /* reg = reg [2] */
    if(len == 4 && line[1] == "=" && line[3].back() == ']')
    {
        auto snum = line[3].substr(0, line[3].length() - 1).substr(1);
        Emit("\tlw\t" + line[0] + "," + snum + "(" + line[2] + ")");
        return;
    }

    /* 0  1    2  3    4    5  */
    /* if reg1 op reg2 goto l3 */
    if(len == 6 && line[4] == "goto")// && Contains(goto_ops, line[2]))
    {
        Emit("\t" + goto_ops[line[2]] + "\t" + line[1] + "," + line[3] 
               + ",." + line[5]);
        return;
    }
    /* label */
    if(len == 1 && line[0].back() == ':')
    {
        Emit("." + line[0]);
        return;
    }

    /* goto Label */
    if(len == 2 && line[0] == "goto")
    {
        Emit("\tj\t." + line[1]);
        return;
    }

    /* call function */
    if(len == 2 && line[0] == "call")
    {
        auto fname = line[1].substr(2);
        Emit("\tcall\t"+fname);
        return;
    }

    /* store r0 4 */
    if(len == 3 && line[0] == "store")
    {
        auto snum = to_string(std::stoi(line[2]) * 4);
        Emit("\tsw\t" + line[1] + "," + snum + "(sp)");
        return;
    }

    /* load int reg */
    if(len == 3 && line[0] == "load")
    {
        if(isNum(line[1]))
        {
            Emit("\tlw\t" + line[2] + "," + to_string(4 * std::stoi(line[1])) + "(sp)");
        }
        else
        {
            Emit("\tlui\t" + line[2] + ",\%hi(" + line[1] + ")");
            Emit("\tlw\t" + line[2] + "," + "\%lo(" + line[1] + ")(" + line[2] + ")");
        }
        return;
    }

    /* loadaddr int/gvar reg */
    if(len == 3 && line[0] == "loadaddr")
    {
        if(isNum(line[1]))
        {
            Emit("\tadd\t" + line[2] + ",sp," + to_string(4 * std::stoi(line[1])));
        }
        else
        {
            Emit("\tlui\t" + line[2] + ",\%hi(" + line[1] + ")");
            Emit("\tadd\t" + line[2] + "," + line[2] + ",\%lo(" + line[1] + ")");
        }
        return;
    }

    /* reg = - reg */
    if(len == 4 && Contains(reg_names, line[0]) && Contains(reg_names, line[3]))
    {
        if(line[2] == "-")
            Emit("\tsub\t" + line[3] + ",x0," + line[3]);
        else    // NOT
        {
            Emit("\tsnez\t" + line[0] + "," + line[3]);
            auto lbl0 = newLabel();
            auto lbl1 = newLabel();
            Emit("\tbeq\t" + line[0] + ",x0," + lbl0);
            Emit("\tli\t" + line[0] + ",0");
            Emit("\tj\t" + lbl1);
            Emit(lbl0 + ":");
            Emit("\tli\t" + line[0] + ",1");
            Emit(lbl1 + ":");
        }
        return;
    }


    /* return */
    if(line[0] == "return")
    {
        Emit("\tlw\tra," + to_string(stk - 8) + "(sp)");
        Emit("\tadd\tsp,sp," + to_string(stk));
        Emit("\tjr\tra");
        return;
    }

    std::cerr<<"Intrin not found!: ";
    for(auto v : line) std::cerr<<v<<" ";
    std::cerr<<std::endl;

    if(len == 5 && Contains(reg_names, line[0]) 
            /*&& Contains(reg_names, line[2]) && Contains(reg_names, line[4])
            && Contains(expr_ops, line[3])*/)
    {
        std::cerr<<"HERE!"<<std::endl;
    }
}



/* MAIN ROUTINE */
int main()
{
    Initialize();
    std::string line;
    while(getline(std::cin, line))
    {
        /* remove comments */
        //if(line.find("//") != std::string::npos)
        line = line.substr(0, line.find("//"));
        std::cerr<<"// "<<line<<std::endl;
        gencode(split(line, ' '));
    }
    Emit("");
    Emit("");
}
