#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

void print_hex(const vector<char>& value) {
  for (char byte : value) {
    cout << hex << setw(2) << setfill('0') << static_cast<unsigned int>(byte) << " ";
  }
  cout << endl;
}