#include "util.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

void print_hex(const vector<char>& value) {
  int space = 4;
  int newLine = 8;
  int cnt = 0;
  for (char byte : value) {
    cout << hex << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(byte)) << " ";
    cnt++;
    if(cnt == 4){
      cout << "  ";
    }
    if(cnt == 8){
      cout << endl;
      cnt = 0;
    }
  }
  cout << endl;
  cout << dec;
}
