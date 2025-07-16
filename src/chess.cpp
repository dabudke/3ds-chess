#include <stdexcept>
#include "chess.hpp"
#include "piece.hpp"

Chess::Chess()
{
  // Initialize the board
  for (int i{0}; i < 64; ++i)
  {
    if (i == 0 || i == 7)
    {
      board[i] = Piece(Piece::White, Piece::Rook); // White Rook
    }
    else if (i == 1 || i == 6)
    {
      board[i] = Piece(Piece::White, Piece::Knight); // White Knight
    }
    else if (i == 2 || i == 5)
    {
      board[i] = Piece(Piece::White, Piece::Bishop); // White Bishop
    }
    else if (i == 3)
    {
      board[i] = Piece(Piece::White, Piece::Queen); // White Queen
    }
    else if (i == 4)
    {
      board[i] = Piece(Piece::White, Piece::King); // White King
    }
    else if (i >= 8 && i < 16)
    {
      board[i] = Piece(Piece::White, Piece::Pawn); // White Pawns
    }
    else if (i >= 48 && i < 56)
    {
      board[i] = Piece(Piece::Black, Piece::Pawn); // Black Pawns
    }
    else if (i == 56 || i == 63)
    {
      board[i] = Piece(Piece::Black, Piece::Rook); // Black Rook
    }
    else if (i == 57 || i == 62)
    {
      board[i] = Piece(Piece::Black, Piece::Knight); // Black Knight
    }
    else if (i == 58 || i == 61)
    {
      board[i] = Piece(Piece::Black, Piece::Bishop); // Black Bishop
    }
    else if (i == 59)
    {
      board[i] = Piece(Piece::Black, Piece::Queen); // Black Queen
    }
    else if (i == 60)
    {
      board[i] = Piece(Piece::Black, Piece::King); // Black King
    }
  }
}

Piece Chess::getPiece(int index) const
{
  if (index < 0 || index >= 64)
  {
    throw std::out_of_range("Index out of bounds");
  }
  return board[index];
}
Piece Chess::getPiece(int row, int col) const
{
  if (row < 0 || row >= 8 || col < 0 || col >= 8)
  {
    throw std::out_of_range("Row or column out of bounds");
  }
  return board[row * 8 + col];
}
