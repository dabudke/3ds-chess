#include <cinttypes>
#include <bit>
#include <array>
#include "bitboards.hpp"

constexpr std::array<uint64_t, 64> orthOccupancyMasks = []() constexpr
{
  std::array<uint64_t, 64> masks{0};

  for (int i{0}; i < 64; i++)
  {
    uint64_t mask{0};
    uint64_t bit{1ull << i};

    for (int row{1}; row < 7 - (i / 8); row++)
    {
      mask |= bit << (row * 8);
    }
    for (int row{1}; row < (i / 8); row++)
    {
      mask |= bit >> (row * 8);
    }

    int col{i % 8};
    for (int colOffset{1}; colOffset < 7 - col; colOffset++)
    {
      mask |= bit << colOffset;
    }
    for (int colOffset{1}; colOffset < col; colOffset++)
    {
      mask |= bit >> colOffset;
    }

    masks[i] = mask;
  }

  return masks;
}();

constexpr std::array<uint64_t, 64> diagOccupancyMasks = []() constexpr
{
  std::array<uint64_t, 64> masks{0};
  // const uint64_t topLeft{0xFF01010101010101ULL};
  // const uint64_t topRight{0xFF80808080808080ULL};
  // const uint64_t bottomRight{0x80808080808080FFULL};
  // const uint64_t bottomLeft{0x01010101010101FFULL};

  for (int i{0}; i < 64; i++)
  {
    uint64_t mask{0};
    uint64_t bit{1ull << i};
    int row{i / 8};
    int col{i % 8};

    // top-left
    // row < 7 & col < 7
    for (int offset{1}; (offset < 7 - row) && (offset < 7 - col); offset++)
    {
      mask |= bit << (offset * 8 + offset);
    }
    for (int offset{1}; (offset < row) && (offset < col); offset++)
    {
      mask |= bit >> ((offset * 8) + offset);
    }

    // bottom-left
    for (int offset{1}; (offset < 7 - row) && (offset < col); offset++)
    {
      mask |= (bit << (offset * 8)) >> offset;
    }
    for (int offset{1}; (offset < row) && (offset < 7 - col); offset++)
    {
      mask |= (bit >> (offset * 8)) << offset;
    }

    masks[i] = mask;
  }

  return masks;
}();

// function to generate all the orthoganal square occupancies and
// correspoding movesets
const std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> orthOccupancySets = []() constexpr
{
  std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> occupancies;

  // for every square
  for (int square{0}; square < 64; square++)
  {
    int row{square / 8};
    int col{square % 8};
    uint64_t occupancyMask = orthOccupancyMasks[square];

    // extract all bits in occupancy mask
    std::vector<unsigned int> occupancyShifts{};
    for (uint64_t occupancyMaskCopy{occupancyMask}; occupancyMaskCopy;)
    {
      int shift = std::countr_zero(occupancyMaskCopy);
      occupancyShifts.push_back(shift);
      occupancyMaskCopy >>= shift + 1;
    }

    // iterate over every possibility in this occupancy, create moveset/move bitboard
    uint32_t possiblities = 1ull << std::popcount(occupancyMask);
    std::vector<std::pair<uint64_t, uint64_t>> occupancySet{};
    for (uint32_t possibility{0}; possibility < possiblities; possibility++)
    {
      // create unique occupancy from mask and possibility
      uint64_t occupancy{0};
      for (int shift{static_cast<int>(occupancyShifts.size()) - 1}; shift >= 0; shift--)
      {
        occupancy <<= 1;
        if (possibility & (1ul << shift))
        {
          occupancy |= 1;
        }
        occupancy <<= occupancyShifts[shift];
      }

      // create moveset from occupancy
      uint64_t moveset{0};
      for (int moveRow{row + 1}; moveRow < 8; moveRow++)
      {
        uint64_t bit{1ull << (moveRow * 8 + col)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      for (int moveRow{row - 1}; moveRow >= 0; moveRow--)
      {
        uint64_t bit{1ull << (moveRow * 8 + col)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      for (int moveCol{col + 1}; moveCol < 8; moveCol++)
      {
        uint64_t bit{1ull << (row * 8 + moveCol)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      for (int moveCol{col - 1}; moveCol >= 0; moveCol--)
      {
        uint64_t bit{1ull << (row * 8 + moveCol)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }

      occupancySet.emplace_back(occupancy, moveset);
    }

    occupancies[square] = occupancySet;
  }

  return occupancies;
}();

const std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> diagOccupancySets = []()
{
  std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> occupancySets{};

  for (int square{0}; square < 64; square++)
  {
    int row{square / 8};
    int col{square % 8};
    uint64_t occupancyMask = diagOccupancyMasks[square];
    int possibilities = 1u << std::popcount(occupancyMask);

    // extract all bits in mask
    std::vector<unsigned int> occupancyShifts{};
    while (occupancyMask)
    {
      unsigned int shift = std::countr_zero(occupancyMask);
      occupancyShifts.push_back(shift);
      occupancyMask >>= shift + 1;
    }

    // iterate over all possibilities - create occupancy and corresponding moveset
    std::vector<std::pair<uint64_t, uint64_t>> occupancySet{};
    occupancySet.reserve(possibilities);
    for (int possibility{0}; possibility < possibilities; possibility++)
    {
      // create occupancy from shifts and possibility mask
      uint64_t occupancy{0};
      for (int shiftIndex{static_cast<int>(occupancyShifts.size()) - 1}; shiftIndex >= 0; shiftIndex--)
      {
        occupancy <<= 1;
        if (possibility & (1u << shiftIndex))
          occupancy |= 1;
        occupancy <<= occupancyShifts[shiftIndex];
      }

      // create moveset
      uint64_t moveset{0};
      // top right (row + offset, col + offset)
      for (int offset{1}; ((row + offset) < 8) && ((col + offset) < 8); offset++)
      {
        uint64_t bit{1ull << ((row + offset) * 8 + col + offset)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      // top left (row + offset, col - offset)
      for (int offset{1}; ((row + offset) < 8) && ((col - offset) >= 0); offset++)
      {
        uint64_t bit{1ull << ((row + offset) * 8 + col - offset)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      // bottom right (row - offset, col + offset)
      for (int offset{1}; ((row - offset) >= 0) && ((col + offset) < 8); offset++)
      {
        uint64_t bit{1ull << ((row - offset) * 8 + col + offset)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }
      // bottom left (row - offset, col - offset)
      for (int offset{1}; ((row - offset) >= 0) && ((col - offset) >= 0); offset++)
      {
        uint64_t bit{1ull << ((row - offset) * 8 + col - offset)};
        moveset |= bit;
        if (occupancy & bit)
          break;
      }

      occupancySet.emplace_back(occupancy, moveset);
    }

    occupancySets[square] = occupancySet;
  }

  return occupancySets;
}();
