#include <iostream>
#include <vector>

using namespace std;

void print_hex(const vector<char>& value) {
  for (char byte : value) {
    cout << hex << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
  }
}