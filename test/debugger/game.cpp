#include "game.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <map>
#include <string>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3_image/SDL_image.h"

#include "chess/board.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"

static SDL_Texture *whitePawn;
static SDL_Texture *blackPawn;
static SDL_Texture *whiteRook;
static SDL_Texture *blackRook;
static SDL_Texture *whiteKnight;
static SDL_Texture *blackKnight;
static SDL_Texture *whiteBishop;
static SDL_Texture *blackBishop;
static SDL_Texture *whiteQueen;
static SDL_Texture *blackQueen;
static SDL_Texture *whiteKing;
static SDL_Texture *blackKing;
static SDL_Texture *moveableSquare;

namespace Debugger {
unsigned long long Game::perft(int depth) {
  if (depth == 0)
    return 1ULL;

  unsigned long long moves{0};
  std::forward_list<Chess::Move> legalMoves = board.getAllLegalMoves();
  for (auto legalMove : legalMoves) {
    board.makeMove(legalMove);
    moves += perft(depth - 1);
    board.unmakeMove();
  }
  return moves;
}
std::map<Chess::Move, unsigned long long> Game::perftDivide(int depth) {
  std::map<Chess::Move, unsigned long long> divided{};
  if (depth == 0)
    return divided;

  std::forward_list<Chess::Move> moves = board.getAllLegalMoves();
  for (auto move : moves) {
    board.makeMove(move);
    divided[move] = perft(depth - 1);
    board.unmakeMove();
  }
  return divided;
}

Game::PerftData Game::startPerft(int depth) {
  auto movesList(board.getAllLegalMoves());
  std::vector<Chess::Move> moves(movesList.begin(), movesList.end());
  Game::PerftData data{true, depth, {{-1, moves}}};
  return data;
}

/// @brief step through a perft operation
/// @param data perft data
/// @return [true] if perft has completed
bool Game::stepPerft(PerftData &data) {
  // for each list of moves, pop entries that are fully explored (index at end of list)
  if (data.moveList.back().index + 1 == data.moveList.back().moves.size()) {
    while (data.moveList.back().index + 1 == data.moveList.back().moves.size()) {
      // unmake the move if there aren't any moves (empty moveset)
      if (data.moveList.back().moves.size() != 0)
        board.unmakeMove();
      data.moveList.pop_back();
    }
    // if we've unmade all moves back to root, perft is done!
    if (data.moveList.size() == 0)
      return true;

    // we've stepped all the way up from leaf to branch with siblings
    // advance to next sibling
    board.unmakeMove();
    // [i cannot believe this is legal C++ code]
    auto move = data.moveList.back().moves.at(++data.moveList.back().index);
    board.makeMove(move);
  } else {
    // if not at max depth, deepen by one layer
    if (data.moveList.size() < data.depth) {
      auto movesList = board.getAllLegalMoves();
      std::vector<Chess::Move> moves(movesList.begin(), movesList.end());
      int index{-1};
      if (moves.size() > 0) {
        auto &move = moves.front();
        board.makeMove(move);
        index++;
      }
      data.moveList.push_back({index, moves});
      // we've made a move (probably) at this point
      return false;
    }

    // we haven't deepened at this point, advance to next sibling
    board.unmakeMove();
    auto &move = data.moveList.back().moves.at(++data.moveList.back().index);
    board.makeMove(move);
  }
  return false;
}

void Game::stopPerft(PerftData &data) {
  data.running = false;
  for (auto &set : data.moveList) {
    if (set.index != -1)
      board.unmakeMove();
  }
  data.moveList.clear();
}
// void Game::generateLegalMoves()
// {
//   // only difference between this and the recursive function is
//   // that this function assigns legal moves based on square
//   std::cout << "Generating all legal moves (depth of " << moveGenDepth << ")\n";
//   for (int i{0}; i < 64; i++)
//   {
//     allLegalMoves[i].clear();
//     const std::vector<Chess::Move> legalMoves = board.getLegalMovesForSquare(i);
//     for (Chess::Move move : legalMoves)
//     {
//       MoveTreeNode moveNode{move};
//       if (board.makeMove(move.startSquare(), move.endSquare()))
//       {
//         perft(&moveNode, moveGenDepth - 1);
//         board.unmakeMove();
//       }
//       else
//       {
//         std::cout << "Error making move: " << move.getNotation(Chess::Piece::Empty) << std::endl;
//         moveNode.errored = true;
//       }
//       allLegalMoves[i].push_back(moveNode);
//     }
//   }
// }

void Game::loadTextures(SDL_Renderer *renderer) {
  whitePawn = IMG_LoadTexture(renderer, "assets/white pawn.png");
  blackPawn = IMG_LoadTexture(renderer, "assets/black pawn.png");
  whiteRook = IMG_LoadTexture(renderer, "assets/white rook.png");
  blackRook = IMG_LoadTexture(renderer, "assets/black rook.png");
  whiteKnight = IMG_LoadTexture(renderer, "assets/white knight.png");
  blackKnight = IMG_LoadTexture(renderer, "assets/black knight.png");
  whiteBishop = IMG_LoadTexture(renderer, "assets/white bishop.png");
  blackBishop = IMG_LoadTexture(renderer, "assets/black bishop.png");
  whiteQueen = IMG_LoadTexture(renderer, "assets/white queen.png");
  blackQueen = IMG_LoadTexture(renderer, "assets/black queen.png");
  whiteKing = IMG_LoadTexture(renderer, "assets/white king.png");
  blackKing = IMG_LoadTexture(renderer, "assets/black king.png");
  moveableSquare = IMG_LoadTexture(renderer, "assets/square-moveable.png");
}

void Game::setSelectedSquare(unsigned char square) {
  // check if square is in bounds
  if (square >= 64) {
    selectedSquare = noSelection; // reset selection
  }
  // if no piece is on that square
  if (board.getPiece(square) == Chess::Piece::Empty) {
    selectedSquare = noSelection; // reset selection
  }
  if (selectedSquare == square) {
    // no work needs to be done
    selectedSquare = noSelection;
  }
  selectedSquare = square;
  generateLegalMovesForSquare();
}

void Game::render(SDL_Renderer *renderer) {
  SDL_FRect destRect;
  Chess::Move lastMove = board.getCurrentState().getPreviousMove();
  Chess::Piece piece;

  uint64_t bitboard{0};
  switch (renderBitboard) {
  case All:
    bitboard = board.bitboards.getAllPiecesBitboard();
    break;
  case White:
    bitboard = board.bitboards.getWhitePiecesBitboard();
    break;
  case Black:
    bitboard = board.bitboards.getBlackPiecesBitboard();
    break;

  default: {
    Chess::Piece::Type bitboardType{0};
    Chess::Piece::Color bitboardColor{0};

    switch (renderBitboard) {
    case WhitePawn:
    case WhiteRook:
    case WhiteKnight:
    case WhiteBishop:
    case WhiteQueen:
    case WhiteKing:
      bitboardColor = Chess::Piece::White;
      break;

    case BlackPawn:
    case BlackRook:
    case BlackKnight:
    case BlackBishop:
    case BlackQueen:
    case BlackKing:
      bitboardColor = Chess::Piece::Black;
    default:
      break;
    }
    switch (renderBitboard) {
    case WhitePawn:
    case BlackPawn:
      bitboardType = Chess::Piece::Pawn;
      break;
    case WhiteRook:
    case BlackRook:
      bitboardType = Chess::Piece::Rook;
      break;
    case WhiteKnight:
    case BlackKnight:
      bitboardType = Chess::Piece::Knight;
      break;
    case WhiteBishop:
    case BlackBishop:
      bitboardType = Chess::Piece::Bishop;
      break;
    case WhiteQueen:
    case BlackQueen:
      bitboardType = Chess::Piece::Queen;
      break;
    case WhiteKing:
    case BlackKing:
      bitboardType = Chess::Piece::King;
    default:
      break;
    }

    bitboard = board.bitboards.getBitboard(bitboardType, bitboardColor);
  }
  }

  const Chess::Board::PieceIndex::Entry *pieceIndex{nullptr};
  if (renderIndexes != None) {
    Chess::Piece::Color indexColor;
    switch (renderIndexes) {
    case RenderDebugInfo::WhitePawn:
    case RenderDebugInfo::WhiteRook:
    case RenderDebugInfo::WhiteKnight:
    case RenderDebugInfo::WhiteBishop:
    case RenderDebugInfo::WhiteQueen:
    case RenderDebugInfo::WhiteKing:
      indexColor = Chess::Piece::White;
      break;

    case RenderDebugInfo::BlackPawn:
    case RenderDebugInfo::BlackRook:
    case RenderDebugInfo::BlackKnight:
    case RenderDebugInfo::BlackBishop:
    case RenderDebugInfo::BlackQueen:
    case RenderDebugInfo::BlackKing:
      indexColor = Chess::Piece::Black;
    default:
      break;
    }

    Chess::Piece::Type indexType;
    switch (renderIndexes) {
    case RenderDebugInfo::WhitePawn:
    case RenderDebugInfo::BlackPawn:
      indexType = Chess::Piece::Pawn;
      break;
    case RenderDebugInfo::WhiteRook:
    case RenderDebugInfo::BlackRook:
      indexType = Chess::Piece::Rook;
      break;
    case RenderDebugInfo::WhiteKnight:
    case RenderDebugInfo::BlackKnight:
      indexType = Chess::Piece::Knight;
      break;
    case RenderDebugInfo::WhiteBishop:
    case RenderDebugInfo::BlackBishop:
      indexType = Chess::Piece::Bishop;
      break;
    case RenderDebugInfo::WhiteQueen:
    case RenderDebugInfo::BlackQueen:
      indexType = Chess::Piece::Queen;
      break;
    case RenderDebugInfo::WhiteKing:
    case RenderDebugInfo::BlackKing:
      indexType = Chess::Piece::King;
      break;
    default:
      renderIndexes = None;
      break;
    }

    if (renderIndexes != None)
      pieceIndex = board.pieceIndex.getIndex(Chess::Piece(indexColor, indexType));
  }

  uint64_t selectedBitboard{0};
  uint64_t attacksBitboard{0};
  if (renderAttackBitboard != None) {
    switch (renderAttackBitboard) {
    case Game::WhitePawn:
    case Game::BlackPawn:
    case Game::WhiteKnight:
    case Game::WhiteKing:
      if (selectedAttackingSquares != 0)
        selectedBitboard = board.bitmaskForSquare(selectedAttackingSquares - 1);
      break;
    case Game::White:
      selectedBitboard = board.bitboards.getWhitePiecesBitboard();
      break;
    case Game::Black:
      selectedBitboard = board.bitboards.getBlackPiecesBitboard();
    default:
      break;
    }
    switch (renderAttackBitboard) {
    case Game::WhitePawn:
      attacksBitboard = board.pawnAttacks[0][selectedAttackingSquares - 1];
      break;
    case Game::BlackPawn:
      attacksBitboard = board.pawnAttacks[1][selectedAttackingSquares - 1];
      break;
    case Game::WhiteKnight:
      attacksBitboard = board.knightAttacks[selectedAttackingSquares - 1];
      break;
    case Game::WhiteKing:
      attacksBitboard = board.kingAttacks[selectedAttackingSquares - 1];
      break;

    case Game::AttacksTo:
      if (selectedAttackingSquares != 0) {
        attacksBitboard = board.attacksToSquare(board.bitboards.getAllPiecesBitboard(), selectedAttackingSquares - 1);
      }
      break;

    default:
      break;
    }
  }

  for (int square{0}; square < 64; ++square) {
    int row = 7 - square / 8;
    int col = square % 8;

    if ((row + col) % 2 == 0)
      SDL_SetRenderDrawColor(renderer, 56, 64, 93, SDL_ALPHA_OPAQUE); /* new color, full alpha. */
    else
      SDL_SetRenderDrawColor(renderer, 165, 141, 137, SDL_ALPHA_OPAQUE); /* new color, full alpha. */

    destRect.x = 5 + col * 30;
    destRect.y = 5 + row * 30;
    destRect.w = 30;
    destRect.h = 30;
    SDL_RenderFillRect(renderer, &destRect);
    if (lastMove != Chess::Move::Empty && (square == lastMove.startSquare() || square == lastMove.endSquare())) {
      SDL_SetRenderDrawColor(renderer, 235, 88, 52, 0x20); /* new color, semi-transparent. */
      SDL_RenderFillRect(renderer, &destRect);
    }
    if (selectedSquare == square) {
      SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0x40); /* new color, semi-transparent. */
      SDL_RenderFillRect(renderer, &destRect);
    }
    piece = board.getPiece(square);

    if (piece != Chess::Piece::Empty) {
      switch (piece.type()) {
      case Chess::Piece::Pawn:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whitePawn, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackPawn, NULL, &destRect);
        }
        break;
      case Chess::Piece::Rook:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whiteRook, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackRook, NULL, &destRect);
        }
        break;
      case Chess::Piece::Knight:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whiteKnight, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackKnight, NULL, &destRect);
        }
        break;
      case Chess::Piece::Bishop:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whiteBishop, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackBishop, NULL, &destRect);
        }
        break;
      case Chess::Piece::Queen:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whiteQueen, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackQueen, NULL, &destRect);
        }
        break;
      case Chess::Piece::King:
        if (piece.color() == Chess::Piece::White) {
          SDL_RenderTexture(renderer, whiteKing, NULL, &destRect);
        } else {
          SDL_RenderTexture(renderer, blackKing, NULL, &destRect);
        }
        break;
      }
    }

    // render bitboard
    if (renderBitboard != None) {
      if (bitboard & board.bitmaskForSquare(square)) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
        SDL_RenderFillRect(renderer, &destRect);
      } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100);
        SDL_RenderFillRect(renderer, &destRect);
      }
    }
    if (renderAttackBitboard != None) {
      uint64_t bit = board.bitmaskForSquare(square);
      if (bit & selectedBitboard) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100);
        SDL_RenderFillRect(renderer, &destRect);
      }
      if (bit & attacksBitboard) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);
        SDL_RenderFillRect(renderer, &destRect);
      }
    }
  }

  const auto legalMoves = getLegalMovesForSelectedSquare();
  for (auto move : legalMoves) {
    int row = 7 - move.endSquare() / 8;
    int col = move.endSquare() % 8;
    SDL_FRect destRect = {static_cast<float>(5 + col * 30), static_cast<float>(5 + row * 30), 30, 30};
    SDL_RenderTexture(renderer, moveableSquare, NULL, &destRect);
  }

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  const auto moves = board.getMoveHistory();
  for (int i{0}; i < moves.size(); ++i) {
    SDL_RenderDebugText(renderer, 485, 5 + 10 * i,
                        (moves[i].getNotation(board.getPiece(moves[i].endSquare()))).c_str());
  }

  SDL_RenderDebugText(renderer, 5, 250, board.getFenString().c_str());
  SDL_RenderDebugText(renderer, 250, 15, ("Selected square: " + std::to_string((int)selectedSquare)).c_str());

  int i{0};
  while (pieceIndex != nullptr) {
    int row = 7 - pieceIndex->square / 8;
    int col = pieceIndex->square % 8;
    SDL_FRect destRect = {static_cast<float>(5 + col * 30), static_cast<float>(5 + row * 30), 30, 30};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);
    SDL_RenderFillRect(renderer, &destRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugTextFormat(renderer, destRect.x, destRect.y, "%d", i++);
    pieceIndex = pieceIndex->next;
  }
}

void Game::handleMouseDown(SDL_MouseButtonEvent *event) {
  int col = (static_cast<int>(floor(event->x)) - 5) / 30;
  int row = 7 - (static_cast<int>(floor(event->y)) - 5) / 30;

  if (col < 0 || col > 7 || row < 0 || row > 7)
    return; // out of bounds

  unsigned char square = col + row * 8;

  if (event->button == SDL_BUTTON_LEFT) {
    for (auto move : getLegalMovesForSelectedSquare()) {
      if (move.endSquare() == square) {
        makeMove(move);
        return;
      }
    }

    Chess::Piece piece = board.getPiece(square);
    if (piece != Chess::Piece::Empty &&
        piece.color() == (board.whiteMove() ? Chess::Piece::White : Chess::Piece::Black)) {
      setSelectedSquare(square);
      dragging = true;
      dragPosition.dx = event->x;
      dragPosition.dy = event->y;
    }
  } else if (event->button == SDL_BUTTON_RIGHT) {
    switch (renderAttackBitboard) {
    case WhitePawn:
    case BlackPawn:
    case WhiteKnight:
    case WhiteKing:
      selectedAttackingSquares = square + 1;
      break;

      // TODO - multi-select squares

    default:
      break;
    }
  }
}

void Game::handleMouseUp(SDL_MouseButtonEvent *event) {
  if (!dragging)
    return;

  uint8_t row = static_cast<int>(dragPosition.dx);

  dragging = false;
}

void Game::resetBoard() {
  board = Chess::Board();
  selectedSquare = noSelection;
  generateLegalMovesForSquare();
}
}; // namespace Debugger
