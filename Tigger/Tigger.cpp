#include <iostream>

#include "common.h"
#include "Intrin.h"
#include "Tigger.h"

int global_debug_flag = 0;

void testSplit()
{
    std::string input{"Hello/ from/ tigger////v////"};
    auto res = split(input, '/');
    for(auto s : res)
    {
        std::cout<<s<<": "<<s.length()<<std::endl;
    }
}

void testInput()
{
    intr::IntrinManager mgr;
    std::string line;
    while(std::getline(std::cin, line))
    {
        mgr.AddIntrin(line);
    }
    mgr.gencode(stdout);
}

int main(int argc, char *argv[])
{
    testInput();
}
