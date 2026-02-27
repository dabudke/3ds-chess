#pragma once
#include <string>
#include "piece.hpp"

namespace Chess
{
  struct Move
  {
    // 16 bit number
    // XXXX YYYYYY ZZZZZZ
    // X - move flag
    // y - start square index
    // z - end square index

    enum Flag : unsigned char // only 0-F!!
    {
      NoFlag = 0x0,
      Capture = 0x1,

      PawnDoubleMove = 0x2,
      EnPassantCapture = 0x3,

      CastleKingside = 0x4,
      CastleQueenside = 0x5,

      RookPromotion = 0x6,
      KnightPromotion = 0x7,
      BishopPromotion = 0x8,
      QueenPromotion = 0x9,

      RookPromotionCapture = 0xA,
      KnightPromotionCapture = 0xB,
      BishopPromotionCapture = 0xC,
      QueenPromotionCapture = 0xD,
    };

    unsigned short move;

    unsigned char startSquare() const
    {
      return move & 0x3F; // Extract the first 6 bits
    };
    unsigned char endSquare() const
    {
      return (move >> 6) & 0x3F; // Extract the next 6 bits
    };
    Flag flags() const
    {
      return static_cast<Flag>((move >> 12) & 0x0F); // Extract the last 4 bits
    };

    Move(unsigned char start, unsigned char end, Flag flags = NoFlag)
    {
      move = (flags << 12) | (end << 6) | start; // Combine into a single 16-bit number
    }
    Move() : move(0) {}

    std::string getNotation(Piece movedPiece) const; // returns algebraic notation of move

    static const Move Empty;

    bool operator==(const Move &other) const
    {
      return move == other.move;
    }
    bool operator!=(const Move &other) const
    {
      return move != other.move;
    }

    bool operator<(const Move &other) const
    {
      return move < other.move;
    }
  };
}
