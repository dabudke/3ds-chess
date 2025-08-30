#pragma once

namespace Chess
{
  struct Move
  {
    // 16 bit number
    // XXXX YYYYYY ZZZZZZ
    // X - move flag
    // y - start square index
    // z - end square index

    enum Flag : unsigned char
    {
      NoFlag,
      Capture,
      PawnDoubleMove,
      EnPassantCapture,
      BishopPromotion,
      KnightPromotion,
      RookPromotion,
      QueenPromotion,
      CastleKingside,
      CastleQueenside
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

    static const Move Empty;

    bool operator==(const Move &other) const
    {
      return move == other.move;
    }
    bool operator!=(const Move &other) const
    {
      return move != other.move;
    }
  };
}
