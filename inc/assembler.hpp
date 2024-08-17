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
  // for jumpOperand instructions
};

struct DataArguments{
  // for dataOperand instructions
};

struct DirectivArguments{
  // for directive lists
};

void asmLabel(string* label);

void asmGlobalDir(DirectivArguments* arguments);
void asmExternDir(DirectivArguments* arguments);
void asmSectionDir(string* name);
void asmWordDir(DirectivArguments* arguments);
void asmSkipDir(string* literal);
void asmEndDir();

void asmHALT();
void asmINT ();
void asmIRET();
void asmCALL(JumpArgument* operand);
void asmRET ();

void asmJMP(JumpArgument* operand);
void asmBEQ(string* gpr1, string* gpr2, JumpArgument* operand);
void asmBNE(string* gpr1, string* gpr2, JumpArgument* operand);
void asmBGT(string* gpr1, string* gpr2, JumpArgument* operand);

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

void asmLD (DataArguments* operand, string* gpr);
void asmST (string* gpr, DataArguments* operand);
void asmCSRRD(string* csr, string* gpr);
void asmCSRWR(string* gpr, string* csr);