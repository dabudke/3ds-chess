#pragma once

#include <vector>
#include "chess/board.hpp"
#include <3ds.h>

// game state
// - chess board
// - picked up piece

// State used for the logic between the chess board and the render loop
// NO GAME LOGIC WHATSOEVER
class Game
{
protected:
  // render state variables
  // piece picked up
  // timestamp piece picked up (for tapping vs. holding)
  // selected piece

  unsigned char selectedSquare{noSelection};
  std::vector<unsigned char> legalMovesForSelectedSquare;
  bool dragging{false};
  struct DragPosition
  {
    u16 dx;
    u16 dy;
  } dragPosition{0, 0};

public:
  Chess::Board board;

  Game() : board() {}
  // TODO - import game state from PGN or FEN

  bool makeMove(unsigned char fromSquare, unsigned char toSquare)
  {
    return board.makeMove(fromSquare, toSquare);
  }

  static const unsigned short noSelection = 65;
  unsigned char getSelectedSquare() const
  {
    return selectedSquare;
  }
  void setSelectedSquare(unsigned char square);
  void setSelectedSquare(int row, int col)
  {
    setSelectedSquare(col + row * 8);
  }

  const std::vector<unsigned char> &getLegalMovesForSelectedSquare() const
  {
    return legalMovesForSelectedSquare;
  }

  void handleInput(u32 kDown, u32 kHeld, u32 kUp, touchPosition &touchPos);
  void render();
};
