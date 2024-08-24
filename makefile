FLEX = flex
BISON = bison
CXX = g++
CXXFLAGS = -Iinc -g

all: emulator linker assembler

emulator: src/emulator.cpp inc/emulator.hpp | linker
	$(CXX) -o $@ $^

linker: src/linker.cpp inc/linker.hpp | assembler
	$(CXX) -o $@ $^

assembler: src/assembler.cpp src/util.cpp inc/assembler.hpp inc/util.hpp lex.yy.cpp parser.tab.cpp parser.tab.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	@$(MAKE) clean_assembly

parser.tab.cpp parser.tab.hpp: misc/parser.y
	$(BISON) -d -o $@ $<

lex.yy.cpp: misc/lexer.lex
	$(FLEX) -o $@ $<

clean_assembly:
	rm -f lex.yy.cpp parser.tab.cpp parser.tab.hpp myparser