#include "chess/move.hpp"
#include "move.hpp"
#include "piece.hpp"

namespace Chess
{
  const Move Move::Empty = Move(); // Define the static Empty move
  std::string Move::getNotation(Piece movedPiece) const
  {
    std::string notation;
    if (flags() == Move::Flag::CastleKingside)
      return std::string("O-O");
    if (flags() == Move::Flag::CastleQueenside)
      return std::string("O-O-O");

    switch (movedPiece.type())
    {
    case Piece::Knight:
      notation.append("N");
      break;
    case Piece::Bishop:
      notation.append("B");
      break;
    case Piece::Rook:
      notation.append("R");
      break;
    case Piece::Queen:
      notation.append("Q");
      break;
    case Piece::King:
      notation.append("K");
      break;
    }

    notation.append(1, 'a' + (startSquare() % 8));
    notation.append(1, '1' + (startSquare() / 8));
    if (flags() == Move::Flag::Capture)
      notation.append("x");
    notation.append(1, 'a' + (endSquare() % 8));
    notation.append(1, '1' + (endSquare() / 8));

    switch (flags())
    {
    case Move::Flag::BishopPromotion:
      notation.append("=B");
      break;
    case Move::Flag::KnightPromotion:
      notation.append("=N");
      break;
    case Move::Flag::RookPromotion:
      notation.append("=R");
      break;
    case Move::Flag::QueenPromotion:
      notation.append("=Q");
      break;
    default:
      break;
    }

    // todo - this needs to be a lot better

    return notation;
  }
}
