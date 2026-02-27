#pragma once
#include "chess/piece.hpp"
#include "chess/move.hpp"
#include <vector>

namespace Chess
{
  class Board
  {
  public:
    // static or helper utils

    static inline unsigned char square(int row, int col)
    {
      return row * 8 + col;
    }
    static inline bool rowOffsetInBounds(unsigned char square, int rowOffset)
    {
      int row = square / 8 + rowOffset;
      return (0 <= row) && (row < 8);
    }
    static inline bool colOffsetInBounds(unsigned char square, int colOffset)
    {
      int col = square % 8 + colOffset;
      return (0 <= col) && (col < 8);
    }
    static inline bool squareOffsetInBounds(unsigned char square, int rowOffset, int colOffset)
    {
      int newRow = square / 8 + rowOffset;
      int newCol = square % 8 + colOffset;

      if (newRow < 0 || newRow >= 8)
        return false;
      if (newCol < 0 || newCol >= 8)
        return false;
      return true;
    }
    static inline unsigned char squareOffset(unsigned char square, int rowOffset, int colOffset)
    {
      return square + rowOffset * 8 + colOffset;
    }

    static inline unsigned char squareRow(unsigned char square)
    {
      return square / 8;
    }
    static inline unsigned char squareCol(unsigned char square)
    {
      return square % 8;
    }

    Board(std::string fen); // forward declaration

    // board state

    class StateHistory; // forward declaration
    /// @brief container for a single snapshot of board state
    class State
    {
      // when initializing from FEN
      friend Board::Board(std::string fen);
      friend StateHistory;
      // `0x0001` - white kingside castle available
      // `0x0002` - black kingside castle available
      // `0x0004` - white queenside castle available
      // `0x0008` - black queenside castle available
      // `0x0010` - en passant available
      // `0x00E0` - en passant target file
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

      static const uint16_t whiteCastleKingsideMask = 0x01;
      static const uint16_t blackCastleKingsideMask = 0x02;
      static const uint16_t whiteCastleQueensideMask = 0x04;
      static const uint16_t blackCastleQueensideMask = 0x08;
      static const uint16_t enPassantAvailabilityMask = 0x10;
      static const unsigned int enPassantFileShift = 5;
      static const uint16_t enPassantFileMask = 0xE0;

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

  private:
    /// @brief the great chessboard, 8 rows and 8 column
    Piece board[64];

    /// @brief helper object to manage state and history of the board
    StateHistory state;

  public:
    static const std::string initialFenString;

    Board() : Board(initialFenString) {};

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

    /// @brief retrieve the piece at the given index
    /// @param index index of the square [0,64)
    /// @return the piece at that given index
    Piece getPiece(int index) const;
    /// @brief retrieve the peice at the given row and column
    /// @param row
    /// @param col
    /// @return the piece at that given row and column
    Piece getPiece(int row, int col) const;

    /// @brief generate legal moves for the specified square
    /// @param square square to generate moves for
    /// @return vector containing all the moves
    const std::vector<Move> getLegalMovesForSquare(unsigned char square) const;
    /// @brief helper function to generate all legal moves on the board
    /// @return all legal moves at current board state
    const std::vector<Move> getLegalMoves() const
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

    /// @brief make the move previously generated by getLegalMoves
    /// @param move move generated by getLegalMoves
    /// @return if the move could be made or not. Might error in the future
    void makeMove(const Move &move);
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
