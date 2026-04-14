#include <cstdint>

#include "chess/board.hpp"
#include "chess/board/state.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"

namespace Chess {

using BoardUtils::StateHistory;

static const uint8_t kingsideRookStart[2] = {7, 63};
static const uint8_t kingsideRookEnd[2] = {5, 61};
static const uint8_t queensideRookStart[2] = {0, 56};
static const uint8_t queensideRookEnd[2] = {3, 59};

#pragma region perform
void Board::makeMove(const Move &move) {
  Piece piece(getPiece(move.startSquare()));

  // we trust that the move we were given is legal. please.

  if (move.flags() == Move::Flag::PawnDoubleMove) {
    movePiece(piece, move.startSquare(), move.endSquare());
    state.pushPawnDoubleMoveState(move);
    return;
  }

  // handle castling (no captures possible)
  if (move.flags() == Move::Flag::CastleKingside) {
    const uint8_t kingStart = move.startSquare();
    const uint8_t kingEnd = move.endSquare();
    const Piece::Color color = piece.color();

    movePiece(piece, kingStart, kingEnd);
    movePiece(getPiece(kingsideRookStart[color]), kingsideRookStart[color], kingsideRookEnd[color]);
    state.pushCastleState(move);
    return;
  }
  if (move.flags() == Move::Flag::CastleQueenside) {
    const uint8_t kingStart = move.startSquare();
    const uint8_t kingEnd = move.endSquare();
    const Piece::Color color = piece.color();

    movePiece(piece, kingStart, kingEnd);
    movePiece(getPiece(queensideRookStart[color]), queensideRookStart[color], queensideRookEnd[color]);
    state.pushCastleState(move);
    return;
  }

  // standard moves beyond this point
  // handle captures
  Piece capturedPiece{Piece::Empty};
  switch (move.flags()) {
  case Move::Flag::RookPromotionCapture:
  case Move::Flag::KnightPromotionCapture:
  case Move::Flag::BishopPromotionCapture:
  case Move::Flag::QueenPromotionCapture:
  case Move::Flag::Capture:
    capturedPiece = getPiece(move.endSquare());
    removePiece(capturedPiece, move.endSquare());
    break;

  // en passant is special. my special little boy.
  case Move::Flag::EnPassantCapture: {
    uint8_t captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
    capturedPiece = getPiece(captureSquare);
    removePiece(capturedPiece, captureSquare);
    break;
  }

  default:
    break;
  }

  // handle promotions
  Piece::Type promotionType{};
  switch (move.flags()) {
  case Move::Flag::RookPromotion:
  case Move::Flag::RookPromotionCapture:
    promotionType = Piece::Rook;
    break;

  case Move::Flag::KnightPromotion:
  case Move::Flag::KnightPromotionCapture:
    promotionType = Piece::Knight;
    break;

  case Move::Flag::BishopPromotion:
  case Move::Flag::BishopPromotionCapture:
    promotionType = Piece::Bishop;
    break;

  case Move::Flag::QueenPromotion:
  case Move::Flag::QueenPromotionCapture:
    promotionType = Piece::Queen;
    break;

  default:
    break;
  }

  // castling change
  StateHistory::CastlingChange castlingChange{StateHistory::CastlingChange::None};
  if (move.startSquare() == square(whiteMove() ? 0 : 7, 0) && piece.isType(Piece::Rook)) {
    castlingChange = StateHistory::CastlingChange::QueensideRookMove;
  } else if (move.startSquare() == square(whiteMove() ? 0 : 7, 7) && piece.isType(Piece::Rook)) {
    castlingChange = StateHistory::CastlingChange::KingsideRookMove;
  } else if (piece.isType(Piece::King)) {
    castlingChange = StateHistory::CastlingChange::KingMove;
  }

  // update state
  if (capturedPiece != Piece::Empty) // no capture
    state.pushCaptureState(move, capturedPiece, castlingChange);
  else // yes capture
    state.pushState(move, piece, castlingChange);

  // update board
  if (promotionType != 0)
    moveAndTransformPiece(piece, move.startSquare(), move.endSquare(), promotionType);
  else
    movePiece(piece, move.startSquare(), move.endSquare());
  return;
}

#pragma region undo
void Board::unmakeMove() {
  const BoardUtils::State &undoState = state.popSnapshot();
  const Move &move = undoState.getPreviousMove();
  const Piece movedPiece(getPiece(move.endSquare()));

  if (move.flags() == Move::Flag::CastleKingside) {
    const uint8_t kingStart = move.startSquare();
    const uint8_t kingEnd = move.endSquare();
    const Piece::Color color = movedPiece.color();

    movePiece(movedPiece, kingEnd, kingStart);
    movePiece(getPiece(kingsideRookEnd[color]), kingsideRookEnd[color], kingsideRookStart[color]);
    return;
  }
  if (move.flags() == Move::Flag::CastleQueenside) {
    const uint8_t kingStart = move.startSquare();
    const uint8_t kingEnd = move.endSquare();
    const Piece::Color color = movedPiece.color();

    movePiece(movedPiece, kingEnd, kingStart);
    movePiece(getPiece(queensideRookEnd[color]), queensideRookEnd[color], queensideRookStart[color]);
    return;
  }

  // back to standard moves

  // handle de-promotions
  switch (move.flags()) {
  case Move::Flag::RookPromotion:
  case Move::Flag::RookPromotionCapture:
  case Move::Flag::KnightPromotion:
  case Move::Flag::KnightPromotionCapture:
  case Move::Flag::BishopPromotion:
  case Move::Flag::BishopPromotionCapture:
  case Move::Flag::QueenPromotion:
  case Move::Flag::QueenPromotionCapture:
    board[move.startSquare()] = Piece(movedPiece, Piece::Pawn);
    break;

  default:
    // no promotion, just put the moved piece back
    movePiece(movedPiece, move.endSquare(), move.startSquare());
    break;
  }

  // restore capture
  const Piece &lastCapture = undoState.getLastCapture();
  switch (move.flags()) {
  case Move::Flag::RookPromotionCapture:
  case Move::Flag::KnightPromotionCapture:
  case Move::Flag::BishopPromotionCapture:
  case Move::Flag::QueenPromotionCapture:
  case Move::Flag::Capture:
    summonPiece(lastCapture, move.endSquare());
    break;

  case Move::Flag::EnPassantCapture: {
    const uint8_t captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
    summonPiece(lastCapture, captureSquare);
    break;
  }

  default:
    break;
  }
}
} // namespace Chess
