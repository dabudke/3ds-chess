#include "game.hpp"
#include "colors.hpp"
#include <cmath>
#include <iostream>
#include <citro2d.h>
#include "common.hpp"

void Game::setSelectedSquare(unsigned char square)
{
  // check if square is in bounds
  if (square >= 64)
  {
    selectedSquare = noSelection; // reset selection
  }
  // if no piece is on that square
  if (board.getPiece(square) == Chess::Piece::Empty)
  {
    selectedSquare = noSelection; // reset selection
  }
  if (selectedSquare == square)
  {
    // no work needs to be done
    selectedSquare = noSelection;
  }
  selectedSquare = square;

  // get legal move spaces for selected square
  legalMovesForSelectedSquare.clear();
  // TODO - get legal moves for selected square
}

void Game::handleInput(u32 kDown, u32 kHeld, u32 kUp, touchPosition &touchPos)
{
  // handle touch down/start drag
  if (kDown & KEY_TOUCH)
  {
    u16 tx{touchPos.px}, ty{touchPos.py};

    if (tx >= 40 && tx < 280)
    {
      // handle touch
      unsigned char col = floor((tx - 40) / 30);
      unsigned char row = 7 - floor(ty / 30);
      std::cout << "touch row: " << (unsigned int)row << " touch col: " << (unsigned int)col << std::endl;

      // TODO - handle move with legalMovesForSelectedSquare

      Chess::Piece piece = board.getPiece(row, col);
      // TODO - piece color check
      if (piece != Chess::Piece::Empty)
      {
        setSelectedSquare(row, col);
        dragging = true;
      }
      else
      {
        board.makeMove(getSelectedSquare(), row + col * 8);
      }
    }
  }

  // handle drag
  if (kHeld & KEY_TOUCH && dragging)
  {
    dragPosition.dx = touchPos.px;
    dragPosition.dy = touchPos.py;
  }

  // handle touch up/end drag
  if (kUp & KEY_TOUCH)
  {
    dragging = false;
    std::cout << "drag end" << std::endl;
    // TODO - move piece??
  }
}

void Game::render()
{
  unsigned char selectedSquare = getSelectedSquare();

  int row{0}, col{0};
  u32 squareColor{0};
  Chess::Piece piece{};
  C2D_Image pieceImage;
  for (int i{0}; i < 64; i++)
  {
    // draw board
    row = 7 - floor(i / 8);
    col = i % 8;
    squareColor = (row + col) % 2 == 0 ? Color::WhiteSquare : Color::BlackSquare;

    C2D_DrawRectSolid(col * 30 + 40, row * 30, 0, 30, 30, squareColor);
    if (selectedSquare == i)
    {
      C2D_DrawRectSolid(col * 30 + 40, row * 30, 0, 30, 30, Color::SelectedSquare);
    }

    piece = board.getPiece(i);

    if (piece != Chess::Piece::Empty)
    {
      pieceImage = C2D_SpriteSheetGetImage(pieces, piece.spriteIndex());
      if (dragging && selectedSquare == i)
      {
        // faded piece!
        C2D_DrawImageAt(pieceImage, col * 30 + 40, row * 30, 0, &transparentPieceTint);
      }
      else
      {
        C2D_DrawImageAt(pieceImage, col * 30 + 40, row * 30, 0);
      }
    }
  }

  // draw legal moves

  // draw held piece
  if (dragging)
  {
    Chess::Piece draggingPiece = board.getPiece(selectedSquare);
    if (draggingPiece != Chess::Piece::Empty)
    {
      pieceImage = C2D_SpriteSheetGetImage(pieces, draggingPiece.spriteIndex());
      C2D_DrawImageAt(pieceImage, dragPosition.dx - 15, dragPosition.dy - 15, 1);
    }
  }
}
