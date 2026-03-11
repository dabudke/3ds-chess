/*
create individual threads for each square for rooks and bishops
64*64 threads

each thread computes a magic number and shift value for the corresponding
square (and direction)'s magic bitboard

s - saves to a file (comma separated values, newline separated lists)
    [orthMagics, orthShifts, diagMagics, diagShifts]
*/

#include <cinttypes>
#include <array>
#include <iostream>
#include <bitset>
#include "bitboards.hpp"
#include "search.hpp"

int main(int, char **)
{
  std::vector<std::unique_ptr<MagicSearch>> magicSearches{};
  std::vector<std::thread> allThreads{};

  // // orthogonal magics
  // for (int i{0}; i < 64; i++)
  // {
  //   magicSearches.emplace_back(orthMagicSignals[i], orthOccupancyMasks[i]);
  // }
  // // diagonal magics
  // for (int)

  magicSearches.push_back(std::make_unique<MagicSearch>(orthOccupancySets[0], std::popcount(orthOccupancyMasks[0])));

  std::this_thread::sleep_for(std::chrono::seconds(3));
  magicSearches.back()->stop();
  std::cout << magicSearches.back()->bestMagic
            << " " << static_cast<int>(magicSearches.back()->bestShift)
            << std::endl;
}
