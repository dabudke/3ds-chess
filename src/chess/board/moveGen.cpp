#include <bit>
#include <cstdint>
#include <forward_list>
#include <stdexcept>

#include "chess/board.hpp"
#include "chess/board/state.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"
#include "magicBitboards.hpp"

namespace Chess {

#pragma region pawn moves

const std::forward_list<Move> Board::getLegalPawnMoves(Piece::Color color, uint8_t square) const {
#ifdef DEBUG
  if (getPiece(square) != Piece(color, Piece::Pawn))
    throw std::runtime_error("creating move for invalid piece");
#endif
  const BoardUtils::State &state = getCurrentState();
  // keep iterator to last move in list, build from bottom up - increment after move added
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  // square indexes
  const int forwardDir = color == Piece::White ? 1 : -1;
  const uint8_t forwardSquare = squareOffset(square, forwardDir, 0);

  // bitmasks
  const uint64_t squareBit = bitmaskForSquare(square);
  const uint64_t forwardBit = bitmaskForSquare(forwardSquare);

  const uint64_t blockerBitboard = bitboards.getAllPiecesBitboard();
  const uint64_t enemyBitboard =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();
  static const uint64_t promotionMask[2] = {bitmaskForRow(7), bitmaskForRow(0)};

  // moving forward, and 2 square first move
  if (forwardBit & ~blockerBitboard) {
    // handle promotions if pawn moves to back rank
    if (forwardBit & promotionMask[color]) {
      // promotions
      moves.emplace_after(lastMove, square, forwardSquare, Move::RookPromotion);
      moves.emplace_after(++lastMove, square, forwardSquare, Move::KnightPromotion);
      moves.emplace_after(++lastMove, square, forwardSquare, Move::RookPromotion);
      moves.emplace_after(++lastMove, square, forwardSquare, Move::QueenPromotion);
      lastMove++;
    } else {
      moves.emplace_after(lastMove, square, forwardSquare);
      lastMove++;

      static const uint64_t startRankMask[2] = {bitmaskForRow(1), bitmaskForRow(6)};
      if (squareBit & startRankMask[color]) {
        uint64_t doublePush = squareOffset(forwardSquare, forwardDir, 0);
        uint64_t doublePushBit = bitmaskForSquare(doublePush);
        if (doublePushBit & ~blockerBitboard) {
          moves.emplace_after(lastMove, square, doublePush, Move::Flag::PawnDoubleMove);
          lastMove++;
        }
      }
    }
  }

  // check if en passant is possible and if this piece is in position to perform
  uint64_t enPassantBit{0};
  if (state.enPassantAvailable()) {
    // get rank, get files, bit-and it all together
    uint8_t enPassantFile = state.getEnPassantFile();
    static uint64_t enPassantRankMask[2] = {bitmaskForRow(6), bitmaskForRow(1)};
    uint64_t enPassantFileMask = bitmaskForCol(enPassantFile);
    enPassantBit = enPassantRankMask[color] & enPassantFileMask;
  }

  // get attack bitboard
  const uint64_t &attackMask = pawnAttacks[color][square];
  uint64_t captureMask = attackMask & enemyBitboard;
  const bool captureWillPromote = captureMask & promotionMask[color];
  while (captureMask) {
    uint8_t captureSquare = std::countr_zero(captureMask);
    if (captureWillPromote) {
      moves.emplace_after(lastMove, square, captureSquare, Move::RookPromotionCapture);
      moves.emplace_after(++lastMove, square, captureSquare, Move::KnightPromotionCapture);
      moves.emplace_after(++lastMove, square, captureSquare, Move::BishopPromotionCapture);
      moves.emplace_after(++lastMove, square, captureSquare, Move::QueenPromotionCapture);
    } else {
      moves.emplace_after(lastMove, square, captureSquare, Move::Capture);
    }
    lastMove++;
    captureMask &= captureMask - 1;
  }
  if (attackMask & enPassantBit) {
    moves.emplace_after(lastMove, square, std::countr_zero(enPassantBit), Move::EnPassantCapture);
  }

  return moves;
}

#pragma region knight moves

const std::forward_list<Move> Board::getLegalKnightMoves(Piece::Color color, uint8_t square) const {
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  // get moves from precomputed knight attack array
  uint64_t attacks = knightAttacks[square];
  uint64_t blockerBitmask = bitboards.getAllPiecesBitboard();
  uint64_t captureBitmask =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

  uint64_t movesBitboard = attacks & ~blockerBitmask;
  while (movesBitboard) {
    uint8_t moveSquare = std::countr_zero(movesBitboard);
    moves.emplace_after(lastMove, square, moveSquare);
    lastMove++;
    movesBitboard &= movesBitboard - 1;
  }

  uint64_t capturesBitboard = attacks & captureBitmask;
  while (capturesBitboard) {
    uint8_t moveSquare = std::countr_zero(capturesBitboard);
    moves.emplace_after(lastMove, square, moveSquare, Move::Flag::Capture);
    lastMove++;
    capturesBitboard &= capturesBitboard - 1;
  }

  return moves;
}

#pragma region bishop moves

const std::forward_list<Move> Board::getLegalBishopMoves(Piece::Color color, uint8_t square) const {
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  const uint64_t *movesets = MagicBitboards::diagMovesets[square];
  const uint64_t &occupancyMask = MagicBitboards::diagMasks[square];
  const uint64_t &magic = MagicBitboards::diagMagics[square];
  const uint8_t &shift = MagicBitboards::diagShifts[square];

  const uint64_t occupancy = bitboards.getAllPiecesBitboard();
  const uint64_t enemyBitboard =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

  const uint64_t &moveset = movesets[((occupancy & occupancyMask) * magic) >> shift];

  uint64_t moveMoveset = moveset & ~occupancy;
  while (moveMoveset) {
    // extract all move positions
    uint8_t moveSquare = std::countr_zero(moveMoveset);
    moves.emplace_after(lastMove, square, moveSquare);
    lastMove++;
    moveMoveset &= moveMoveset - 1;
  }

  uint64_t captureMoveset = moveset & enemyBitboard;
  while (captureMoveset) {
    uint8_t captureSquare = std::countr_zero(captureMoveset);
    moves.emplace_after(lastMove, square, captureSquare, Move::Capture);
    lastMove++;
    captureMoveset &= captureMoveset - 1;
  }

  return moves;
}

#pragma region rook moves

const std::forward_list<Move> Board::getLegalRookMoves(Piece::Color color, uint8_t square) const {
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  const uint64_t *movesets = MagicBitboards::orthMovesets[square];
  const uint64_t &occupancyMask = MagicBitboards::orthMasks[square];
  const uint64_t &magic = MagicBitboards::orthMagics[square];
  const uint8_t &shift = MagicBitboards::orthShifts[square];

  const uint64_t occupancyBitboard = bitboards.getAllPiecesBitboard();
  const uint64_t enemyBitboard =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

  const uint64_t &moveset = movesets[((occupancyBitboard & occupancyMask) * magic) >> shift];

  uint64_t moveMoveset = moveset & ~occupancyBitboard;
  while (moveMoveset) {
    uint8_t moveSquare = std::countr_zero(moveMoveset);
    moves.emplace_after(lastMove, square, moveSquare);
    lastMove++;
    moveMoveset &= moveMoveset - 1;
  }

  uint64_t captureMoveset = moveset & enemyBitboard;
  while (captureMoveset) {
    uint8_t captureSquare = std::countr_zero(captureMoveset);
    moves.emplace_after(lastMove, square, captureSquare, Move::Capture);
    lastMove++;
    captureMoveset &= captureMoveset - 1;
  }

  return moves;
}

#pragma region queen moves

const std::forward_list<Move> Board::getLegalQueenMoves(Piece::Color color, uint8_t square) const {
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  const uint64_t *orthMovesets = MagicBitboards::orthMovesets[square];
  const uint64_t &orthOccupancyMask = MagicBitboards::orthMasks[square];
  const uint64_t &orthMagic = MagicBitboards::orthMagics[square];
  const uint8_t &orthShift = MagicBitboards::orthShifts[square];

  const uint64_t *diagMovesets = MagicBitboards::diagMovesets[square];
  const uint64_t &diagOccupancyMask = MagicBitboards::diagMasks[square];
  const uint64_t &diagMagic = MagicBitboards::diagMagics[square];
  const uint8_t &diagShift = MagicBitboards::diagShifts[square];

  const uint64_t occupancyBitboard = bitboards.getAllPiecesBitboard();
  const uint64_t enemyBitboard =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

  const uint64_t moveset = orthMovesets[((occupancyBitboard & orthOccupancyMask) * orthMagic) >> orthShift] |
                           diagMovesets[((occupancyBitboard & diagOccupancyMask) * diagMagic) >> diagShift];

  uint64_t moveMoveset = moveset & ~occupancyBitboard;
  while (moveMoveset) {
    uint8_t moveSquare = std::countr_zero(moveMoveset);
    moves.emplace_after(lastMove, square, moveSquare);
    lastMove++;
    moveMoveset ^= 1ull << moveSquare;
  }

  uint64_t captureMoveset = moveset & enemyBitboard;
  while (captureMoveset) {
    uint8_t captureSquare = std::countr_zero(captureMoveset);
    moves.emplace_after(lastMove, square, captureSquare, Move::Capture);
    lastMove++;
    captureMoveset ^= 1ull << captureSquare;
  }

  return moves;
}

#pragma region king moves

const std::forward_list<Move> Board::getLegalKingMoves(Piece::Color color, uint8_t square) const {
  const BoardUtils::State &currentState = getCurrentState();
  std::forward_list<Move> moves;
  std::forward_list<Move>::iterator lastMove{moves.before_begin()};

  const uint64_t blockers = bitboards.getAllPiecesBitboard();
  const uint64_t captures =
      color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();
  const uint64_t &possibleMoves = kingAttacks[square];

  uint64_t movesBitboard = possibleMoves & ~blockers;
  while (movesBitboard) {
    uint8_t moveSquare = std::countr_zero(movesBitboard);
    moves.emplace_after(lastMove, square, moveSquare);
    lastMove++;
    movesBitboard &= movesBitboard - 1;
  }

  uint64_t capturesBitboard = possibleMoves & captures;
  while (capturesBitboard) {
    uint8_t captureSquare = std::countr_zero(capturesBitboard);
    moves.emplace_after(lastMove, square, captureSquare, Move::Capture);
    lastMove++;
    capturesBitboard &= capturesBitboard - 1;
  }

  // much more *elegant* castle logic
  // 0x60 = 0b01100000 -> . . . . . X X .
  static const uint64_t kingsideCastleBlockers[2] = {0x60ull, 0x60ull << 56};
  if (color == Piece::White ? currentState.canWhiteCastleKingside() : currentState.canBlackCastleKingside()) {
    if ((blockers & kingsideCastleBlockers[color]) == 0) {
      moves.emplace_after(lastMove, square, squareOffset(square, 0, 2), Move::Flag::CastleKingside);
      lastMove++;
    }
  }
  // 0xE = 0b00001110 -> . X X X . . . .
  static const uint64_t queensideCastleBlockers[2] = {0xEull, 0xEull << 56};
  if (color == Piece::White ? currentState.canWhiteCastleQueenside() : currentState.canBlackCastleQueenside()) {
    if ((blockers & queensideCastleBlockers[color]) == 0) {
      moves.emplace_after(lastMove, square, squareOffset(square, 0, -2), Move::Flag::CastleQueenside);
    }
  }

  return moves;
}
} // namespace Chess
