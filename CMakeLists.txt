cmake_minimum_required(VERSION 3.13)

project(3DS-Chess)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

set(CHESS_SOURCES
  "src/chess/board.cpp"
  "src/chess/piece.cpp"
  "src/chess/move.cpp"
  "src/chess/boardState.cpp"
)
set(CHESS_ASSETS
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

set(3DS_SOURCES
  ${CHESS_SOURCES}
  "src/game.cpp"
  "src/main.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Nintendo3DS")

# 3DS App

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gfx" "${CMAKE_BINARY_DIR}/romfs")
add_executable(3DS-Chess ${3DS_SOURCES})

dkp_add_asset_target(romfs ${CMAKE_BINARY_DIR}/romfs)
ctr_add_graphics_target(pieces ATLAS OUTPUT gfx/pieces.t3x INPUTS
  ${CHESS_ASSETS}
)
dkp_install_assets(romfs TARGETS pieces)

target_link_libraries(3DS-Chess PRIVATE
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

else()

# PC Debugging Builds (or UCI engine)

set(IMGUI_SOURCES
  "vendored/imgui/backends/imgui_impl_sdlrenderer3.cpp"
  "vendored/imgui/backends/imgui_impl_sdl3.cpp"
  "vendored/imgui/imgui.cpp"
  "vendored/imgui/imgui_demo.cpp"
  "vendored/imgui/imgui_draw.cpp"
  "vendored/imgui/imgui_tables.cpp"
  "vendored/imgui/imgui_widgets.cpp"
)

set(DEBUG_SOURCES
  ${CHESS_SOURCES}
  ${IMGUI_SOURCES}
  "test/debugger.cpp"
  "test/debugger/game.cpp"
  "test/debugger/search.cpp"
  "test/debugger/debug.cpp"
)

SET(UCI_SOURCES
  ${CHESS_SOURCES}
  "test/uci.cpp"
)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

foreach(ASSET ${CHESS_ASSETS} "assets/square-moveable.png")
  configure_file(${ASSET} "${CMAKE_BINARY_DIR}/${ASSET}" COPYONLY)
endforeach()

add_executable(debugger ${DEBUG_SOURCES})
target_link_libraries(debugger PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
target_include_directories(debugger PRIVATE "vendored/imgui" "vendored/imgui/backends")

endif()
