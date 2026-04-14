#pragma once
#include <array>
#include <cstdint>
#include <forward_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "chess/board/state.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"

namespace Chess {
class Board {
public:
#pragma region static/helper utils

  static constexpr uint8_t square(int row, int col) { return (row << 3) + col; }
  static constexpr bool rowOffsetInBounds(unsigned char square, int rowOffset) {
    int row = (square >> 3) + rowOffset;
    return (0 <= row) && (row < 8);
  }
  static constexpr uint8_t rowOffset(unsigned char square, int rowOffset) { return square + (rowOffset << 3); }
  static constexpr bool colOffsetInBounds(unsigned char square, int colOffset) {
    int col = (square & 0x7) + colOffset;
    return (0 <= col) && (col < 8);
  }
  static constexpr uint8_t colOffset(unsigned char square, int colOffset) { return square + colOffset; }
  static constexpr bool squareOffsetInBounds(unsigned char square, int rowOffset, int colOffset) {
    int newRow = (square >> 3) + rowOffset;
    int newCol = (square & 0x7) + colOffset;

    if (newRow < 0 || newRow >= 8)
      return false;
    if (newCol < 0 || newCol >= 8)
      return false;
    return true;
  }
  static constexpr uint8_t squareOffset(uint8_t square, int rowOffset, int colOffset) {
    return square + rowOffset * 8 + colOffset;
  }

  static constexpr uint8_t squareRow(uint8_t square) { return square >> 3; }
  static constexpr uint8_t squareCol(uint8_t square) { return square & 0x7; }

  static constexpr uint64_t bitmaskForSquare(uint8_t square) { return static_cast<uint64_t>(1) << square; }
  static constexpr uint64_t bitmaskForRow(uint8_t row) { return static_cast<uint64_t>(0xFF) << (row * 8); }
  static constexpr uint64_t bitmaskForCol(uint8_t col) { return 0x0101010101010101ULL << col; }

  inline static constexpr int pieceToIndex(Piece::Type type, Piece::Color color) { return type - Piece::Pawn + color; }
  inline static constexpr int pieceToIndex(const Piece &piece) { return piece.piece - Piece::Pawn; }

  static const std::array<std::array<uint64_t, 64>, 2> pawnAttacks;
  static const std::array<uint64_t, 64> knightAttacks;
  static const std::array<uint64_t, 64> kingAttacks;

  /// @brief create a board from FEN string
  /// @param fen FEN string to parse
  Board(std::string fen);

// board state
#pragma region board state

  struct PieceIndex {
    /// @brief piece index entry; singly linked list
    struct Entry {
      uint8_t square;
      Entry *next;
      Entry(uint8_t square) : square{square}, next{nullptr} {}
    };

  private:
    Entry *entryRoots[12]{nullptr};

  public:
    PieceIndex() {
      for (int i{0}; i < 12; i++) {
        entryRoots[i] = nullptr;
      }
    }

    /// @brief get pointer to first entry in piece index
    /// @param piece piece to get the index for
    /// @return pointer to the first index entry
    inline const Entry *getIndex(Piece::Type type, Piece::Color color) const {
      return entryRoots[pieceToIndex(type, color)];
    }
    inline const Entry *getIndex(const Piece &piece) const { return entryRoots[pieceToIndex(piece)]; }
    /// @brief get a specific entry in the piece index
    /// @param piece piece to retrieve the entry for
    /// @param square square to look for in index
    /// @return reference to that specific entry
    inline Entry &getEntry(const Piece &piece, uint8_t square) {
      Entry *entry{entryRoots[pieceToIndex(piece)]};
      while (entry && entry->square != square)
        entry = entry->next;
      if (!entry)
        throw std::runtime_error("requesting entry that does not exist");
      return *entry;
    }

    /// @brief remove an entry from the piece index
    /// @param piece piece index to remove from
    /// @param square square to remove from piece index
    inline void popEntry(const Piece &piece, uint8_t square) {
      Entry **entryRoot{&(entryRoots[pieceToIndex(piece)])};
      if ((*entryRoot)->square == square) {
        // entry root is the node to delete
        Entry *toDelete{*entryRoot};
        *entryRoot = toDelete->next;
        delete toDelete;
      } else {
        // start from root, iterate until the next entry is the square we want
        Entry *entry{*entryRoot};
        while (entry->next->square != square)
          entry = entry->next;
        // store pointer to entry to delete
        Entry *toDelete = entry->next;
        // unlink from list
        entry->next = toDelete->next;
        delete toDelete;
      }
    }

    /// @brief add an entry to the piece index
    /// @param piece piece index to add to
    /// @param square square to place in the index
    inline void addEntry(const Piece &piece, uint8_t square) {
      Entry **entryRoot{&(entryRoots[pieceToIndex(piece)])};
      if (*entryRoot == nullptr)
        // index root is empty
        *entryRoot = new Entry(square);
      else {
        Entry *entry{*entryRoot};
        while (entry->next != nullptr)
          entry = entry->next;
        entry->next = new Entry(square);
      }
    }
  };

private:
  /// @brief helper object to manage state and history of the board
  BoardUtils::StateHistory state;

#pragma region pieces
  /// @brief the great chessboard, 8 rows and 8 column
  /// @note to discuss - do we need this representation?
  /// theoretically, bitboards and piece indexes give all the info we need
  Piece board[64];

  /// @brief structure for accessing and modifying pieces indices
  PieceIndex pieceIndex;

  /// @brief structure for accessing bitboards for all pieces
  struct Bitboards {
  private:
    uint64_t bitboards[12]{0};

  public:
    /// @brief get a reference to a bitboard with piece type and color
    /// @param type type of piece to retrieve bitboard for
    /// @param color color of piece to retrieve bitboard for
    /// @return reference to the bitboard for that piece
    uint64_t &getBitboard(const Piece::Type &type, const Piece::Color &color) {
      // piece type - rook = values 0-6 -> *2 to get even values 0-12
      // add color to get odd indexes for black
      return bitboards[pieceToIndex(type, color)];
    }
    const uint64_t &getBitboard(const Piece::Type &type, const Piece::Color &color) const {
      return bitboards[pieceToIndex(type, color)];
    }
    /// @brief get a reference to a bitboard for a certain piece
    /// @param piece piece to retrieve the bitboard for
    /// @return reference to the bitboard for the piece
    uint64_t &getBitboard(const Piece &piece) { return bitboards[pieceToIndex(piece)]; }
    const uint64_t &getBitboard(const Piece &piece) const { return bitboards[pieceToIndex(piece)]; }

    /// @brief get a bitboard of both colors of piece type
    /// @param type
    /// @return
    uint64_t getPieceTypeBitboard(const Piece::Type &type) const {
      return bitboards[pieceToIndex(type, Piece::White)] | bitboards[pieceToIndex(type, Piece::Black)];
    }
    uint64_t getPieceTypeBitboard(const Piece &piece) const {
      return bitboards[pieceToIndex(piece.type(), Piece::White)] | bitboards[pieceToIndex(piece.type(), Piece::Black)];
    }

    /// @brief calculate a bitboard for all white pieces
    /// @return bitboard of all white pieces
    uint64_t getWhitePiecesBitboard() const {
      uint64_t bitboard{0};
      for (int i{0}; i < 12; i += 2) {
        bitboard |= bitboards[i];
      }
      return bitboard;
    }
    /// @brief calculate bitboard for all black pieces
    /// @return bitboard of all black pieces
    uint64_t getBlackPiecesBitboard() const {
      uint64_t bitboard{0};
      for (int i{1}; i < 12; i += 2) {
        bitboard |= bitboards[i];
      }
      return bitboard;
    }
    /// @brief calculate bitboard for all pieces
    /// @return bitboard of all pieces
    uint64_t getAllPiecesBitboard() const {
      uint64_t bitboard{0};
      for (int i{0}; i < 12; i++) {
        bitboard |= bitboards[i];
      };
      return bitboard;
    }
  } bitboards;

  /// @brief move a piece in the board array and corresponding bitboard and pieceindex
  /// @param piece reference to piece to move
  /// @param target square the piece is on
  /// @param destination square to move the piece to
  inline void movePiece(const Piece &piece, unsigned char square, unsigned char destination) {
    pieceIndex.getEntry(piece, square).square = destination;
    bitboards.getBitboard(piece) ^= bitmaskForSquare(square) | bitmaskForSquare(destination);
    board[destination] = piece;
    board[square] = Piece::Empty;
  }

  /// @brief remove a piece from the board array and corresponding bitboard and pieceindex
  /// @param piece reference to the piece to remove
  /// @param square square the piece is on
  inline void removePiece(const Piece &piece, unsigned char square) {
    // remove entry in piece index
    pieceIndex.popEntry(piece, square);
    // unset bit in bitboard
    bitboards.getBitboard(piece) &= ~bitmaskForSquare(square);
    // remove piece from board array
    board[square] = Piece::Empty;
  }

  /// @brief summon a piece that was previously removed (or just spawn a new one idc)
  /// @param piece reference to the piece to manifest
  /// @param square square to summon the piece on
  inline void summonPiece(const Piece &piece, unsigned char square) {
    pieceIndex.addEntry(piece, square);
    bitboards.getBitboard(piece) |= bitmaskForSquare(square);
    board[square] = piece;
  }

  /// @brief transform the piece from its current type to the new type and move it
  /// @param piece reference to the piece as it is
  /// @param square square the piece is currently on
  /// @param destination square to move the piece to
  /// @param newType type to transform the piece into
  inline void moveAndTransformPiece(const Piece &piece, unsigned char square, unsigned char destination,
                                    Piece::Type newType) {
    // create new piece, so possible references to old piece aren't invalidated
    Piece newPiece = Piece(piece, newType);

    // remove piece from initial bitboard
    pieceIndex.popEntry(piece, square);
    bitboards.getBitboard(piece) &= ~bitmaskForSquare(square);
    board[square] = Piece::Empty;

    pieceIndex.addEntry(newPiece, destination);
    bitboards.getBitboard(newPiece) |= bitmaskForSquare(destination);
    board[destination] = newPiece;
  }

#pragma region attacks/pins

  /// @brief check if square is attacked by any enemy piece
  /// @param occupancy occupancy bitboard
  /// @param square square to check for attacks on
  /// @return if the square is attacked by any piece
  bool squareIsAttacked(const uint64_t &occupancy, uint8_t square);

  /// @brief get pieces that attack given square (i.e. get the knight
  /// that needs capturing as it is giving check)
  /// @param occupancy occupancy bitboard
  /// @param square square to check for attackers on
  /// @return bitboard of all attackers
  uint64_t squareAttackedBy(uint64_t occupancy, uint8_t square);

public:
#pragma region getters
  inline uint64_t getBitboard(Piece::Type type, Piece::Color color) { return bitboards.getBitboard(type, color); }

  inline uint64_t getWhitePieceBitboard() const { return bitboards.getWhitePiecesBitboard(); }
  inline uint64_t getBlackPieceBitboard() const { return bitboards.getBlackPiecesBitboard(); }
  inline uint64_t getPieceBitboard() const { return bitboards.getAllPiecesBitboard(); }

  inline const PieceIndex::Entry *getPieceIndex(const Piece &piece) { return pieceIndex.getIndex(piece); }

  /// @brief return a snapshot of the current state
  /// @return snapshot of current state
  const BoardUtils::State &getCurrentState() const { return state.getCurrentState(); }
  bool canWhiteCastleKingside() const { return state.getCurrentState().canWhiteCastleKingside(); }
  bool canBlackCastleKingside() const { return state.getCurrentState().canBlackCastleKingside(); }
  bool canWhiteCastleQueenside() const { return state.getCurrentState().canWhiteCastleQueenside(); }
  bool canBlackCastleQueenside() const { return state.getCurrentState().canBlackCastleQueenside(); }

  /// @brief Get the FEN string of the current position
  /// @return FEN string of current position
  std::string getFenString() const;

  /// @brief get a vector containing all previous moves made
  /// @return vector containing all previous moves made
  const std::vector<Move> getMoveHistory() const { return state.getMoveHistory(); }

  inline unsigned short getHalfmove() const { return state.currentHalfmove(); }
  inline bool whiteMove() const { return state.currentTurnIsWhite(); }
  inline bool blackMove() const { return state.currentTurnIsBlack(); }

  /// @brief retrieve the piece at the given index
  /// @param index index of the square [0,64)
  /// @return the piece at that given index
  const Piece getPiece(int index) const {
#ifdef DEBUG
    if (index < 0 || index >= 64)
      throw std::out_of_range("board index out of bounds");
#endif
    return board[index];
  }
  /// @brief retrieve the peice at the given row and column
  /// @param row
  /// @param col
  /// @return the piece at that given row and column
  const Piece getPiece(int row, int col) const {
#ifdef DEBUG
    if (row < 0 || row >= 8)
      throw std::out_of_range("row out of bounds");
    if (col < 0 || col >= 8)
      throw std::out_of_range("col out of bounds");
#endif
    return board[(row << 3) + col];
  }

#pragma region gameplay

  static const std::string initialFenString;
  Board() : Board(initialFenString) {}

  const std::forward_list<Move> getLegalPawnMoves(Piece::Color color, unsigned char square) const;
  const std::forward_list<Move> getLegalRookMoves(Piece::Color color, unsigned char square) const;
  const std::forward_list<Move> getLegalKnightMoves(Piece::Color color, unsigned char square) const;
  const std::forward_list<Move> getLegalBishopMoves(Piece::Color color, unsigned char square) const;
  const std::forward_list<Move> getLegalQueenMoves(Piece::Color color, unsigned char square) const;
  const std::forward_list<Move> getLegalKingMoves(Piece::Color color, unsigned char square) const;

  /// @brief generate legal moves for the specified square
  /// @param square square to generate moves for
  /// @return vector containing all the moves
  inline const std::forward_list<Move> getLegalMovesForSquare(unsigned char square) const {
    const Piece &piece = getPiece(square);
    switch (piece.type()) {
    case Piece::Pawn:
      return getLegalPawnMoves(piece.color(), square);
    case Piece::Rook:
      return getLegalRookMoves(piece.color(), square);
    case Piece::Knight:
      return getLegalKnightMoves(piece.color(), square);
    case Piece::Bishop:
      return getLegalBishopMoves(piece.color(), square);
    case Piece::Queen:
      return getLegalQueenMoves(piece.color(), square);
    case Piece::King:
      return getLegalKingMoves(piece.color(), square);
    default:
      return std::forward_list<Move>{};
    }
  }
  /// @brief helper function to generate all legal moves on the board
  /// @return all legal moves at current board state
  // TODO - rename getAllLegalMoves
  inline std::forward_list<Move> getLegalMoves() const {
    std::forward_list<Move> legalMoves;
    Piece::Color currentTurn{whiteMove() ? Piece::White : Piece::Black};

    for (auto piece{pieceIndex.getIndex(Piece::Pawn, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalPawnMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }
    for (auto piece{pieceIndex.getIndex(Piece::Rook, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalRookMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }
    for (auto piece{pieceIndex.getIndex(Piece::Knight, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalKnightMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }
    for (auto piece{pieceIndex.getIndex(Piece::Bishop, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalBishopMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }
    for (auto piece{pieceIndex.getIndex(Piece::Queen, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalQueenMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }
    for (auto piece{pieceIndex.getIndex(Piece::King, currentTurn)}; piece != nullptr; piece = piece->next) {
      auto moves = getLegalKingMoves(currentTurn, piece->square);
      legalMoves.merge(moves);
    }

    return legalMoves;
  }

  /// @brief make (apply) the move previously generated by getLegalMoves
  /// technically a helper for making moves in search
  /// @param move move generated by getLegalMoves
  void makeMove(const Move &move);

  /// @brief attempt to make a move on the board, checking legality
  /// @param startSquare
  /// @param endSquare
  /// @return whether or not the move could be successfully made
  bool makeMove(uint8_t startSquare, uint8_t endSquare);

  /// @brief unmake the previous move
  void unmakeMove();
};
} // namespace Chess
