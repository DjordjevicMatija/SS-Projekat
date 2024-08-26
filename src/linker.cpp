#include "linker.hpp"
#include <sstream>

using namespace std;

vector<ifstream> *files = new vector<ifstream>();
map<string, int> *sectionAddress = new map<string, int>();

int Symbol::ID = 0;
int Symbol::SYMBOL_NAME_INCREMENT = 0;
map<string, Section *> *sections = new map<string, Section *>();
map<string, Symbol *> *symbolTable = new map<string, Symbol *>();
map<int, vector<RelocationEntry *>> *relocationTable = new map<int, vector<RelocationEntry *>>();

vector<Section *> *asmSections = new vector<Section *>();
map<string, Symbol *> *asmSymbolTable = new map<string, Symbol *>();
map<int, vector<RelocationEntry *>> *asmRelocationTable = new map<int, vector<RelocationEntry *>>();

map<int, vector<RelocationEntry *>> *tmpRelocationTable = new map<int, vector<RelocationEntry *>>();

int main(int argc, char *argv[])
{
  string output = "default_output.hex";
  auto result = processArguments(argc, argv, output);
  files = &result;

  startLinker();

  printSymbolTable(cout);
  printRelocationTable(cout);
  printSections(cout);

  return 0;
}

vector<ifstream> processArguments(int argc, char *argv[], string &output)
{
  vector<ifstream> inputFiles;
  bool valid = false;

  for (int i = 1; i < argc; ++i)
  {
    string arg = argv[i];

    if (arg == "-o")
    {
      if (i + 1 < argc)
      {
        output = argv[++i];
      }
      else
      {
        cerr << "Error: Output file name expected after '-o'" << endl;
        exit(-1);
      }
    }
    else if (arg.find("--place=") == 0)
    {
      string placeArg = arg.substr(8);
      size_t atPos = placeArg.find('@');
      if (atPos != string::npos)
      {
        string sectionName = placeArg.substr(0, atPos);
        string addressString = placeArg.substr(atPos + 1);
        int address = stoul(addressString.substr(2), nullptr, 16);
        (*sectionAddress)[sectionName] = address;
      }
      else
      {
        cerr << "Error: Invalid format for '--place'. Expected format: --place=<section>@<address>" << endl;
        exit(-1);
      }
    }
    else if (arg == "-hex")
    {
      valid = true;
    }
    else if (arg[0] == '-')
    {
      cerr << "Error: Unknown option '" << arg << "'" << endl;
      exit(-1);
    }
    else
    {
      ifstream inputFile(arg);
      if (inputFile)
      {
        inputFiles.push_back(move(inputFile));
      }
      else
      {
        cerr << "Error: Cannot open input file '" << arg << "'" << endl;
        exit(-1);
      }
    }
  }

  if (inputFiles.empty())
  {
    cerr << "Error: No input files provided" << endl;
    exit(-1);
  }
  if (!valid)
  {
    cerr << "Error: No '-hex' or '-relocatable' provided" << endl;
    exit(-1);
  }

  return inputFiles;
}

void startLinker()
{
  for (int i = 0; i < files->size(); i++)
  {

    ifstream &file = (*files)[i];
    if (!file.is_open())
    {
      cerr << "Error: Unable to open file" << endl;
      exit(-1);
    }

    string line;
    while (getline(file, line))
    {
      if (line == "SYMBOL_TABLE")
      {
        getline(file, line);
        getSymbolTable(file);
      }
      else if (line == "RELOCATION_TABLE")
      {
        getline(file, line);
        getRelocationTable(file);
      }
      else if (line == "SECTIONS")
      {
        getline(file, line);
        getSections(file);
      }
    }

    file.close();

    processSections();
  }
}

void getSymbolTable(ifstream &file)
{
  string line;
  while (getline(file, line))
  {
    if (line.empty())
    {
      break;
    }

    istringstream iss(line);
    string indexS, valueS, typeIntS, sectionS;
    string label;
    bool defined;
    iss >> indexS >> valueS >> typeIntS >> sectionS >> defined >> label;

    int index = stoul(indexS, nullptr, 16);
    int value = stoul(valueS, nullptr, 16);
    int section = stoul(sectionS, nullptr, 16);
    BindingType type;
    type = static_cast<BindingType>(stoul(typeIntS, nullptr, 16));
    Symbol *newSymbol = new Symbol(index, value, type, section, defined);
    (*asmSymbolTable)[label] = newSymbol;
  }
}

void getRelocationTable(ifstream &file)
{
  string line;
  while (getline(file, line))
  {
    if (line.empty())
    {
      break;
    }

    istringstream iss(line);
    string sectionIndexS, offsetS, relocationTypeIntS, symbolIndexS;
    iss >> sectionIndexS >> offsetS >> relocationTypeIntS >> symbolIndexS;

    int sectionIndex = stoul(sectionIndexS, nullptr, 16);
    int offset = stoul(offsetS, nullptr, 16);
    int relocationTypeInt = stoul(relocationTypeIntS, nullptr, 16);
    int symbolIndex = stoul(symbolIndexS, nullptr, 16);
    RelocationType relocationType;
    relocationType = static_cast<RelocationType>(relocationTypeInt);
    RelocationEntry *newReloc = new RelocationEntry(offset, relocationType, symbolIndex);
    addRelocationEntry(asmRelocationTable, sectionIndex, newReloc);
  }
}

void getSections(ifstream &file)
{
  string line;
  while (getline(file, line))
  {
    if (line.empty())
    {
      getline(file, line);
      if (line.empty())
      {
        break;
      }
    }
    istringstream iss(line);
    string indexS, sizeS;
    string name;
    iss >> indexS >> name >> sizeS;

    int index = stoul(indexS, nullptr, 16);
    int size = stoul(sizeS, nullptr, 16);
    Section *newSection = new Section(name, index, size);
    // ucitaj sadrzaj sekcije
    while (getline(file, line))
    {
      if (line.empty())
      {
        break;
      }

      istringstream hexStream(line);
      string hexByte;
      while (hexStream >> hexByte)
      {
        if (hexByte == "")
        {
          continue;
        }
        char byte = static_cast<char>(stoi(hexByte, nullptr, 16));
        newSection->value->push_back(byte);
      }
    }

    asmSections->push_back(newSection);
  }
}

void addRelocationEntry(map<int, vector<RelocationEntry *>> *relocTable, int sectionIndex, RelocationEntry *newReloc)
{
  auto entry = relocTable->find(sectionIndex);
  if (entry == relocTable->end())
  {
    (*relocTable)[sectionIndex] = vector<RelocationEntry *>();
  }
  ((*relocTable)[sectionIndex]).push_back(newReloc);
}

void processSections()
{
  // prolazimo kroz sve nove sekcije
  for (int i = 0; i < asmSections->size(); i++)
  {
    // provera da li postoji sekcija sa tim imenom
    auto asmSection = (*asmSections)[i];
    auto sectionIterator = sections->find(*asmSection->name);
    if (sectionIterator == sections->end())
    {
      // ne postoji sekcija sa tim imenom
      (*sections)[*asmSection->name] = asmSection;
    }
    else
    {
      // postoji sekcija sa tim imenom
      // na sadrzaj postojece sekcije zalepimo novu sekciju
      auto lnSection = sectionIterator->second;
      for (int j = 0; j < asmSection->value->size(); j++)
      {
        lnSection->value->push_back((*asmSection->value)[i]);
      }

      // prolazimo kroz relokacione zapise te sekcije i azuriramo ih
      auto relocationIterator = asmRelocationTable->find(asmSection->sectionIndex);
      if (relocationIterator != asmRelocationTable->end())
      {
        // postoje relokacioni zapisi za sekciju
        auto reloc = relocationIterator->second;
        for (int k = 0; k < reloc.size(); k++)
        {
          // uvecavamo offset u relokacionom zapisu
          reloc[k]->offset += lnSection->size;
        }
      }

      // azuriramo velicinu sekcije
      lnSection->size += asmSection->size;
    }
  }
}

void processSymbols()
{
  // prolazimo kroz sve nove simbole
  for (auto i = asmSymbolTable->cbegin(); i != asmSymbolTable->cend(); i++)
  {
    // provera da li postoji simbol sa tim imenom
    auto asmSymbol = i->second;
    auto asmSymbolName = i->first;
    auto symIterator = symbolTable->find(asmSymbolName);
    if (symIterator == symbolTable->end())
    {
      // ne postoji simbol sa tim imenom
      addNewSymbol(asmSymbol, asmSymbolName);
    }
    else
    {
      // postoji simbol sa tim imenom
      // proveravamo binding type novog simbola
      string newSymbolName = "";
      switch (asmSymbol->type)
      {
      case BindingType::LOCAL:
        // preimenuj novi simbol i dodaj ga u tabelu simbola
        newSymbolName = asmSymbolName + to_string(Symbol::SYMBOL_NAME_INCREMENT++);
        addNewSymbol(asmSymbol, newSymbolName);
        break;
      case BindingType::GLOBAL:
        // stari simbol LOCAL -> preimenuj stari LOCAL simbol
        // stari simbol GLOBAL -> greska
        // stari simbol EXTERN -> stari postaje GLOBAL, azuriramo mu sekciju u kojoj je definisan
        // azuriraj novi relokacioni zapis da ukazuje na redefinisan EXTERN simbol (koji je postao GLOBAL)
        // stari simbol SECTION -> greska
        // stari simbol TYPE_FILE -> greska
        break;
      case BindingType::EXTERN:
        // stari simbol LOCAL -> preimenuj stari LOCAL simbol
        // stari simbol GLOBAL -> azuriraj relokacione zapise
        // stari simbol EXTERN -> azuriraj novi relokacioni zapis da ukazuje na vec definisan EXTERN simbol
        // stari simbol SECTION -> greska
        // stari simbol TYPE_FILE -> greska
        break;
      case BindingType::SECTION:
        // stari simbol LOCAL -> preimenuj stari LOCAL simbol
        // stari simbol GLOBAL -> greska
        // stari simbol EXTERN -> greska
        // stari simbol SECTION -> ne radi nista
        // stari simbol TYPE_FILE -> greska
        break;
      case BindingType::TYPE_FILE:
        // greska
        break;
      }
    }
  }
}

void addNewSymbol(Symbol *newSymbol, string newSymolName)
{
  // azuriram mu indeks za tabelu simbola linkera
  newSymbol->index = ++Symbol::ID;
  (*symbolTable)[newSymolName] = newSymbol;

  // prodji kroz sve relokacione zapise i azuriraj symbol index ili section index
  for (auto relocIterator = asmRelocationTable->begin(); relocIterator != asmRelocationTable->end(); relocIterator++)
  {
    auto sectionIndex = relocIterator->first;
    auto sectionRelocations = relocIterator->second;
    if (newSymbol->type == BindingType::SECTION)
    {
      // azuriraj section index
      // nova tabela relokacija sa izmenjenim section indexima
      asmRelocationTable->erase(sectionIndex);
      for (int j = 0; j < sectionRelocations.size(); j++)
      {
        addRelocationEntry(tmpRelocationTable, newSymbol->index, sectionRelocations[j]);
      }
    }
    else
    {
      // azuriraj simbol index u asmRelocationTable
      for (int j = 0; j < sectionRelocations.size(); j++)
      {
        // proveravamo da li se novi simbol pojavljuje u zapisu
        if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
        {
          sectionRelocations[j]->symbolIndex = newSymbol->index;
        }
      }
    }
  }

  // azuriraj simbol index u tmpRelocationTable
  for (auto relocIterator = tmpRelocationTable->begin(); relocIterator != tmpRelocationTable->end(); relocIterator++)
  {
    auto sectionIndex = relocIterator->first;
    auto sectionRelocations = relocIterator->second;
    for (int j = 0; j < sectionRelocations.size(); j++)
    {
      // proveravamo da li se novi simbol pojavljuje u zapisu
      if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
      {
        sectionRelocations[j]->symbolIndex = newSymbol->index;
      }
    }
  }
}

void printSymbolTable(ostream &out)
{
  out << "SYMBOL_TABLE" << endl;
  out << "index|" << "value|" << "type|" << "section|" << "defined|" << "label" << endl;
  for (auto i = asmSymbolTable->cbegin(); i != asmSymbolTable->cend(); i++)
  {
    i->second->print(out);
    out << i->first << endl;
  }
  out << endl;
}

void printSections(ostream &out)
{
  out << "SECTIONS" << endl;
  out << "index|" << "name|" << "size" << endl;
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
  for (auto i = asmRelocationTable->cbegin(); i != asmRelocationTable->cend(); i++)
  {
    for (int j = 0; j < i->second.size(); j++)
    {
      out << i->first << " ";
      (i->second)[j]->print(out);
    }
  }
  out << endl;
}

void writeToOutput(const string &output)
{
}