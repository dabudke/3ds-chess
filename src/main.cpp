#include <cstdlib>

#include <3ds.h>
#include <3ds/console.h>
#include <3ds/gfx.h>
#include <3ds/result.h>
#include <3ds/romfs.h>
#include <3ds/services/apt.h>
#include <3ds/services/hid.h>
#include <3ds/types.h>

#include <c3d/base.h>
#include <c3d/renderqueue.h>
#include <citro3d.h>

#include <c2d/base.h>
#include <c2d/spritesheet.h>
#include <citro2d.h>

#include "colors.hpp"
#include "common.hpp"
#include "game.hpp"

C2D_SpriteSheet pieces;
C2D_ImageTint transparentPieceTint{};

int main(int argc, char **argv) {
  // initalize graphics
  gfxInitDefault();
  hidInit();
  romfsInit();

  consoleInit(GFX_TOP, nullptr);

  if (R_FAILED(C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))) {
    gfxExit();
    hidExit();
    return EXIT_FAILURE;
  }
  if (R_FAILED(C2D_Init(C2D_DEFAULT_MAX_OBJECTS))) {
    C3D_Fini();
    gfxExit();
    hidExit();
    return EXIT_FAILURE;
  }

  C2D_Prepare();

  pieces = C2D_SpriteSheetLoad("romfs:/pieces.t3x");
  C2D_AlphaImageTint(&transparentPieceTint, 0.6f);

  Game game;

  C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  while (aptMainLoop()) {
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START) {
      break;
    }
    u32 kHeld = hidKeysHeld();
    u32 kUp = hidKeysUp();

    touchPosition touchPos;
    hidTouchRead(&touchPos);

    game.handleInput(kDown, kHeld, kUp, touchPos);

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(bottom, Color::Background); // Clear the screen with black
    C2D_SceneBegin(bottom);

    game.render();

    C3D_FrameEnd(0);
  }

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  hidExit();
  romfsExit();

  return EXIT_SUCCESS;
}
