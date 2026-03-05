#include <stdexcept>
#include <algorithm>
#include "chess/board.hpp"
#include "chess/piece.hpp"
#include <iostream>
#include <cmath>
#include <sstream>

namespace Chess
{
  const std::string Board::initialFenString = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  Board::Board(std::string fen)
  {
    // extract parts of FEN string
    std::stringstream fenStream(fen);
    std::string boardPart, activeColorPart, castlingPart, enPassantPart, halfmovePart, fullmovePart;
    fenStream >> boardPart >> activeColorPart >> castlingPart;
    // handle optional en passant - next (mandatory) part is halfmove clock
    if (!isdigit(fenStream.peek()))
      fenStream >> enPassantPart;
    fenStream >> halfmovePart >> fullmovePart;

    // parse board
    int row = 7, col = 0;
    for (char c : boardPart)
    {
      if (c == '/')
      {
        row--;
        col = 0;
      }
      else if (isdigit(c))
      {
        int emptySquares = c - '0';
        for (int i = 0; i < emptySquares; i++)
        {
          board[row * 8 + col] = Piece::Empty;
          col++;
        }
      }
      else
      {
        const bool isWhite = isupper(c);
        Piece::Color color = isWhite ? Piece::White : Piece::Black;
        Piece::Type type;
        switch (tolower(c))
        {
        case 'p':
          type = Piece::Pawn;
          break;
        case 'r':
          type = Piece::Rook;
          break;
        case 'n':
          type = Piece::Knight;
          break;
        case 'b':
          type = Piece::Bishop;
          break;
        case 'q':
          type = Piece::Queen;
          break;
        case 'k':
          type = Piece::King;
          break;
        default:
          throw std::invalid_argument("Invalid FEN string: unknown piece character");
        }

        uint8_t squareIndex = square(row, col);
        Piece piece = Piece(color, type);
        summonPiece(piece, squareIndex);

        col++;
      }
    }

    // state setup
    State initialState;

    int halfmove{0};
    // parse active color
    if (activeColorPart == "b")
      halfmove++;

    // castling availability
    if (castlingPart.find("K") == std::string::npos)
      initialState.state &= ~initialState.whiteCastleKingsideMask;
    if (castlingPart.find("Q") == std::string::npos)
      initialState.state &= ~initialState.whiteCastleQueensideMask;
    if (castlingPart.find("k") == std::string::npos)
      initialState.state &= ~initialState.blackCastleKingsideMask;
    if (castlingPart.find("q") == std::string::npos)
      initialState.state &= ~initialState.blackCastleQueensideMask;

    // parse en passant
    if (!enPassantPart.empty() && enPassantPart != "-")
    {
      int file = enPassantPart[0] - 'a';
      if (file < 0 || file > 7)
      {
        throw std::invalid_argument("Invalid FEN string: invalid en passant file");
      }
      initialState.state |= (file << initialState.enPassantFileShift) | initialState.enPassantAvailabilityMask;
    }

    // parse halfmove
    if (!halfmovePart.empty())
    {
      int halfmoveClock = std::stoi(halfmovePart);
      if (halfmoveClock < 0 || halfmoveClock > 50)
      {
        throw std::invalid_argument("Invalid FEN string: invalid halfmove clock");
      }
      initialState.fiftyMoveCounter = halfmoveClock;
    }

    // parse fullmove
    if (!fullmovePart.empty())
    {
      int fullmove = std::stoi(fullmovePart);
      if (fullmove < 1)
      {
        throw std::invalid_argument("Invalid FEN string: invalid fullmove number");
      }
      halfmove += (fullmove - 1) * 2;
    }

    state.pushInitialState(halfmove, initialState);
  }

  Piece Board::getPiece(int index) const
  {
    if (index < 0 || index >= 64)
    {
      throw std::out_of_range("Index out of bounds");
    }
    return board[index];
  }
  Piece Board::getPiece(int row, int col) const
  {
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
    {
      throw std::out_of_range("Row or column out of bounds");
    }
    return board[row * 8 + col];
  }

#pragma region move generation

  const std::vector<Move> Board::getLegalPawnMoves(Piece::Color color, uint8_t square) const
  {
    const State &state = getCurrentState();
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

    // TODO - bitboards for attacks
    // capture left (if it doesn't move across files)
    if (squareOffsetInBounds(forwardSquare, 0, -1))
    {
      uint8_t captureSquare{squareOffset(forwardSquare, 0, -1)};
      uint64_t captureBit{bitmaskForSquare(captureSquare)};
      // regular capture
      if (captureBit & enemyBitboard)
      {
        if (captureBit & promotionMask)
        {
          moves.push_back(Move(square, captureSquare, Move::Flag::RookPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::KnightPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::BishopPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::QueenPromotionCapture));
        }
        else
          moves.push_back(Move(square, captureSquare, Move::Flag::Capture));
      }
      // holy hell
      else if (captureBit & enPassantBit)
      {
        moves.push_back(Move(square, captureSquare, Move::Flag::EnPassantCapture));
      }
    }
    // capture right
    if (squareOffsetInBounds(forwardSquare, 0, 1))
    {
      uint8_t captureSquare{squareOffset(forwardSquare, 0, 1)};
      uint64_t captureBit{bitmaskForSquare(captureSquare)};
      // regular capture
      if (captureBit & enemyBitboard)
      {
        if (captureBit & promotionMask)
        {
          moves.push_back(Move(square, captureSquare, Move::Flag::RookPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::KnightPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::BishopPromotionCapture));
          moves.push_back(Move(square, captureSquare, Move::Flag::QueenPromotionCapture));
        }
        else
          moves.push_back(Move(square, captureSquare, Move::Flag::Capture));
      }
      // en passant capture
      else if (captureBit & enPassantBit)
      {
        moves.push_back(Move(square, captureSquare, Move::Flag::EnPassantCapture));
      }
    }
    return moves;
  }

  const std::vector<Move> Board::getLegalKnightMoves(Piece::Color color, uint8_t square) const
  {
    std::vector<Move> moves;
    Piece capturePiece;
    // knights jump over, in an L shape
    if (rowOffsetInBounds(square, 2))
    {
      if (colOffsetInBounds(square, 1))
      {
        const int newSquare = squareOffset(square, 2, 1);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
      if (colOffsetInBounds(square, -1))
      {
        const int newSquare = squareOffset(square, 2, -1);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
    }

    if (rowOffsetInBounds(square, -2))
    {
      if (colOffsetInBounds(square, 1))
      {
        const int newSquare = squareOffset(square, -2, 1);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
      if (colOffsetInBounds(square, -1))
      {
        const int newSquare = squareOffset(square, -2, -1);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
    }

    if (colOffsetInBounds(square, 2))
    {
      if (rowOffsetInBounds(square, 1))
      {
        const unsigned char newSquare = squareOffset(square, 1, 2);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
      if (rowOffsetInBounds(square, -1))
      {
        const unsigned char newSquare = squareOffset(square, -1, 2);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
    }

    if (colOffsetInBounds(square, -2))
    {
      if (rowOffsetInBounds(square, 1))
      {
        const unsigned char newSquare = squareOffset(square, 1, -2);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
      if (rowOffsetInBounds(square, -1))
      {
        const unsigned char newSquare = squareOffset(square, -1, -2);
        capturePiece = getPiece(newSquare);
        if (capturePiece == Piece::Empty || capturePiece.color() != color)
          moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
      }
    }
    return moves;
  }

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

  const std::vector<Move> Board::getLegalKingMoves(Piece::Color color, uint8_t square) const
  {
    const State &currentState = getCurrentState();
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

#pragma region move perform
  void Board::makeMove(const Move &move)
  {
    Piece piece(getPiece(move.startSquare()));

    // we trust that the move we were given is legal. please.

    if (move.flags() == Move::Flag::PawnDoubleMove)
    {
      movePiece(piece, move.startSquare(), move.endSquare());
      state.pushPawnDoubleMoveState(move);
      return;
    }

    // handle castling (no captures possible)
    if (move.flags() == Move::Flag::CastleKingside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(move.startSquare(), 0, 2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(move.endSquare(), 0, -2);

      movePiece(piece, kingStart, kingEnd);
      movePiece(getPiece(rookStart), rookStart, rookEnd);
      state.pushCastleState(move);
      return;
    }
    if (move.flags() == Move::Flag::CastleQueenside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(move.startSquare(), 0, -2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(move.endSquare(), 0, 3);

      movePiece(piece, kingStart, kingEnd);
      movePiece(getPiece(rookStart), rookStart, rookEnd);
      state.pushCastleState(move);
      return;
    }

    // standard moves beyond this point
    // handle captures
    Piece capturedPiece{Piece::Empty};
    switch (move.flags())
    {
    case Move::Flag::RookPromotionCapture:
    case Move::Flag::KnightPromotionCapture:
    case Move::Flag::BishopPromotionCapture:
    case Move::Flag::QueenPromotionCapture:
    case Move::Flag::Capture:
      capturedPiece = getPiece(move.endSquare());
      removePiece(capturedPiece, move.endSquare());
      break;

    // en passant is special. my special little boy.
    case Move::Flag::EnPassantCapture:
    {
      unsigned char captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
      capturedPiece = getPiece(captureSquare);
      removePiece(capturedPiece, captureSquare);
      break;
    }

    default:
      break;
    }

    // handle promotions
    Piece::Type promotionType{};
    switch (move.flags())
    {
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
    if (move.startSquare() == square(whiteMove() ? 0 : 7, 0) && piece.isType(Piece::Rook))
    {
      castlingChange = StateHistory::CastlingChange::QueensideRookMove;
    }
    else if (move.startSquare() == square(whiteMove() ? 0 : 7, 7) && piece.isType(Piece::Rook))
    {
      castlingChange = StateHistory::CastlingChange::KingsideRookMove;
    }
    else if (piece.isType(Piece::King))
    {
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

#pragma region move reverse
  void Board::unmakeMove()
  {
    const State &undoState = state.popSnapshot();
    const Move &move = undoState.getPreviousMove();

    if (move.flags() == Move::Flag::CastleKingside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(kingStart, 0, 2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(rookStart, 0, -2);

      movePiece(getPiece(kingEnd), kingEnd, kingStart);
      movePiece(getPiece(rookEnd), rookEnd, rookStart);
      return;
    }
    if (move.flags() == Move::Flag::CastleQueenside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(kingStart, 0, -2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(rookStart, 0, 3);

      movePiece(getPiece(kingEnd), kingEnd, kingStart);
      movePiece(getPiece(rookEnd), rookEnd, rookStart);
      return;
    }

    // back to standard moves
    const Piece movedPiece(getPiece(move.endSquare()));

    // handle de-promotions
    switch (move.flags())
    {
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
    switch (move.flags())
    {
    case Move::Flag::RookPromotionCapture:
    case Move::Flag::KnightPromotionCapture:
    case Move::Flag::BishopPromotionCapture:
    case Move::Flag::QueenPromotionCapture:
    case Move::Flag::Capture:
      summonPiece(lastCapture, move.endSquare());
      break;

    case Move::Flag::EnPassantCapture:
    {
      const uint8_t captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
      summonPiece(lastCapture, captureSquare);
      break;
    }

    default:
      break;
    }
  }

  std::string Board::getFenString() const
  {
    static const char *pieceToChar = ".PNBRQKpnbrqk"; // Index 0 is empty, 1-6 are white pieces, 7-12 are black pieces
    std::string fenString;
    int emptyCount = 0;
    Piece piece;

    for (int row{7}; row >= 0; --row)
    {
      for (int col{0}; col < 8; ++col)
      {
        int square = row * 8 + col;
        piece = getPiece(square);

        if (piece == Piece::Empty)
        {
          emptyCount++;
          continue;
        }

        if (emptyCount > 0)
        {
          fenString.append(std::to_string(emptyCount));
          emptyCount = 0;
        }

        switch (piece.type())
        {
        case Piece::Pawn:
          fenString.append(1, piece.color() == Piece::White ? 'P' : 'p');
          break;
        case Piece::Knight:
          fenString.append(1, piece.color() == Piece::White ? 'N' : 'n');
          break;
        case Piece::Bishop:
          fenString.append(1, piece.color() == Piece::White ? 'B' : 'b');
          break;
        case Piece::Rook:
          fenString.append(1, piece.color() == Piece::White ? 'R' : 'r');
          break;
        case Piece::Queen:
          fenString.append(1, piece.color() == Piece::White ? 'Q' : 'q');
          break;
        case Piece::King:
          fenString.append(1, piece.color() == Piece::White ? 'K' : 'k');
          break;
        }
      }
      if (emptyCount > 0)
      {
        fenString.append(std::to_string(emptyCount));
        emptyCount = 0;
      }
      if (row > 0)
        fenString.append("/");
    }

    if (emptyCount > 0)
    {
      fenString.append(std::to_string(emptyCount));
    }

    if (whiteMove())
    {
      fenString.append(" w ");
    }
    else
    {
      fenString.append(" b ");
    }

    const State &currentState = state.getCurrentState();

    bool canCastle{false};
    if (currentState.canWhiteCastleKingside())
    {
      fenString.append("K");
      canCastle = true;
    }
    if (currentState.canWhiteCastleQueenside())
    {
      fenString.append("Q");
      canCastle = true;
    }
    if (currentState.canBlackCastleKingside())
    {
      fenString.append("k");
      canCastle = true;
    }
    if (currentState.canBlackCastleQueenside())
    {
      fenString.append("q");
      canCastle = true;
    }
    if (!canCastle)
    {
      fenString.append("-");
    }

    if (!currentState.enPassantAvailable())
    {
      char enPassantFile = currentState.getEnPassantFile();
      fenString.append(" ");
      fenString.push_back('a' + enPassantFile);
    }

    std::stringstream stream;
    stream << " " << currentState.getFiftyMoveCounter() << " " << state.currentFullmove();
    fenString.append(stream.str());

    return fenString;
  }

#pragma region state change
  void Board::StateHistory::pushState(const Move &move, Piece movedPiece, CastlingChange castlingChange)
  {
    State newState(history[current++], move, movedPiece.isType(Piece::Pawn));

    switch (castlingChange)
    {
    case CastlingChange::KingMove:
      newState.state &= currentTurnIsWhite() ? ~(State::whiteCastleKingsideMask | State::whiteCastleQueensideMask) : ~(State::blackCastleKingsideMask | State::blackCastleQueensideMask);
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

  void Board::StateHistory::pushCaptureState(const Move &move, Piece capturedPiece, CastlingChange castlingChange)
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
      newState.state &= currentTurnIsWhite() ? ~(State::whiteCastleKingsideMask | State::whiteCastleQueensideMask) : ~(State::blackCastleKingsideMask | State::blackCastleQueensideMask);
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

  void Board::StateHistory::pushCastleState(const Move &move)
  {
    State newState(history[current], move);

    // disable castling after this move
    newState.state &= ~(currentTurnIsWhite() ? (State::whiteCastleKingsideMask | State::whiteCastleQueensideMask) : (State::blackCastleKingsideMask | State::blackCastleQueensideMask));

    history[++current] = newState;
  }

  void Board::StateHistory::pushPawnDoubleMoveState(const Move &move)
  {
    State newState(history[current], move, true);

    // mark en passant as available and add the file to state
    newState.state |= State::enPassantAvailabilityMask | (squareCol(move.endSquare()) << State::enPassantFileShift);

    history[++current] = newState;
  }

#pragma region piece state changes
  /// @brief move a piece in the board array and corresponding bitboard and pieceindex
  /// @param piece reference to piece to move
  /// @param target square the piece is on
  /// @param destination square to move the piece to
  inline void Board::movePiece(const Piece &piece, unsigned char square, unsigned char destination)
  {
    pieceIndex.setSquare(piece, square, destination);

    bitboards.getBitboard(piece) ^= bitmaskForSquare(square) | bitmaskForSquare(destination);

    board[destination] = piece;
    board[square] = Piece::Empty;
  }

  /// @brief remove a piece from the board array and corresponding bitboard and pieceindex
  /// @param piece reference to the piece to remove
  /// @param square square the piece is on
  inline void Board::removePiece(const Piece &piece, unsigned char square)
  {
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
  inline void Board::summonPiece(const Piece &piece, unsigned char square)
  {
    pieceIndex.addEntry(piece, square);

    bitboards.getBitboard(piece) |= bitmaskForSquare(square);

    board[square] = piece;
  }

  /// @brief transform the piece from its current type to the new type and move it
  /// @param piece reference to the piece as it is
  /// @param square square the piece is currently on
  /// @param destination square to move the piece to
  /// @param newType type to transform the piece into
  inline void Board::moveAndTransformPiece(const Piece &piece, unsigned char square, unsigned char destination, Piece::Type newType)
  {
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
}
