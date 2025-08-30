#include "game.hpp"
#include "colors.hpp"
#include <cmath>
#include <iostream>
#include <citro2d.h>
#include "common.hpp"
#include "chess/move.hpp"
#include <algorithm>

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
  for (auto move : board.getLegalMovesForSquare(square))
  {
    legalMovesForSelectedSquare.push_back(move);
  }
}

void Game::handleInput(u32 kDown, u32 kHeld, u32 kUp, touchPosition &touchPos)
{
  // handle touch down/start drag
  if (kDown & KEY_TOUCH)
  {
    u16 touchX{touchPos.px}, touchY{touchPos.py};

    if (touchX >= 40 && touchX < 280)
    {
      // handle touch
      unsigned char col = (touchX - 40) / 30;
      unsigned char row = 7 - touchY / 30;

      // TODO - handle move with legalMovesForSelectedSquare
      for (auto move : legalMovesForSelectedSquare)
      {
        if (move.endSquare() == row * 8 + col)
        {
          makeMove(move);
          return;
        }
      }

      Chess::Piece piece = board.getPiece(row, col);
      // TODO - piece color check
      if (piece != Chess::Piece::Empty)
      {
        setSelectedSquare(row, col);
        dragging = true;
      }
    }
  }
  if (kDown & KEY_B)
  {
    setSelectedSquare(noSelection);
    dragging = false;
    board.unmakeMove();
    return;
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

    // TODO - legal piece moves
    if ((dragPosition.dx) >= 40 && (dragPosition.dx < 280))
    {
      unsigned char col = (dragPosition.dx - 40) / 30;
      unsigned char row = 7 - dragPosition.dy / 30;

      if (row * 8 + col == selectedSquare)
      {
        return; // same square, no move
      }

      makeMove(selectedSquare, row * 8 + col);
    }
  }
}

void Game::render()
{
  TickCounter renderTickCounter;
  osTickCounterStart(&renderTickCounter);
  unsigned char selectedSquare = getSelectedSquare();

  int row{0}, col{0};
  u32 squareColor{0};
  Chess::Piece piece{};
  C2D_Image pieceImage;
  Chess::Move lastMove = board.getLastMove();
  bool previousMove = lastMove != Chess::Move::Empty;
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

    if (previousMove && (lastMove.startSquare() == i || lastMove.endSquare() == i))
    {
      C2D_DrawRectSolid(col * 30 + 40, row * 30, 0, 30, 30, Color::PreviousMove);
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
  for (auto move : legalMovesForSelectedSquare)
  {
    row = 7 - floor(move.endSquare() / 8);
    col = move.endSquare() % 8;
    C2D_DrawCircleSolid(col * 30 + 40 + 15, row * 30 + 15, 0, 5, Color::LegalMove);
  }

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

  osTickCounterUpdate(&renderTickCounter);
  // std::cout << "Render time: " << osTickCounterRead(&renderTickCounter) << "ms" << std::endl;
}
