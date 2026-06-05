# VelenDungeons

Real-time witcher infinite room clearing dungeon game. Built with C++17 and raylib.

## Demo

https://github.com/user-attachments/assets/168b36c3-2d8d-4ef6-b9e7-dcd4b5eb655f

## Build

You need MSYS2 UCRT64 with gcc, cmake, ninja, and raylib:

```sh
pacman -S mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,raylib}
```

Then, from a UCRT64 shell (or with `C:\msys64\ucrt64\bin` on your PATH):

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/witcher_dungeon.exe
```

## Running it on another machine

The build drops a `dist/` folder: the exe plus the only two DLLs it can't
live without (`glfw3.dll`, `libwinpthread-1.dll`). raylib and the C++ runtime are linked in statically. Copy `dist/` to a USB and it runs on any Windows box. The whole thing is about 3 MB.

## Controls

| Key       | Action                              |
|-----------|-------------------------------------|
| WASD      | Move                                |
| Mouse     | Aim                                 |
| Hold LMB  | Shoot                               |
| `[` `]`   | Cycle weapon                        |
| Q         | Aard — cone knockback               |
| F         | Quen — damage shield                |
| E         | Interact (read the letter / exit)   |
| Esc       | Pause                               |

Each combat room has a red exit tile in a far corner;
clear the room first, then stand on it and press E.

## Tests

```sh
./build/tests.exe
```

57 assertions across the six structures plus the damage math.
