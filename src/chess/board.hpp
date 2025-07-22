#pragma once
#include "chess/piece.hpp"
#include "chess/move.hpp"

namespace Chess
{
  // 18.072 KB
  class Board
  {
  protected:
    Piece board[64]; // 8x8 chess board represented as a 1D array
    bool whiteToMove{true};

    // state variable
    // 0x0001 white castle kingside
    // 0x0002 white castle queenside
    // 0x0004 black castle kingside
    // 0x0008 black castle queenside
    // 0x00F0 last captured piece
    // 0x0F00 50 move rule
    unsigned short state; // state variable

    Move moveHistory[6000];               // move history
    unsigned char stateFlagHistory[6000]; // state flags for each move
    int moveCount;                        // latest move

  public:
    Board();

    Piece getPiece(int index) const;
    Piece getPiece(int row, int col) const;

    // move flags same as Move struct
    bool makeMove(unsigned char fromSquare, unsigned char toSquare);
  };
}
