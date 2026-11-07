#include <cstdint>
#include <iomanip>
#include <iostream>
using namespace std;

// We define aliases for word  and address types so we have a single source of
// truth. Words can be negative values, so we use int16_t instead of uint16_t
using word_t = int16_t;
using addr_t = uint16_t;

const uint16_t CACHE_LINES_COUNT = 8;
const uint16_t RAM_SIZE = 512;
const uint16_t REGISTER_FILE_SIZE = 8;

struct CacheBlock {
  // TODO: ASK about tag value type
  uint16_t tag;
  bool staleBit;
  bool validBit;
  bool dirtyBit;
  word_t words[2];
};

struct CacheLine {
  // Our cache is 2-way associative, so each line
  // corresponds to 2 blocks
  CacheBlock blocks[2];
};

word_t registerFile[REGISTER_FILE_SIZE] = {0};
word_t ram[RAM_SIZE] = {0};
CacheLine cache[CACHE_LINES_COUNT];

void initCache() {
  for (int i = 0; i < CACHE_LINES_COUNT; i++) {
    // Second block per line is fresh
    cache[i].blocks[0] = {0, 1, 0, 0, {0, 0}};
    cache[i].blocks[1] = {0, 0, 0, 0, {0, 0}};
  }
}

void invalidateCache() {
  for (int i = 0; i < CACHE_LINES_COUNT; i++) {
    cache[i].blocks[0].validBit = 0;
    cache[i].blocks[1].validBit = 0;
  }
}

void printCache() {
  cout << left << setw(3) << "L" << setw(3) << "B" << setw(3) << "S" << setw(3)
       << "V" << setw(3) << "D" << setw(6) << "W1"
       << "W2" << endl;

  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 2; ++j) {
      const CacheBlock& blk = cache[i].blocks[j];
      cout << left << setw(3) << i << setw(3) << j << setw(3) << blk.staleBit
           << setw(3) << blk.validBit << setw(3) << blk.dirtyBit << setw(6)
           << blk.words[0] << blk.words[1] << endl;
    }
  }
}

void printRam() {
  cout << left << setw(6) << "ADDR" << "VALUE" << endl;
  for (addr_t addr = 0; addr < 512; addr++) {
    cout << left << setw(6) << addr << ram[addr] << endl;
  }
}

int main() {
  cout << "Program Started" << endl;

  initCache();

  char command;
  while (true) {
    uint16_t regAddr;
    addr_t memAddr;
    int16_t immediate;

    cout << "$ ";
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
          printCache();
          break;
        case 'r':
          printRam();
          break;
      }
    } else if (command == 'i') {
      invalidateCache();
    } else {
      cout << "Invalid command! Try again." << endl;
    }
  }

  cout << "Program Finished" << endl;
  return 0;
}