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

int main(int, char **)
{
  // pre-create all bitboards

  for (auto mask : diagOccupancyMasks)
  {
    for (int row{0}; row < 8; row++)
      std::cout
          << std::bitset<8>((mask >> (row * 8)) & 0xFF) << std::endl;
    if (&mask != &diagOccupancyMasks.back())
      std::cout << "------------\n";
  }
}
