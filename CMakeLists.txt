cmake_minimum_required(VERSION 3.10)

project(3DS-Chess)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(3DS-Chess ${SOURCES})

ctr_generate_smdh(3DS-Chess.smdh
    NAME "3DS Chess"
    AUTHOR "Your Name"
    DESCRIPTION "A simple chess game for the Nintendo 3DS"
    VERSION "1.0.0"
    # ICON "assets/icon.png"
)

ctr_create_3dsx(3DS-Chess
  SMDH 3DS-Chess.smdh
  # ROMFS romfs
)
