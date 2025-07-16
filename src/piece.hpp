#pragma once

// 0000 - empty
// 0010 - black pawn
// 0011 - white pawn
// 0100 - black rook
// 0101 - white rook
// 0110 - black knight
// 0111 - white knight
// 1000 - black bishop
// 1001 - white bishop
// 1010 - black queen
// 1011 - white queen
// 1100 - black king
// 1101 - white king

struct Piece
{
  static const unsigned char Black = 0b0000;
  static const unsigned char White = 0b0001;

  static const unsigned char Pawn = 0b0010;
  static const unsigned char Rook = 0b0100;
  static const unsigned char Knight = 0b0110;
  static const unsigned char Bishop = 0b1000;
  static const unsigned char Queen = 0b1010;
  static const unsigned char King = 0b1100;

  unsigned char pieceType;

  Piece() : pieceType(0x00) {} // Default constructor for empty piece
  Piece(unsigned char color, unsigned char type) : pieceType(color | type) {}

  unsigned char color() const
  {
    return pieceType & 0x01;
  }
  unsigned char type() const
  {
    return pieceType & 0b1110; // Mask to get the type bits
  }

  static const Piece Empty;

  bool operator==(const Piece &other) const
  {
    return pieceType == other.pieceType;
  }
  bool operator!=(const Piece &other) const
  {
    return pieceType != other.pieceType;
  }
};
