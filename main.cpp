#include "raylib.h"

int main() {
    constexpr int kCellPx = 32;
    constexpr int kCols   = 20;
    constexpr int kRows   = 15;
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);

    constexpr Color kBackground = { 0x0d, 0x0d, 0x0f, 0xff };
    constexpr Color kFloor      = { 0x1e, 0x1c, 0x14, 0xff };
    constexpr Color kPlayer     = { 0xc8, 0xa8, 0x4b, 0xff };

    int playerX = kCols / 2;
    int playerY = kRows / 2;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_W) && playerY > 0)         playerY--;
        if (IsKeyPressed(KEY_S) && playerY < kRows - 1) playerY++;
        if (IsKeyPressed(KEY_A) && playerX > 0)         playerX--;
        if (IsKeyPressed(KEY_D) && playerX < kCols - 1) playerX++;

        BeginDrawing();
        ClearBackground(kBackground);

        for (int y = 0; y < kRows; y++) {
            for (int x = 0; x < kCols; x++) {
                DrawRectangle(x * kCellPx + 1, y * kCellPx + 1,
                              kCellPx - 2, kCellPx - 2, kFloor);
            }
        }

        DrawRectangle(playerX * kCellPx + 4, playerY * kCellPx + 4,
                      kCellPx - 8, kCellPx - 8, kPlayer);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
