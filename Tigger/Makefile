
OBJ = common.o Resources.o RegAlloc.o Intrin.o Tigger.o

FLAGS = -std=c++14 -g -Ofast

tigger: ${OBJ} #Tigger.tab.cpp lex.yy.cpp
	g++ $^ -o $@ -lfl ${FLAGS}
	#cp tigger test/tigger

%.o: %.cpp %.h
	g++ -c $^ ${FLAGS}

.PHONY: clean clean_intermediate lines

clean_intermediate:
	- rm -f *.gch
	- rm -f *.tab.* lex.yy.*
	- rm -f ${OBJ}

clean:
	- rm -f *.gch
	- rm -f *.tab.* lex.yy.*
	- rm -f ${OBJ}
	- rm -f tigger

lines:
	- wc -l *.cpp *.h 
