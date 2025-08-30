#include <stdexcept>
#include <algorithm>
#include "chess/board.hpp"
#include "chess/piece.hpp"
#include <iostream>
#include <cmath>

namespace Chess
{
  Board::Board()
  {
    stateHistory[0] = initialState;
    // Initialize the board
    for (int i{0}; i < 64; ++i)
    {
      if (i == 0 || i == 7)
      {
        board[i] = Piece(Piece::White, Piece::Rook); // White Rook
      }
      else if (i == 1 || i == 6)
      {
        board[i] = Piece(Piece::White, Piece::Knight); // White Knight
      }
      else if (i == 2 || i == 5)
      {
        board[i] = Piece(Piece::White, Piece::Bishop); // White Bishop
      }
      else if (i == 3)
      {
        board[i] = Piece(Piece::White, Piece::Queen); // White Queen
      }
      else if (i == 4)
      {
        board[i] = Piece(Piece::White, Piece::King); // White King
      }
      else if (i >= 8 && i < 16)
      {
        board[i] = Piece(Piece::White, Piece::Pawn); // White Pawns
      }
      else if (i >= 48 && i < 56)
      {
        board[i] = Piece(Piece::Black, Piece::Pawn); // Black Pawns
      }
      else if (i == 56 || i == 63)
      {
        board[i] = Piece(Piece::Black, Piece::Rook); // Black Rook
      }
      else if (i == 57 || i == 62)
      {
        board[i] = Piece(Piece::Black, Piece::Knight); // Black Knight
      }
      else if (i == 58 || i == 61)
      {
        board[i] = Piece(Piece::Black, Piece::Bishop); // Black Bishop
      }
      else if (i == 59)
      {
        board[i] = Piece(Piece::Black, Piece::Queen); // Black Queen
      }
      else if (i == 60)
      {
        board[i] = Piece(Piece::Black, Piece::King); // Black King
      }
    }
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

  std::vector<Move> Board::getLegalMovesForSquare(unsigned char square) const
  {
    std::vector<Move> moves;
    Piece piece = getPiece(square);
    Piece capturePiece{Piece::Empty};

    switch (piece.type())
    {
    case Piece::Pawn:
    {
      int forward = piece.color() == Piece::White ? 8 : -8;

      // moving forward, and 2 square first move
      if (getPiece(square + forward) == Piece::Empty)
      {
        moves.push_back(Move(square, square + forward));
      }
      if (square / 8 == (piece.color() == Piece::White ? 1 : 6) && getPiece(square + forward * 2) == Piece::Empty)
      {
        moves.push_back(Move(square, square + forward * 2, Move::Flag::PawnDoubleMove));
      }

      // check if en passant is possible and if this piece is in position to perform
      bool enPassantPossible = moveHistory[moveCount - 1].flags() == Move::Flag::PawnDoubleMove && (square + forward) / 8 == (piece.color() == Piece::White ? 5 : 2);
      int enPassantFile = getEnPassantFile();

      // capture left (if it doesn't move across files)
      if ((square + forward) % 8 != 0)
      {
        // regular capture
        if (getPiece(square + forward - 1) != Piece::Empty)
        {
          moves.push_back(Move(square, square + forward - 1, Move::Flag::Capture));
        }
        // holy heck
        else if (enPassantPossible && enPassantFile == (square % 8 - 1))
        {
          moves.push_back(Move(square, square + forward - 1, Move::Flag::EnPassantCapture));
        }
      }
      // capture right
      if ((square + forward) % 8 != 7)
      {
        // standard piece capture
        if (getPiece(square + forward + 1) != Piece::Empty)
        {
          moves.push_back(Move(square, square + forward + 1, Move::Flag::Capture));
        }
        // en passant capture
        else if (enPassantPossible && enPassantFile == (square % 8) + 1 && floor((square + forward) / 8) == (piece.color() == Piece::White ? 7 : 1))
        {
          moves.push_back(Move(square, square + forward - 1, Move::Flag::EnPassantCapture));
        }
      }
      break;
    }

    case Piece::Knight:
      // knights jump over, in an L shape
      if (square / 8 + 2 < 8)
      {
        if (square % 8 + 1 < 8)
        {
          capturePiece = getPiece(square + 17);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square + 17, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (square % 8 - 1 >= 0)
        {
          capturePiece = getPiece(square + 15);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square + 15, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (square / 8 - 2 >= 0)
      {
        if (square % 8 + 1 < 8)
        {
          capturePiece = getPiece(square - 15);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square - 15, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (square % 8 >= 1)
        {
          capturePiece = getPiece(square - 17);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square - 17, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (square % 8 + 2 < 8)
      {
        if (square / 8 + 1 < 8)
        {
          capturePiece = getPiece(square + 10);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square + 10, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (square / 8 - 1 >= 0)
        {
          capturePiece = getPiece(square - 6);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square - 6, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
      }

      if (square % 8 - 2 >= 0)
      {
        if (square / 8 + 1 < 8)
        {
          capturePiece = getPiece(square + 6);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square + 6, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
        }
        if (square / 8 - 1 >= 0)
        {
          capturePiece = getPiece(square - 10);
          if (capturePiece == Piece::Empty || capturePiece.color() != piece.color())
            moves.push_back(Move(square, square - 10, capturePiece == Piece::Empty ? Move::Flag::NoFlag : Move::Flag::Capture));
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
    }

    return moves;
  }

  bool Board::makeMove(unsigned char fromSquare, unsigned char toSquare)
  {
    if (fromSquare >= 64 || toSquare >= 64)
    {
      throw std::out_of_range("Square index out of bounds");
    }
    if (fromSquare == toSquare)
    {
      return false;
    }

    // make move
    Piece piece = getPiece(fromSquare);
    Piece capturePiece = getPiece(toSquare);
    if (piece == Piece::Empty)
    {
      return false; // No piece to move
    }

    // TODO - capture pieces
    if (capturePiece != Piece::Empty)
    {
      if (capturePiece.color() == piece.color())
      {
        return false;
      }
      moveHistory[moveCount] = Move(fromSquare, toSquare, Move::Flag::Capture);
      ++moveCount;
      stateHistory[moveCount] = stateHistory[moveCount - 1] | (capturePiece.piece << 4);
    }
    else
    {
      moveHistory[moveCount] = Move(fromSquare, toSquare);
      ++moveCount;
      stateHistory[moveCount] = stateHistory[moveCount - 1] & 0xFF0F; // Clear the last captured piece bits
    }
    stateHistory[moveCount] ^= 0x0001; // toggle side to move

    // TODO - legal moves

    board[toSquare] = piece;          // move piece to new square
    board[fromSquare] = Piece::Empty; // clear the original square

    return true;
  }

  void Board::unmakeMove()
  {
    std::cout << moveCount << std::endl;
    if (moveCount < 1)
    {
      return;
    }

    Move lastMove = moveHistory[moveCount - 1];

    // Restore the piece to the original square
    board[lastMove.startSquare()] = getPiece(lastMove.endSquare());

    if (lastMove.flags() == Move::Flag::Capture)
    {
      board[lastMove.endSquare()] = getLastCapturedPiece();
    }
    if (lastMove.flags() == Move::Flag::EnPassantCapture)
    {
      int direction = getPiece(lastMove.endSquare()).color() == Piece::White ? -8 : 8;
      board[lastMove.endSquare() + direction] = getLastCapturedPiece();
      board[lastMove.endSquare()] = Piece::Empty;
    }
    else
    {
      board[lastMove.endSquare()] = Piece::Empty;
    }

    --moveCount;
  }
}
