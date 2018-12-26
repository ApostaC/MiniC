
OBJ = common.o Resources.o RegAlloc.o Intrin.o Tigger.o

tigger: ${OBJ} #Tigger.tab.cpp lex.yy.cpp
	g++ $^ -o $@ -lfl -std=c++14 -g #-Ofast

%.o: %.cpp %.h
	g++ -c $^ -std=c++14 -g #-Ofast

#Tigger.tab.cpp: Tigger.y
#	bison -d Tigger.y -o Tigger.tab.cpp

#lex.yy.cpp: Tigger.l
#	flex -o lex.yy.cpp Tigger.l

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
