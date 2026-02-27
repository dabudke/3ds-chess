#pragma once
#include <vector>
#include "chess/board.hpp"

class Search
{
protected:
  int totalMoves{0};
  int currentDepth{0};
  Chess::Board &board;
  std::vector<std::vector<Chess::Move>> moveTree;
  std::vector<int> currentMoveIndex;
  std::vector<int> totalMovesAtDepth;
  bool searchStopped{false};

  void incrementMoveCount()
  {
    totalMoves++;
    for (auto &i : totalMovesAtDepth)
    {
      i++;
    }
  }

public:
  const int searchPly;

  Search(int depth, Chess::Board &board) : searchPly(depth),
                                           board(board)
  {
  }

  void continueSearch()
  {
    // check if we are at max ply
    // if not, continue deepening search
    if (currentDepth < searchPly)
    {
      // gather legal moves and add them to tree
      std::vector<Chess::Move> legalMoves = board.getLegalMoves();
      moveTree.push_back(legalMoves);

      // add index for move counting at depth
      totalMovesAtDepth.push_back(0);
      incrementMoveCount();

      // make first move
      currentMoveIndex.push_back(0);

      // increment depth
      currentDepth++;
      return;
    }
    // maximum ply reached, backtrack
    while (currentMoveIndex.back() == moveTree.back().size() - 1)
    {
      board.unmakeMove(); // up one ply
      // moves from this branch not needed anymore
      moveTree.pop_back();
      currentMoveIndex.pop_back();

      // if we are at root node, search is complete
      if (currentDepth <= 0)
      {
        searchStopped = true;
        return; // search complete
      }
      // decrement depth
      currentDepth--;
    }

    // common logic for proceeding to next branch
    board.unmakeMove();
    Chess::Move nextMove = moveTree.back()[++currentMoveIndex.back()];
    if (!board.makeMove(nextMove.startSquare(), nextMove.endSquare()))
    {
      throw std::runtime_error("Failed to make move during search: " + nextMove.getNotation(board.getPiece(nextMove.startSquare())));
    }
    incrementMoveCount();
  }

  int getTotalMoves() const
  {
    return totalMoves;
  }

  int getMovesAtDepth(int depth) const
  {
    if (depth < 0 || depth >= totalMovesAtDepth.size())
    {
      return 0;
    }
    return totalMovesAtDepth[depth];
  }

  int getCurrentDepth() const
  {
    return currentDepth;
  }
  std::string getCurrentMove() const
  {
    return moveTree[currentDepth][currentMoveIndex[currentDepth]].getNotation(board.getPiece(moveTree[currentDepth][currentMoveIndex[currentDepth]].startSquare()));
  }
  bool hasSearchStopped() const
  {
    return searchStopped;
  }
};
