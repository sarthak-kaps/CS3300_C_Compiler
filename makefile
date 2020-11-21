a.out: y.tab.cpp lex.yy.cpp IRGenerator.hpp codeGenerator.hpp optimizer.hpp
	g++ -O3 lex.yy.cpp y.tab.cpp IRGenerator.hpp codeGenerator.hpp
	@echo "Run the program as ./a.out < [input_file]"

y.tab.cpp: a.y
	yacc -d a.y -o y.tab.cpp

lex.yy.cpp: a.l y.tab.hpp
	lex -o lex.yy.cpp a.l

clean:
	@rm -f lex.yy.cpp y.tab.hpp y.tab.cpp a.out
