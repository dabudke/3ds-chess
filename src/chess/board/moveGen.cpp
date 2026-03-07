#include "chess/board.hpp"
#include <bit>

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
        moves.push_back(Move(square, forwardSquare, Move::Flag::RookPromotion));
        moves.push_back(Move(square, forwardSquare, Move::Flag::KnightPromotion));
        moves.push_back(Move(square, forwardSquare, Move::Flag::RookPromotion));
        moves.push_back(Move(square, forwardSquare, Move::Flag::QueenPromotion));
      }
      else
      {
        moves.push_back(Move(square, forwardSquare));

        uint64_t startRankMask = bitmaskForRow(color == Piece::White ? 1 : 6);
        if (squareBit & startRankMask)
        {
          uint64_t doublePush = squareOffset(forwardSquare, forwardDir, 0);
          uint64_t doublePushBit = bitmaskForSquare(doublePush);
          if (doublePushBit & ~blockerBitboard)
            moves.push_back(Move(square, doublePush, Move::Flag::PawnDoubleMove));
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
        moves.push_back(Move(square, captureSquare, Move::Flag::RookPromotionCapture));
        moves.push_back(Move(square, captureSquare, Move::Flag::KnightPromotionCapture));
        moves.push_back(Move(square, captureSquare, Move::Flag::BishopPromotionCapture));
        moves.push_back(Move(square, captureSquare, Move::Flag::QueenPromotionCapture));
      }
      else
        moves.push_back(Move(square, captureSquare, Move::Flag::Capture));
      captureMask ^= 1ULL << captureSquare;
    }
    if (attackMask & enPassantBit)
    {
      moves.push_back(Move(square, std::countr_zero(enPassantBit), Move::Flag::EnPassantCapture));
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
      moves.push_back(Move(square, moveSquare));
      movesBitboard ^= 1ull << moveSquare;
    }

    uint64_t capturesBitboard = attacks & captureBitmask;
    while (capturesBitboard)
    {
      uint8_t moveSquare = std::countr_zero(capturesBitboard);
      moves.push_back(Move(square, moveSquare, Move::Flag::Capture));
      capturesBitboard ^= 1ull << moveSquare;
    }

    return moves;
  }

#pragma region bishop moves

  const std::vector<Move> Board::getLegalBishopMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    Piece capturePiece;
    // up-right
    for (int i{1}; i < 8 && square % 8 + i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 9 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 9 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 9 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // up-left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 7 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 7 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 7 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down-right
    for (int i{1}; i < 8 && square % 8 + i < 8 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 7 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 7 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 7 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down-left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 9 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 9 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 9 * i, Move::Flag::Capture));
      }
    }

    return moves;
  }

#pragma region rook moves

  const std::vector<Move> Board::getLegalRookMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    Piece capturePiece;
    // up
    for (int i{1}; i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 8 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 8 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 8 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // right
    for (int i{1}; i < 8 && square % 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down
    for (int i{1}; i < 8 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 8 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 8 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 8 * i, Move::Flag::Capture));
      }
    }

    return moves;
  }

#pragma region queen moves

  const std::vector<Move> Board::getLegalQueenMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;

    Piece capturePiece;
    // up-right
    for (int i{1}; i < 8 && square % 8 + i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 9 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 9 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 9 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // up-left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 7 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 7 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 7 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down-right
    for (int i{1}; i < 8 && square % 8 + i < 8 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 7 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 7 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 7 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down-left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 9 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 9 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 9 * i, Move::Flag::Capture));
      }
    }

    // up
    for (int i{1}; i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + 8 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + 8 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + 8 * i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // right
    for (int i{1}; i < 8 && square % 8 + i < 8 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square + i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square + i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square + i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // left
    for (int i{1}; i < 8 && square % 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - i, Move::Flag::Capture));
      }
    }

    capturePiece = Piece::Empty;
    // down
    for (int i{1}; i < 8 && square / 8 - i >= 0 && capturePiece == Piece::Empty; ++i)
    {
      capturePiece = getPiece(square - 8 * i);
      if (capturePiece == Piece::Empty)
      {
        moves.push_back(Move(square, square - 8 * i));
      }
      else if (capturePiece.color() != color)
      {
        moves.push_back(Move(square, square - 8 * i, Move::Flag::Capture));
      }
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
            moves.push_back(Move(square, squareOffset(square, dRow, dCol)));
        }
      }
    }

    // screw it. castle logic.
    if (whiteMove() ? currentState.canWhiteCastleKingside() : currentState.canBlackCastleKingside())
    {
      if (getPiece(squareOffset(square, 0, 1)) == Piece::Empty && getPiece(squareOffset(square, 0, 2)) == Piece::Empty)
      {
        moves.push_back(Move(square, squareOffset(square, 0, 3), Move::Flag::CastleKingside));
      }
    }
    if (whiteMove() ? currentState.canWhiteCastleQueenside() : currentState.canBlackCastleQueenside())
    {
      if (getPiece(squareOffset(square, 0, -1)) == Piece::Empty && getPiece(squareOffset(square, 0, -2)) == Piece::Empty && getPiece(squareOffset(square, 0, -3)) == Piece::Empty)
      {
        moves.push_back(Move(square, squareOffset(square, 0, -4), Move::Flag::CastleQueenside));
      }
    }

    return moves;
  }
}
