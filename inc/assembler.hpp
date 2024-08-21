#include <string>
#include <vector>
#include <iostream>

using namespace std;

extern void print_hex(const vector<char> &value);

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
  GLOBAL_ABSOLUTE,
  GLOBAL_RELATIVE,
  LOCAL_ABSOLUTE,
  LOCAL_RELATIVE
};

struct ForwardReference
{
  int sectionIndex;
  int patchOffset;
  ForwardReference *nextRef;
};

struct Symbol
{
  static int ID;

  int index;
  int value; // offset ili adresa simbola
  BindingType type;
  string section; // kojoj sekciji pripada, ABS, UNDEF, COMMON
  bool defined;
  ForwardReference *flink;

  Symbol(int value, BindingType symType, string symSection, bool isDefined)
      : value(value), type(symType), section(symSection), defined(isDefined)
  {
    index = ++ID;
    flink = nullptr;
  }

  void print()
  {
    cout << index << ": " << value << " " << type << " " << section << " " << defined << " ";
  }
};

struct RelocationEntry
{
  int sectionIndex;
  int offset; // offset u odnosu na sekciju
  RelocationType relocationType;
  int symbolIndex; // koji simblol treba da se doda
  int addend;
  bool resolved;

  RelocationEntry(int sectionIndex, int offset, RelocationType relocationType, int symbolIndex, int addend)
      : sectionIndex(sectionIndex), offset(offset), relocationType(relocationType),
        symbolIndex(symbolIndex), addend(addend), resolved(false) {}

  void print()
  {
    cout << sectionIndex << " " << offset << " " << relocationType << " " << symbolIndex << " " << addend << endl;
  }
};

struct Section
{
  string *name;
  int sectionIndex;
  int locationCounter; // u bajtovima
  int size; // u bajtovima
  int startingAddress;
  vector<char> *value;

  Section(string name, int sectionIndex)
      : sectionIndex(sectionIndex), locationCounter(0), size(0)
  {
    this->name = new string(name);
    value = new vector<char>();
  }

  void print()
  {
    cout << sectionIndex << ": " << *name << " section: (LC = " << locationCounter << ")" << endl;
    cout << "Value:" << endl;
    print_hex(*value);
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