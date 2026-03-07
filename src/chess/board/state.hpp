#pragma once
#include <vector>
#include "chess/move.hpp"

namespace Chess
{
  namespace BoardUtils
  {
    class StateHistory; // forward declaration
    /// @brief container for a single snapshot of board state
    class State
    {
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

      State() = default;

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
      /// @brief copy constructor for captures
      /// @param previous previous state to copy from
      /// @param moveMade previous move made at this state
      /// @param capture captured piece
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

      static const uint8_t whiteCastleMask = 0x05;
      static const uint8_t blackCastleMask = 0x0A;

    public:
      // const getters for state retrieval
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
      // history of state (long games around 150 moves long)
      static const size_t initialHistorySize{300};
      size_t historySize{initialHistorySize};
      State history[initialHistorySize];

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
      void setInitialState(uint16_t halfmove,
                           bool canWhiteCastleKingside,
                           bool canBlackCastleKingside,
                           bool canWhiteCastleQueenside,
                           bool canBlackCastleQueenside,
                           bool enPassantAvailable,
                           uint8_t enPassantFile,
                           uint8_t fiftyMoveCounter)
      {
        if (startHalfmove != 0 || current != 0)
          throw std::runtime_error("trying to push initial state and rewrite history");
        startHalfmove = halfmove;
        State &initialState = history[0];
        if (!canWhiteCastleKingside)
          initialState.state ^= State::whiteCastleKingsideMask;
        if (!canBlackCastleKingside)
          initialState.state ^= State::blackCastleKingsideMask;
        if (!canWhiteCastleQueenside)
          initialState.state ^= State::whiteCastleQueensideMask;
        if (!canBlackCastleQueenside)
          initialState.state ^= State::blackCastleQueensideMask;

        if (enPassantAvailable)
          initialState.state |= State::enPassantAvailabilityMask | (enPassantFile << State::enPassantFileShift);

        initialState.fiftyMoveCounter = fiftyMoveCounter;
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
  }
}
