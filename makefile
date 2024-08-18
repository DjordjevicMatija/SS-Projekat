FLEX = flex
BISON = bison
CXX = g++
CXXFLAGS = -Iinc

all: emulator linker asembler

emulator: src/emulator.cpp inc/emulator.hpp | linker
	$(CXX) -o $@ $^

linker: src/linker.cpp inc/linker.hpp | asembler
	$(CXX) -o $@ $^

asembler: src/main.cpp src/assembler.cpp inc/assembler.hpp inc/structs.hpp lex.yy.cpp parser.tab.cpp parser.tab.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	@$(MAKE) clean_assembly

parser.tab.cpp parser.tab.hpp: misc/parser.y
	$(BISON) -d -o $@--defines=parser.tab.hpp $<

lex.yy.cpp: misc/lexer.lex
	$(FLEX) -o $@ $<

parser_lexer:
	$(BISON) -d -o parser.tab.cpp --defines=parser.tab.hpp misc/parser.y
	$(FLEX) -o lex.yy.cpp misc/lexer.lex
	$(CXX) $(CXXFLAGS) -o myparser parser.tab.cpp lex.yy.cpp -lfl

clean_assembly:
	rm -f lex.yy.cpp parser.tab.cpp parser.tab.hpp