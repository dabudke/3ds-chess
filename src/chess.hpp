#pragma once
#include "piece.hpp"

class Chess
{
protected:
  Piece board[64]; // 8x8 chess board represented as a 1D array
public:
  Chess();

  Piece getPiece(int index) const;
  Piece getPiece(int row, int col) const;
};
