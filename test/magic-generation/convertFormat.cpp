#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

struct MagicMapEntry {
  int shift{0};
  uint64_t magic{0};
  std::map<uint64_t, uint64_t> moveMap;
};

int main(int argc, char **argv) {
  char opt;
  std::ifstream input;
  std::ofstream output;
  while ((opt = getopt(argc, argv, "i:o:n:")) != -1) {
    switch (opt) {
    case 'i':
      input = std::ifstream(optarg);
      break;
    case 'o':
      std::remove(optarg);
      output = std::ofstream(optarg);
      break;
    }
  }
  if (!input || !output) {
    std::cout << "Must specify input and output file.\n";
    return -1;
  }

  std::array<std::pair<MagicMapEntry, MagicMapEntry>, 64> magicMap{};

  // this format could be improved like 17-fold however. there is no need.
  // 3 minutes later - screw it we ball
  input.ignore(); // '{'
  for (int i{0}; i < 64; i++) {
    uint64_t magic{0};
    input >> magic;
    magicMap[i].first.magic = magic;
    input.ignore(4); // 'ull,' / 'ull}'
  }
  input.ignore(2); // '\n{'
  for (int i{0}; i < 64; i++) {
    uint shift{0};
    input >> shift;
    magicMap[i].first.shift = shift;
    input.ignore(2); // 'u,' / 'u}'
  }
  input.ignore(2); // '\n{'
  for (int i{0}; i < 64; i++) {
    uint64_t index{0};
    std::map<uint64_t, uint64_t> map{};
    while (!input.eof() && input.peek() != '}') {
      input.ignore(1); // '{' / ','
      uint64_t bitboard{0};
      input >> bitboard;
      if (bitboard > 0ull)
        map[index] = bitboard;
      input.ignore(3); // 'ull'
      index++;
    }
    magicMap[i].first.moveMap = map;
    input.ignore(3); // '},\n'
  }
  input.ignore(2); // '\n{'

  for (int i{0}; i < 64; i++) {
    uint64_t magic{0};
    input >> magic;
    magicMap[i].second.magic = magic;
    input.ignore(4); // 'ull,' / 'ull}'
  }
  input.ignore(2); // '\n{'
  for (int i{0}; i < 64; i++) {
    uint shift{0};
    input >> shift;
    magicMap[i].second.shift = shift;
    input.ignore(2); // 'u,' \ 'u}'
  }
  input.ignore(2); // '\n{'
  for (int i{0}; i < 64; i++) {
    uint64_t index{0};
    std::map<uint64_t, uint64_t> map{};
    while (!input.eof() && input.peek() != '}') {
      input.ignore(1); // '{' / ','
      uint64_t bitboard{0};
      input >> bitboard;
      if (bitboard > 0ull)
        map[index] = bitboard;
      input.ignore(3); // 'ull'
      index++;
    }
    magicMap[i].second.moveMap = map;
    input.ignore(3); // '},\n'
  }

  // new format! easy to read AND write
  // orthMagic orthShift orthMoveset orthMoveset....
  // magic shift moveset...
  // ... (x 64)
  // diagMagic diagShift diagMoveset...
  // ... (x 64)
  for (int i{0}; i < 64; i++) {
    auto &[orth, diag] = magicMap[i];
    output << orth.magic << " " << orth.shift << " ";
    uint32_t maxOrthIndex = orth.moveMap.rbegin()->first;
    for (uint32_t j{0}; j <= maxOrthIndex; j++) {
      if (orth.moveMap.contains(j))
        output << orth.moveMap[j];
      else
        output << "0";
      if (j < maxOrthIndex)
        output << " ";
    }
    output << ",\n" << diag.magic << " " << diag.shift << " ";
    uint32_t maxDiagIndex = diag.moveMap.rbegin()->first;
    for (uint32_t j{0}; j <= maxDiagIndex; j++) {
      if (diag.moveMap.contains(j))
        output << diag.moveMap[j];
      else
        output << "0";
      if (j < maxDiagIndex)
        output << " ";
    }
    output << ",\n";
  }
}
