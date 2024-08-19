#include "assembler.hpp"
#include <map>

using namespace std;

int Symbol::ID = 0;
map<int, Section*>* sections = new map<int, Section*>();
map<string, Symbol*>* symbolTable = new map<string, Symbol*>();
Section* currentSection = nullptr;
int locationCounter = 0;

void asmLabel(string* label){
  auto iterator = symbolTable->find(*label);
  
  if(iterator == symbolTable->end()){
    //simbol se ne nalazi u tabeli simbola

    Symbol* newSym = new Symbol(locationCounter, BindingType::LOCAL, to_string(currentSection->sectionIndex), true);
    (*symbolTable)[*label] = newSym;
  }
  else{
    //simbol se nalazi u tabeli simbola

    auto symbol = iterator->second;

    if(symbol->defined){
      //dvostruko definisan simbol

      cerr << "Multiple symbol definition" << endl;
      exit(-2);
    }
    else{
      //simbol nije definisan

      if(symbol->section == "UNDEF"){
        symbol->value = locationCounter;
        symbol->section = to_string(currentSection->sectionIndex);
        symbol->defined = true;

        //relokacioni zapisi

        //backpatching
        
      }
    }
  }
}



string output;

extern int start_parser(int argc, char **argv);

int main(int argc, char **argv){
  string input = argv[1];

  if(!(input.size() > 2 && input.substr(input.size() - 2) == ".s")){
    cerr << "Input file must end with '.s'" << endl;
    return -1;
  }

  if(argc == 2){
    output = input.substr(0, input.size() - 2) + ".0";
  }
  else if(argc == 4){
    output = argv[3];
  }
  else{
    cerr << "Invalid number of arguments" << endl;
    return -1;
  }

  if(!(output.size() > 2 && output.substr(output.size() - 2) == ".o")){
    cerr << "Output file must end with '.o'" << endl;
    return -1;
  }

  Symbol* program = new Symbol(locationCounter, BindingType::FILE, "ABS", true);
  (*symbolTable)[argv[0]] = program;

  return start_parser(argc, argv);
}
