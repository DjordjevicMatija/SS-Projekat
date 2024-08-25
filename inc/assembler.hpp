#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>

using namespace std;

extern void print_hex(const vector<char> &value, ostream &out);

enum BindingType
{
  LOCAL,
  GLOBAL,
  EXTERN,
  SECTION,
  TYPE_FILE
};

enum OperandType
{
  TYPE_LITERAL,
  TYPE_SYMBOL,
  REG
};

enum AddressType
{
  VALUE_LITERAL,
  VALUE_SYMBOL,
  MEM_LITERAL,
  MEM_SYMBOL,
  VALUE_REG,
  MEM_REG,
  MEM_REG_LITERAL,
  MEM_REG_SYMBOL
};

enum RelocationType
{
  ABSOLUTE
};

struct ForwardReference
{
  int sectionIndex;
  int patchOffset; // pocetak instrukcije
  ForwardReference *nextRef;
  bool isWordDir;

  ForwardReference(int secInd, int offset, bool wordDir)
      : sectionIndex(secInd), patchOffset(offset), nextRef(nullptr), isWordDir(wordDir) {}
};

struct Symbol
{
  static int ID;
  int index;
  int value; // offset simbola
  BindingType type;
  int section; // kojoj sekciji pripada, ABS - 1, UNDEF - 0
  bool defined;
  ForwardReference *flink;

  Symbol(int value, BindingType symType, int symSection, bool isDefined)
      : value(value), type(symType), section(symSection), defined(isDefined)
  {
    index = ++ID;
    flink = nullptr;
  }

  void print(ostream &out)
  {
    out << hex  << index << " "<< value << " " << type << " " << section << " " << defined << " ";
  }
};

struct RelocationEntry
{
  int offset; // offset u odnosu na sekciju
  RelocationType relocationType;
  int symbolIndex; // koji simblol treba da se doda

  RelocationEntry(int offset, RelocationType relocationType, int symbolIndex)
      : offset(offset), relocationType(relocationType), symbolIndex(symbolIndex) {}

  void print(ostream &out)
  {
    out << hex << offset << " " << relocationType << " " << symbolIndex << endl;
  }
};

struct Section
{
  string *name;
  int sectionIndex;
  int locationCounter; // u bajtovima
  int size;            // u bajtovima
  int startingAddress;
  vector<char> *value;
  map<int, vector<int>> *symbolPool;  //<index, vector<offset>>
  map<int, vector<int>> *literalPool; //<value, vector<offset>

  Section(string name, int sectionIndex)
      : sectionIndex(sectionIndex), locationCounter(0), size(0)
  {
    this->name = new string(name);
    value = new vector<char>();
    symbolPool = new map<int, vector<int>>();
    literalPool = new map<int, vector<int>>();
  }

  void print(ostream &out)
  {
    out << hex << sectionIndex << " " << *name << " " << size << endl;
    print_hex(*value, out);
  }

  ~Section() {
        delete value;
        delete symbolPool;
        delete literalPool;
    }
};

struct JumpArgument
{
  string *operand;
  OperandType operandType;

  JumpArgument(string *name, OperandType type)
      : operand(name), operandType(type) {}
};

struct DataArguments
{
  vector<string *> *operand;
  vector<OperandType> *operandType;
  AddressType addressType;

  DataArguments(string *name, OperandType type, AddressType addrType)
      : addressType(addrType)
  {
    operand = new vector<string *>();
    operand->push_back(name);
    operandType = new vector<OperandType>();
    operandType->push_back(type);
  }
};

struct DirectiveArguments
{
  vector<string *> *operand;
  vector<OperandType> *operandType;

  DirectiveArguments(string *name, OperandType type)
  {
    operand = new vector<string *>();
    operand->push_back(name);
    operandType = new vector<OperandType>();
    operandType->push_back(type);
  }
};

void asmLabel(string *label);

void asmGlobalDir(DirectiveArguments *arguments);
void asmExternDir(DirectiveArguments *arguments);
void asmSectionDir(string *name);
void asmWordDir(DirectiveArguments *arguments);
void asmSkipDir(string *literal);
void asmEndDir();

void asmHALT();
void asmINT();
void asmIRET();
void asmCALL(JumpArgument *argument);
void asmRET();

void asmJMP(JumpArgument *operand);
void asmBEQ(string *gpr1, string *gpr2, JumpArgument *argument);
void asmBNE(string *gpr1, string *gpr2, JumpArgument *argument);
void asmBGT(string *gpr1, string *gpr2, JumpArgument *argument);

void asmPUSH(string *gpr);
void asmPOP(string *gpr);

void asmXCHG(string *gprS, string *gprD);

void asmADD(string *gprS, string *gprD);
void asmSUB(string *gprS, string *gprD);
void asmMUL(string *gprS, string *gprD);
void asmDIV(string *gprS, string *gprD);

void asmNOT(string *gpr);
void asmAND(string *gprS, string *gprD);
void asmOR(string *gprS, string *gprD);
void asmXOR(string *gprS, string *gprD);
void asmSHL(string *gprS, string *gprD);
void asmSHR(string *gprS, string *gprD);

void asmLD(DataArguments *arguments, string *gpr);
void asmST(string *gpr, DataArguments *arguments);
void asmCSRRD(string *csr, string *gpr);
void asmCSRWR(string *gpr, string *csr);

void printSymbolTable(ostream &out);
void printSections(ostream &out);
void printRelocationTable(ostream &out);
void writeToOutput(const string &output);

void backpatch(Symbol *symbol);
void addRelocationEntry(Section *section, RelocationEntry *newReloc);
void addToPool(map<int, vector<int>> *pool, int index, int offset);
void writeToSection(Section *section, int firstByte, int secondByte, int thirdByte, int fourthByte);

void callOrJumpInstruction(JumpArgument *argument, int code, int reg1, int reg2);
int literalToValue(string *literal);
void checkSymbolExistence(string *symbol);
int symbolToValue(string *symbolName);
int toSigned12b(int value);