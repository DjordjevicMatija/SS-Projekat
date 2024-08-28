#include "emulator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <string>
#include <vector>
#include <map>

using namespace std;

map<unsigned int, int> memory = map<unsigned int, int>();
Cpu cpu;

bool halt = false;
unsigned int programStart = 0x40000000;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cerr << "Invalid number of arguments" << endl;
    return -1;
  }

  string input = argv[1];

  ifstream inputFile(input);
  if (!inputFile)
  {
    cerr << "Error opening file: " << input << endl;
    return 1;
  }

  initializeMemory(inputFile);

  executeProgram();

  printFinishState();

  return 0;
}

void initializeMemory(ifstream &inputFile)
{
  string line;
  while (getline(inputFile, line))
  {
    stringstream ss(line);
    unsigned int address;
    string hexValues;

    ss >> hex >> address;

    ss.ignore(numeric_limits<streamsize>::max(), ':');
    ss.ignore();

    while (ss >> hexValues)
    {
      int value = stoi(hexValues, nullptr, 16);
      memory[address] = value;

      address++;
    }
  }
}

int readFromMemory(unsigned int address)
{
  int data = 0;
  for (int i = 0; i < 4; i++)
  {
    data |= memory[address + i] << (8 * (3 - i));
  }

  return data;
}

void writeToMemory(unsigned int address, int data)
{
  for (int i = 0; i < 4; i++)
  {
    memory[address + i] = (data >> (8 * (3 - i))) & 0xFF;
  }
}

void executeProgram()
{
  cpu.reg[PC] = programStart;
  while (memory.find(cpu.reg[PC]) != memory.end() && !halt)
  {
    int instruction = readFromMemory(cpu.reg[PC]);
    // pc ukazuje na sledecu instrukciju
    cpu.reg[PC] += 4;

    int code = (instruction & 0xf0000000) >> 28;
    int mode = (instruction & 0x0f000000) >> 24;
    int regA = (instruction & 0x00f00000) >> 20;
    int regB = (instruction & 0x000f0000) >> 16;
    int regC = (instruction & 0x0000f000) >> 12;
    int disp = instruction & 0x00000fff;

    cout << hex << "Instruction code: " << instruction << endl;
    switch (code)
    {
    case HALT:
      handleHALT();
      break;
    case INT:
      handleINT();
      break;
    case CALL:
      handleCALL(mode, regA, regB, disp);
      break;
    case JMP:
      handleJMP(mode, regA, regB, regC, disp);
      break;
    case XCHG:
      handleXCHG(regB, regC);
      break;
    case ARITHMETIC:
      handleARITHMETIC(mode, regA, regB, regC);
      break;
    case LOGICAL:
      handleLOGICAL(mode, regA, regB, regC);
      break;
    case SHIFT:
      handleSHIFT(mode, regA, regB, regC);
      break;
    case STORE:
      handleSTORE(mode, regA, regB, regC, disp);
      break;
    case LOAD:
      handleLOAD(mode, regA, regB, regC, disp);
      break;
    default:
      cerr << "Error: Bad operation code" << endl;
      break;
    }
  }
}

void printFinishState()
{
  cout << "-----------------------------------------------------------------\n";
  cout << "Emulated processor executed halt instruction\n";
  cout << "Emulated processor state:\n";

  // Print all general-purpose registers (r0 - r15)
  for (int i = 0; i < 16; ++i)
  {
    cout << dec << "r" << i << "=0x"
         << setw(8) << setfill('0') << hex << uppercase
         << (cpu.reg[i] & 0xFFFFFFFF);

    // Print a newline after every 4 registers
    if ((i + 1) % 4 == 0)
    {
      cout << endl;
    }
    else
    {
      cout << " ";
    }
  }
}

void handleHALT()
{
  halt = true;
}

void handleINT()
{
  // push status
  cpu.reg[SP] -= 4;
  writeToMemory(cpu.reg[SP], cpu.csr[STATUS]);
  // push pc
  cpu.reg[SP] -= 4;
  writeToMemory(cpu.reg[SP], cpu.reg[PC]);
  // cause <= 4
  cpu.csr[CAUSE] = 4;
  // status<=status&(~0x4)
  cpu.csr[STATUS] &= (~0x4);
  // pc<=handler
  cpu.reg[PC] = cpu.csr[HANDLER];
}

void handleCALL(int mode, int regA, int regB, int disp)
{
  switch (mode)
  {
  case 0x00:
    // push pc
    cpu.reg[SP] -= 4;
    writeToMemory(cpu.reg[SP], cpu.reg[PC]);
    // pc<=gpr[A]+gpr[B]+D
    cpu.reg[PC] = cpu.reg[regA] + cpu.reg[regB] + disp;
    break;
  case 0x01:
    // push pc
    cpu.reg[SP] -= 4;
    writeToMemory(cpu.reg[SP], cpu.reg[PC]);
    // pc<=mem32[gpr[A]+gpr[B]+D]
    cpu.reg[PC] = readFromMemory(cpu.reg[regA] + cpu.reg[regB] + disp);
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleJMP(int mode, int regA, int regB, int regC, int disp)
{
  switch (mode)
  {
  case 0x00:
    cpu.reg[PC] = cpu.reg[regA] + disp;
    break;
  case 0x01:
    if (cpu.reg[regB] == cpu.reg[regC])
    {
      cpu.reg[PC] = cpu.reg[regA] + disp;
    }
    break;
  case 0x02:
    if (cpu.reg[regB] != cpu.reg[regC])
    {
      cpu.reg[PC] = cpu.reg[regA] + disp;
    }
    break;
  case 0x03:
    if (cpu.reg[regB] > cpu.reg[regC])
    {
      cpu.reg[PC] = cpu.reg[regA] + disp;
    }
    break;
  case 0x08:
    cpu.reg[PC] = readFromMemory(cpu.reg[regA] + disp);
    break;
  case 0x09:
    if (cpu.reg[regB] == cpu.reg[regC])
    {
      cpu.reg[PC] = readFromMemory(cpu.reg[regA] + disp);
    }
    break;
  case 0x0a:
    if (cpu.reg[regB] != cpu.reg[regC])
    {
      cpu.reg[PC] = readFromMemory(cpu.reg[regA] + disp);
    }
    break;
  case 0x0b:
    if (cpu.reg[regB] > cpu.reg[regC])
    {
      cpu.reg[PC] = readFromMemory(cpu.reg[regA] + disp);
    }
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleXCHG(int regB, int regC)
{
  int tmp = cpu.reg[regB];
  cpu.reg[regB] = cpu.reg[regC];
  cpu.reg[regC] = tmp;
}

void handleARITHMETIC(int mode, int regA, int regB, int regC)
{
  switch (mode)
  {
  case 0x00:
    cpu.reg[regA] = cpu.reg[regB] + cpu.reg[regC];
    break;
  case 0x01:
    cpu.reg[regA] = cpu.reg[regB] - cpu.reg[regC];
    break;
  case 0x02:
    cpu.reg[regA] = cpu.reg[regB] * cpu.reg[regC];
    break;
  case 0x03:
    cpu.reg[regA] = cpu.reg[regB] / cpu.reg[regC];
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleLOGICAL(int mode, int regA, int regB, int regC)
{
  switch (mode)
  {
  case 0x00:
    cpu.reg[regA] = ~cpu.reg[regB];
    break;
  case 0x01:
    cpu.reg[regA] = cpu.reg[regB] & cpu.reg[regC];
    break;
  case 0x02:
    cpu.reg[regA] = cpu.reg[regB] | cpu.reg[regC];
    break;
  case 0x03:
    cpu.reg[regA] = cpu.reg[regB] ^ cpu.reg[regC];
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleSHIFT(int mode, int regA, int regB, int regC)
{
  switch (mode)
  {
  case 0x00:
    cpu.reg[regA] = cpu.reg[regB] << cpu.reg[regC];
    break;
  case 0x01:
    cpu.reg[regA] = cpu.reg[regB] >> cpu.reg[regC];
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleSTORE(int mode, int regA, int regB, int regC, int disp)
{
  int addr = 0;
  switch (mode)
  {
  case 0x00:
    writeToMemory(cpu.reg[regA] + cpu.reg[regB] + disp, cpu.reg[regC]);
    break;
  case 0x02:
    addr = readFromMemory(cpu.reg[regA] + cpu.reg[regB]);
    writeToMemory(addr, cpu.reg[regC]);
    break;
  case 0x01:
    cpu.reg[regA] = cpu.reg[regA] + disp;
    writeToMemory(cpu.reg[regA], cpu.reg[regC]);
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}

void handleLOAD(int mode, int regA, int regB, int regC, int disp)
{
  switch (mode)
  {
  case 0x00:
    cpu.reg[regA] = cpu.csr[regB];
    break;
  case 0x01:
    cpu.reg[regA] = cpu.reg[regB] + disp;
    break;
  case 0x02:
    cpu.reg[regA] = readFromMemory(cpu.reg[regB] + cpu.reg[regC] + disp);
    break;
  case 0x03:
    cpu.reg[regA] = readFromMemory(cpu.reg[regB]);
    cpu.reg[regB] = cpu.reg[regB] + disp;
    break;
  case 0x04:
    cpu.csr[regA] = cpu.reg[regB];
    break;
  case 0x05:
    cpu.csr[regA] = cpu.csr[regB] | disp;
    break;
  case 0x06:
    cpu.csr[regA] = readFromMemory(cpu.reg[regB] + cpu.reg[regC] + disp);
    break;
  case 0x07:
    cpu.csr[regA] = readFromMemory(cpu.reg[regB]);
    cpu.reg[regB] = cpu.reg[regB] + disp;
    break;
  default:
    cerr << "Error: Bad operation code" << endl;
    break;
  }
}
