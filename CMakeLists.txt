# CMakeList.txt : CMake project for Chip8VM, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(Chip8VM)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2")

add_executable (Chip8VM "Chip8VM.cpp" "Chip8VM.h"   "Chip8Renderer.h" "Chip8Renderer.cpp" "main.cpp" "test.cpp")
target_link_libraries(Chip8VM PRIVATE Catch2::Catch2)

set(SDL2_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2-2.0.14/include")
set(SDL2_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2-2.0.14/lib/x64/")

include_directories(${SDL2_INCLUDE_PATH})
target_include_directories(Chip8VM PRIVATE ${SDL2_INCLUDE_PATH})

target_link_libraries(Chip8VM PRIVATE "${SDL2_LIB_PATH}/SDL2.lib")
target_link_libraries(Chip8VM PRIVATE "${SDL2_LIB_PATH}/SDL2main.lib")
target_link_libraries(Chip8VM PRIVATE Catch2::Catch2)

file(
    COPY ${SDL2_LIB_PATH}/SDL2.dll
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)
