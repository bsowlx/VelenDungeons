#include "raylib.h"
#include "raymath.h"
#include "dungeon.h"
#include <cmath>
#include <cstdio>

int main() {
    constexpr int kCellPx = 32;
    constexpr int kCols   = 20;
    constexpr int kRows   = 15;
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    constexpr float kPlayerSize  = 24.0f;   // px (centered AABB)
    constexpr float kPlayerHalf  = kPlayerSize / 2.0f;
    constexpr float kPlayerSpeed = 160.0f;  // px/sec  =  5 tiles/sec

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

    Dungeon dungeon;
    int aId = dungeon.addRoom("A", {2, 2, 7, 6});
    int bId = dungeon.addRoom("B", {11, 2, 17, 5});
    int cId = dungeon.addRoom("C", {5, 10, 14, 13});
    dungeon.addEdge(aId, bId, /*A->B door inside B*/ {11, 3}, /*B->A door inside A*/ {7, 3});
    dungeon.addEdge(aId, cId, /*A->C door inside C*/ {5, 10}, /*C->A door inside A*/ {5, 6});
    dungeon.addEdge(bId, cId, /*B->C door inside C*/ {14, 10}, /*C->B door inside B*/ {14, 5});

    if (std::string err = dungeon.validate(kMap, kRows, kCols); !err.empty()) {
        std::fprintf(stderr, "Dungeon validation failed: %s\n", err.c_str());
        return 1;
    }

    // Axis-separated AABB-vs-tile collision: move on one axis, then if the
    // player overlaps any solid tile, snap to the offending wall edge. The
    // scan range is at most 2x2 tiles since the player AABB is < 1 tile wide.
    auto resolveX = [&](Vector2& pos, float dx) {
        if (dx == 0.0f) return;
        pos.x += dx;
        int minX = (int)std::floor((pos.x - kPlayerHalf) / (float)kCellPx);
        int maxX = (int)std::floor((pos.x + kPlayerHalf - 0.0001f) / (float)kCellPx);
        int minY = (int)std::floor((pos.y - kPlayerHalf) / (float)kCellPx);
        int maxY = (int)std::floor((pos.y + kPlayerHalf - 0.0001f) / (float)kCellPx);
        if (dx > 0) {
            for (int tx = minX; tx <= maxX; tx++)
                for (int ty = minY; ty <= maxY; ty++)
                    if (!isWalkable(tx, ty)) { pos.x = tx * (float)kCellPx - kPlayerHalf; return; }
        } else {
            for (int tx = maxX; tx >= minX; tx--)
                for (int ty = minY; ty <= maxY; ty++)
                    if (!isWalkable(tx, ty)) { pos.x = (tx + 1) * (float)kCellPx + kPlayerHalf; return; }
        }
    };
    auto resolveY = [&](Vector2& pos, float dy) {
        if (dy == 0.0f) return;
        pos.y += dy;
        int minX = (int)std::floor((pos.x - kPlayerHalf) / (float)kCellPx);
        int maxX = (int)std::floor((pos.x + kPlayerHalf - 0.0001f) / (float)kCellPx);
        int minY = (int)std::floor((pos.y - kPlayerHalf) / (float)kCellPx);
        int maxY = (int)std::floor((pos.y + kPlayerHalf - 0.0001f) / (float)kCellPx);
        if (dy > 0) {
            for (int ty = minY; ty <= maxY; ty++)
                for (int tx = minX; tx <= maxX; tx++)
                    if (!isWalkable(tx, ty)) { pos.y = ty * (float)kCellPx - kPlayerHalf; return; }
        } else {
            for (int ty = maxY; ty >= minY; ty--)
                for (int tx = minX; tx <= maxX; tx++)
                    if (!isWalkable(tx, ty)) { pos.y = (ty + 1) * (float)kCellPx + kPlayerHalf; return; }
        }
    };

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);

    constexpr Color kBackground = { 0x0d, 0x0d, 0x0f, 0xff };
    constexpr Color kFloor      = { 0x1e, 0x1c, 0x14, 0xff };
    constexpr Color kWall       = { 0x2a, 0x26, 0x1e, 0xff };
    constexpr Color kPlayer     = { 0xc8, 0xa8, 0x4b, 0xff };

    // Spawn at the center of tile (4, 4) inside room A.
    Vector2 playerPos = { 4.5f * kCellPx, 4.5f * kCellPx };

    Camera2D camera = { 0 };
    camera.target   = playerPos;
    camera.offset   = { kWidth / 2.0f, kHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    while (!WindowShouldClose()) {
        Vector2 dir = { 0.0f, 0.0f };
        if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
        if (IsKeyDown(KEY_S)) dir.y += 1.0f;
        if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
        if (IsKeyDown(KEY_D)) dir.x += 1.0f;
        if (dir.x != 0.0f || dir.y != 0.0f) dir = Vector2Normalize(dir);

        float step = kPlayerSpeed * GetFrameTime();
        resolveX(playerPos, dir.x * step);
        resolveY(playerPos, dir.y * step);

        camera.target = playerPos;
        int playerTx = (int)(playerPos.x / (float)kCellPx);
        int playerTy = (int)(playerPos.y / (float)kCellPx);
        int activeRoom = dungeon.roomAt(playerTx, playerTy);

        BeginDrawing();
        ClearBackground(kBackground);
        BeginMode2D(camera);

        for (int y = 0; y < kRows; y++) {
            for (int x = 0; x < kCols; x++) {
                Color c = (tileAt(x, y) == Tile::Wall) ? kWall : kFloor;
                DrawRectangle(x * kCellPx + 1, y * kCellPx + 1,
                              kCellPx - 2, kCellPx - 2, c);
            }
        }

        DrawRectangle((int)(playerPos.x - kPlayerHalf), (int)(playerPos.y - kPlayerHalf),
                      (int)kPlayerSize, (int)kPlayerSize, kPlayer);

        EndMode2D();

        const char* roomName = (activeRoom >= 0) ? dungeon.room(activeRoom).name.c_str() : "—";
        DrawText(TextFormat("Room: %s", roomName), 16, 16, 24, kPlayer);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
