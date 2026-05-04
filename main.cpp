#include "raylib.h"
#include "raymath.h"
#include "dungeon.h"
#include "undo.h"
#include "collision.h"
#include "enemy.h"
#include "projectile.h"
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    constexpr int kCellPx = 32;
    constexpr int kCols   = 20;
    constexpr int kRows   = 15;
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    constexpr float kPlayerSize  = 24.0f;   // px (centered AABB)
    constexpr float kPlayerHalf  = kPlayerSize / 2.0f;
    constexpr float kPlayerSpeed = 160.0f;  // px/sec  =  5 tiles/sec
    constexpr int   kPlayerMaxHp = 3;

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

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);

    constexpr Color kBackground = { 0x0d, 0x0d, 0x0f, 0xff };
    constexpr Color kFloor      = { 0x1e, 0x1c, 0x14, 0xff };
    constexpr Color kWall       = { 0x2a, 0x26, 0x1e, 0xff };
    constexpr Color kPlayer     = { 0xc8, 0xa8, 0x4b, 0xff };
    constexpr Color kEnemy      = { 0xa0, 0x40, 0x40, 0xff };
    constexpr Color kProjectile = { 0xff, 0xe6, 0xa0, 0xff };

    // Spawn at the center of tile (4, 4) inside room A.
    Vector2 playerPos = { 4.5f * kCellPx, 4.5f * kCellPx };
    int playerHp = kPlayerMaxHp;
    bool gameOver = false;
    float damageCooldown = 0.0f;  // seconds of i-frames remaining
    float fireCooldown   = 0.0f;

    std::vector<Enemy> enemies = {
        { {14.5f * kCellPx, 3.5f * kCellPx}, bId, kEnemyMaxHp, true },
        { { 9.5f * kCellPx, 11.5f * kCellPx}, cId, kEnemyMaxHp, true },
    };
    std::vector<Projectile> projectiles;

    UndoStack undoStack;
    int prevActiveRoom = -1;

    Camera2D camera = { 0 };
    camera.target   = playerPos;
    camera.offset   = { kWidth / 2.0f, kHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    while (!WindowShouldClose()) {
        int activeRoom = -1;
        float dt = GetFrameTime();

        if (!gameOver) {
            Vector2 dir = { 0.0f, 0.0f };
            if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
            if (IsKeyDown(KEY_S)) dir.y += 1.0f;
            if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
            if (IsKeyDown(KEY_D)) dir.x += 1.0f;
            if (dir.x != 0.0f || dir.y != 0.0f) dir = Vector2Normalize(dir);

            float step = kPlayerSpeed * dt;
            resolveX(playerPos, dir.x * step, kPlayerHalf, isWalkable, kCellPx);
            resolveY(playerPos, dir.y * step, kPlayerHalf, isWalkable, kCellPx);

            int playerTx = (int)(playerPos.x / (float)kCellPx);
            int playerTy = (int)(playerPos.y / (float)kCellPx);
            activeRoom = dungeon.roomAt(playerTx, playerTy);

            updateEnemies(enemies, playerPos, activeRoom, dt, isWalkable, kCellPx);

            // Fire on held left-click, rate-limited.
            if (fireCooldown > 0.0f) fireCooldown -= dt;
            if (fireCooldown <= 0.0f && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                Vector2 toMouse = Vector2Subtract(mouseWorld, playerPos);
                if (Vector2Length(toMouse) > 0.001f) {
                    fireProjectile(projectiles, playerPos, Vector2Normalize(toMouse));
                    fireCooldown = kFireCooldown;
                }
            }

            // Update projectiles after firing so the just-fired one moves this frame.
            // Runs before contact-damage so an enemy can't deal damage on the same
            // frame it dies.
            updateProjectiles(projectiles, enemies, dt, isWalkable, kCellPx);

            // Contact damage with i-frames.
            if (damageCooldown > 0.0f) damageCooldown -= dt;
            if (damageCooldown <= 0.0f) {
                Rectangle pr = { playerPos.x - kPlayerHalf, playerPos.y - kPlayerHalf,
                                 kPlayerSize, kPlayerSize };
                for (const Enemy& e : enemies) {
                    if (!e.alive || e.roomId != activeRoom) continue;
                    Rectangle er = { e.pos.x - kEnemyHalf, e.pos.y - kEnemyHalf,
                                     kEnemySize, kEnemySize };
                    if (CheckCollisionRecs(pr, er)) {
                        playerHp--;
                        damageCooldown = 0.5f;
                        break;
                    }
                }
            }

            // Debug rewind: pop one snapshot, teleport to it. Suppress the re-push
            // by syncing prevActiveRoom to the restored room. Reset cooldown so the
            // restored player gets a grace window.
            if (IsKeyPressed(KEY_U)) {
                if (auto snap = undoStack.pop(); snap) {
                    playerPos = snap->playerPos;
                    activeRoom = snap->activeRoom;
                    prevActiveRoom = snap->activeRoom;
                    playerHp = snap->hp;
                    damageCooldown = 0.5f;
                }
            }

            // Death: auto-pop. Empty stack -> game over.
            if (playerHp <= 0) {
                if (auto snap = undoStack.pop(); snap) {
                    playerPos = snap->playerPos;
                    activeRoom = snap->activeRoom;
                    prevActiveRoom = snap->activeRoom;
                    playerHp = snap->hp;
                    damageCooldown = 0.5f;
                } else {
                    gameOver = true;
                }
            }

            // Push on entering a (different) room — corridors are activeRoom == -1
            // and don't push. At startup this captures the spawn snapshot too.
            if (!gameOver && activeRoom >= 0 && activeRoom != prevActiveRoom) {
                undoStack.push({playerPos, activeRoom, playerHp});
            }
            prevActiveRoom = activeRoom;
        }

        camera.target = playerPos;

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

        for (const Enemy& e : enemies) {
            if (!e.alive) continue;
            DrawRectangle((int)(e.pos.x - kEnemyHalf), (int)(e.pos.y - kEnemyHalf),
                          (int)kEnemySize, (int)kEnemySize, kEnemy);
        }

        for (const Projectile& p : projectiles) {
            DrawRectangle((int)(p.pos.x - kProjectileHalf), (int)(p.pos.y - kProjectileHalf),
                          (int)kProjectileSize, (int)kProjectileSize, kProjectile);
        }

        DrawRectangle((int)(playerPos.x - kPlayerHalf), (int)(playerPos.y - kPlayerHalf),
                      (int)kPlayerSize, (int)kPlayerSize, kPlayer);

        EndMode2D();

        const char* roomName = (activeRoom >= 0) ? dungeon.room(activeRoom).name.c_str() : "—";
        DrawText(TextFormat("Room: %s   HP: %d/%d", roomName, playerHp, kPlayerMaxHp),
                 16, 16, 24, kPlayer);
        DrawText(TextFormat("[checkpoints: %d]   U = rewind   LMB = fire",
                            undoStack.size()),
                 16, 44, 20, kPlayer);

        if (gameOver) {
            const char* msg = "GAME OVER";
            int fs = 64;
            int tw = MeasureText(msg, fs);
            DrawText(msg, (kWidth - tw) / 2, kHeight / 2 - fs / 2, fs, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
