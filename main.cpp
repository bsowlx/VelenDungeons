#include "raylib.h"

int main() {
    constexpr int kCellPx = 32;
    constexpr int kCols   = 20;
    constexpr int kRows   = 15;
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    enum class Tile : char { Floor = '.', Wall = '#' };

    // Three rooms connected by corridors.
    //   A: cols 2-7,  rows 2-6   B: cols 11-17, rows 2-5   C: cols 5-14, rows 10-13
    //   A<->B horizontal corridor at row 3, cols 8-10
    //   A<->C vertical corridor  at col 5,  rows 7-9
    //   B<->C vertical corridor  at col 14, rows 6-9
    constexpr const char* kMap[kRows] = {
        "####################",
        "####################",
        "##......###.......##",
        "##................##",
        "##......###.......##",
        "##......###.......##",
        "##......######.#####",
        "#####.########.#####",
        "#####.########.#####",
        "#####.########.#####",
        "#####..........#####",
        "#####..........#####",
        "#####..........#####",
        "#####..........#####",
        "####################",
    };

    auto tileAt = [&](int x, int y) -> Tile {
        return static_cast<Tile>(kMap[y][x]);
    };
    auto isWalkable = [&](int x, int y) {
        if (x < 0 || x >= kCols || y < 0 || y >= kRows) return false;
        return tileAt(x, y) == Tile::Floor;
    };

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);

    constexpr Color kBackground = { 0x0d, 0x0d, 0x0f, 0xff };
    constexpr Color kFloor      = { 0x1e, 0x1c, 0x14, 0xff };
    constexpr Color kWall       = { 0x2a, 0x26, 0x1e, 0xff };
    constexpr Color kPlayer     = { 0xc8, 0xa8, 0x4b, 0xff };

    int playerX = 4;
    int playerY = 4;

    while (!WindowShouldClose()) {
        int nx = playerX, ny = playerY;
        if      (IsKeyPressed(KEY_W)) ny--;
        else if (IsKeyPressed(KEY_S)) ny++;
        else if (IsKeyPressed(KEY_A)) nx--;
        else if (IsKeyPressed(KEY_D)) nx++;
        if (isWalkable(nx, ny)) { playerX = nx; playerY = ny; }

        BeginDrawing();
        ClearBackground(kBackground);

        for (int y = 0; y < kRows; y++) {
            for (int x = 0; x < kCols; x++) {
                Color c = (tileAt(x, y) == Tile::Wall) ? kWall : kFloor;
                DrawRectangle(x * kCellPx + 1, y * kCellPx + 1,
                              kCellPx - 2, kCellPx - 2, c);
            }
        }

        DrawRectangle(playerX * kCellPx + 4, playerY * kCellPx + 4,
                      kCellPx - 8, kCellPx - 8, kPlayer);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
