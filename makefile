FLEX = flex
BISON = bison
CXX = g++
CXXFLAGS = -Iinc -g

all: emulator linker assembler

emulator: src/emulator.cpp | linker
	$(CXX) $(CXXFLAGS) -o $@ $^

linker: src/linker.cpp src/util.cpp | assembler
	$(CXX) $(CXXFLAGS) -o $@ $^

assembler: src/assembler.cpp src/util.cpp lex.yy.cpp parser.tab.cpp parser.tab.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^
	@$(MAKE) clean_assembly

parser.tab.cpp parser.tab.hpp: misc/parser.y
	$(BISON) -d -o $@ $<

lex.yy.cpp: misc/lexer.lex
	$(FLEX) -o $@ $<

clean_assembly:
	rm -f lex.yy.cpp parser.tab.cpp parser.tab.hpp myparser

clean:
	rm -f assembler linker emulator
	@$(MAKE) clean_object_files

clean_object_files:
	rm -f *.o *.hex