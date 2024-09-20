#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

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

enum RelocationType
{
  ABSOLUTE
};

struct Symbol
{
  static int ID;
  static int SYMBOL_NAME_INCREMENT;
  int index;
  int oldIndex; // symbol index iz asemblera
  int value;    // adresa simbola
  BindingType type;
  int section; // kojoj sekciji pripada, ABS - 1, UNDEF - 0
  int oldSection;
  bool defined;

  Symbol(int oldIndex, int value, BindingType symType, int oldSymSection)
      : index(0), oldIndex(oldIndex), value(value), type(symType),
        section(oldSymSection), oldSection(oldSymSection), defined(false) {}

  void print(ostream &out)
  {
    out << hex << index << " " << value << " " << type << " " << section << " " << defined << " ";
  }
};

struct RelocationEntry
{
  int offset; // offset u odnosu na sekciju
  RelocationType relocationType;
  int symbolIndex; // koji simblol treba da se doda
  int newSymbolIndex;
  RelocationEntry(int offset, RelocationType relocationType, int symbolIndex)
      : offset(offset), relocationType(relocationType), symbolIndex(symbolIndex), newSymbolIndex(symbolIndex) {}

  void print(ostream &out)
  {
    out << hex << offset << " " << relocationType << " " << symbolIndex << endl;
  }
};

struct Section
{
  string *name;
  int sectionIndex;
  int oldSectionIndex;
  int size; // u bajtovima
  unsigned int startingAddress;
  vector<char> *value;

  Section(string name, int sectionIndex, int size)
      : sectionIndex(sectionIndex), oldSectionIndex(sectionIndex), size(size), startingAddress(0)
  {
    this->name = new string(name);
    value = new vector<char>();
  }

  void print(ostream &out)
  {
    out << hex << sectionIndex << " " << *name << " " << size << endl;
    print_hex(*value, out);
  }

  ~Section()
  {
    delete value;
  }
};

vector<ifstream> processArguments(int argc, char *argv[], string &output);
void startLinker();
void getSymbolTable(ifstream &file);
void getRelocationTable(ifstream &file);
void getSections(ifstream &file);

void addRelocationEntry(map<int, vector<RelocationEntry *>> *relocTable, int sectionIndex, RelocationEntry *newReloc);

void processSections();
void processSymbols();
void addNewSymbol(Symbol *newSymbol, string newSymolName);
void processRelocations();
void resolveSymbolConflict(Symbol *oldSymbol, Symbol *newSymbol, string symbolName);
string renameLocalSymbol(string oldSymbolName);
void changeExternToGlobal(Symbol *externSymbol, Symbol *globalSymbol);
void updateExternGlobalRelocations(Symbol *oldSymbol, Symbol *newSymbol);

void assignStartingAddresses();
void assignSymbolAddressForSection(Section *section);
void performRelocations();
void writeToSection(Section *section, int offset, int firstByte, int secondByte, int thirdByte, int fourthByte);

void validateLinker();

void printSymbolTable(ostream &out);
void printSections(ostream &out);
void printRelocationTable(ostream &out);
void writeToOutput(const string &output);
void printHexRepresentation(ostream &out);