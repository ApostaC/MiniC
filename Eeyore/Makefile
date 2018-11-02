
OBJ = Symbols.o Express.o Statement.o

eeyore: ${OBJ} Eeyore.tab.cpp lex.yy.cpp common.h Eeyore.cpp
	g++ $^ -o $@ -lfl -std=c++14 -g

%.o: %.cpp %.h
	g++ -c $^ -std=c++14 -g

Eeyore.tab.cpp: Eeyore.y
	bison -d Eeyore.y -o Eeyore.tab.cpp

lex.yy.cpp: Eeyore.l
	flex -o lex.yy.cpp Eeyore.l

cmake:
	mkdir -p trybuild;
	(cd trybuild && cmake .. && make)

.PHONY: clean clean_intermediate

clean_intermediate:
	- rm *.gch
	- rm *.tab.* lex.yy.*
	- rm ${OBJ}

clean:
	- rm *.gch
	- rm *.tab.* lex.yy.*
	- rm ${OBJ}
	- rm eeyore
