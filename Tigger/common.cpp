#include <sstream>
#include <cstdio>
#include "common.h"

std::vector<std::string> split(const std::string &s, char c)
{
    std::stringstream sin;
    std::vector<std::string> ret;
    sin.str(s);
    std::string part;
    while(std::getline(sin, part, c))
    {
        if(part.length())
            ret.emplace_back(std::move(part));
    }
    return ret;
}

void Emit(FILE *f, const std::string &str)
{
    fprintf(f, "%s", str.c_str());
}
