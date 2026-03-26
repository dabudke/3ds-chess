#include "chess/board.hpp"
#include <bit>
#include "magicBitboards.hpp"

namespace Chess
{

#pragma region pawn moves

  const std::vector<Move> Board::getLegalPawnMoves(Piece::Color color, uint8_t square) const
  {
    const BoardUtils::State &state = getCurrentState();
    std::vector<Move> moves;

    // square indexes
    int forwardDir = color == Piece::White ? 1 : -1;
    uint8_t forwardSquare = squareOffset(square, forwardDir, 0);

    // bitmasks
    uint64_t squareBit = bitmaskForSquare(square);
    uint64_t forwardBit = bitmaskForSquare(forwardSquare);

    uint64_t blockerBitboard = bitboards.getAllPiecesBitboard();
    uint64_t enemyBitboard = color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();
    uint64_t promotionMask = bitmaskForRow(color == Piece::White ? 7 : 0);

    // moving forward, and 2 square first move
    if (forwardBit & ~blockerBitboard)
    {
      // handle promotions if pawn moves to back rank
      if (forwardBit & promotionMask)
      {
        // promotions
        moves.emplace_back(square, forwardSquare, Move::RookPromotion);
        moves.emplace_back(square, forwardSquare, Move::KnightPromotion);
        moves.emplace_back(square, forwardSquare, Move::RookPromotion);
        moves.emplace_back(square, forwardSquare, Move::QueenPromotion);
      }
      else
      {
        moves.emplace_back(square, forwardSquare);

        uint64_t startRankMask = bitmaskForRow(color == Piece::White ? 1 : 6);
        if (squareBit & startRankMask)
        {
          uint64_t doublePush = squareOffset(forwardSquare, forwardDir, 0);
          uint64_t doublePushBit = bitmaskForSquare(doublePush);
          if (doublePushBit & ~blockerBitboard)
            moves.emplace_back(square, doublePush, Move::Flag::PawnDoubleMove);
        }
      }
    }

    // check if en passant is possible and if this piece is in position to perform
    uint64_t enPassantBit{0};
    int enPassantFile = state.getEnPassantFile();
    if (state.enPassantAvailable())
    {
      // get rank, get files, bit-and it all together
      uint64_t enPassantRankMask = bitmaskForRow(color == Piece::White ? 5 : 2);
      uint64_t enPassantFileMask = bitmaskForCol(enPassantFile);
      enPassantBit = enPassantRankMask & enPassantFileMask;
    }

    // get attack bitboard
    uint64_t attackMask = pawnAttacks[color][square];
    uint64_t captureMask = attackMask & enemyBitboard;
    bool captureWillPromote = captureMask & promotionMask;
    while (captureMask)
    {
      uint8_t captureSquare = std::countr_zero(captureMask);
      if (captureWillPromote)
      {
        moves.emplace_back(square, captureSquare, Move::RookPromotionCapture);
        moves.emplace_back(square, captureSquare, Move::KnightPromotionCapture);
        moves.emplace_back(square, captureSquare, Move::BishopPromotionCapture);
        moves.emplace_back(square, captureSquare, Move::QueenPromotionCapture);
      }
      else
        moves.emplace_back(square, captureSquare, Move::Capture);
      captureMask ^= 1ULL << captureSquare;
    }
    if (attackMask & enPassantBit)
    {
      moves.emplace_back(square, std::countr_zero(enPassantBit), Move::EnPassantCapture);
    }

    return moves;
  }

#pragma region knight moves

  const std::vector<Move> Board::getLegalKnightMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    // get moves from precomputed knight attack array
    uint64_t attacks = knightAttacks[square];
    uint64_t blockerBitmask = getPieceBitboard();
    uint64_t captureBitmask = color == Piece::White ? getBlackPieceBitboard() : getWhitePieceBitboard();

    uint64_t movesBitboard = attacks & ~blockerBitmask;
    while (movesBitboard)
    {
      uint8_t moveSquare = std::countr_zero(movesBitboard);
      moves.emplace_back(square, moveSquare);
      movesBitboard ^= 1ull << moveSquare;
    }

    uint64_t capturesBitboard = attacks & captureBitmask;
    while (capturesBitboard)
    {
      uint8_t moveSquare = std::countr_zero(capturesBitboard);
      moves.emplace_back(square, moveSquare, Move::Flag::Capture);
      capturesBitboard ^= 1ull << moveSquare;
    }

    return moves;
  }

#pragma region bishop moves

  const std::vector<Move> Board::getLegalBishopMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    const uint64_t *movesets = MagicBitboards::diagMovesets[square];
    const uint64_t &occupancyMask = MagicBitboards::diagMasks[square];
    const uint64_t &magic = MagicBitboards::diagMagics[square];
    const uint8_t &shift = MagicBitboards::diagShifts[square];

    const uint64_t occupancy = bitboards.getAllPiecesBitboard();
    const uint64_t enemyBitboard = color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

    const uint64_t &moveset = movesets[((occupancy & occupancyMask) * magic) >> shift];

    uint64_t moveMoveset = moveset & ~occupancy;
    while (moveMoveset)
    {
      // extract all move positions
      uint8_t moveSquare = std::countr_zero(moveMoveset);
      moves.emplace_back(square, moveSquare);
      moveMoveset ^= 1ull << moveSquare;
    }

    uint64_t captureMoveset = moveset & enemyBitboard;
    while (captureMoveset)
    {
      uint8_t captureSquare = std::countr_zero(captureMoveset);
      moves.emplace_back(square, captureSquare, Move::Capture);
      captureMoveset ^= 1ull << captureSquare;
    }

    return moves;
  }

#pragma region rook moves

  const std::vector<Move> Board::getLegalRookMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    const uint64_t *movesets = MagicBitboards::orthMovesets[square];
    const uint64_t &occupancyMask = MagicBitboards::orthMasks[square];
    const uint64_t &magic = MagicBitboards::orthMagics[square];
    const uint8_t &shift = MagicBitboards::orthShifts[square];

    const uint64_t occupancyBitboard = bitboards.getAllPiecesBitboard();
    const uint64_t enemyBitboard = color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

    const uint64_t &moveset = movesets[((occupancyBitboard & occupancyMask) * magic) >> shift];

    uint64_t moveMoveset = moveset & ~occupancyBitboard;
    while (moveMoveset)
    {
      uint8_t moveSquare = std::countr_zero(moveMoveset);
      moves.emplace_back(square, moveSquare);
      moveMoveset ^= 1ull << moveSquare;
    }

    uint64_t captureMoveset = moveset & enemyBitboard;
    while (captureMoveset)
    {
      uint8_t captureSquare = std::countr_zero(captureMoveset);
      moves.emplace_back(square, captureSquare, Move::Capture);
      captureMoveset ^= 1ull << captureSquare;
    }

    return moves;
  }

#pragma region queen moves

  const std::vector<Move> Board::getLegalQueenMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    const uint64_t *orthMovesets = MagicBitboards::orthMovesets[square];
    const uint64_t &orthOccupancyMask = MagicBitboards::orthMasks[square];
    const uint64_t &orthMagic = MagicBitboards::orthMagics[square];
    const uint8_t &orthShift = MagicBitboards::orthShifts[square];

    const uint64_t *diagMovesets = MagicBitboards::diagMovesets[square];
    const uint64_t &diagOccupancyMask = MagicBitboards::diagMasks[square];
    const uint64_t &diagMagic = MagicBitboards::diagMagics[square];
    const uint8_t &diagShift = MagicBitboards::diagShifts[square];

    const uint64_t occupancyBitboard = bitboards.getAllPiecesBitboard();
    const uint64_t enemyBitboard = color == Piece::White ? bitboards.getBlackPiecesBitboard() : bitboards.getWhitePiecesBitboard();

    const uint64_t moveset = orthMovesets[((occupancyBitboard & orthOccupancyMask) * orthMagic) >> orthShift] | diagMovesets[((occupancyBitboard & diagOccupancyMask) * diagMagic) >> diagShift];

    uint64_t moveMoveset = moveset & ~occupancyBitboard;
    while (moveMoveset)
    {
      uint8_t moveSquare = std::countr_zero(moveMoveset);
      moves.emplace_back(square, moveSquare);
      moveMoveset ^= 1ull << moveSquare;
    }

    uint64_t captureMoveset = moveset & enemyBitboard;
    while (captureMoveset)
    {
      uint8_t captureSquare = std::countr_zero(captureMoveset);
      moves.emplace_back(square, captureSquare, Move::Capture);
      captureMoveset ^= 1ull << captureSquare;
    }

    return moves;
  }

#pragma region king moves

  const std::vector<Move> Board::getLegalKingMoves(Piece::Color color, uint8_t square) const
  {
    const BoardUtils::State &currentState = getCurrentState();
    std::vector<Move> moves;
    // kings move one square in any direction
    for (int dCol{-1}; dCol <= 1; dCol++)
    {
      for (int dRow{-1}; dRow <= 1; dRow++)
      {
        if (dCol == 0 && dRow == 0)
          continue;
        if (squareOffsetInBounds(square, dRow, dCol))
        {
          unsigned char moveSquare{squareOffset(square, dRow, dCol)};
          Piece piece = getPiece(moveSquare);
          if (piece == Piece::Empty || !getPiece(moveSquare).isColor(piece.color()))
            moves.emplace_back(square, squareOffset(square, dRow, dCol));
        }
      }
    }

    // screw it. castle logic.
    if (whiteMove() ? currentState.canWhiteCastleKingside() : currentState.canBlackCastleKingside())
    {
      if (getPiece(squareOffset(square, 0, 1)) == Piece::Empty && getPiece(squareOffset(square, 0, 2)) == Piece::Empty)
      {
        moves.emplace_back(square, squareOffset(square, 0, 3), Move::Flag::CastleKingside);
      }
    }
    if (whiteMove() ? currentState.canWhiteCastleQueenside() : currentState.canBlackCastleQueenside())
    {
      if (getPiece(squareOffset(square, 0, -1)) == Piece::Empty && getPiece(squareOffset(square, 0, -2)) == Piece::Empty && getPiece(squareOffset(square, 0, -3)) == Piece::Empty)
      {
        moves.emplace_back(square, squareOffset(square, 0, -4), Move::Flag::CastleQueenside);
      }
    }

    return moves;
  }
}
