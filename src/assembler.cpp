#include "assembler.hpp"
#include <map>

using namespace std;

int Symbol::ID = 0;
map<int, Section *> *sections = new map<int, Section *>();
map<string, Symbol *> *symbolTable = new map<string, Symbol *>();
vector<RelocationEntry *> *relocationTable = new vector<RelocationEntry *>();

Section *currentSection = nullptr;
int locationCounter = 0;

void printSymbolTable()
{
  cout << "index " << "value " << "type " << "section " << "defined " << "label" << endl;
  for (auto i = symbolTable->cbegin(); i != symbolTable->cend(); i++)
  {
    i->second->print();
    cout << i->first << endl;
  }
}

void printSections()
{
  cout << "index " << "name " << endl;
  for (auto i = sections->cbegin(); i != sections->cend(); i++)
  {
    i->second->print();
  }
}

void printRelocationTable()
{
  cout << "section index " << "offset " << "relocation type " << "symbol index " << "addend " << endl;
  for (int i = 0; i < relocationTable->size(); i++)
  {
    (*relocationTable)[i]->print();
  }
}

void asmLabel(string *label)
{
  cout << "asmLabel start" << endl;

  if (currentSection == nullptr)
  {
    cerr << "Label must be declared in section" << endl;
    exit(-2);
  }

  auto symbolIterator = symbolTable->find(*label);

  if (symbolIterator == symbolTable->end())
  {
    // simbol se ne nalazi u tabeli simbola
    Symbol *newSym = new Symbol(locationCounter, BindingType::LOCAL, to_string(currentSection->sectionIndex), true);
    (*symbolTable)[*label] = newSym;
  }
  else
  {
    // simbol se nalazi u tabeli simbola
    auto symbol = symbolIterator->second;

    if (symbol->defined)
    {
      // dvostruko definisan simbol

      cerr << "Multiple symbol definition" << endl;
      exit(-2);
    }
    else
    {
      // simbol nije definisan

      if (symbol->section == "UNDEF")
      {
        symbol->value = locationCounter;
        symbol->section = to_string(currentSection->sectionIndex);
        symbol->defined = true;

        // dopunjavanje relokacionih zapisa za lokalne relokacije
        for (int i = 0; i < relocationTable->size(); i++)
        {
          if (!(*relocationTable)[i]->resolved && (*relocationTable)[i]->symbolIndex == symbol->index)
          {
            (*relocationTable)[i]->symbolIndex = stoi(symbol->section);
            (*relocationTable)[i]->addend += symbol->value;
            (*relocationTable)[i]->resolved = true;
          }
        }

        // backpatching
        for (auto forwardRef = symbol->flink; forwardRef != nullptr; forwardRef = forwardRef->nextRef)
        {
          // symbol->value treba da se upise na sectionIndex->value[patchOffset]
          auto sectionToPatch = sections->find(forwardRef->sectionIndex);
          if (sectionToPatch != sections->end())
          {
            (*(sectionToPatch->second->value))[forwardRef->patchOffset] = (symbol->value & 0xff);
            (*(sectionToPatch->second->value))[forwardRef->patchOffset + 1] = ((symbol->value >> 8) & 0x0f);
          }
        }
      }
    }
  }

  printSymbolTable();
}

void asmGlobalDir(DirectiveArguments *arguments)
{
  cout << "asmGlobalDir start" << endl;

  for (int i = 0; i < arguments->operand->size(); i++)
  {
    auto symbolIterator = symbolTable->find(*(*arguments->operand)[i]);
    if (symbolIterator == symbolTable->end())
    {
      // simbol se ne nalazi u tabeli simbola
      Symbol *newSym = new Symbol(0, BindingType::GLOBAL, "UNDEF", false);
      (*symbolTable)[*(*arguments->operand)[i]] = newSym;
    }
    else
    {
      // simbol se nalazi u tabeli simbola
      symbolIterator->second->type = BindingType::GLOBAL;
    }
  }

  printSymbolTable();
}

void asmExternDir(DirectiveArguments *arguments)
{
  cout << "asmExternDir start" << endl;

  for (int i = 0; i < arguments->operand->size(); i++)
  {
    auto symbolIterator = symbolTable->find(*(*arguments->operand)[i]);
    if (symbolIterator == symbolTable->end())
    {
      // simbol se ne nalazi u tabeli simbola
      Symbol *newSym = new Symbol(0, BindingType::EXTERN, "UNDEF", false);
      (*symbolTable)[*(*arguments->operand)[i]] = newSym;
    }
    else
    {
      // simbol se nalazi u tabeli simbola
      symbolIterator->second->type = BindingType::EXTERN;
    }
  }

  printSymbolTable();
}

void asmSectionDir(string *name)
{
  cout << "asmSectionDir start" << endl;

  auto symbolIterator = symbolTable->find(*name);
  if (symbolIterator == symbolTable->end())
  {
    // simbol se ne nalazi u tabeli simbola
    Symbol *newSym = new Symbol(0, BindingType::SECTION, "UNDEF", true);
    newSym->section = to_string(newSym->index);
    (*symbolTable)[*name] = newSym;

    Section *newSec = new Section(*name, newSym->index);
    (*sections)[newSym->index] = newSec;

    currentSection = newSec;
    locationCounter = 0;
  }
  else
  {
    // simbol se nalazi u tabeli simbola
    auto section = sections->find(symbolIterator->second->index);
    currentSection = section->second;
    locationCounter = section->second->locationCounter;
  }

  printSymbolTable();
  printSections();
}

void asmWordDir(DirectiveArguments *arguments)
{
}

void asmSkipDir(string *literal)
{
  cout << "asmSkipDir start" << endl;

  if (currentSection == nullptr)
  {
    cerr << "Skip must be declared in section" << endl;
    exit(-2);
  }

  int base = 10;
  if (literal->substr(0, 2) == "0x")
  {
    base = 16;
    *literal = literal->substr(2);
  }
  else if (literal->substr(0, 1) == "0")
  {
    base = 8;
    *literal = literal->substr(1);
  }

  unsigned long skip = stoul(*literal, nullptr, base);
  for (unsigned long i = 0; i < skip; i++)
  {
    currentSection->value->push_back((char)0);
  }
  locationCounter += skip;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;

  printSections();
}

void asmEndDir()
{
}

void asmHALT()
{
  for (int i = 0; i < 4; i++)
  {
    currentSection->value->push_back((char)0);
  }

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmINT()
{
  currentSection->value->push_back((char)0x10);
  for (int i = 0; i < 3; i++)
  {
    currentSection->value->push_back((char)0);
  }

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmIRET()
{
}

void asmCALL(JumpArgument *argument)
{
}

void asmRET()
{
}

void asmJMP(JumpArgument *operand)
{
}

void asmBEQ(string *gpr1, string *gpr2, JumpArgument *argument)
{
}

void asmBNE(string *gpr1, string *gpr2, JumpArgument *argument)
{
}

void asmBGT(string *gpr1, string *gpr2, JumpArgument *argument)
{
}

void asmPUSH(string *gpr)
{
}

void asmPOP(string *gpr)
{
}

void asmXCHG(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x40);
  currentSection->value->push_back((char)dstReg);
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmADD(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x50);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmSUB(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x51);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmMUL(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x52);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmDIV(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x53);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmNOT(string *gpr)
{
  int reg = stoi((*gpr).substr(1));

  currentSection->value->push_back((char)0x60);
  currentSection->value->push_back((char)((reg << 4) | reg));
  currentSection->value->push_back((char)0);
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmAND(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x61);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmOR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x62);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmXOR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x63);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmSHL(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x70);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmSHR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  currentSection->value->push_back((char)0x71);
  currentSection->value->push_back((char)((dstReg << 4) | dstReg));
  currentSection->value->push_back((char)(srcReg << 4));
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmLD(DataArguments *arguments, string *gpr)
{
}

void asmST(string *gpr, DataArguments *arguments)
{
}

void asmCSRRD(string *csr, string *gpr)
{
  int srcReg;
  if(*csr == "STATUS"){
    srcReg = 0;
  }
  else if(*csr == "HANDLER"){
    srcReg = 1;
  }
  else{
    srcReg = 2;
  }
  int dstReg = stoi((*gpr).substr(1));

  currentSection->value->push_back((char)0x91);
  currentSection->value->push_back((char)((dstReg << 4) | srcReg));
  currentSection->value->push_back((char)0);
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmCSRWR(string *gpr, string *csr)
{
  int srcReg = stoi((*gpr).substr(1));
  int dstReg;
  if(*csr == "STATUS"){
    dstReg = 0;
  }
  else if(*csr == "HANDLER"){
    dstReg = 1;
  }
  else{
    dstReg = 2;
  }

  currentSection->value->push_back((char)0x94);
  currentSection->value->push_back((char)((dstReg << 4) | srcReg));
  currentSection->value->push_back((char)0);
  currentSection->value->push_back((char)0);

  locationCounter += 4;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

string output;

extern int start_parser(int argc, char **argv);

int main(int argc, char **argv)
{
  string input = argv[1];

  if (!(input.size() > 2 && input.substr(input.size() - 2) == ".s"))
  {
    cerr << "Input file must end with '.s'" << endl;
    return -1;
  }

  if (argc == 2)
  {
    output = input.substr(0, input.size() - 2) + ".o";
  }
  else if (argc == 4)
  {
    output = argv[3];
  }
  else
  {
    cerr << "Invalid number of arguments" << endl;
    return -1;
  }

  if (!(output.size() > 2 && output.substr(output.size() - 2) == ".o"))
  {
    cerr << "Output file must end with '.o'" << endl;
    return -1;
  }

  Symbol *program = new Symbol(locationCounter, BindingType::TYPE_FILE, "ABS", true);
  (*symbolTable)[argv[0]] = program;

  return start_parser(argc, argv);
}
