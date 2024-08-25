#include "assembler.hpp"
#include <map>
#include <iomanip>
#include <fstream>

using namespace std;

int Symbol::ID = 0;
map<int, Section *> *sections = new map<int, Section *>();
map<string, Symbol *> *symbolTable = new map<string, Symbol *>();
map<int, vector<RelocationEntry *>> *relocationTable = new map<int, vector<RelocationEntry *>>();

Section *currentSection = nullptr;
int locationCounter = 0;

void asmLabel(string *label)
{
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
      if (symbol->section == "UNDEF" && symbol->type != BindingType::EXTERN)
      {
        symbol->value = locationCounter;
        symbol->section = to_string(currentSection->sectionIndex);
        symbol->defined = true;

        backpatch(symbol);
      }
      else
      {
        // redefinisan eksterni simbol
        cerr << "Extern symbol multiple definition" << endl;
        exit(-2);
      }
    }
  }
}

void asmGlobalDir(DirectiveArguments *arguments)
{
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
}

void asmExternDir(DirectiveArguments *arguments)
{
  for (int i = 0; i < arguments->operand->size(); i++)
  {
    auto symbolIterator = symbolTable->find(*(*arguments->operand)[i]);
    if (symbolIterator == symbolTable->end())
    {
      // simbol se ne nalazi u tabeli simbola
      Symbol *newSym = new Symbol(0, BindingType::EXTERN, "UNDEF", true);
      (*symbolTable)[*(*arguments->operand)[i]] = newSym;
    }
    else
    {
      // simbol se nalazi u tabeli simbola
      auto symbol = symbolIterator->second;
      if (symbol->defined)
      {
        // redefinisan lokalni simbol
        cerr << "Symbol already defined as local" << endl;
        exit(-2);
      }

      symbol->type = BindingType::EXTERN;
      symbol->defined = true;

      backpatch(symbol);
    }
  }
}

void asmSectionDir(string *name)
{
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
}

void asmWordDir(DirectiveArguments *arguments)
{
  if (currentSection == nullptr)
  {
    cerr << "Word must be declared in section" << endl;
    exit(-2);
  }

  for (int i = 0; i < arguments->operand->size(); i++)
  {
    auto operand = (*(arguments->operand))[i];
    auto operandType = (*(arguments->operandType))[i];

    if (operandType == OperandType::TYPE_LITERAL)
    {
      int value = literalToValue(operand);
      writeToSection(currentSection, value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff, (value >> 24) & 0xff);
    }
    else if (operandType == OperandType::TYPE_SYMBOL)
    {
      auto symbolIterator = symbolTable->find(*operand);
      if (symbolIterator == symbolTable->end())
      {
        // simbol se ne nalazi u tabeli simbola
        Symbol *newSym = new Symbol(0, BindingType::LOCAL, "UNDEF", false);
        (*symbolTable)[*operand] = newSym;

        // kreiraj flink
        ForwardReference *newFlink = new ForwardReference(currentSection->sectionIndex, currentSection->locationCounter, true);
        newSym->flink = newFlink;
      }
      else
      {
        // simbol se nalazi u tabeli simbola
        auto symbol = symbolIterator->second;
        if (symbol->defined)
        {
          // kreiraj relokacioni zapis
          RelocationEntry *newReloc = new RelocationEntry(currentSection->locationCounter, RelocationType::ABSOLUTE, symbol->index);
          addRelocationEntry(currentSection, newReloc);
        }
        else
        {
          // kreiraj flink
          ForwardReference *newFlink = new ForwardReference(currentSection->sectionIndex, currentSection->locationCounter, true);
          newFlink->nextRef = symbol->flink;
          symbol->flink = newFlink;
        }
      }
      // rezervisanje prostora za vrednost simbola
      writeToSection(currentSection, 0, 0, 0, 0);
    }
  }
}

void asmSkipDir(string *literal)
{
  if (currentSection == nullptr)
  {
    cerr << "Skip must be declared in section" << endl;
    exit(-2);
  }

  int skip = literalToValue(literal);
  for (int i = 0; i < skip; i++)
  {
    currentSection->value->push_back((char)0);
  }
  locationCounter += skip;
  currentSection->locationCounter = locationCounter;
  currentSection->size = locationCounter;
}

void asmEndDir()
{
  // provera da li su svi simboli definisani
  for (auto symbolIterator = symbolTable->cbegin(); symbolIterator != symbolTable->cend(); symbolIterator++)
  {
    auto symbol = symbolIterator->second;
    if (!symbol->defined)
    {
      cerr << "Symbol " << symbolIterator->first << " is not defined" << endl;
      exit(-3);
    }
  }

  // kreiranje symbol i literal pool-a za sekciju
  for (auto sectionIterator = sections->begin(); sectionIterator != sections->end(); sectionIterator++)
  {
    auto section = sectionIterator->second;
    auto symPool = sectionIterator->second->symbolPool;
    auto litPool = sectionIterator->second->literalPool;

    // symbolPool
    for (auto symbolIterator = symPool->begin(); symbolIterator != symPool->end(); symbolIterator++)
    {
      int symbolIndex = symbolIterator->first;
      auto offsets = symbolIterator->second;

      // kreiranje relokacionog zapisa

      RelocationEntry *newReloc = new RelocationEntry(currentSection->locationCounter, RelocationType::ABSOLUTE, symbolIndex);
      addRelocationEntry(currentSection, newReloc);
      // prolazimo kroz offsets
      for (int i = 0; i < offsets.size(); i++)
      {
        // za svaki off racuna displacement = locationCounter - off
        // section[off] = displacement  - samo poslednjih 12b menjamo
        int offset = offsets[i];
        int displacement = section->locationCounter - offset;
        (*(section->value))[offset + 2] |= ((displacement >> 8) & 0x0f);
        (*(section->value))[offset + 3] = (displacement & 0xff);
      }
      // alociraj prostor
      writeToSection(section, 0, 0, 0, 0);
    }
    // literalPool
    for (auto literalIterator = litPool->begin(); literalIterator != litPool->end(); literalIterator++)
    {
      int value = literalIterator->first;
      auto offsets = literalIterator->second;

      // prolazimo kroz offsets
      for (int i = 0; i < offsets.size(); i++)
      {
        // za svaki off racuna displacement = locationCounter - off
        // section[off] = displacement  - samo poslednjih 12b menjamo
        int offset = offsets[i];
        int displacement = section->locationCounter - offset;

        (*(section->value))[offset + 2] |= ((displacement >> 8) & 0x0f);
        (*(section->value))[offset + 3] = (displacement & 0xff);
      }
      // alociraj prostor
      writeToSection(section, (value >> 24) & 0xff, (value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
    }
  }

  printSymbolTable(cout);
  printRelocationTable(cout);
  printSections(cout);
}

void asmHALT()
{
  writeToSection(currentSection, 0, 0, 0, 0);
}

void asmINT()
{
  writeToSection(currentSection, 0x10, 0, 0, 0);
}

void asmIRET()
{
  // uvecavamo SP za 8
  writeToSection(currentSection, 0x91, (14 << 4) | 14, 0, 8);

  // pop STATUS
  writeToSection(currentSection, 0x96, 14, (-4 >> 8) & 0x0f, -4 & 0xff);

  // pop PC
  writeToSection(currentSection, 0x96, (15 << 4) | 14, (-8 >> 8) & 0x0f, -8 & 0xff);
}

void asmCALL(JumpArgument *argument)
{
  callOrJumpInstruction(argument, 0x20, 0, 0);
}

void asmRET()
{
  string pc = "R15";
  asmPOP(&pc);
}

void asmJMP(JumpArgument *argument)
{
  callOrJumpInstruction(argument, 0x30, 0, 0);
}

void asmBEQ(string *gpr1, string *gpr2, JumpArgument *argument)
{
  int reg1 = stoi((*gpr1).substr(1));
  int reg2 = stoi((*gpr1).substr(1));

  callOrJumpInstruction(argument, 0x31, reg1, reg2);
}

void asmBNE(string *gpr1, string *gpr2, JumpArgument *argument)
{
  int reg1 = stoi((*gpr1).substr(1));
  int reg2 = stoi((*gpr1).substr(1));

  callOrJumpInstruction(argument, 0x32, reg1, reg2);
}

void asmBGT(string *gpr1, string *gpr2, JumpArgument *argument)
{
  int reg1 = stoi((*gpr1).substr(1));
  int reg2 = stoi((*gpr1).substr(1));

  callOrJumpInstruction(argument, 0x33, reg1, reg2);
}

void asmPUSH(string *gpr)
{
  int reg = stoi((*gpr).substr(1));

  writeToSection(currentSection, 0x81, 14 << 4, (reg << 4) | ((-4 >> 8) & 0x0f), -4 & 0xff);
}

void asmPOP(string *gpr)
{
  int reg = stoi((*gpr).substr(1));

  writeToSection(currentSection, 0x93, (reg << 4) | 14, 0, 4);
}

void asmXCHG(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x40, dstReg, srcReg << 4, 0);
}

void asmADD(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x50, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmSUB(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x51, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmMUL(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x52, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmDIV(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x53, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmNOT(string *gpr)
{
  int reg = stoi((*gpr).substr(1));

  writeToSection(currentSection, 0x60, (reg << 4) | reg, 0, 0);
}

void asmAND(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x61, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmOR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x62, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmXOR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x63, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmSHL(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x70, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmSHR(string *gprS, string *gprD)
{
  int srcReg = stoi((*gprS).substr(1));
  int dstReg = stoi((*gprD).substr(1));

  writeToSection(currentSection, 0x71, (dstReg << 4) | dstReg, srcReg << 4, 0);
}

void asmLD(DataArguments *arguments, string *gpr)
{
  auto addressType = arguments->addressType;
  int reg = stoi((*gpr).substr(1));
  auto firstOperand = (*(arguments->operand))[0];
  string *secondOperand;
  if (addressType == AddressType::MEM_REG_LITERAL || addressType == AddressType::MEM_REG_SYMBOL)
  {
    secondOperand = (*(arguments->operand))[1];
  }

  int value = 0;
  int regOperand = 0;
  switch (addressType)
  {
  case AddressType::VALUE_LITERAL:
    value = literalToValue(firstOperand);
    if (value <= 2047 && value >= -2048)
    {
      // literal moze da stane u 12b
      value = toSigned12b(value);
      writeToSection(currentSection, 0x91, reg << 4, (value >> 8) & 0x0f, value & 0xff);
    }
    else
    {
      // litera ne moze da stane u 12b
      addToPool(currentSection->literalPool, value, currentSection->locationCounter);
      writeToSection(currentSection, 0x92, reg << 4, 15 << 4, 0);
    }
    break;
  case AddressType::VALUE_SYMBOL:
    checkSymbolExistence(firstOperand);
    writeToSection(currentSection, 0x92, reg << 4, 15 << 4, 0);
    break;
  case AddressType::MEM_LITERAL:
    value = literalToValue(firstOperand);
    if (value <= 2047 && value >= -2048)
    {
      // literal moze da stane u 12b
      value = toSigned12b(value);
      writeToSection(currentSection, 0x92, reg << 4, (value >> 8) & 0x0f, value & 0xff);
    }
    else
    {
      // litera ne moze da stane u 12b
      addToPool(currentSection->literalPool, value, currentSection->locationCounter);
      writeToSection(currentSection, 0x92, reg << 4, 15 << 4, 0);
    }
    break;
  case AddressType::MEM_SYMBOL:
    checkSymbolExistence(firstOperand);
    writeToSection(currentSection, 0x92, reg << 4, 15 << 4, 0);
    break;
  case AddressType::VALUE_REG:
    regOperand = stoi((*firstOperand).substr(1));
    writeToSection(currentSection, 0x91, (reg << 4) | regOperand, 0, 0);
    break;
  case AddressType::MEM_REG:
    regOperand = stoi((*firstOperand).substr(1));
    writeToSection(currentSection, 0x92, (reg << 4) | regOperand, 0, 0);
    break;
  case AddressType::MEM_REG_LITERAL:
    regOperand = stoi((*firstOperand).substr(1));
    value = literalToValue(secondOperand);
    if (value <= 2047 && value >= -2048)
    {
      value = toSigned12b(value);
      writeToSection(currentSection, 0x92, (reg << 4) | regOperand, (value >> 8) & 0x0f, value & 0xff);
    }
    else
    {
      cerr << "Operand cant be represented on 12b:" << *secondOperand << endl;
      exit(-4);
    }
    break;
  case AddressType::MEM_REG_SYMBOL:
    regOperand = stoi((*firstOperand).substr(1));
    value = symbolToValue(secondOperand);
    if (value <= 2047 && value >= -2048)
    {
      value = toSigned12b(value);
      writeToSection(currentSection, 0x92, (reg << 4) | regOperand, (value >> 8) & 0x0f, value & 0xff);
    }
    else
    {
      cerr << "Operand cant be represented on 12b: " << *secondOperand << endl;
      exit(-4);
    }
    break;
  }
}

void asmST(string *gpr, DataArguments *arguments)
{
  auto addressType = arguments->addressType;
  if (addressType == AddressType::VALUE_LITERAL || addressType == AddressType::VALUE_SYMBOL || addressType == AddressType::VALUE_REG)
  {
    cerr << "Immidiate addressing with ST instruction" << endl;
    exit(-4);
  }

  int reg = stoi((*gpr).substr(1));
  auto firstOperand = (*(arguments->operand))[0];
  string *secondOperand;
  if (addressType == AddressType::MEM_REG_LITERAL || addressType == AddressType::MEM_REG_SYMBOL)
  {
    secondOperand = (*(arguments->operand))[1];
  }

  int value = 0;
  int regOperand = 0;
  switch (addressType)
  {
  case AddressType::MEM_LITERAL:
    value = literalToValue(firstOperand);
    if (value <= 2047 && value >= -2048)
    {
      // literal moze da stane u 12b
      value = toSigned12b(value);
      writeToSection(currentSection, 0x80, 0, (reg << 4) | ((value >> 8) & 0x0f), value & 0xff);
    }
    else
    {
      // litera ne moze da stane u 12b
      addToPool(currentSection->literalPool, value, currentSection->locationCounter);
      writeToSection(currentSection, 0x82, 15, reg << 4, 0);
    }
    break;
  case AddressType::MEM_SYMBOL:
    checkSymbolExistence(firstOperand);
    writeToSection(currentSection, 0x82, 15, reg << 4, 0);
    break;
  case AddressType::MEM_REG:
    regOperand = stoi((*firstOperand).substr(1));
    writeToSection(currentSection, 0x80, regOperand << 4, reg << 4, 0);
    break;
  case AddressType::MEM_REG_LITERAL:
    regOperand = stoi((*firstOperand).substr(1));
    value = literalToValue(secondOperand);
    if (value <= 2047 && value >= -2048)
    {
      value = toSigned12b(value);
      writeToSection(currentSection, 0x80, regOperand << 4, (reg << 4) | ((value >> 8) & 0x0f), value & 0xff);
    }
    else
    {
      cerr << "Operand cant be represented on 12b: " << *secondOperand << endl;
      exit(-4);
    }
    break;
  case AddressType::MEM_REG_SYMBOL:
    regOperand = stoi((*firstOperand).substr(1));
    value = symbolToValue(secondOperand);
    if (value <= 2047 && value >= -2048)
    {
      value = toSigned12b(value);
      writeToSection(currentSection, 0x80, regOperand << 4, (reg << 4) | ((value >> 8) & 0x0f), value & 0xff);
    }
    else
    {
      cerr << "Operand cant be represented on 12b: " << *secondOperand << endl;
      exit(-4);
    }
    break;
  }
}

void asmCSRRD(string *csr, string *gpr)
{
  int srcReg;
  if ((*csr).compare("STATUS") == 0)
  {
    srcReg = 0;
  }
  else if ((*csr).compare("HANDLER") == 0)
  {
    srcReg = 1;
  }
  else
  {
    srcReg = 2;
  }
  int dstReg = stoi((*gpr).substr(1));

  writeToSection(currentSection, 0x90, (dstReg << 4) | srcReg, 0, 0);
}

void asmCSRWR(string *gpr, string *csr)
{
  int srcReg = stoi((*gpr).substr(1));
  int dstReg;
  if (*csr == "STATUS")
  {
    dstReg = 0;
  }
  else if (*csr == "HANDLER")
  {
    dstReg = 1;
  }
  else
  {
    dstReg = 2;
  }
  writeToSection(currentSection, 0x94, (dstReg << 4) | srcReg, 0, 0);
}

void printSymbolTable(ostream &out)
{
  out << "SYMBOL_TABLE" << endl;
  out << "index|" << "value|" << "type|" << "section|" << "defined|" << "label" << endl;
  for (auto i = symbolTable->cbegin(); i != symbolTable->cend(); i++)
  {
    i->second->print(out);
    out << i->first << endl;
  }
  out << endl;
}

void printSections(ostream &out)
{
  out << "SECTIONS" << endl;
  out << "index|" << "name " << endl;
  for (auto i = sections->cbegin(); i != sections->cend(); i++)
  {
    i->second->print(out);
    out << endl;
  }
}

void printRelocationTable(ostream &out)
{
  out << "RELOCATION_TABLE" << endl;
  out << "section index|" << "offset|" << "relocation type|" << "symbol index" << endl;
  for (auto i = relocationTable->cbegin(); i != relocationTable->cend(); i++)
  {
    out << i->first << " ";
    for (int j = 0; j < i->second.size(); j++)
    {
      (i->second)[j]->print(out);
    }
  }
  out << endl;
}

void backpatch(Symbol *symbol)
{
  for (auto forwardRef = symbol->flink; forwardRef != nullptr; forwardRef = forwardRef->nextRef)
  {
    auto sectionToPatch = sections->find(forwardRef->sectionIndex);
    if (sectionToPatch != sections->end())
    {
      auto section = sectionToPatch->second;

      if (forwardRef->isWordDir)
      {
        // ako je flink nastao u word dir, napravi relokacioni zapis

        RelocationEntry *newReloc = new RelocationEntry(section->locationCounter, RelocationType::ABSOLUTE, symbol->index);
        addRelocationEntry(section, newReloc);
      }
      else
      {
        // ako je flink nije nastao u word dir, dodaj simbol u pool
        addToPool(section->symbolPool, symbol->index, forwardRef->patchOffset);
      }
    }
  }
}

void addRelocationEntry(Section *section, RelocationEntry *newReloc)
{
  auto entry = relocationTable->find(section->sectionIndex);
  if (entry == relocationTable->end())
  {
    (*relocationTable)[section->sectionIndex] = vector<RelocationEntry *>();
  }
  ((*relocationTable)[section->sectionIndex]).push_back(newReloc);
}

void addToPool(map<int, vector<int>> *pool, int index, int offset)
{
  auto item = pool->find(index);
  if (item == pool->end())
  {
    // vrednost ne postoji u pool-u
    (*pool)[index] = vector<int>();
  }
  // vrednost postoji u pool-u
  ((*pool)[index]).push_back(offset);
}

void writeToSection(Section *section, int firstByte, int secondByte, int thirdByte, int fourthByte)
{
  section->value->push_back((char)firstByte);
  section->value->push_back((char)secondByte);
  section->value->push_back((char)thirdByte);
  section->value->push_back((char)fourthByte);

  section->locationCounter += 4;
  section->size = section->locationCounter;
}

void callOrJumpInstruction(JumpArgument *argument, int code, int reg1, int reg2)
{
  auto operand = argument->operand;
  auto operandType = argument->operandType;

  if (operandType == OperandType::TYPE_LITERAL)
  {
    int value = literalToValue(operand);
    if (value <= 2047 && value >= -2048)
    {
      // literal moze da stane u 12b
      value = toSigned12b(value);
      writeToSection(currentSection, code, reg1, (reg2 << 4) | ((value >> 8) & 0x0f), value & 0xff);
    }
    else
    {
      // litera ne moze da stane u 12b
      addToPool(currentSection->literalPool, value, currentSection->locationCounter);
      writeToSection(currentSection, code + 0b1000, (15 << 4) | reg1, reg2 << 4, 0);
    }
  }
  else if (operandType == OperandType::TYPE_SYMBOL)
  {
    checkSymbolExistence(operand);
    writeToSection(currentSection, code + 0b1000, (15 << 4) | reg1, reg2 << 4, 0);
  }
}

int literalToValue(string *literal)
{
  int base = 10;
  if (literal->substr(0, 2) == "0x")
  {
    base = 16;
    *literal = literal->substr(2);
  }
  else if (literal->substr(0, 1) == "0")
  {
    base = 8;
  }

  int value = stoul(*literal, nullptr, base);
  return value;
}

void checkSymbolExistence(string *symbolName)
{
  auto symbolIterator = symbolTable->find(*symbolName);
  if (symbolIterator == symbolTable->end())
  {
    // simbol se ne nalazi u tabeli simbola
    Symbol *newSym = new Symbol(0, BindingType::LOCAL, "UNDEF", false);
    (*symbolTable)[*symbolName] = newSym;

    // kreiraj flink
    ForwardReference *newFlink = new ForwardReference(currentSection->sectionIndex, currentSection->locationCounter, false);
    newSym->flink = newFlink;
  }
  else
  {
    // simbol se nalazi u tabeli simbola
    auto symbol = symbolIterator->second;
    if (symbol->defined)
    {
      // dodaj simbol u pool
      addToPool(currentSection->symbolPool, symbol->index, currentSection->locationCounter);
    }
    else
    {
      // kreiraj flink
      ForwardReference *newFlink = new ForwardReference(currentSection->sectionIndex, currentSection->locationCounter, false);
      newFlink->nextRef = symbol->flink;
      symbol->flink = newFlink;
    }
  }
}

int symbolToValue(string *symbolName)
{
  auto symbolIterator = symbolTable->find(*symbolName);
  if (symbolIterator == symbolTable->end())
  {
    // simbol se ne nalazi u tabeli simbola
    cerr << "Undefined symbol used in LD/ST" << endl;
    exit(-4);
  }
  else
  {
    // simbol se nalazi u tabeli simbola
    auto symbol = symbolIterator->second;
    if (!symbol->defined)
    {
      cerr << "Undefined symbol used in LD/ST" << endl;
      exit(-4);
    }
    if (symbol->section != "ABS")
    {
      cerr << "Undefined symbol used in LD/ST" << endl;
      exit(-4);
    }
    return symbol->value;
  }
}

int toSigned12b(int value)
{
  if (value < 0)
  {
    value += 0x1000;
  }
  return value & 0xFFF;
}

string output;
extern int start_parser(int argc, char **argv);

int main(int argc, char **argv)
{
  if (argc != 2 && argc != 4)
  {
    cerr << "Invalid number of arguments" << endl;
    return -1;
  }

  string input, output;

  if (argc == 2)
  {
    input = argv[1];
    output = input.substr(0, input.size() - 2) + ".o";
  }
  else if (argc == 4)
  {
    if (string(argv[1]) != "-o")
    {
      cerr << "Invalid option. Use '-o' to specify the output file." << endl;
      return -1;
    }
    output = argv[2];
    input = argv[3];
  }

  if (!(input.size() > 2 && input.substr(input.size() - 2) == ".s"))
  {
    cerr << "Input file must end with '.s'" << endl;
    return -1;
  }

  if (!(output.size() > 2 && output.substr(output.size() - 2) == ".o"))
  {
    cerr << "Output file must end with '.o'" << endl;
    return -1;
  }

  Symbol *program = new Symbol(locationCounter, BindingType::TYPE_FILE, "ABS", true);
  (*symbolTable)[argv[0]] = program;

  int success = start_parser(argc, argv);
  if (success)
  {
    writeToOutput(output);
  }

  return 0;
}

void writeToOutput(const string &output)
{
  ofstream outfile(output);
  if (!outfile)
  {
    cerr << "Failed to open output file: " << output << endl;
    exit(-1);
  }

  printSymbolTable(outfile);
  printRelocationTable(outfile);
  printSections(outfile);

  outfile.close();
  if (!outfile.good())
  {
    std::cerr << "Error occurred while writing to output file: " << output << std::endl;
  }
  else
  {
    std::cout << "Output written to file: " << output << std::endl;
  }
}