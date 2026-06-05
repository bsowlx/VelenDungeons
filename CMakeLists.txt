cmake_minimum_required(VERSION 3.20)
project(witcher_dungeon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# On Windows, link raylib statically so the game ships without MSYS2 or raylib
# installed (see the dist/ bundle below). Must be set before find_package. On
# Linux/macOS we build against the system raylib, so this stays off.
if(WIN32)
    set(raylib_USE_STATIC_LIBS ON)
endif()
find_package(raylib CONFIG REQUIRED)

add_library(witcher_lib STATIC
    src/collision.cpp
    src/dungeon.cpp
    src/enemy.cpp
    src/inventory.cpp
    src/leaderboard.cpp
    src/pathfind.cpp
    src/pcg.cpp
    src/projectile.cpp
    src/signs.cpp
    src/undo.cpp
    src/wave.cpp
)
target_include_directories(witcher_lib PUBLIC src)
target_link_libraries(witcher_lib PUBLIC raylib)
# Static raylib needs these Windows system libs; raylib.pc lists none of them.
if(WIN32)
    target_link_libraries(witcher_lib PUBLIC opengl32 gdi32 winmm)
endif()

add_executable(witcher_dungeon src/main.cpp)
target_link_libraries(witcher_dungeon PRIVATE witcher_lib)
# On Windows, bake the C++/gcc runtimes into the exe so no MSYS2 DLLs are needed
# at runtime. (GLFW stays a DLL — MSYS2's libraylib.a calls it via dllimport.)
if(WIN32)
    target_link_options(witcher_dungeon PRIVATE -static-libgcc -static-libstdc++)
endif()

add_executable(tests tests/main.cpp)
target_link_libraries(tests PRIVATE witcher_lib)

# Assemble a portable bundle in dist/: the exe plus the only two non-system DLLs
# it still needs. DLLs are pulled from the compiler's own bin dir, so this works
# regardless of where MSYS2 is installed. Copy dist/ to a USB and it runs on any
# Windows 10/11 machine with no setup.
if(WIN32)
    get_filename_component(MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET witcher_dungeon POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:witcher_dungeon>"
            "${MINGW_BIN}/glfw3.dll"
            "${MINGW_BIN}/libwinpthread-1.dll"
            "${CMAKE_SOURCE_DIR}/dist"
        COMMENT "Assembling portable bundle in dist/")
endif()
