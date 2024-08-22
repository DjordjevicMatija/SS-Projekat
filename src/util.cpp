#include "util.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

void print_hex(const vector<char>& value) {
  for (char byte : value) {
    cout << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
  }
  cout << endl;
  cout << dec;
}
