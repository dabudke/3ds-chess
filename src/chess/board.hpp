#pragma once
#include "chess/piece.hpp"
#include "chess/move.hpp"
#include <vector>

namespace Chess
{
  // 30.066 KB
  class Board
  {
  protected:
    Piece board[64]; // 8x8 chess board represented as a 1D array

    // current board state
    // `0x0001` - white to move
    // `0x0002` - white castle kingside
    // `0x0004` - white castle queenside
    // `0x0008` - black castle kingside
    // `0x0010` - black castle queenside
    // `0x00E0` - en passant square
    // `0x0F00` - last captured piece
    unsigned short stateHistory[6000];

    // set move in moveHistory[moveCount]
    // copy state to stateFlagHistory[++moveCount]
    // increment and copy fiftyMoveRuleCounter[++moveCount]
    // change state according to move
    unsigned short moveCount{0};     // latest move
    Move moveHistory[6000];          // move history
    char fiftyMoveRuleCounter[6000]; // counter for fifty move rule

  public:
    static const unsigned short initialState = 0x001FU;

    Board();

    bool whiteMove() const
    {
      return stateHistory[moveCount] & 0x0001 == 1;
    }
    bool blackMove() const
    {
      return stateHistory[moveCount] & 0x0001 == 0;
    }

    Piece getPiece(int index) const;
    Piece getPiece(int row, int col) const;

    std::vector<Move> getLegalMovesForSquare(unsigned char square) const;
    std::vector<Move> getLegalMoves() const
    {
      std::vector<Move> legalMoves;
      for (int i = 0; i < 64; i++)
      {
        if (getPiece(i) == Piece::Empty)
          continue; // skip empty squares
        auto moves = getLegalMovesForSquare(i);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      return legalMoves;
    }

    // move flags same as Move struct
    bool makeMove(unsigned char fromSquare, unsigned char toSquare);
    void unmakeMove();

    Move getLastMove() const
    {
      if (moveCount == 0)
      {
        return Move::Empty;
      }

      return moveHistory[moveCount - 1];
    }

    bool getWhiteCanCastleKingside() const
    {
      return stateHistory[moveCount] & 0x0002 == 0x0002;
    }
    bool getWhiteCanCastleQueenside() const
    {
      return stateHistory[moveCount] & 0x0004 == 0x0004;
    }
    bool getBlackCanCastleKingside() const
    {
      return stateHistory[moveCount] & 0x0008 == 0x0008;
    }
    bool getBlackCanCastleQueenside() const
    {
      return stateHistory[moveCount] & 0x0010 == 0x0010;
    }
    unsigned char getEnPassantFile() const
    {
      return (stateHistory[moveCount] & 0x00E0) >> 5; // Extract the en passant square
    }
    Piece getLastCapturedPiece() const
    {
      return Piece((stateHistory[moveCount] & 0x0F00) >> 8);
    }
    int getFiftyMoveRuleCounter() const
    {
      return fiftyMoveRuleCounter[moveCount];
    }
  };
}
