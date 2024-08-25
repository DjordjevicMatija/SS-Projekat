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
  int index;
  int oldIndex; // symbol index iz asemblera
  int value;    // adresa simbola
  BindingType type;
  int section; // kojoj sekciji pripada, ABS - 1, UNDEF - 0
  bool defined;

  Symbol(int oldIndex, int value, BindingType symType, int symSection, bool isDefined)
      : oldIndex(oldIndex), value(value), type(symType), section(symSection), defined(isDefined)
  {
    index = ++ID;
  }

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
  int size; // u bajtovima
  int startingAddress;
  vector<char> *value;

  Section(string name, int sectionIndex, int size)
      : sectionIndex(sectionIndex), size(size), startingAddress(0)
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

void printSymbolTable(ostream &out);
void printSections(ostream &out);
void printRelocationTable(ostream &out);
void writeToOutput(const string &output);