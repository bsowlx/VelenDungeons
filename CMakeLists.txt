cmake_minimum_required(VERSION 3.20)
project(witcher_dungeon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(witcher_dungeon
    src/main.cpp
    src/collision.cpp
    src/dungeon.cpp
    src/enemy.cpp
    src/inventory.cpp
    src/leaderboard.cpp
    src/pathfind.cpp
    src/projectile.cpp
    src/signs.cpp
    src/undo.cpp
    src/wave.cpp
)
target_include_directories(witcher_dungeon PRIVATE src)
target_link_libraries(witcher_dungeon PRIVATE raylib)
