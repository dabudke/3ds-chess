#include <iostream>
#include "chess/board.hpp"
#include "debugger/game.hpp"
#include "debugger/debug.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

int main(int argc, char **argv)
{
  SDL_SetAppMetadata("3DS Chess Engine Debugger", "1.0", "net.dabudke.3dschess.engineDebugger");

  SDL_Window *window;
  SDL_Renderer *renderer;

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return -1;
  }

  if (!SDL_CreateWindowAndRenderer("Engine Debugger", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer))
  {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    SDL_Quit();
    return -1;
  }
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(1.0f);
  style.FontScaleDpi = 1.0f;
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  SDL_SetRenderVSync(renderer, true);
  SDL_SetDefaultTextureScaleMode(renderer, SDL_ScaleMode::SDL_SCALEMODE_PIXELART);

  Debugger::Game game;

  if (argc > 1)
  {
    std::string fen = argv[1];
    std::cout << "Importing FEN: " << fen << std::endl;
    game = Debugger::Game(fen);
  }

  game.loadTextures(renderer);

  Debugger::DebugEngine debugger = Debugger::DebugEngine(&game);

  bool running = true;
  SDL_Event event;

  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);

      switch (event.type)
      {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        game.resizeWindow(event.window.data1, event.window.data2);
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!io.WantCaptureMouse)
          game.handleMouseDown((SDL_MouseButtonEvent *)&event);
        break;
      case SDL_EVENT_KEY_DOWN:
        if (!io.WantCaptureKeyboard)
          game.handleKeyboard((SDL_KeyboardEvent *)&event);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (!io.WantCaptureKeyboard)
          game.handleDrag((SDL_MouseMotionEvent *)&event);
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (!io.WantCaptureKeyboard)
          game.handleMouseUp((SDL_MouseButtonEvent *)&event);
        break;
      }
    }

    const double now = ((double)SDL_GetTicks()) / 1000.0; /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    SDL_SetRenderDrawColor(renderer, 24, 25, 35, SDL_ALPHA_OPAQUE); /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);

    game.render(renderer);

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    debugger.draw();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  // cleanup

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
