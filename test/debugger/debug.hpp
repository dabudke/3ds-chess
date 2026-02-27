#pragma once
#include "game.hpp"

namespace Debugger
{
  class DebugEngine
  {
    Game *game;

  public:
    DebugEngine(Game *game) : game(game) {}
    void draw();
  };
}
