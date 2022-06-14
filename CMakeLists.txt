cmake_minimum_required(VERSION 3.14)
project(chip8)
set(CMAKE_STANDARD 99)
find_package(SDL2 REQUIRED)
add_executable(
        CHIP8_EMU
        Chip8.c
        main.c
        render.c)
target_compile_options(CHIP8_EMU PRIVATE -Wall)
target_link_libraries(CHIP8_EMU PRIVATE SDL2::SDL2)