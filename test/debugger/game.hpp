#pragma once
#include <vector>
#include <map>
#include <iostream>
#include <SDL3/SDL.h>
#include "chess/board.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"

namespace Debugger
{
  class Game
  {
  private:
    // render state variables
    // piece picked up
    // timestamp piece picked up (for tapping vs. holding)
    // selected piece

    unsigned char selectedSquare{noSelection};

    std::vector<Chess::Move> legalMovesForSelectedSquare{};
    bool dragging{false};
    struct DragPosition
    {
      int dx;
      int dy;
    } dragPosition{0, 0};

    void generateLegalMovesForSquare()
    {
      legalMovesForSelectedSquare.clear();
      if (selectedSquare != noSelection)
      {
        auto moves = board.getLegalMovesForSquare(selectedSquare);
        legalMovesForSelectedSquare.insert(legalMovesForSelectedSquare.begin(), moves.begin(), moves.end());
      }
    }

  public:
    struct PerftMoveList
    {
      int index;
      std::vector<Chess::Move> moves;
    };
    struct PerftData
    {
      bool running{false};
      int depth;
      std::vector<PerftMoveList> moveList;
    };
    unsigned long long perft(int depth);
    std::map<Chess::Move, unsigned long long> perftDivide(int depth);
    Game::PerftData startPerft(int depth);
    bool stepPerft(PerftData &data);
    void stopPerft(PerftData &data);

    enum RenderDebugInfo
    {
      None,
      Col0,
      Row0,
      All,
      White,
      Black,
      WhitePawn,
      BlackPawn,
      WhiteRook,
      BlackRook,
      WhiteKnight,
      BlackKnight,
      WhiteBishop,
      BlackBishop,
      WhiteQueen,
      BlackQueen,
      WhiteKing,
      BlackKing,
    };
    RenderDebugInfo renderBitboard{None};

    RenderDebugInfo renderIndexes{None};

    RenderDebugInfo renderAttackBitboard{None};
    uint64_t selectedAttackingSquares{0};

    Chess::Board board;

    Game() : board() {}
    Game(std::string fen) : board(fen) {}

    void loadTextures(SDL_Renderer *renderer);

    void makeMove(const Chess::Move &move)
    {
      selectedSquare = noSelection; // reset selected square
      generateLegalMovesForSquare();
      board.makeMove(move);
    };

    void unmakeMove()
    {
      board.unmakeMove();
      selectedSquare = noSelection;
      generateLegalMovesForSquare();
    }

    static const unsigned char noSelection = 65;
    unsigned char getSelectedSquare() const
    {
      return selectedSquare;
    }
    void setSelectedSquare(unsigned char square);
    void setSelectedSquare(int row, int col)
    {
      setSelectedSquare(col + row * 8);
    }

    const std::vector<Chess::Move> &getLegalMovesForSelectedSquare() const
    {
      return legalMovesForSelectedSquare;
    }

    void handleKeyboard(SDL_KeyboardEvent *event)
    {
    }
    void handleMouseDown(SDL_MouseButtonEvent *event);
    void handleDrag(SDL_MouseMotionEvent *event)
    {
      if (!dragging)
        return;
      dragPosition.dx = event->x;
      dragPosition.dy = event->y;
    }
    void handleMouseUp(SDL_MouseButtonEvent *event);
    void render(SDL_Renderer *renderer);

    void resetBoard();
    void importFEN(std::string &fen)
    {
      board = Chess::Board(fen);
    }
  };
};
