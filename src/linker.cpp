#include "linker.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

vector<ifstream> *files = new vector<ifstream>();
map<string, int> *sectionAddress = new map<string, int>();

// za cuvanje opsega zauzetih adresa
vector<int> *sectionStartAdresses = new vector<int>();
vector<int> *sectionEndAdresses = new vector<int>();

// za odredjivanje adrese smestanja sekcija
unsigned int highestStartingAddress = 0;

// za mapiranje symbol index u symbol value
map<int, int> *symbolIndexValue = new map<int, int>();

// za mapiranje section index u section
map<int, Section *> *symbolIndexSection = new map<int, Section *>();

int Symbol::ID = 0;
int Symbol::SYMBOL_NAME_INCREMENT = 0;

// strukture linkera
map<string, Section *> *sections = new map<string, Section *>();
map<string, Symbol *> *symbolTable = new map<string, Symbol *>();
map<int, vector<RelocationEntry *>> *relocationTable = new map<int, vector<RelocationEntry *>>();

// pomocne strukture za obradu objektnih fajlova
map<string, Section *> *asmSections = new map<string, Section *>();
map<string, Symbol *> *asmSymbolTable = new map<string, Symbol *>();
map<int, vector<RelocationEntry *>> *asmRelocationTable = new map<int, vector<RelocationEntry *>>();
map<int, vector<RelocationEntry *>> *tmpRelocationTable = new map<int, vector<RelocationEntry *>>();

// hex
bool isHex = false;

void printAsmSymbolTable(ostream &out)
{
  out << "ASM SYMBOL_TABLE" << endl;
  out << "index|" << "value|" << "type|" << "section|" << "defined|" << "label" << endl;
  for (auto i = asmSymbolTable->cbegin(); i != asmSymbolTable->cend(); i++)
  {
    i->second->print(out);
    out << i->first << endl;
  }
  out << endl;
}
void printAsmSections(ostream &out)
{
  out << "ASM SECTIONS" << endl;
  out << "index|" << "name|" << "size" << endl;
  for (auto i = asmSections->cbegin(); i != asmSections->cend(); i++)
  {
    i->second->print(out);
    out << endl;
  }
}
void printAsmRelocationTable(ostream &out)
{
  out << " ASM RELOCATION_TABLE" << endl;
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

int main(int argc, char *argv[])
{
  string output = "default_output.hex";
  auto result = processArguments(argc, argv, output);
  files = &result;

  startLinker();

  validateLinker();

  printSymbolTable(cout);
  printRelocationTable(cout);
  printSections(cout);

  if (isHex)
  {
    printHexRepresentation(cout);
    writeToOutput(output);
  }

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
    else if (arg.find("-place=") == 0)
    {
      string placeArg = arg.substr(7);
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
      if (valid)
      {
        cerr << "Error: Both '-hex' and '-relocatable' options are present" << endl;
        exit(-1);
      }
      valid = true;
      isHex = true;
    }
    else if (arg == "-relocatable")
    {
      if (valid)
      {
        cerr << "Error: Both '-hex' and '-relocatable' options are present" << endl;
        exit(-1);
      }
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

    // ocisti pomocne strukture za asembler
    asmSections = new map<string, Section *>();
    asmSymbolTable = new map<string, Symbol *>();
    asmRelocationTable = new map<int, vector<RelocationEntry *>>();
    tmpRelocationTable = new map<int, vector<RelocationEntry *>>();

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
    processSymbols();
    processRelocations();
  }

  if (isHex)
  {
    assignStartingAddresses();
    performRelocations();
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
    Symbol *newSymbol = new Symbol(index, value, type, section);
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

    (*asmSections)[*newSection->name] = newSection;
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
  for (auto i = asmSections->cbegin(); i != asmSections->cend(); i++)
  {
    // provera da li postoji sekcija sa tim imenom
    auto asmSection = i->second;
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
        lnSection->value->push_back((*asmSection->value)[j]);
      }

      // pronadji tu sekciju u tabeli simbola
      auto linkerSymbol = symbolTable->find(*asmSection->name);

      // azuriraj section index u sekciji
      asmSection->sectionIndex = linkerSymbol->second->index;

      // trazimo relokacione zapise za tu sekciju i azuriramo im offset i section index
      auto relocationIterator = asmRelocationTable->find(asmSection->oldSectionIndex);
      if (relocationIterator != asmRelocationTable->end())
      {
        // postoje relokacioni zapisi za sekciju
        auto &reloc = relocationIterator->second;

        if (linkerSymbol != symbolTable->end())
        {
          for (int k = 0; k < reloc.size(); k++)
          {
            // uvecavamo offset u relokacionom zapisu
            reloc[k]->offset += lnSection->size;

            // nova tabela relokacija sa izmenjenim section indexima
            addRelocationEntry(tmpRelocationTable, asmSection->sectionIndex, reloc[k]);
          }

          // stari relokacioni zapis se brise
          asmRelocationTable->erase(relocationIterator);
        }
      }

      // prolazi kroz asmSimbole i azurira im offset u odnosu na pocetak sekcije
      for (auto symbIter = asmSymbolTable->begin(); symbIter != asmSymbolTable->end(); symbIter++)
      {
        auto name = symbIter->first;
        auto symbol = symbIter->second;
        if (symbol->section == asmSection->oldSectionIndex)
        {
          symbol->value += lnSection->size;
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
      auto oldSymbol = symIterator->second;
      // razresi konflikte
      resolveSymbolConflict(oldSymbol, asmSymbol, asmSymbolName);
    }
  }

  // prolazimo kroz sve asm sekcije
  for (auto i = asmSections->cbegin(); i != asmSections->cend(); i++)
  {
    // prolazimo kroz sve simbole i ako ima nedefinisanih ne-EXTERN simbola
    // azuriraj im oldSection
    auto section = i->second;
    for (auto symbIterator = symbolTable->begin(); symbIterator != symbolTable->end(); symbIterator++)
    {
      auto symbol = symbIterator->second;
      if (symbol->oldSection == section->oldSectionIndex && !symbol->defined)
      {
        symbol->section = section->sectionIndex;
        symbol->defined = true;
      }
    }
  }
}

void addNewSymbol(Symbol *newSymbol, string newSymbolName)
{
  // azuriram mu indeks za tabelu simbola linkera
  newSymbol->index = ++Symbol::ID;
  (*symbolTable)[newSymbolName] = newSymbol;

  // ako je tipa fajl ne treba nista da azurira
  if (newSymbol->type == BindingType::TYPE_FILE)
  {
    newSymbol->defined = true;
    return;
  }

  // prodji kroz sve relokacione zapise i azuriraj symbol index ili section index
  for (auto relocIterator = asmRelocationTable->begin(); relocIterator != asmRelocationTable->end();)
  {
    auto sectionIndex = relocIterator->first;
    auto &sectionRelocations = relocIterator->second;
    if (sectionIndex == newSymbol->oldIndex && newSymbol->type == BindingType::SECTION)
    {
      // azuriraj section index
      // nova tabela relokacija sa izmenjenim section indexima
      for (int j = 0; j < sectionRelocations.size(); j++)
      {
        addRelocationEntry(tmpRelocationTable, newSymbol->index, sectionRelocations[j]);
      }
      // stari relokacioni zapis se brise
      relocIterator = asmRelocationTable->erase(relocIterator);
    }
    else
    {
      // azuriraj simbol index u asmRelocationTable
      for (int j = 0; j < sectionRelocations.size(); j++)
      {
        // proveravamo da li se novi simbol pojavljuje u zapisu
        if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
        {
          sectionRelocations[j]->newSymbolIndex = newSymbol->index;
        }
      }

      // prelazimo na sledeci element
      ++relocIterator;
    }
  }

  if (newSymbol->type != BindingType::SECTION)
  {
    // azuriraj simbol index u tmpRelocationTable
    for (auto relocIterator = tmpRelocationTable->begin(); relocIterator != tmpRelocationTable->end(); relocIterator++)
    {
      auto sectionIndex = relocIterator->first;
      auto &sectionRelocations = relocIterator->second;
      for (int j = 0; j < sectionRelocations.size(); j++)
      {
        // proveravamo da li se novi simbol pojavljuje u zapisu
        if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
        {
          sectionRelocations[j]->newSymbolIndex = newSymbol->index;
        }
      }
    }
  }
  else
  {
    // azuriraj section index u sekciji
    auto sectionIter = sections->find(newSymbolName);
    if (sectionIter != sections->end())
    {
      auto section = sectionIter->second;
      section->sectionIndex = newSymbol->index;
    }

    // azuriraj oldSection index svim simbolima iz te sekcije
    for (auto symbIterator = symbolTable->begin(); symbIterator != symbolTable->end(); symbIterator++)
    {
      auto symbol = symbIterator->second;
      if (symbol->oldSection == newSymbol->oldIndex && !symbol->defined)
      {
        symbol->section = newSymbol->index;
        symbol->defined = true;
      }
    }
  }
}

void processRelocations()
{
  // ubaci sve relokacione zapise iz asmRelocationTable
  for (auto relocIterator = asmRelocationTable->cbegin(); relocIterator != asmRelocationTable->cend(); relocIterator++)
  {
    auto index = relocIterator->first;
    auto reloc = relocIterator->second;
    for (int i = 0; i < reloc.size(); i++)
    {
      if (reloc[i]->symbolIndex != reloc[i]->newSymbolIndex)
      {
        reloc[i]->symbolIndex = reloc[i]->newSymbolIndex;
      }
      addRelocationEntry(relocationTable, index, reloc[i]);
    }
  }

  // ubaci sve relokacione zapise iz tmpRelocationTable
  for (auto relocIterator = tmpRelocationTable->cbegin(); relocIterator != tmpRelocationTable->cend(); relocIterator++)
  {
    auto index = relocIterator->first;
    auto reloc = relocIterator->second;
    for (int i = 0; i < reloc.size(); i++)
    {
      if (reloc[i]->symbolIndex != reloc[i]->newSymbolIndex)
      {
        reloc[i]->symbolIndex = reloc[i]->newSymbolIndex;
      }
      addRelocationEntry(relocationTable, index, reloc[i]);
    }
  }
}

void resolveSymbolConflict(Symbol *oldSymbol, Symbol *newSymbol, string symbolName)
{
  string newName = "";
  switch (newSymbol->type)
  {
  case BindingType::LOCAL:
    // preimenuj novi simbol i dodaj ga u tabelu simbola
    newName = renameLocalSymbol(symbolName);
    addNewSymbol(newSymbol, newName);
    break;
  case BindingType::GLOBAL:
    // stari simbol LOCAL -> preimenuj stari LOCAL simbol
    // stari simbol GLOBAL -> greska
    // stari simbol EXTERN -> stari postaje GLOBAL, azuriraj mu sekciju u kojoj je definisan, azuriraj mu value
    // azuriraj relokacione zapise
    // stari simbol SECTION -> greska
    // stari simbol TYPE_FILE -> greska
    switch (oldSymbol->type)
    {
    case BindingType::LOCAL:
      newName = renameLocalSymbol(symbolName);
      // izbaci stari simbol
      symbolTable->erase(symbolName);
      // ponovo ubaci stari simbol samo sa novim imenom
      (*symbolTable)[newName] = oldSymbol;
      // dodaj novi simbol
      addNewSymbol(newSymbol, symbolName);
      break;
    case BindingType::GLOBAL:
      cerr << "Error: Multiple global symbol definitions" << endl;
      exit(-3);
      break;
    case BindingType::EXTERN:
      changeExternToGlobal(oldSymbol, newSymbol);
      break;
    case BindingType::SECTION:
      cerr << "Error: Symbol already defined as section" << endl;
      exit(-3);
      break;
    case BindingType::TYPE_FILE:
      cerr << "Error: Symbol already defined as file" << endl;
      exit(-3);
      break;
    }
    break;
  case BindingType::EXTERN:
    // stari simbol LOCAL -> preimenuj stari LOCAL simbol
    // stari simbol GLOBAL -> azuriraj relokacione zapise
    // stari simbol EXTERN -> azuriraj relokacione zapise
    // stari simbol SECTION -> greska
    // stari simbol TYPE_FILE -> greska
    switch (oldSymbol->type)
    {
    case BindingType::LOCAL:
      newName = renameLocalSymbol(symbolName);
      // izbaci stari simbol
      symbolTable->erase(symbolName);
      // ponovo ubaci stari simbol samo sa novim imenom
      (*symbolTable)[newName] = oldSymbol;
      // dodaj novi simbol
      addNewSymbol(newSymbol, symbolName);
      break;
    case BindingType::GLOBAL:
      updateExternGlobalRelocations(oldSymbol, newSymbol);
      break;
    case BindingType::EXTERN:
      updateExternGlobalRelocations(oldSymbol, newSymbol);
      break;
    case BindingType::SECTION:
      cerr << "Error: Symbol already defined as section" << endl;
      exit(-3);
      break;
    case BindingType::TYPE_FILE:
      cerr << "Error: Symbol already defined as file" << endl;
      exit(-3);
      break;
    }
    break;
  case BindingType::SECTION:
    // stari simbol LOCAL -> preimenuj stari LOCAL simbol
    // stari simbol GLOBAL -> greska
    // stari simbol EXTERN -> greska
    // stari simbol SECTION -> ne radi nista (azuriranje se vrsi u obradi sekcije)
    // stari simbol TYPE_FILE -> greska
    switch (oldSymbol->type)
    {
    case BindingType::LOCAL:
      newName = renameLocalSymbol(symbolName);
      // izbaci stari simbol
      symbolTable->erase(symbolName);
      // ponovo ubaci stari simbol samo sa novim imenom
      (*symbolTable)[newName] = oldSymbol;
      // dodaj novi simbol
      addNewSymbol(newSymbol, symbolName);
      break;
    case BindingType::GLOBAL:
      cerr << "Error: Symbol already defined as global" << endl;
      exit(-3);
      break;
    case BindingType::EXTERN:
      cerr << "Error: Symbol already defined as extern" << endl;
      exit(-3);
      break;
    case BindingType::SECTION:
      break;
    case BindingType::TYPE_FILE:
      cerr << "Error: Symbol already defined as file" << endl;
      exit(-3);
      break;
    }
    break;
  case BindingType::TYPE_FILE:
    cerr << "Error: Symbol '" << symbolName << "' already defined as file" << endl;
    exit(-3);
  }
}

string renameLocalSymbol(string symbolName)
{
  string newName = symbolName + to_string(Symbol::SYMBOL_NAME_INCREMENT++);
  auto symbol = symbolTable->find(newName);
  // kreiraj novo ime sve dok simbol sa datim imenom postoji
  while (symbol != symbolTable->end())
  {
    symbol = symbolTable->find(newName);
    newName += to_string(Symbol::SYMBOL_NAME_INCREMENT++);
  }

  return newName;
}

void changeExternToGlobal(Symbol *externSymbol, Symbol *globalSymbol)
{
  externSymbol->type = BindingType::GLOBAL;
  externSymbol->oldSection = globalSymbol->oldSection;
  externSymbol->value = globalSymbol->value;

  updateExternGlobalRelocations(externSymbol, globalSymbol);
}

void updateExternGlobalRelocations(Symbol *oldSymbol, Symbol *newSymbol)
{
  // prodji kroz sve relokacione zapise i azuriraj symbol index
  // asmRelocationTable zapisi
  for (auto relocIterator = asmRelocationTable->begin(); relocIterator != asmRelocationTable->end(); relocIterator++)
  {
    auto sectionIndex = relocIterator->first;
    auto &sectionRelocations = relocIterator->second;

    // azuriraj simbol index u asmRelocationTable
    for (int j = 0; j < sectionRelocations.size(); j++)
    {
      // proveravamo da li se novi simbol pojavljuje u zapisu
      if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
      {
        sectionRelocations[j]->newSymbolIndex = oldSymbol->index;
      }
    }
  }

  // tmpRelocationTable zapisi
  for (auto relocIterator = tmpRelocationTable->begin(); relocIterator != tmpRelocationTable->end(); relocIterator++)
  {
    auto sectionIndex = relocIterator->first;
    auto &sectionRelocations = relocIterator->second;

    // azuriraj simbol index u tmpRelocationTable
    for (int j = 0; j < sectionRelocations.size(); j++)
    {
      // proveravamo da li se novi simbol pojavljuje u zapisu
      if (sectionRelocations[j]->symbolIndex == newSymbol->oldIndex)
      {
        sectionRelocations[j]->newSymbolIndex = oldSymbol->index;
      }
    }
  }
}

void assignStartingAddresses()
{
  map<string, bool> skipSections = map<string, bool>();

  // prodji kroz sve odredjene pocetne adrese za sekcije
  for (auto secAddrIter = sectionAddress->cbegin(); secAddrIter != sectionAddress->cend(); secAddrIter++)
  {
    auto secName = secAddrIter->first;
    auto secStart = secAddrIter->second;

    // provera da li postoji sekcija sa tim imenom
    auto sectionIter = sections->find(secName);
    if (sectionIter != sections->end())
    {
      auto section = sectionIter->second;
      int secEnd = secStart + section->size;

      // dodati tu sekciju u listu sekcija koje se preskacu
      skipSections[secName] = true;

      // provera da li je moguce staviti sekciju na tu adresu
      for (int i = 0; i < sectionStartAdresses->size(); i++)
      {
        if (secStart >= (*sectionStartAdresses)[i] && secStart <= (*sectionEndAdresses)[i] ||
            secEnd >= (*sectionStartAdresses)[i] && secEnd <= (*sectionEndAdresses)[i])
        {
          cerr << "Error: Placing " << secName << " section at " << secStart << " location will overwrite other sections" << endl;
          exit(-5);
        }
      }

      section->startingAddress = secStart;
      // azuriramo pocetnu adresu za smestanje sekcija bez unapred oderedjene adrese
      if (highestStartingAddress < section->startingAddress + section->size)
      {
        highestStartingAddress = section->startingAddress + section->size;
      }

      assignSymbolAddressForSection(section);

      // sacuvaj par sectionIndex, section
      (*symbolIndexSection)[section->sectionIndex] = section;
    }
  }

  // prodji kroz sve sekcije i dodeli im pocetne adrese
  for (auto sectionIter = sections->begin(); sectionIter != sections->end(); sectionIter++)
  {
    auto secName = sectionIter->first;
    auto section = sectionIter->second;

    // provera da li preskacemo sekciju
    if (skipSections.find(secName) != skipSections.end())
    {
      continue;
    }

    // ako sekcija nije preskocena dodaj joj pocetnu adresu
    section->startingAddress = highestStartingAddress;
    highestStartingAddress += section->size;

    assignSymbolAddressForSection(section);

    // sacuvaj par sectionIndex, section
    (*symbolIndexSection)[section->sectionIndex] = section;
  }
}

void assignSymbolAddressForSection(Section *section)
{
  // prodji kroz sve simbole te sekcije i dodeli im adrese
  for (auto symbIter = symbolTable->begin(); symbIter != symbolTable->end(); symbIter++)
  {
    auto symbol = symbIter->second;

    // provera da li simbol pripada sekciji
    if (symbol->section == section->sectionIndex)
    {

      // dodeli mu adresu
      symbol->value += section->startingAddress;

      // sacuvaj par symbol index, symbol value
      (*symbolIndexValue)[symbol->index] = symbol->value;
    }
  }
}

void performRelocations()
{
  // prolazimo kroz tabelu relokacija
  for (auto relocIter = relocationTable->begin(); relocIter != relocationTable->end(); relocIter++)
  {
    auto sectionIndex = relocIter->first;
    auto reloc = relocIter->second;

    // pronadji sekciju iz symbolIndexSection sa sectionIndex
    auto secIter = symbolIndexSection->find(sectionIndex);
    auto section = secIter->second;

    // prolazimo kroz sve relokacione zapise sekcije
    for (int i = 0; i < reloc.size(); i++)
    {
      // pronadji simbol za koji se vrsi relokacija
      auto symbIter = symbolIndexValue->find(reloc[i]->symbolIndex);
      auto value = symbIter->second;

      // u sekciji na offsetu reloc[i]->offset upisi symbValue na 4 bajta
      writeToSection(section, reloc[i]->offset, (value >> 24) & 0xff, (value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
    }
  }
}

void writeToSection(Section *section, int offset, int firstByte, int secondByte, int thirdByte, int fourthByte)
{
  (*section->value)[offset] = (char)firstByte;
  (*section->value)[offset + 1] = (char)secondByte;
  (*section->value)[offset + 2] = (char)thirdByte;
  (*section->value)[offset + 3] = (char)fourthByte;
}

void validateLinker()
{
  if (!isHex)
  {
    return;
  }
  for (auto symbIter = symbolTable->cbegin(); symbIter != symbolTable->cend(); symbIter++)
  {
    if (!symbIter->second->defined)
    {
      cerr << "Error: Symbol '" << symbIter->first << "' not defined" << endl;
      exit(-6);
    }
  }
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
  for (auto i = relocationTable->cbegin(); i != relocationTable->cend(); i++)
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
  ofstream outfile(output);
  if (!outfile)
  {
    cerr << "Failed to open output file: " << output << endl;
    exit(-1);
  }

  printHexRepresentation(outfile);

  outfile.close();
  if (!outfile.good())
  {
    cerr << "Error occurred while writing to output file: " << output << endl;
  }
  else
  {
    cout << "Output written to file: " << output << endl;
  }
}

void printHexRepresentation(ostream &out)
{
  vector<pair<string, Section *>> sortedSections(sections->begin(), sections->end());

  sort(sortedSections.begin(), sortedSections.end(), [](const auto &a, const auto &b)
       { return a.second->startingAddress < b.second->startingAddress; });

  for (auto sectIter : sortedSections)
  {
    auto section = sectIter.second;
    int startingAddress = section->startingAddress;
    auto value = section->value;

    unsigned int address = startingAddress;
    int cnt = 0;

    for (char byte : *value)
    {
      if (cnt % 8 == 0)
      {
        if (cnt != 0)
          out << endl;
        out << hex << setw(8) << setfill('0') << address << ": ";
      }
      out << hex << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(byte)) << " ";
      address++;
      cnt++;
    }
    out << endl;
  }
}