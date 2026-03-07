#include <stdexcept>
#include "chess/board.hpp"
#include "chess/piece.hpp"
#include "chess/board/state.hpp"

namespace Chess
{

#pragma region state change
  void BoardUtils::StateHistory::pushState(const Move &move, Piece movedPiece, CastlingChange castlingChange)
  {
    State newState(history[current++], move, movedPiece.isType(Piece::Pawn));

    switch (castlingChange)
    {
    case CastlingChange::KingMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleMask : ~State::blackCastleMask;
      break;
    case CastlingChange::KingsideRookMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleKingsideMask : State::blackCastleKingsideMask;
      break;
    case CastlingChange::QueensideRookMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleQueensideMask : State::blackCastleQueensideMask;
      break;
    default:
      break;
    }

    history[current] = newState;
  }

  void BoardUtils::StateHistory::pushCaptureState(const Move &move, Piece capturedPiece, CastlingChange castlingChange)
  {
    State newState(history[current], move, capturedPiece);

    // castling changes
    if (capturedPiece.isType(Piece::Rook) && Board::squareCol(move.endSquare()) == 0)
    {
      // kingside rook capture
      newState.state &= currentTurnIsWhite() ? ~State::blackCastleKingsideMask : ~State::whiteCastleKingsideMask;
    }
    if (capturedPiece.isType(Piece::Rook) && Board::squareCol(move.endSquare()) == 7)
    {
      // queenside rook capture
      newState.state &= currentTurnIsWhite() ? ~State::blackCastleQueensideMask : ~State::whiteCastleQueensideMask;
    }

    switch (castlingChange)
    {
    case CastlingChange::KingMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleMask : ~State::blackCastleMask;
      break;
    case CastlingChange::KingsideRookMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleKingsideMask : ~State::blackCastleKingsideMask;
      break;
    case CastlingChange::QueensideRookMove:
      newState.state &= currentTurnIsWhite() ? ~State::whiteCastleQueensideMask : ~State::blackCastleQueensideMask;
      break;
    default:
      break;
    }

    history[++current] = newState;
  }

  void BoardUtils::StateHistory::pushCastleState(const Move &move)
  {
    State newState(history[current], move);
    // disable castling after this move
    newState.state &= ~(currentTurnIsWhite() ? State::whiteCastleMask : State::blackCastleMask);
    history[++current] = newState;
  }

  void BoardUtils::StateHistory::pushPawnDoubleMoveState(const Move &move)
  {
    State newState(history[current], move, true);
    // mark en passant as available and add the file to state
    newState.state |= State::enPassantAvailabilityMask | (Board::squareCol(move.endSquare()) << State::enPassantFileShift);
    history[++current] = newState;
  }
}
