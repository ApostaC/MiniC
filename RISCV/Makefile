

all: riscv64

riscv64: riscv.cpp
	g++ riscv.cpp -o riscv64 -g -std=c++11

test: riscv64 
	./riscv64 < test/test.tig

.PHONY: clean check

clean:
	rm -f riscv64

check:
	python3 ../../参考文档/riscv64.py < test/check_test.tig > o1.txt
	./riscv64 < test/test.tig > o2.txt
	vimdiff o1.txt o2.txt
