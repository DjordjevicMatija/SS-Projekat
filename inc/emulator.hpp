#include <fstream>

using namespace std;

#define PC 15
#define SP 14
#define STATUS 0
#define HANDLER 1
#define CAUSE 2

struct Cpu{
  // pc - 15, sp -14
  int reg[16];
  // status - 0, handler - 1, cause - 2
  int csr[3];
};

#define HALT 0x0
#define INT 0x1
#define CALL 0x2
#define JMP 0x3
#define XCHG 0x4
#define ARITHMETIC 0x5
#define LOGICAL 0x6
#define SHIFT 0x7
#define STORE 0x8
#define LOAD 0x9

void handleHALT();
void handleINT();
void handleCALL(int mode, int regA, int regB, int disp);
void handleJMP(int mode, int regA, int regB, int regC, int disp);
void handleXCHG(int regB, int regC);
void handleARITHMETIC(int mode, int regA, int regB, int regC);
void handleLOGICAL(int mode, int regA, int regB, int regC);
void handleSHIFT(int mode, int regA, int regB, int regC);
void handleSTORE(int mode, int regA, int regB, int regC, int disp);
void handleLOAD(int mode, int regA, int regB, int regC, int disp);

void initializeMemory(ifstream& inputFile);
int readFromMemory(unsigned int address);
void writeToMemory(unsigned int address, int data);
void executeProgram();
void printFinishState();
