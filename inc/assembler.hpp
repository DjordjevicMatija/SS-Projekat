#include <string>
#include <vector>

using namespace std;

struct Symbol{
  // for symbol table
};

struct RelocationEntry{
  // for relocation table
};

struct Section{
  // section data
};

struct ForwardReference{
  int patch;
  ForwardReference* nextRef;
};

struct JumpArgument{
  string* operand;
  int operandType;  //0 - LITERAL, 1 - SYMBOL

  JumpArgument(string* name, int type){
    operand = name;
    operandType = type;
  }
};

struct DataArguments{
  vector<string*>* operand;
  vector<int>* operandType; //0 - LITERAL, 1 - SYMBOL, 2 - REG
  int addressType;  //1 - vrednost LITERAL, 2 - vrednost SYMBOL, 3 - mem[LITERAL], 4 - mem[SYMBOL]
                    //5 - vrednost iz REG, 6 - mem[REG], 7 - mem[REG + LITERAL], 8 - mem[REG + SYMBOL]

  DataArguments(string* name, int type, int addrType){
    operand = new vector<string*>();
    operand->push_back(name);
    operandType = new vector<int>();
    operandType->push_back(type);
    addressType = addrType;
  }
};

struct DirectiveArguments{
  vector<string*>* operand;
  vector<int>* operandType; //0 - LITERAL, 1 - SYMBOL

  DirectiveArguments(string* name, int type){
    operand = new vector<string*>();
    operand->push_back(name);
    operandType = new vector<int>();
    operandType->push_back(type);
  }
};

void asmLabel(string* label);

void asmGlobalDir(DirectiveArguments* arguments);
void asmExternDir(DirectiveArguments* arguments);
void asmSectionDir(string* name);
void asmWordDir(DirectiveArguments* arguments);
void asmSkipDir(string* literal);
void asmEndDir();

void asmHALT();
void asmINT ();
void asmIRET();
void asmCALL(JumpArgument* argument);
void asmRET ();

void asmJMP(JumpArgument* operand);
void asmBEQ(string* gpr1, string* gpr2, JumpArgument* argument);
void asmBNE(string* gpr1, string* gpr2, JumpArgument* argument);
void asmBGT(string* gpr1, string* gpr2, JumpArgument* argument);

void asmPUSH(string* gpr);
void asmPOP (string* gpr);

void asmXCHG(string* gprS, string* gprD);

void asmADD(string* gprS, string* gprD);
void asmSUB(string* gprS, string* gprD);
void asmMUL(string* gprS, string* gprD);
void asmDIV(string* gprS, string* gprD);

void asmNOT(string* gpr);
void asmAND(string* gprS, string* gprD);
void asmOR (string* gprS, string* gprD);
void asmXOR(string* gprS, string* gprD);
void asmSHL(string* gprS, string* gprD);
void asmSHR(string* gprS, string* gprD);

void asmLD (DataArguments* arguments, string* gpr);
void asmST (string* gpr, DataArguments* arguments);
void asmCSRRD(string* csr, string* gpr);
void asmCSRWR(string* gpr, string* csr);