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

#include <cstdint>
#include <stdexcept>

namespace Chess {
struct Piece {

  enum Color : uint8_t { White = 0b0000, Black = 0b0001 };

  enum Type : uint8_t {
    Pawn = 0b0010,
    Rook = 0b0100,
    Knight = 0b0110,
    Bishop = 0b1000,
    Queen = 0b1010,
    King = 0b1100,
  };

  unsigned char piece;

  constexpr Piece() : piece(0x00) {} // Default constructor for empty piece
  constexpr Piece(Color color, Type type) : piece(color | type) {
#ifdef DEBUG
    if (type < Pawn)
      throw std::runtime_error("invalid piece created");
    if (type > King)
      throw std::runtime_error("invalid piece created");
#endif
  }
  constexpr Piece(unsigned char piece) : piece(piece) {
#ifdef DEBUG
    if ((piece & 0b1110) < Pawn)
      throw std::runtime_error("invalid piece created");
    if ((piece & 0b1110) > King)
      throw std::runtime_error("invalid piece created");
#endif
  }

  constexpr Color color() const { return static_cast<Color>(piece & 0x01); }
  constexpr Type type() const {
    return static_cast<Type>(piece & 0b1110); // Mask to get the type bits
  }

  /// @brief promotion constructor
  /// @param other piece that is being promoted
  /// @param type type to promote to
  constexpr Piece(const Piece &other, Type type) : piece(other.color() | type) {}

  int spriteIndex() const { return piece - 2; }

  static const Piece Empty;

  bool operator==(const Piece &other) const { return piece == other.piece; }
  bool operator!=(const Piece &other) const { return piece != other.piece; }

  constexpr bool isType(const Piece::Type &other) const { return type() == other; }
  constexpr bool isColor(const Piece::Color &other) const { return color() == other; }
  constexpr bool isWhite() const { return color() == White; }
  constexpr bool isBlack() const { return color() == Black; }
};
} // namespace Chess
