#include <cstdint>
#include <iostream>
using namespace std;

using word_t = int16_t;
using addr_t = uint16_t;

int main() {
  cout << "Program Started" << endl;

  char command;
  while (true) {
    uint16_t regAddr;
    addr_t memAddr;
    int16_t immediate;

    cin >> command;
    command = tolower(command);
    if (command == 'q') {
      break;
    } else if (command == 'l') {
      cin >> regAddr >> memAddr;
      // TODO Load Word
    } else if (command == 's') {
      cin >> regAddr >> memAddr;
      // TODO Store Word
    } else if (command == 'c') {
      cin >> regAddr >> immediate;
      // TODO: Store Immediate
    } else if (command == 'p') {
      char target;
      cin >> target;
      target = tolower(target);
      switch (target) {
        case 'c':
          // TODO Print Cache
          break;
        case 'r':
          // TODO Print Ram
          break;
      }
    } else if (command == 'i') {
      // TODO Invalidate cache
    } else {
      cout << "Invalid command! Try again." << endl;
    }
  }

  cout << "Program Finished" << endl;
  return 0;
}