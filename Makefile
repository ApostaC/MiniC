all: bin/eeyore bin/tigger bin/riscv64

bin/eeyore: Eeyore/Makefile
	make -C Eeyore
	cp Eeyore/eeyore bin/

bin/tigger: Tigger/Makefile
	make -C Tigger
	cp Tigger/tigger bin/

bin/riscv64: RISCV/Makefile
	make -C RISCV
	cp RISCV/riscv64 bin/

.PHONY: clean

clean:
	- (cd Eeyore; make clean)
	- (cd Tigger; make clean)
	- (cd RISCV; make clean)
