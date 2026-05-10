#include "raylib.h"
#include "raymath.h"
#include "dungeon.h"
#include "undo.h"
#include "collision.h"
#include "enemy.h"
#include "projectile.h"
#include "wave.h"
#include "inventory.h"
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    constexpr int kCellPx = 32;
    constexpr int kCols   = 20;
    constexpr int kRows   = 15;
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    constexpr float kPlayerSize  = 24.0f;
    constexpr float kPlayerHalf  = kPlayerSize / 2.0f;
    constexpr float kPlayerSpeed = 160.0f;
    constexpr int   kPlayerMaxHp = 3;

    enum class Tile : char { Floor = '.', Wall = '#' };

    constexpr const char* kMap[kRows] = {
        "####################",
        "####################",
        "##......###.......##",
        "##...........#.#..##",
        "##......###.......##",
        "##......###.......##",
        "##......######.#####",
        "#####.########.#####",
        "#####.########.#####",
        "#####.########.#####",
        "#####..........#####",
        "#####..#....#..#####",
        "#####....#.....#####",
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
    dungeon.addEdge(aId, bId, {11, 3}, {7, 3});
    dungeon.addEdge(aId, cId, {5, 10}, {5, 6});
    dungeon.addEdge(bId, cId, {14, 10}, {14, 5});

    if (std::string err = dungeon.validate(kMap, kRows, kCols); !err.empty()) {
        std::fprintf(stderr, "Dungeon validation failed: %s\n", err.c_str());
        return 1;
    }

    std::vector<std::vector<EnemySpawn>> rosters(dungeon.roomCount());
    rosters[bId] = {
        { {12.5f * kCellPx,  3.5f * kCellPx} },
        { {16.5f * kCellPx,  4.5f * kCellPx} },
    };
    rosters[cId] = {
        { { 6.5f * kCellPx, 11.5f * kCellPx} },
        { {10.5f * kCellPx, 12.5f * kCellPx} },
        { {13.5f * kCellPx, 11.5f * kCellPx} },
    };

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);
    SetExitKey(0);  // Esc is now a normal key — state-machine handles it

    constexpr Color kBackground = { 0x0d, 0x0d, 0x0f, 0xff };
    constexpr Color kFloor      = { 0x1e, 0x1c, 0x14, 0xff };
    constexpr Color kWall       = { 0x2a, 0x26, 0x1e, 0xff };
    constexpr Color kPlayer     = { 0xc8, 0xa8, 0x4b, 0xff };
    constexpr Color kEnemy      = { 0xa0, 0x40, 0x40, 0xff };
    constexpr Color kProjectile = { 0xff, 0xe6, 0xa0, 0xff };
    constexpr Color kMenuDim    = { 0x00, 0x00, 0x00, 0xb0 };
    constexpr Color kMenuFade   = { 0xc8, 0xa8, 0x4b, 0x80 };

    enum class GameState { Title, Playing, Paused, ReadingLetter, GameOver, Leaderboard };
    enum class Difficulty { Easy, Normal, Hard };

    auto difficultyName = [](Difficulty d) -> const char* {
        switch (d) {
            case Difficulty::Easy:   return "Easy";
            case Difficulty::Normal: return "Normal";
            case Difficulty::Hard:   return "Hard";
        }
        return "?";
    };

    // Run state — reset by initRun on Title->Playing and after GameOver.
    Vector2 playerPos;
    int playerHp;
    float damageCooldown;
    float fireCooldown;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    WaveState waves;
    Inventory inv;
    UndoStack undoStack;
    int prevActiveRoom;
    int activeRoom;

    auto initRun = [&]() {
        playerPos = { 4.5f * kCellPx, 4.5f * kCellPx };
        playerHp = kPlayerMaxHp;
        damageCooldown = 0.0f;
        fireCooldown = 0.0f;
        enemies.clear();
        projectiles.clear();
        waves = {};
        initWaves(waves, dungeon.roomCount(), { aId });
        inv = {};
        inv.add({"Pistol",   0.20f, 1, 400.0f, 1,  0.0f});
        inv.add({"Shotgun",  0.55f, 1, 350.0f, 3, 30.0f});
        inv.add({"Crossbow", 0.45f, 2, 550.0f, 1,  0.0f});
        undoStack = {};
        prevActiveRoom = -1;
        activeRoom = -1;
    };

    initRun();

    GameState state = GameState::Title;
    Difficulty difficulty = Difficulty::Normal;
    int titleSel = 0;     // 0=Start, 1=Leaderboard, 2=Difficulty, 3=Quit
    int pauseSel = 0;     // 0=Resume, 1=Quit to Title
    int gameOverSel = 0;  // 0=Title, 1=Leaderboard
    bool quit = false;

    Camera2D camera = { 0 };
    camera.target   = playerPos;
    camera.offset   = { kWidth / 2.0f, kHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    auto cycleSel = [](int sel, int delta, int n) {
        return ((sel + delta) % n + n) % n;
    };

    auto drawCentered = [&](const char* msg, int y, int fs, Color c) {
        int tw = MeasureText(msg, fs);
        DrawText(msg, (kWidth - tw) / 2, y, fs, c);
    };

    while (!WindowShouldClose() && !quit) {
        float dt = GetFrameTime();

        switch (state) {
            case GameState::Title: {
                if (IsKeyPressed(KEY_UP))   titleSel = cycleSel(titleSel, -1, 4);
                if (IsKeyPressed(KEY_DOWN)) titleSel = cycleSel(titleSel, +1, 4);
                if (titleSel == 2) {
                    if (IsKeyPressed(KEY_LEFT))  difficulty = (Difficulty)cycleSel((int)difficulty, -1, 3);
                    if (IsKeyPressed(KEY_RIGHT)) difficulty = (Difficulty)cycleSel((int)difficulty, +1, 3);
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    if (titleSel == 0) { initRun(); state = GameState::Playing; }
                    else if (titleSel == 1) state = GameState::Leaderboard;
                    else if (titleSel == 3) quit = true;
                }
                if (IsKeyPressed(KEY_ESCAPE)) quit = true;
                break;
            }

            case GameState::Playing: {
                if (IsKeyPressed(KEY_ESCAPE)) { state = GameState::Paused; pauseSel = 0; break; }
                if (IsKeyPressed(KEY_L))      { state = GameState::ReadingLetter; break; }

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

                if (activeRoom >= 0 && activeRoom != prevActiveRoom) {
                    onEnterRoom(waves, activeRoom, rosters);
                }

                updateWaves(waves, enemies, activeRoom, dt);
                updateEnemies(enemies, playerPos, activeRoom, dt, isWalkable, kCellPx, kCols, kRows);

                if (IsKeyPressed(KEY_LEFT_BRACKET))  inv.cyclePrev();
                if (IsKeyPressed(KEY_RIGHT_BRACKET)) inv.cycleNext();

                if (fireCooldown > 0.0f) fireCooldown -= dt;
                if (fireCooldown <= 0.0f && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !inv.empty()) {
                    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                    Vector2 toMouse = Vector2Subtract(mouseWorld, playerPos);
                    if (Vector2Length(toMouse) > 0.001f) {
                        Vector2 baseDir = Vector2Normalize(toMouse);
                        const Weapon& w = inv.current();
                        for (int i = 0; i < w.spread; i++) {
                            float t = (w.spread <= 1) ? 0.0f
                                                      : (float)i / (float)(w.spread - 1) - 0.5f;
                            float a = t * w.spreadAngleDeg * (PI / 180.0f);
                            float c = std::cos(a), s = std::sin(a);
                            Vector2 fdir = { baseDir.x * c - baseDir.y * s,
                                             baseDir.x * s + baseDir.y * c };
                            fireProjectile(projectiles, playerPos, fdir, w.projectileSpeed, w.damage);
                        }
                        fireCooldown = w.cooldown;
                    }
                }

                updateProjectiles(projectiles, enemies, dt, isWalkable, kCellPx);

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

                if (IsKeyPressed(KEY_U)) {
                    if (auto snap = undoStack.pop(); snap) {
                        playerPos = snap->playerPos;
                        activeRoom = snap->activeRoom;
                        prevActiveRoom = snap->activeRoom;
                        playerHp = snap->hp;
                        damageCooldown = 0.5f;
                    }
                }

                if (playerHp <= 0) {
                    if (auto snap = undoStack.pop(); snap) {
                        playerPos = snap->playerPos;
                        activeRoom = snap->activeRoom;
                        prevActiveRoom = snap->activeRoom;
                        playerHp = snap->hp;
                        damageCooldown = 0.5f;
                    } else {
                        state = GameState::GameOver;
                        gameOverSel = 0;
                    }
                }

                if (state == GameState::Playing && activeRoom >= 0 && activeRoom != prevActiveRoom
                    && isCleared(waves, activeRoom)) {
                    undoStack.push({playerPos, activeRoom, playerHp});
                }
                prevActiveRoom = activeRoom;
                break;
            }

            case GameState::Paused: {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) pauseSel = cycleSel(pauseSel, +1, 2);
                if (IsKeyPressed(KEY_ESCAPE)) state = GameState::Playing;
                if (IsKeyPressed(KEY_ENTER)) {
                    if (pauseSel == 0) state = GameState::Playing;
                    else state = GameState::Title;
                }
                break;
            }

            case GameState::ReadingLetter: {
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) {
                    state = GameState::Playing;
                }
                break;
            }

            case GameState::GameOver: {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) gameOverSel = cycleSel(gameOverSel, +1, 2);
                if (IsKeyPressed(KEY_ESCAPE)) state = GameState::Title;
                if (IsKeyPressed(KEY_ENTER)) {
                    if (gameOverSel == 0) state = GameState::Title;
                    else state = GameState::Leaderboard;
                }
                break;
            }

            case GameState::Leaderboard: {
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) state = GameState::Title;
                break;
            }
        }

        camera.target = playerPos;

        BeginDrawing();
        ClearBackground(kBackground);

        const bool worldVisible =
            state == GameState::Playing || state == GameState::Paused
            || state == GameState::ReadingLetter || state == GameState::GameOver;

        if (worldVisible) {
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
            const char* stateName = "—";
            if (activeRoom >= 0) {
                switch (stateOf(waves, activeRoom)) {
                    case RoomState::Untouched:  stateName = "untouched";  break;
                    case RoomState::InProgress: stateName = "in-progress"; break;
                    case RoomState::Cleared:    stateName = "cleared";    break;
                }
            }
            DrawText(TextFormat("Room: %s (%s)   HP: %d/%d",
                                roomName, stateName, playerHp, kPlayerMaxHp),
                     16, 16, 24, kPlayer);
            DrawText(TextFormat("[checkpoints: %d]   Weapon: %s   [/]=cycle  U=rewind  L=letter  Esc=pause",
                                undoStack.size(),
                                inv.empty() ? "—" : inv.current().name.c_str()),
                     16, 44, 20, kPlayer);
        }

        switch (state) {
            case GameState::Title: {
                drawCentered("THE WITCHER DUNGEON", 120, 56, kPlayer);
                drawCentered("a DSA dungeon crawl", 184, 22, kMenuFade);

                const char* items[4] = { "Start", "Leaderboard", "Difficulty", "Quit" };
                for (int i = 0; i < 4; i++) {
                    Color c = (i == titleSel) ? kPlayer : kMenuFade;
                    char buf[64];
                    if (i == 2) std::snprintf(buf, sizeof(buf), "Difficulty: < %s >", difficultyName(difficulty));
                    else        std::snprintf(buf, sizeof(buf), "%s", items[i]);
                    drawCentered(buf, 320 + i * 56, 32, c);
                }
                drawCentered("up/down + enter   left/right adjusts difficulty", 660, 18, kMenuFade);
                break;
            }

            case GameState::Paused: {
                DrawRectangle(0, 0, kWidth, kHeight, kMenuDim);
                drawCentered("PAUSED", 200, 64, kPlayer);
                const char* items[2] = { "Resume", "Quit to Title" };
                for (int i = 0; i < 2; i++) {
                    Color c = (i == pauseSel) ? kPlayer : kMenuFade;
                    drawCentered(items[i], 360 + i * 56, 32, c);
                }
                drawCentered("Esc = resume", 600, 18, kMenuFade);
                break;
            }

            case GameState::ReadingLetter: {
                DrawRectangle(0, 0, kWidth, kHeight, kMenuDim);
                int boxW = 800, boxH = 400;
                int boxX = (kWidth - boxW) / 2, boxY = (kHeight - boxH) / 2;
                DrawRectangle(boxX, boxY, boxW, boxH, kBackground);
                DrawRectangleLines(boxX, boxY, boxW, boxH, kPlayer);
                drawCentered("WITCHER ORDER  (placeholder)", boxY + 32, 28, kPlayer);
                drawCentered("Letter contents will be generated by LLM at step 9.", boxY + 120, 20, kMenuFade);
                drawCentered("This is a stub to exercise the ReadingLetter state.", boxY + 156, 20, kMenuFade);
                drawCentered("Press E or Esc or Enter to dismiss", boxY + boxH - 56, 18, kMenuFade);
                break;
            }

            case GameState::GameOver: {
                DrawRectangle(0, 0, kWidth, kHeight, kMenuDim);
                drawCentered("GAME OVER", 180, 72, RED);
                drawCentered("Score: 0  (placeholder, lands at step 10)", 280, 22, kMenuFade);
                const char* items[2] = { "Back to Title", "View Leaderboard" };
                for (int i = 0; i < 2; i++) {
                    Color c = (i == gameOverSel) ? kPlayer : kMenuFade;
                    drawCentered(items[i], 400 + i * 56, 32, c);
                }
                break;
            }

            case GameState::Leaderboard: {
                drawCentered("LEADERBOARD", 120, 56, kPlayer);
                drawCentered("(empty — heap lands at step 10)", 220, 22, kMenuFade);
                drawCentered("Esc / Enter to return", 660, 20, kMenuFade);
                break;
            }

            case GameState::Playing: break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
