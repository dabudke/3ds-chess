#include "../board.hpp"

namespace Chess
{
#pragma region simple predet
  const auto Board::pawnAttacks = []() constexpr
  {
    std::array<std::array<uint64_t, 64>, 2> attacks{0};

    for (uint8_t i{8}; i < 56; i++)
    {
      uint64_t squareBit = bitmaskForSquare(i);

      uint64_t whiteForward = squareBit << 8;
      uint64_t whiteAttackLeft = (whiteForward << 1) & ~bitmaskForCol(0);
      uint64_t whiteAttackRight = (whiteForward >> 1) & ~bitmaskForCol(7);
      attacks[0][i] = whiteAttackLeft | whiteAttackRight;

      uint64_t blackForward = squareBit >> 8;
      uint64_t blackAttackLeft = (blackForward << 1) & ~bitmaskForCol(0);
      uint64_t blackAttackRight = (blackForward >> 1) & ~bitmaskForCol(7);
      attacks[1][i] = blackAttackLeft | blackAttackRight;
    }

    return attacks;
  }();

  const auto Board::knightAttacks = []() constexpr
  {
    std::array<uint64_t, 64> attacks{0};

    for (uint8_t i{0}; i < 64; i++)
    {
      int row = squareRow(i);
      int col = squareCol(i);
      uint64_t rowMajorRows{0};
      uint64_t rowMajorCols{0};
      uint64_t colMajorRows{0};
      uint64_t colMajorCols{0};

      if (rowOffsetInBounds(i, 2))
      {
        rowMajorRows |= bitmaskForRow(row + 2);
        colMajorRows |= bitmaskForRow(row + 1);
      }
      else if (rowOffsetInBounds(i, 1))
        colMajorRows |= bitmaskForRow(row + 1);

      if (rowOffsetInBounds(i, -2))
      {
        rowMajorRows |= bitmaskForRow(row - 2);
        colMajorRows |= bitmaskForRow(row - 1);
      }
      else if (rowOffsetInBounds(i, -1))
        colMajorRows |= bitmaskForRow(row - 1);

      if (colOffsetInBounds(i, 2))
      {
        colMajorCols |= bitmaskForCol(col + 2);
        rowMajorCols |= bitmaskForCol(col + 1);
      }
      else if (colOffsetInBounds(i, 1))
        rowMajorCols |= bitmaskForCol(col + 1);

      if (colOffsetInBounds(i, -2))
      {
        colMajorCols |= bitmaskForCol(col - 2);
        rowMajorCols |= bitmaskForCol(col - 1);
      }
      else if (colOffsetInBounds(i, -1))
        rowMajorCols |= bitmaskForCol(col - 1);

      attacks[i] = (rowMajorRows & rowMajorCols) | (colMajorRows & colMajorCols);
    }

    return attacks;
  }();

  const auto Board::kingAttacks = []() constexpr
  {
    std::array<uint64_t, 64> attacks{0};

    for (uint8_t i{0}; i < 64; i++)
    {
      int row = squareRow(i);
      int col = squareCol(i);

      uint64_t rows{bitmaskForRow(row)};
      if (rowOffsetInBounds(i, 1))
        rows |= bitmaskForRow(row + 1);
      if (rowOffsetInBounds(i, -1))
        rows |= bitmaskForRow(row - 1);

      uint64_t cols{bitmaskForCol(col)};
      if (colOffsetInBounds(i, 1))
        cols |= bitmaskForCol(col + 1);
      if (colOffsetInBounds(i, -1))
        cols |= bitmaskForCol(col - 1);

      attacks[i] = rows & cols ^ bitmaskForSquare(i);
    }

    return attacks;
  }();

#pragma region opponent attacks

  /// @brief get a mask containing all squares a piece can move to, being pinned
  /// can also be used to get legal squares when king is in check
  /// @param square
  /// @return
  // uint64_t Board::getPinMask(uint8_t square)
  // {
  //   return 0ull;
  // }

  // uint64_t Board::squareAttackedBy(uint64_t occupancy, uint8_t square)
  // {
  //   return 0ull;
  // }
}
