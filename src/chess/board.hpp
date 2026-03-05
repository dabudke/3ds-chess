#pragma once
#include "chess/piece.hpp"
#include "chess/move.hpp"
#include <vector>

namespace Chess
{
  class Board
  {
  public:
#pragma region static/helper utils

    static constexpr uint8_t square(int row, int col)
    {
      return (row << 3) + col;
    }
    static constexpr bool rowOffsetInBounds(unsigned char square, int rowOffset)
    {
      int row = (square >> 3) + rowOffset;
      return (0 <= row) && (row < 8);
    }
    static constexpr uint8_t rowOffset(unsigned char square, int rowOffset)
    {
      return square + (rowOffset << 3);
    }
    static constexpr bool colOffsetInBounds(unsigned char square, int colOffset)
    {
      int col = (square & 0x7) + colOffset;
      return (0 <= col) && (col < 8);
    }
    static constexpr uint8_t colOffset(unsigned char square, int colOffset)
    {
      return square + colOffset;
    }
    static constexpr bool squareOffsetInBounds(unsigned char square, int rowOffset, int colOffset)
    {
      int newRow = (square >> 3) + rowOffset;
      int newCol = (square & 0x7) + colOffset;

      if (newRow < 0 || newRow >= 8)
        return false;
      if (newCol < 0 || newCol >= 8)
        return false;
      return true;
    }
    static constexpr uint8_t squareOffset(uint8_t square, int rowOffset, int colOffset)
    {
      return square + rowOffset * 8 + colOffset;
    }

    static constexpr uint8_t squareRow(uint8_t square)
    {
      return square >> 3;
    }
    static constexpr uint8_t squareCol(uint8_t square)
    {
      return square & 0x7;
    }

    static constexpr uint64_t bitmaskForSquare(uint8_t square)
    {
      return static_cast<uint64_t>(1) << square;
    }
    static constexpr uint64_t bitmaskForRow(uint8_t row)
    {
      return static_cast<uint64_t>(0xFF) << (row * 8);
    }
    static constexpr uint64_t bitmaskForCol(uint8_t col)
    {
      return 0x0101010101010101ULL << col;
    }

    inline static int pieceToIndex(Piece::Type type, Piece::Color color)
    {
      return type - Piece::Pawn + color;
    }
    inline static constexpr int pieceToIndex(const Piece &piece)
    {
      return piece.piece - Piece::Pawn;
    }

    /// @brief create a board from FEN string
    /// @param fen FEN string to parse
    Board(std::string fen);

// board state
#pragma region board state

    class StateHistory; // forward declaration
    /// @brief container for a single snapshot of board state
    class State
    {
      // when initializing from FEN
      friend Board::Board(std::string fen);
      friend StateHistory;
      // `0x01` - white kingside castle available
      // `0x02` - black kingside castle available
      // `0x04` - white queenside castle available
      // `0x08` - black queenside castle available
      // `0x10` - en passant available
      // `0xE0` - en passant target file
      uint8_t state{0x0F};
      // last made move (uint16)
      Move move{Move::Empty};
      // fifty move counter
      uint8_t fiftyMoveCounter{0};
      // last capture (uint8)
      Chess::Piece lastCapture{Piece::Empty};

      // u8 + u16 + u8 + u8 = 5 bytes
      // probably the smallest i can make this

      State() {}
      State(uint16_t state, Move &move, uint8_t fiftyMoveCounter, Chess::Piece &lastCapture) : state(state), move(move), fiftyMoveCounter(fiftyMoveCounter), lastCapture(lastCapture) {}

      /// @brief copy state for making a move - all pushState functions should
      /// only need to deal with castling legality changes
      /// @param other previous state to copy from
      /// @param moveMade move that this state will represent
      /// @param resetFiftyMove reset the fifty move counter instead of increment
      State(const State &previous, const Move &moveMade, bool resetFiftyMove = false)
      {
        // copy state
        state = previous.state & 0x0F; // only keep castling state, not en passant
        if (!resetFiftyMove)
          fiftyMoveCounter = previous.fiftyMoveCounter + 1;
        move = moveMade;
      }
      State(const State &previous, const Move &moveMade, Piece &capture) : State(previous, moveMade, true)
      {
        lastCapture = capture;
      }

      static const uint8_t whiteCastleKingsideMask = 0x01;
      static const uint8_t blackCastleKingsideMask = 0x02;
      static const uint8_t whiteCastleQueensideMask = 0x04;
      static const uint8_t blackCastleQueensideMask = 0x08;
      static const uint8_t enPassantAvailabilityMask = 0x10;
      static const unsigned int enPassantFileShift = 5;
      static const uint8_t enPassantFileMask = 0xE0;

    public:
      bool canWhiteCastleKingside() const
      {
        return (state & whiteCastleKingsideMask) != 0;
      }
      bool canBlackCastleKingside() const
      {
        return (state & blackCastleKingsideMask) != 0;
      }
      bool canWhiteCastleQueenside() const
      {
        return (state & whiteCastleQueensideMask) != 0;
      }
      bool canBlackCastleQueenside() const
      {
        return (state & blackCastleQueensideMask) != 0;
      }
      bool enPassantAvailable() const
      {
        return (state & enPassantAvailabilityMask) != 0;
      }
      int getEnPassantFile() const
      {
        return (state & enPassantFileMask) >> enPassantFileShift;
      }
      unsigned char getFiftyMoveCounter() const
      {
        return fiftyMoveCounter;
      }

      const Piece &getLastCapture() const
      {
        return lastCapture;
      }
      const Move &getPreviousMove() const
      {
        return move;
      }
    };

    /// @brief container of all board history
    class StateHistory
    {
      // importing games from FEN/PGN - starting halfmove (fullmove - 1 * 2) + black turn?
      unsigned short startHalfmove{0};
      // turn is decided by halfmove
      unsigned short current{0};
      // history of state (6000 is an arbitrary number)
      static const size_t historySize{6000};
      State history[historySize];

    public:
      inline unsigned short currentHalfmove() const
      {
        return current + startHalfmove;
      }
      inline unsigned short currentFullmove() const
      {
        return (current + startHalfmove) / 2 + 1;
      }

      inline bool currentTurnIsWhite() const
      {
        return current % 2 == 0;
      }
      inline bool currentTurnIsBlack() const
      {
        return current % 2 == 1;
      }

      inline const State &getStateAtHalfmove(int halfmove) const
      {
        return history[halfmove - startHalfmove];
      }

      inline const State &getStateMovesAgo(int halfmovesAgo) const
      {
        return history[current - halfmovesAgo];
      }

      inline const State &getCurrentState() const
      {
        return history[current];
      }

      inline const std::vector<Move> getMoveHistory() const
      {
        std::vector<Move> moves;
        if (current >= 1)
          for (int i{1}; i <= current; i++)
          {
            moves.push_back(history[i].move);
          }
        return moves;
      }

      // state change operations
      // all Board functions that return StateHistory (which should be *none*)
      // can't call these non-const functions. thanks c++.
      void pushInitialState(int halfmove, State &initialState)
      {
        if (startHalfmove != 0 || current != 0)
          throw std::runtime_error("trying to push initial state and rewrite history");
        startHalfmove = halfmove;
        history[0] = initialState;
      }

      enum CastlingChange
      {
        None = 0x0,
        KingMove = 0x1,
        KingsideRookMove = 0x2,
        QueensideRookMove = 0x4,
      };

      /// @brief push ~~boring~~ state to the history
      /// @param move move that has been made
      /// @param movedPiece piece that has been moved (mainly for 50-move rule)
      /// @param castlingChange specific piece movement that cancels castling right
      void pushState(const Move &move, Chess::Piece movedPiece, CastlingChange castlingChange = None);

      /// @brief push state showing capture
      /// @param move move that has been made
      /// @param capturedPiece piece that was just captured
      /// @param castlingChange piece movement that cancels castling rights
      void pushCaptureState(const Move &move, Chess::Piece capturedPiece, CastlingChange castlingChange = None);

      void pushPawnDoubleMoveState(const Move &move);

      /// @brief push state representing a castle
      /// @param move castle move performed
      void pushCastleState(const Move &move);

      /// @brief 'unmake' previous move in state
      /// @return state reference for the move that is being unmade
      /// \attention Do not use returned state after pushing new state.
      const State &popSnapshot()
      {
        if (current <= 0)
          throw std::runtime_error("popping state beyond initial state");
        State &prevState = history[current];
        current--;
        return prevState;
      }
    };

    struct PieceIndex
    {
      /// @brief piece index entry; singly linked list
      struct Entry
      {
        uint8_t square;
        Entry *next;
        Entry(uint8_t square) : square{square}, next{nullptr} {}
      };

    private:
      Entry *entryRoots[12]{nullptr};

    public:
      PieceIndex()
      {
        for (int i{0}; i < 12; i++)
        {
          entryRoots[i] = nullptr;
        }
      }

      /// @brief get pointer to first entry in piece index
      /// @param piece piece to get the index for
      /// @return pointer to the first index entry
      inline const Entry *getIndex(Piece::Type type, Piece::Color color) const
      {
        return entryRoots[pieceToIndex(type, color)];
      }
      inline const Entry *getIndex(const Piece &piece) const
      {
        return entryRoots[pieceToIndex(piece)];
      }
      /// @brief get a specific entry in the piece index
      /// @param piece piece to retrieve the entry for
      /// @param square square to look for in index
      /// @return reference to that specific entry
      inline Entry &getEntry(const Piece &piece, uint8_t square)
      {
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
      inline void popEntry(const Piece &piece, uint8_t square)
      {
        Entry **entryRoot{&(entryRoots[pieceToIndex(piece)])};
        if ((*entryRoot)->square == square)
        {
          // entry root is the node to delete
          Entry *toDelete{*entryRoot};
          *entryRoot = toDelete->next;
          delete toDelete;
        }
        else
        {
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
      inline void addEntry(const Piece &piece, uint8_t square)
      {
        Entry **entryRoot{&(entryRoots[pieceToIndex(piece)])};
        if (*entryRoot == nullptr)
          // index root is empty
          *entryRoot = new Entry(square);
        else
        {
          Entry *entry{*entryRoot};
          while (entry->next != nullptr)
            entry = entry->next;
          entry->next = new Entry(square);
        }
      }
    };

  private:
    /// @brief helper object to manage state and history of the board
    StateHistory state;

#pragma region pieces
    /// @brief the great chessboard, 8 rows and 8 column
    /// @note to discuss - do we need this representation?
    /// theoretically, bitboards and piece indexes give all the info we need
    Piece board[64];

    /// @brief structure for accessing and modifying pieces indices
    PieceIndex pieceIndex;

    /// @brief structure for accessing bitboards for all pieces
    struct Bitboards
    {
    private:
      uint64_t bitboards[12]{0};

    public:
      /// @brief get a reference to a bitboard with piece type and color
      /// @param type type of piece to retrieve bitboard for
      /// @param color color of piece to retrieve bitboard for
      /// @return reference to the bitboard for that piece
      uint64_t &getBitboard(const Piece::Type &type, const Piece::Color &color)
      {
        // piece type - rook = values 0-6 -> *2 to get even values 0-12
        // add color to get odd indexes for black
        return bitboards[pieceToIndex(type, color)];
      }
      /// @brief get a reference to a bitboard for a certain piece
      /// @param piece piece to retrieve the bitboard for
      /// @return reference to the bitboard for the piece
      uint64_t &getBitboard(const Piece &piece)
      {
        return bitboards[pieceToIndex(piece)];
      }

      /// @brief calculate a bitboard for all white pieces
      /// @return bitboard of all white pieces
      uint64_t getWhitePiecesBitboard() const
      {
        uint64_t bitboard{0};
        for (int i{0}; i < 12; i += 2)
        {
          bitboard |= bitboards[i];
        }
        return bitboard;
      }
      /// @brief calculate bitboard for all black pieces
      /// @return bitboard of all black pieces
      uint64_t getBlackPiecesBitboard() const
      {
        uint64_t bitboard{0};
        for (int i{1}; i < 12; i += 2)
        {
          bitboard |= bitboards[i];
        }
        return bitboard;
      }
      /// @brief calculate bitboard for all pieces
      /// @return bitboard of all pieces
      uint64_t getAllPiecesBitboard() const
      {
        uint64_t bitboard{0};
        for (int i{0}; i < 12; i++)
        {
          bitboard |= bitboards[i];
        };
        return bitboard;
      }
    } bitboards;

    /// @brief move a piece in the board array and corresponding bitboard and pieceindex
    /// @param piece reference to piece to move
    /// @param target square the piece is on
    /// @param destination square to move the piece to
    inline void movePiece(const Piece &piece, unsigned char square, unsigned char destination);

    /// @brief remove a piece from the board array and corresponding bitboard and pieceindex
    /// @param piece reference to the piece to remove
    /// @param square square the piece is on
    inline void removePiece(const Piece &piece, unsigned char square);

    /// @brief summon a piece that was previously removed (or just spawn a new one idc)
    /// @param piece reference to the piece to manifest
    /// @param square square to summon the piece on
    inline void summonPiece(const Piece &piece, unsigned char square);

    /// @brief transform the piece from its current type to the new type and move it
    /// @param piece reference to the piece as it is
    /// @param square square the piece is currently on
    /// @param destination square to move the piece to
    /// @param newType type to transform the piece into
    inline void moveAndTransformPiece(const Piece &piece, unsigned char square, unsigned char destination, Piece::Type newType);

  public:
#pragma region piece getters
    inline uint64_t getBitboard(Piece::Type type, Piece::Color color)
    {
      return bitboards.getBitboard(type, color);
    }

    inline uint64_t getWhitePieceBitboard() const
    {
      return bitboards.getWhitePiecesBitboard();
    }
    inline uint64_t getBlackPieceBitboard() const
    {
      return bitboards.getBlackPiecesBitboard();
    }
    inline uint64_t getPieceBitboard() const
    {
      return bitboards.getAllPiecesBitboard();
    }

    inline const PieceIndex::Entry *getPieceIndex(const Piece &piece)
    {
      return pieceIndex.getIndex(piece);
    }

    /// @brief retrieve the piece at the given index
    /// @param index index of the square [0,64)
    /// @return the piece at that given index
    Piece getPiece(int index) const;
    /// @brief retrieve the peice at the given row and column
    /// @param row
    /// @param col
    /// @return the piece at that given row and column
    Piece getPiece(int row, int col) const;

#pragma region gameplay

    static const std::string initialFenString;

    Board() : Board(initialFenString) {}

    inline unsigned short getHalfmove() const
    {
      return state.currentHalfmove();
    }
    inline bool whiteMove() const
    {
      return state.currentTurnIsWhite();
    }
    inline bool blackMove() const
    {
      return state.currentTurnIsBlack();
    }

    const std::vector<Move> getLegalPawnMoves(Piece::Color color, unsigned char square) const;
    const std::vector<Move> getLegalRookMoves(Piece::Color color, unsigned char square) const;
    const std::vector<Move> getLegalKnightMoves(Piece::Color color, unsigned char square) const;
    const std::vector<Move> getLegalBishopMoves(Piece::Color color, unsigned char square) const;
    const std::vector<Move> getLegalQueenMoves(Piece::Color color, unsigned char square) const;
    const std::vector<Move> getLegalKingMoves(Piece::Color color, unsigned char square) const;

    /// @brief generate legal moves for the specified square
    /// @param square square to generate moves for
    /// @return vector containing all the moves
    inline const std::vector<Move> getLegalMovesForSquare(unsigned char square) const
    {
      const Piece &piece = getPiece(square);
      switch (piece.type())
      {
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
        return std::vector<Move>{};
      }
    }
    /// @brief helper function to generate all legal moves on the board
    /// @return all legal moves at current board state
    // TODO - rename getAllLegalMoves
    inline const std::vector<Move> getLegalMoves() const
    {
      std::vector<Move> legalMoves;
      Piece::Color currentTurn{whiteMove() ? Piece::White : Piece::Black};

      for (auto piece{pieceIndex.getIndex(Piece::Pawn, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalPawnMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      for (auto piece{pieceIndex.getIndex(Piece::Rook, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalRookMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      for (auto piece{pieceIndex.getIndex(Piece::Knight, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalKnightMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      for (auto piece{pieceIndex.getIndex(Piece::Bishop, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalBishopMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      for (auto piece{pieceIndex.getIndex(Piece::Queen, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalQueenMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
      for (auto piece{pieceIndex.getIndex(Piece::King, currentTurn)};
           piece != nullptr; piece = piece->next)
      {
        auto moves = getLegalKingMoves(currentTurn, piece->square);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
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

    /// @brief return a snapshot of the current state
    /// @return snapshot of current state
    const State &getCurrentState() const
    {
      return state.getCurrentState();
    }
    bool canWhiteCastleKingside() const
    {
      return state.getCurrentState().canWhiteCastleKingside();
    }
    bool canBlackCastleKingside() const
    {
      return state.getCurrentState().canBlackCastleKingside();
    }
    bool canWhiteCastleQueenside() const
    {
      return state.getCurrentState().canWhiteCastleQueenside();
    }
    bool canBlackCastleQueenside() const
    {
      return state.getCurrentState().canBlackCastleQueenside();
    }

    /// @brief Get the FEN string of the current position
    /// @return FEN string of current position
    std::string getFenString() const;

    /// @brief get a vector containing all previous moves made
    /// @return vector containing all previous moves made
    const std::vector<Move> getMoveHistory() const
    {
      return state.getMoveHistory();
    }
  };
}
