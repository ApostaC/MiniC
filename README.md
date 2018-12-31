# MiniC
MiniC compiler for compiling practise in PKU
# Compile and Install
To compile, please ensure that there are `flex`and `bison` in the system, and the host c++ compiler should support `c++14` standard.
> cd ${ROOT_DIR_OF_MINIC}/ <p>
> make

Three executables will be available in `bin/`.

# Usage
> cd bin/ <p>
> ./riscv64-linux-minic-as ${source_code}

Assume ${source_code} is `test.c`, after the execution, check `test.S` in the current directory. 
The output of `eeyore`, `tigger` and `riscv64` are also stored in the current dir.
