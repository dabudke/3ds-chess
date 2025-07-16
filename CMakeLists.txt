cmake_minimum_required(VERSION 3.10)

project(3DS-Chess)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

set(SOURCES
src/piece.cpp
src/chess.cpp
src/main.cpp
)

add_executable(3DS-Chess ${SOURCES})

dkp_add_asset_target(romfs romfs)
ctr_add_graphics_target(pieces ATLAS OUTPUT gfx/pieces.t3x INPUTS
  "assets/white pawn.png"
  "assets/black pawn.png"
  "assets/white rook.png"
  "assets/black rook.png"
  "assets/white knight.png"
  "assets/black knight.png"
  "assets/white bishop.png"
  "assets/black bishop.png"
  "assets/white queen.png"
  "assets/black queen.png"
  "assets/white king.png"
  "assets/black king.png"
)
dkp_install_assets(romfs TARGETS pieces)

target_link_libraries(3DS-Chess
    citro2d
    citro3d
    ctru
)

ctr_generate_smdh(3DS-Chess.smdh
    NAME "3DS Chess"
    AUTHOR "Your Name"
    DESCRIPTION "A simple chess game for the Nintendo 3DS"
    VERSION "1.0.0"
    # ICON "assets/icon.png"
)

ctr_create_3dsx(3DS-Chess
  SMDH 3DS-Chess.smdh
  ROMFS romfs
)
