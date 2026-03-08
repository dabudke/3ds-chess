#include <cinttypes>
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
