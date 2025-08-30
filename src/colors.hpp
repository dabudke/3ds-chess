#pragma once

#include <citro2d.h>

enum Color : u32
{
  Background = C2D_Color32(24, 25, 35, 0xFF),
  WhiteSquare = C2D_Color32(165, 141, 137, 0xFF),
  BlackSquare = C2D_Color32(56, 64, 93, 0xFF),
  SelectedSquare = C2D_Color32(0xFF, 0x00, 0x00, 0xB2),
  LegalMove = C2D_Color32(0x00, 0x00, 0x00, 0xB2),
  PreviousMove = C2D_Color32(235, 88, 52, 0xA2)
};
