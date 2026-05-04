cmake_minimum_required(VERSION 3.20)
project(witcher_dungeon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(witcher_dungeon main.cpp dungeon.cpp undo.cpp collision.cpp enemy.cpp)
target_link_libraries(witcher_dungeon PRIVATE raylib)
