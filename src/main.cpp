#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <citro3d.h>
#include <citro2d.h>

#include "chess.hpp"
#include "colors.hpp"

C2D_SpriteSheet pieces;

int main(int argc, char **argv)
{
  // initalize graphics
  gfxInitDefault();
  hidInit();
  romfsInit();

  if (R_FAILED(C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)))
  {
    gfxExit();
    hidExit();
    return EXIT_FAILURE;
  }
  if (R_FAILED(C2D_Init(C2D_DEFAULT_MAX_OBJECTS)))
  {
    C3D_Fini();
    gfxExit();
    hidExit();
    return EXIT_FAILURE;
  }

  C2D_Prepare();

  pieces = C2D_SpriteSheetLoad("romfs:/pieces.t3x");

  Chess game;

  C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  while (aptMainLoop())
  {
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
    {
      break;
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(bottom, Color::Background); // Clear the screen with black
    C2D_SceneBegin(bottom);

    for (int i{0}; i < 64; i++)
    {
      // draw board
      int row = floor(i / 8);
      int col = i % 8;
      u32 squareColor = (row + col) % 2 == 0 ? Color::WhiteSquare : Color::BlackSquare;

      C2D_DrawRectSolid(col * 30 + 40, row * 30, 0, 30, 30, squareColor);

      // TODO: draw pieces
      Piece piece = game.getPiece(i);
      if (piece != Piece::Empty)
      {
        C2D_Image sprite;

        switch (piece.type())
        {
        case Piece::Pawn:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 0 : 1);
          break;
        case Piece::Rook:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 2 : 3);
          break;
        case Piece::Knight:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 4 : 5);
          break;
        case Piece::Bishop:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 6 : 7);
          break;
        case Piece::Queen:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 8 : 9);
          break;
        case Piece::King:
          sprite = C2D_SpriteSheetGetImage(pieces, piece.color() == Piece::Black ? 10 : 11);
          break;
        }
        C2D_DrawImageAt(sprite, col * 30 + 40, row * 30, 0);
      }
    }

    C3D_FrameEnd(0);
  }

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  hidExit();
  romfsExit();

  return EXIT_SUCCESS;
}
