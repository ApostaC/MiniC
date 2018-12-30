

all: riscv64

riscv64: riscv.cpp
	g++ riscv.cpp -o riscv64 -g -std=c++11 -Ofast

test: riscv64 
	./riscv64 < test/test.tig

.PHONY: clean check

clean:
	rm -f riscv64

