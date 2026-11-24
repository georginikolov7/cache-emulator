#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
using namespace std;

// We define aliases for word and address types so we have a single source of
// truth. Words can be negative values, so we use int16_t instead of uint16_t
using word_t = int16_t;
using addr_t = uint16_t;

const uint16_t CACHE_LINES_COUNT = 8;
const uint16_t RAM_SIZE = 512;
const uint16_t REGISTER_FILE_SIZE = 8;

struct AddressParts {
  uint16_t tag;        // Bits 4-15
  uint16_t lineIndex;  // Bits 1-3
  uint16_t offset;     // Bit  0
};

struct CacheBlock {
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

AddressParts breakDownAddress(addr_t memAddr) {
  AddressParts parts;
  // Mask lowest 1 bit
  parts.offset = memAddr & 0b1;

  // Shift out bit 0, mask lowest 3 bits
  parts.lineIndex = (memAddr >> 1) & 0b111;
  
  // Shift out bit 0, mask lowest 3 bits
  parts.tag = (memAddr >> 4) & 0b1111'1111'1111;
  return parts;
}

addr_t reconstructEvenAddress(uint16_t tag, uint16_t lineIndex) {
  // slide the 12 bit tag 4 bits to the left and slide the 3 bit index 1 bit to
  // the left. OR the two to get the original address (last bit left at 0 to get
  // the even address)
  return (tag << 4) | (lineIndex << 1);
}

void initCache() {
  for (int i = 0; i < CACHE_LINES_COUNT; i++) {
    // First block per line is stale on cache initialization
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

CacheBlock& writeBack(const AddressParts& addressParts, addr_t memAddr) {
  CacheLine& line = cache[addressParts.lineIndex];

  int blockIndex = -1;

  // Check if current address is in the cache (we have a cache hit)
  for (int i = 0; i < 2; i++) {
    if (line.blocks[i].validBit && line.blocks[i].tag == addressParts.tag) {
      blockIndex = i;
      break;
    }
  }

  if (blockIndex == -1) {
    // If we have a cache miss, we must evict the stale block:
    int staleBlockIndex = -1;
    if (line.blocks[0].staleBit) {
      staleBlockIndex = 0;
    } else {
      staleBlockIndex = 1;
    }

    CacheBlock& staleBlock = line.blocks[staleBlockIndex];

    // Write-back scheme: if stale block is valid and dirty we need to save
    // its old words to RAM:
    if (staleBlock.validBit && staleBlock.dirtyBit) {
      addr_t oldEvenAddress =
          reconstructEvenAddress(staleBlock.tag, addressParts.lineIndex);

      ram[oldEvenAddress] = staleBlock.words[0];
      ram[oldEvenAddress + 1] = staleBlock.words[1];
    }

    // We fetch both words for the new address from RAM (otherwise
    // one of the words in the block would be garbage after we put the new one
    // in)
    addr_t newEvenAddress = memAddr - addressParts.offset;

    staleBlock.words[0] = ram[newEvenAddress];
    staleBlock.words[1] = ram[newEvenAddress + 1];
    staleBlock.tag = addressParts.tag;
    staleBlock.validBit = 1;
    // The block is now synced with RAM:
    staleBlock.dirtyBit = 0;

    blockIndex = staleBlockIndex;
  }

  // Flip stale bit:
  line.blocks[blockIndex].staleBit = 0;
  line.blocks[1 - blockIndex].staleBit = 1;

  return line.blocks[blockIndex];
}

void setImmediate(uint16_t regAddr, word_t immediate) {
  registerFile[regAddr] = immediate;
}

void loadWord(uint16_t regAddr, addr_t memAddr) {
  AddressParts addressParts = breakDownAddress(memAddr);

  // Trigger the writeBack logic and get the block we need:
  CacheBlock& block = writeBack(addressParts, memAddr);

  registerFile[regAddr] = block.words[addressParts.offset];
}

void storeWord(uint16_t regAddr, addr_t memAddr) {
  word_t word = registerFile[regAddr];

  AddressParts addressParts = breakDownAddress(memAddr);

  // Trigger the writeBack logic and get the block we need:
  CacheBlock& block = writeBack(addressParts, memAddr);

  block.words[addressParts.offset] = word;
  // Set the block dirty - the new value is not in RAM yet
  block.dirtyBit = 1;
}

void printCache() {
  cout << left << setw(3) << "L" << setw(3) << "B" << setw(3) << "S" << setw(3)
       << "V" << setw(3) << "D" << setw(8) << "W1"
       << "W2" << endl;

  for (int i = 0; i < CACHE_LINES_COUNT; ++i) {
    for (int j = 0; j < 2; ++j) {
      const CacheBlock& blk = cache[i].blocks[j];

      cout << left << setw(3) << i << setw(3) << j << setw(3) << blk.staleBit
           << setw(3) << blk.validBit << setw(3) << blk.dirtyBit << setw(8)
           << blk.words[0] << blk.words[1] << endl;
    }
  }
}

void printRam() {
  cout << left << setw(8) << "ADDR" << "VALUE" << endl;

  for (addr_t addr = 0; addr < 512; addr++) {
    cout << left << setw(8) << addr << ram[addr] << endl;
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

      loadWord(regAddr, memAddr);
    } else if (command == 's') {
      cin >> regAddr >> memAddr;

      storeWord(regAddr, memAddr);
    } else if (command == 'c') {
      cin >> regAddr >> immediate;

      setImmediate(regAddr, immediate);
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