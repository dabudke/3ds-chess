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
        Piece::Color color = isupper(c) ? Piece::White : Piece::Black;
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
        board[row * 8 + col] = Piece(color, type);
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

  const std::vector<Move> Board::getLegalMovesForSquare(unsigned char square) const
  {
    std::vector<Move> moves;
    Piece piece = getPiece(square);
    Piece capturePiece{Piece::Empty};

    if (piece == Piece::Empty)
    {
      return moves; // No piece to move
    }

    if (piece.color() != (whiteMove() ? Piece::White : Piece::Black))
    {
      return moves; // Not this side's turn
    }

    int row = square / 8;
    int col = square % 8;

    const State &state = getCurrentState();

    switch (piece.type())
    {
    case Piece::Pawn:
    {
      // TODO: refactor code to use helper functions. this is borderline unreadable
      int forwardDir = piece.isWhite() ? 1 : -1;
      unsigned char forwardSquare = squareOffset(square, forwardDir, 0);

      // moving forward, and 2 square first move
      if (getPiece(squareOffset(square, forwardDir, 0)) == Piece::Empty)
      {
        // handle promotions if pawn moves to back rank
        if (squareRow(forwardSquare) == (piece.isWhite() ? 7 : 0))
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
          if (squareRow(square) == (piece.isWhite() ? 1 : 6) && getPiece(squareOffset(forwardSquare, forwardDir, 0)) == Piece::Empty)
          {
            moves.push_back(Move(square, squareOffset(forwardSquare, forwardDir, 0), Move::Flag::PawnDoubleMove));
          }
        }
      }

      // check if en passant is possible and if this piece is in position to perform
      bool enPassantPossible{false};
      int enPassantFile = state.getEnPassantFile();
      if (state.enPassantAvailable())
        enPassantPossible = squareRow(forwardSquare) == (piece.isWhite() ? 5 : 2);

      // capture left (if it doesn't move across files)
      if (squareOffsetInBounds(forwardSquare, 0, -1))
      {
        unsigned char captureSquare{squareOffset(forwardSquare, 0, -1)};
        capturePiece = getPiece(captureSquare);
        // regular capture
        if (capturePiece != Piece::Empty && capturePiece.color() != piece.color())
        {
          if (squareRow(captureSquare) == (piece.isWhite() ? 7 : 0))
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
        else if (enPassantPossible && enPassantFile == squareCol(captureSquare))
        {
          moves.push_back(Move(square, captureSquare, Move::Flag::EnPassantCapture));
        }
      }
      // capture right
      if (squareOffsetInBounds(forwardSquare, 0, 1))
      {
        unsigned char captureSquare{squareOffset(forwardSquare, 0, 1)};
        capturePiece = getPiece(captureSquare);
        // standard piece capture
        if (capturePiece != Piece::Empty && capturePiece.color() != piece.color())
        {

          if (squareRow(captureSquare) == (piece.isWhite() ? 7 : 0))
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
        else if (enPassantPossible && enPassantFile == squareCol(captureSquare))
        {
          moves.push_back(Move(square, captureSquare, Move::Flag::EnPassantCapture));
        }
      }
      break;
    }

    case Piece::Knight:
      // knights jump over, in an L shape
      if (rowOffsetInBounds(square, 2))
      {
        if (colOffsetInBounds(square, 1))
        {
          const int newSquare = squareOffset(square, 2, 1);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (colOffsetInBounds(square, -1))
        {
          const int newSquare = squareOffset(square, 2, -1);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (rowOffsetInBounds(square, -2))
      {
        if (colOffsetInBounds(square, 1))
        {
          const int newSquare = squareOffset(square, -2, 1);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (colOffsetInBounds(square, -1))
        {
          const int newSquare = squareOffset(square, -2, -1);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (colOffsetInBounds(square, 2))
      {
        if (rowOffsetInBounds(square, 1))
        {
          const unsigned char newSquare = squareOffset(square, 1, 2);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (rowOffsetInBounds(square, -1))
        {
          const unsigned char newSquare = squareOffset(square, -1, 2);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (colOffsetInBounds(square, -2))
      {
        if (rowOffsetInBounds(square, 1))
        {
          const unsigned char newSquare = squareOffset(square, 1, -2);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (rowOffsetInBounds(square, -1))
        {
          const unsigned char newSquare = squareOffset(square, -1, -2);
          capturePiece = getPiece(newSquare);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, newSquare, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }
      break;

    case Piece::Queen:
    case Piece::Bishop:
      // up-right
      for (int i{1}; i < 8 && square % 8 + i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
      {
        capturePiece = getPiece(square + 9 * i);
        if (capturePiece == Piece::Empty)
        {
          moves.push_back(Move(square, square + 9 * i));
        }
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
        {
          moves.push_back(Move(square, square - 9 * i, Move::Flag::Capture));
        }
      }

      // conditionally break for bishops
      if (piece.type() == Piece::Bishop)
      {
        break;
      }

    case Piece::Rook:
      capturePiece = Piece::Empty;
      // up
      for (int i{1}; i < 8 && square / 8 + i < 8 && capturePiece == Piece::Empty; ++i)
      {
        capturePiece = getPiece(square + 8 * i);
        if (capturePiece == Piece::Empty)
        {
          moves.push_back(Move(square, square + 8 * i));
        }
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
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
        else if (capturePiece.color() != piece.color())
        {
          moves.push_back(Move(square, square - 8 * i, Move::Flag::Capture));
        }
      }
      break;

    case Piece::King:
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
      if (whiteMove() ? state.canWhiteCastleKingside() : state.canBlackCastleKingside())
      {
        if (getPiece(squareOffset(square, 0, 1)) == Piece::Empty && getPiece(squareOffset(square, 0, 2)) == Piece::Empty)
        {
          moves.push_back(Move(square, squareOffset(square, 0, 3), Move::Flag::CastleKingside));
        }
      }
      if (whiteMove() ? state.canWhiteCastleQueenside() : state.canBlackCastleQueenside())
      {
        if (getPiece(squareOffset(square, 0, -1)) == Piece::Empty && getPiece(squareOffset(square, 0, -2)) == Piece::Empty && getPiece(squareOffset(square, 0, -3)) == Piece::Empty)
        {
          moves.push_back(Move(square, squareOffset(square, 0, -4), Move::Flag::CastleQueenside));
        }
      }
    }

    return moves;
  }

  void Board::makeMove(const Move &move)
  {
    Piece piece(getPiece(move.startSquare()));

    // we trust that the move we were given is legal. please.

    if (move.flags() == Move::Flag::PawnDoubleMove)
    {
      board[move.startSquare()] = Piece::Empty;
      board[move.endSquare()] = piece;
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

      board[rookEnd] = board[rookStart];
      board[rookStart] = Piece::Empty;
      board[kingEnd] = board[kingStart];
      board[kingStart] = Piece::Empty;
      state.pushCastleState(move);
      return;
    }
    if (move.flags() == Move::Flag::CastleQueenside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(move.startSquare(), 0, -2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(move.endSquare(), 0, 3);

      board[rookEnd] = board[rookStart];
      board[rookStart] = Piece::Empty;
      board[kingEnd] = board[kingStart];
      board[kingStart] = Piece::Empty;
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
      break;

    // en passant is special. my special little boy.
    case Move::Flag::EnPassantCapture:
    {
      unsigned char captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
      capturedPiece = getPiece(captureSquare);
      board[captureSquare] = Piece::Empty;
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
    board[move.endSquare()] = promotionType != 0 ? Piece(piece, promotionType) : piece;
    board[move.startSquare()] = Piece::Empty;
    return;
  }

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

      board[rookStart] = board[rookEnd];
      board[rookEnd] = Piece::Empty;
      board[kingStart] = board[kingEnd];
      board[kingEnd] = Piece::Empty;
      return;
    }
    if (move.flags() == Move::Flag::CastleQueenside)
    {
      unsigned char kingStart = move.startSquare();
      unsigned char kingEnd = squareOffset(kingStart, 0, -2);
      unsigned char rookStart = move.endSquare();
      unsigned char rookEnd = squareOffset(rookStart, 0, 3);

      board[rookStart] = board[rookEnd];
      board[kingStart] = board[kingEnd];
      board[rookEnd] = Piece::Empty;
      board[kingEnd] = Piece::Empty;
      return;
    }

    // back to standard moves
    const Piece movedPiece(getPiece(move.endSquare()));

    // restore capture
    const Piece &lastCapture = undoState.getLastCapture();
    switch (move.flags())
    {
    case Move::Flag::RookPromotionCapture:
    case Move::Flag::KnightPromotionCapture:
    case Move::Flag::BishopPromotionCapture:
    case Move::Flag::QueenPromotionCapture:
    case Move::Flag::Capture:
      board[move.endSquare()] = lastCapture;
      break;

    case Move::Flag::EnPassantCapture:
    {
      unsigned char captureSquare = squareOffset(move.endSquare(), whiteMove() ? -1 : 1, 0);
      board[captureSquare] = lastCapture;
    }
    default:
      board[move.endSquare()] = Piece::Empty;
      break;
    }

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
      board[move.startSquare()] = movedPiece;
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
}
#include "board.hpp"

namespace Chess
{
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
}
