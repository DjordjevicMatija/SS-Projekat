#include "assembler.hpp"
#include <map>

using namespace std;

int Symbol::ID = 0;
map<int, Section*>* sections = new map<int, Section*>();
map<string, Symbol*>* symbolTable = new map<string, Symbol*>();
vector<RelocationEntry*>* relocationTable = new vector<RelocationEntry*>();

Section* currentSection = nullptr;
int locationCounter = 0;

void asmLabel(string* label){
  auto symbolIterator = symbolTable->find(*label);
  
  if(symbolIterator == symbolTable->end()){
    //simbol se ne nalazi u tabeli simbola

    Symbol* newSym = new Symbol(locationCounter, BindingType::LOCAL, to_string(currentSection->sectionIndex), true);
    (*symbolTable)[*label] = newSym;
  }
  else{
    //simbol se nalazi u tabeli simbola

    auto symbol = symbolIterator->second;

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

        //dopunjavanje relokacionih zapisa za lokalne relokacije
        for(int i = 0; i < relocationTable->size(); i++){
          if(!(*relocationTable)[i]->resolved && (*relocationTable)[i]->symbolIndex == symbol->index){
            (*relocationTable)[i]->symbolIndex = stoi(symbol->section);
            (*relocationTable)[i]->addend += symbol->value;
            (*relocationTable)[i]->resolved = true;
          }
        }
        
        //backpatching
        for(auto forwardRef = symbol->flink; forwardRef != nullptr; forwardRef = forwardRef->nextRef){
          //symbol->value treba da se upise na sectionIndex->value->patchOffset
          auto sectionToPatch = sections->find(forwardRef->sectionIndex);
          if(sectionToPatch != sections->end()){
            (*(sectionToPatch->second->value))[forwardRef->patchOffset] = (symbol->value & 0xFF);
            (*(sectionToPatch->second->value))[forwardRef->patchOffset + 1] = ((symbol->value >> 8) & 0x0F);
          }
        }

      }
    }
  }
}

void asmGlobalDir(DirectiveArguments* arguments){

}

void asmExternDir(DirectiveArguments* arguments){

}

void asmSectionDir(string* name){

}

void asmWordDir(DirectiveArguments* arguments){

}

void asmSkipDir(string* literal){

}

void asmEndDir(){

}

void asmHALT(){

}

void asmINT (){

}

void asmIRET(){

}

void asmCALL(JumpArgument* argument){

}

void asmRET (){

}

void asmJMP(JumpArgument* operand){

}

void asmBEQ(string* gpr1, string* gpr2, JumpArgument* argument){

}

void asmBNE(string* gpr1, string* gpr2, JumpArgument* argument){

}

void asmBGT(string* gpr1, string* gpr2, JumpArgument* argument){

}

void asmPUSH(string* gpr){

}

void asmPOP (string* gpr){

}

void asmXCHG(string* gprS, string* gprD){

}

void asmADD(string* gprS, string* gprD){

}

void asmSUB(string* gprS, string* gprD){

}

void asmMUL(string* gprS, string* gprD){

}

void asmDIV(string* gprS, string* gprD){

}

void asmNOT(string* gpr){

}

void asmAND(string* gprS, string* gprD){

}

void asmOR (string* gprS, string* gprD){

}

void asmXOR(string* gprS, string* gprD){

}

void asmSHL(string* gprS, string* gprD){

}

void asmSHR(string* gprS, string* gprD){

}

void asmLD (DataArguments* arguments, string* gpr){

}

void asmST (string* gpr, DataArguments* arguments){

}

void asmCSRRD(string* csr, string* gpr){

}

void asmCSRWR(string* gpr, string* csr){
  
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
