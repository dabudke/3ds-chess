#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

#include "chess/board.hpp"
#include "chess/board/state.hpp"
#include "chess/piece.hpp"

namespace Chess {
#pragma region parsing
const std::string Board::initialFenString = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
Board::Board(std::string fen) {
  // extract parts of FEN string
  std::stringstream fenStream(fen);
  std::string boardPart, activeColorPart, castlingPart, enPassantPart, halfmovePart, fullmovePart;
  fenStream >> boardPart >> activeColorPart >> castlingPart;
  // handle optional en passant - next (mandatory) part is halfmove clock
  if (!isdigit(fenStream.peek()))
    fenStream >> enPassantPart;
  fenStream >> halfmovePart >> fullmovePart;

  // parse board
  int row = 7, col = 0;
  for (char c : boardPart) {
    if (c == '/') {
      row--;
      col = 0;
    } else if (isdigit(c)) {
      int emptySquares = c - '0';
      for (int i = 0; i < emptySquares; i++) {
        board[row * 8 + col] = Piece::Empty;
        col++;
      }
    } else {
      const bool isWhite = isupper(c);
      Piece::Color color = isWhite ? Piece::White : Piece::Black;
      Piece::Type type;
      switch (tolower(c)) {
      case 'p':
        type = Piece::Pawn;
        break;
      case 'r':
        type = Piece::Rook;
        break;
      case 'n':
        type = Piece::Knight;
        break;
      case 'b':
        type = Piece::Bishop;
        break;
      case 'q':
        type = Piece::Queen;
        break;
      case 'k':
        type = Piece::King;
        break;
      default:
        throw std::invalid_argument("Invalid FEN string: unknown piece character");
      }

      uint8_t squareIndex = square(row, col);
      Piece piece = Piece(color, type);
      summonPiece(piece, squareIndex);

      col++;
    }
  }

  // state setup
  int halfmove{0};
  // parse active color
  if (activeColorPart == "b")
    halfmove++;

  // castling availability
  bool canWhiteCastleKingside = castlingPart.find("K") != std::string::npos;
  bool canWhiteCastleQueenside = castlingPart.find("Q") == std::string::npos;
  bool canBlackCastleKingside = castlingPart.find("k") == std::string::npos;
  bool canBlackCastleQueenside = castlingPart.find("q") == std::string::npos;

  // parse en passant
  bool enPassantAvailable{false};
  uint8_t enPassantFile{0};
  if (!enPassantPart.empty() && enPassantPart != "-") {
    enPassantAvailable = true;
    enPassantFile = enPassantPart[0] - 'a';
    if (enPassantFile < 0 || enPassantFile > 7) {
      throw std::invalid_argument("Invalid FEN string: invalid en passant file");
    }
  }

  // parse halfmove
  uint8_t fiftyMoveCounter{0};
  if (!halfmovePart.empty()) {
    int halfmoveClock = std::stoi(halfmovePart);
    if (halfmoveClock < 0 || halfmoveClock > 50) {
      throw std::invalid_argument("Invalid FEN string: invalid halfmove clock");
    }
    fiftyMoveCounter = halfmoveClock;
  }

  // parse fullmove
  if (!fullmovePart.empty()) {
    int fullmove = std::stoi(fullmovePart);
    if (fullmove < 1) {
      throw std::invalid_argument("Invalid FEN string: invalid fullmove number");
    }
    halfmove += (fullmove - 1) * 2;
  }

  state.setInitialState(halfmove, canWhiteCastleKingside, canBlackCastleKingside, canWhiteCastleQueenside,
                        canBlackCastleQueenside, enPassantAvailable, enPassantFile, fiftyMoveCounter);
}

#pragma region generate
std::string Board::getFenString() const {
  static const char *pieceToChar = ".PNBRQKpnbrqk"; // Index 0 is empty, 1-6 are white pieces, 7-12 are black pieces
  std::string fenString;
  int emptyCount = 0;
  Piece piece;

  for (int row{7}; row >= 0; --row) {
    for (int col{0}; col < 8; ++col) {
      int square = row * 8 + col;
      piece = getPiece(square);

      if (piece == Piece::Empty) {
        emptyCount++;
        continue;
      }

      if (emptyCount > 0) {
        fenString.append(std::to_string(emptyCount));
        emptyCount = 0;
      }

      switch (piece.type()) {
      case Piece::Pawn:
        fenString.append(1, piece.color() == Piece::White ? 'P' : 'p');
        break;
      case Piece::Knight:
        fenString.append(1, piece.color() == Piece::White ? 'N' : 'n');
        break;
      case Piece::Bishop:
        fenString.append(1, piece.color() == Piece::White ? 'B' : 'b');
        break;
      case Piece::Rook:
        fenString.append(1, piece.color() == Piece::White ? 'R' : 'r');
        break;
      case Piece::Queen:
        fenString.append(1, piece.color() == Piece::White ? 'Q' : 'q');
        break;
      case Piece::King:
        fenString.append(1, piece.color() == Piece::White ? 'K' : 'k');
        break;
      }
    }
    if (emptyCount > 0) {
      fenString.append(std::to_string(emptyCount));
      emptyCount = 0;
    }
    if (row > 0)
      fenString.append("/");
  }

  if (emptyCount > 0) {
    fenString.append(std::to_string(emptyCount));
  }

  if (whiteMove()) {
    fenString.append(" w ");
  } else {
    fenString.append(" b ");
  }

  const BoardUtils::State &currentState = state.getCurrentState();

  bool canCastle{false};
  if (currentState.canWhiteCastleKingside()) {
    fenString.append("K");
    canCastle = true;
  }
  if (currentState.canWhiteCastleQueenside()) {
    fenString.append("Q");
    canCastle = true;
  }
  if (currentState.canBlackCastleKingside()) {
    fenString.append("k");
    canCastle = true;
  }
  if (currentState.canBlackCastleQueenside()) {
    fenString.append("q");
    canCastle = true;
  }
  if (!canCastle) {
    fenString.append("-");
  }

  if (!currentState.enPassantAvailable()) {
    char enPassantFile = currentState.getEnPassantFile();
    fenString.append(" ");
    fenString.push_back('a' + enPassantFile);
  }

  std::stringstream stream;
  stream << " " << currentState.getFiftyMoveCounter() << " " << state.currentFullmove();
  fenString.append(stream.str());

  return fenString;
}
} // namespace Chess
