#pragma once

// 0000 - empty
// 0010 - white pawn
// 0011 - black pawn
// 0100 - white rook
// 0101 - black rook
// 0110 - white knight
// 0111 - black knight
// 1000 - white bishop
// 1001 - black bishop
// 1010 - white queen
// 1011 - black queen
// 1100 - white king
// 1101 - black king

namespace Chess {
struct Piece {
  typedef unsigned char Color;
  static constexpr Color White = 0b0000;
  static constexpr Color Black = 0b0001;

  typedef unsigned char Type;
  static constexpr Type Pawn = 0b0010;
  static constexpr Type Rook = 0b0100;
  static constexpr Type Knight = 0b0110;
  static constexpr Type Bishop = 0b1000;
  static constexpr Type Queen = 0b1010;
  static constexpr Type King = 0b1100;

  unsigned char piece;

  constexpr Piece() : piece(0x00) {} // Default constructor for empty piece
  constexpr Piece(Color color, Type type) : piece(color | type) {}
  constexpr Piece(unsigned char piece) : piece(piece) {}

  unsigned char color() const { return piece & 0x01; }
  unsigned char type() const {
    return piece & 0b1110; // Mask to get the type bits
  }

  /// @brief promotion constructor
  /// @param other piece that is being promoted
  /// @param type type to promote to
  constexpr Piece(const Piece &other, Type type) : piece(other.color() | type) {}

  int spriteIndex() const { return piece - 2; }

  static const Piece Empty;

  bool operator==(const Piece &other) const { return piece == other.piece; }
  bool operator!=(const Piece &other) const { return piece != other.piece; }

  inline bool isType(const Piece::Type &other) const { return type() == other; }
  inline bool isColor(const Piece::Color &other) const { return color() == other; }
  inline bool isWhite() const { return color() == White; }
  inline bool isBlack() const { return color() == Black; }
};
} // namespace Chess
