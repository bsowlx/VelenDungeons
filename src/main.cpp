#include "raylib.h"
#include "raymath.h"
#include "dungeon.h"
#include "undo.h"
#include "collision.h"
#include "enemy.h"
#include "projectile.h"
#include "wave.h"
#include "inventory.h"
#include "signs.h"
#include "leaderboard.h"
#include "pcg.h"
#include "grid.h"
#include "player.h"
#include "palette.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

int main() {
    constexpr int kWidth  = 1280;
    constexpr int kHeight = 720;

    InitWindow(kWidth, kHeight, "The Witcher Dungeon");
    SetTargetFPS(60);
    SetExitKey(0);  // Esc is now a normal key — state-machine handles it

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

    // Placeholder letter content per level. LLM-generated copy lands later.
    struct LetterContent { const char* title; const char* line1; const char* line2; };
    auto letterFor = [](int lvl) -> LetterContent {
        switch (lvl) {
            case 1: return {"WITCHER ORDER  —  LEVEL 1", "Bandits raid the lower vaults.", "Drive them out before nightfall."};
            case 2: return {"WITCHER ORDER  —  LEVEL 2", "The waters grow restless.",       "Drowners crawl from the marsh."};
            case 3: return {"WITCHER ORDER  —  LEVEL 3", "The fens have turned foul.",      "Ghouls and drowners walk together."};
            case 4: return {"WITCHER ORDER  —  LEVEL 4", "A berserker warband descends.",   "Beware their charge."};
            case 5: return {"WITCHER ORDER  —  LEVEL 5", "Only Quen will keep you alive.",  "Use it sparingly."};
            default:return {"WITCHER ORDER",             "The road grows darker.",          "Press on."};
        }
    };

    // Run state — reset by initRun on Title->Playing.
    Vector2 playerPos;
    int playerHp;
    float damageCooldown;
    float fireCooldown;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    WaveState waves;
    Inventory inv;
    UndoStack undoStack;
    SignState signs;
    int prevActiveRoom;
    int activeRoom;
    int kills;
    int currentLevel;
    int hearts;
    float exitHintTimer;
    std::uint32_t runSeed;
    LevelData level;

    auto loadLevel = [&](int lvl) {
        level = generateLevel(lvl, runSeed ^ ((std::uint32_t)lvl * 2654435761u));

        std::vector<const char*> rowPtrs;
        rowPtrs.reserve(level.tileRows.size());
        for (const std::string& s : level.tileRows) rowPtrs.push_back(s.c_str());
        if (std::string err = level.dungeon.validate(rowPtrs.data(), kRows, kCols);
            !err.empty()) {
            std::fprintf(stderr, "PCG validation failed (lvl %d): %s\n", lvl, err.c_str());
        }

        playerPos = level.playerSpawn;
        enemies.clear();
        projectiles.clear();
        waves = {};
        initWaves(waves, level.dungeon.roomCount(), {0});  // antechamber starts cleared
        undoStack = {};
        signs = {};
        prevActiveRoom = -1;
        activeRoom = -1;
        damageCooldown = 0.0f;
        fireCooldown = 0.0f;
        exitHintTimer = 0.0f;
    };

    auto initRun = [&]() {
        runSeed = (std::uint32_t)std::time(nullptr);
        currentLevel = 1;
        kills = 0;
        playerHp = kPlayerMaxHp;
        hearts = 3;
        inv = {};
        inv.add({"Pistol",   0.20f, 1, 400.0f, 1,  0.0f});
        inv.add({"Shotgun",  0.55f, 1, 350.0f, 3, 30.0f});
        inv.add({"Crossbow", 0.45f, 2, 550.0f, 1,  0.0f});
        loadLevel(currentLevel);
    };

    auto nextLevel = [&]() {
        currentLevel++;
        playerHp = std::min(playerHp + 1, kPlayerMaxHp);
        hearts = 3;
        loadLevel(currentLevel);
    };

    initRun();

    auto isWalkable = [&](int x, int y) -> bool {
        if (x < 0 || x >= kCols || y < 0 || y >= kRows) return false;
        return level.tileRows[y][x] == '.';
    };

    GameState state = GameState::Title;
    Difficulty difficulty = Difficulty::Normal;
    int titleSel = 0;     // 0=Start, 1=Leaderboard, 2=Difficulty, 3=Quit
    int pauseSel = 0;     // 0=Resume, 1=Quit to Title
    int gameOverSel = 0;  // 0=Title, 1=Leaderboard
    bool quit = false;

    Leaderboard leaderboard;  // session-scoped, survives initRun
    int lastScore = 0;

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
                if (IsKeyPressed(KEY_N))      { nextLevel(); break; }

                if (exitHintTimer > 0.0f) exitHintTimer -= dt;

                {
                    int tx = (int)(playerPos.x / (float)kCellPx);
                    int ty = (int)(playerPos.y / (float)kCellPx);
                    if (IsKeyPressed(KEY_E) && tx == level.letterTile.x && ty == level.letterTile.y) {
                        state = GameState::ReadingLetter;
                        break;
                    }
                    if (IsKeyPressed(KEY_E) && tx == level.exitTile.x && ty == level.exitTile.y) {
                        int lastRoomId = level.dungeon.roomCount() - 1;
                        if (stateOf(waves, lastRoomId) == RoomState::Cleared) {
                            nextLevel();
                            break;
                        } else {
                            exitHintTimer = 1.5f;
                        }
                    }
                }

                if (IsKeyPressed(KEY_ONE))   signs.aardLevel = 1;
                if (IsKeyPressed(KEY_TWO))   signs.aardLevel = 2;
                if (IsKeyPressed(KEY_THREE)) signs.aardLevel = 3;

                if (IsKeyPressed(KEY_Q)) {
                    Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
                    Vector2 toMouse = Vector2Subtract(mw, playerPos);
                    if (Vector2Length(toMouse) > 0.001f) {
                        Vector2 aim = Vector2Normalize(toMouse);
                        castAard(signs, enemies, playerPos, aim, activeRoom,
                                 isWalkable, kCellPx);
                    }
                }
                if (IsKeyPressed(KEY_F)) castQuen(signs);

                tickSigns(signs, dt);

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
                activeRoom = level.dungeon.roomAt(playerTx, playerTy);

                if (activeRoom >= 0 && activeRoom != prevActiveRoom) {
                    onEnterRoom(waves, activeRoom, level.rosters);
                }

                updateWaves(waves, enemies, activeRoom, dt);
                updateEnemies(enemies, projectiles, playerPos, activeRoom, dt, isWalkable, kCellPx, kCols, kRows);

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

                updateProjectiles(projectiles, enemies, dt, isWalkable, kCellPx, kills);

                if (damageCooldown > 0.0f) damageCooldown -= dt;
                Rectangle pr = { playerPos.x - kPlayerHalf, playerPos.y - kPlayerHalf,
                                 kPlayerSize, kPlayerSize };
                if (damageCooldown <= 0.0f) {
                    for (const Enemy& e : enemies) {
                        if (!e.alive || e.roomId != activeRoom) continue;
                        Rectangle er = { e.pos.x - kEnemyHalf, e.pos.y - kEnemyHalf,
                                         kEnemySize, kEnemySize };
                        if (CheckCollisionRecs(pr, er)) {
                            int dmg = contactDamage(e);
                            if (signs.shieldHp > 0) signs.shieldHp--;
                            else                    playerHp -= dmg;
                            damageCooldown = 0.5f;
                            break;
                        }
                    }
                }

                bool projHitThisFrame = false;
                for (Projectile& p : projectiles) {
                    if (!p.alive || !p.damagesPlayer) continue;
                    Rectangle prj = { p.pos.x - kProjectileHalf, p.pos.y - kProjectileHalf,
                                      kProjectileSize, kProjectileSize };
                    if (!CheckCollisionRecs(pr, prj)) continue;
                    if (!projHitThisFrame && damageCooldown <= 0.0f) {
                        if (signs.shieldHp > 0) signs.shieldHp--;
                        else                    playerHp -= p.damage;
                        damageCooldown = 0.5f;
                        projHitThisFrame = true;
                    }
                    p.alive = false;
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
                    hearts--;
                    if (hearts <= 0) {
                        state = GameState::GameOver;
                        gameOverSel = 0;
                        lastScore = kills * 100 * currentLevel;
                        leaderboard.submit(lastScore);
                    } else if (auto snap = undoStack.pop(); snap) {
                        playerPos = snap->playerPos;
                        activeRoom = snap->activeRoom;
                        prevActiveRoom = snap->activeRoom;
                        playerHp = snap->hp;
                        damageCooldown = 0.5f;
                    } else {
                        // Hearts left but no checkpoint — fall back to antechamber spawn.
                        playerPos = level.playerSpawn;
                        activeRoom = -1;
                        prevActiveRoom = -1;
                        playerHp = kPlayerMaxHp;
                        damageCooldown = 0.5f;
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
                    break;
                }

                // World tick (enemies / projectiles / signs / waves) is paused while reading,
                // but player movement stays live so walking off the letter tile dismisses.
                Vector2 dir = { 0.0f, 0.0f };
                if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
                if (IsKeyDown(KEY_S)) dir.y += 1.0f;
                if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
                if (IsKeyDown(KEY_D)) dir.x += 1.0f;
                if (dir.x != 0.0f || dir.y != 0.0f) dir = Vector2Normalize(dir);
                float step = kPlayerSpeed * dt;
                resolveX(playerPos, dir.x * step, kPlayerHalf, isWalkable, kCellPx);
                resolveY(playerPos, dir.y * step, kPlayerHalf, isWalkable, kCellPx);

                int tx = (int)(playerPos.x / (float)kCellPx);
                int ty = (int)(playerPos.y / (float)kCellPx);
                if (tx != level.letterTile.x || ty != level.letterTile.y) {
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
                    Color c = (level.tileRows[y][x] == '#') ? kWall : kFloor;
                    if (level.tileRows[y][x] == '.') {
                        if (x == level.letterTile.x && y == level.letterTile.y) {
                            c = Fade(kPlayer, 0.4f);  // letter pickup glow (gold)
                        } else if (x == level.exitTile.x && y == level.exitTile.y) {
                            c = Fade(RED, 0.4f);      // level-exit glow (red)
                        }
                    }
                    DrawRectangle(x * kCellPx + 1, y * kCellPx + 1,
                                  kCellPx - 2, kCellPx - 2, c);
                }
            }

            for (const Enemy& e : enemies) {
                if (!e.alive) continue;
                Color ec = (e.stunTimer > 0.0f) ? Color{0x60, 0x80, 0xc0, 0xff}
                                                : kEnemyKinds[(int)e.kind].color;
                DrawRectangle((int)(e.pos.x - kEnemyHalf), (int)(e.pos.y - kEnemyHalf),
                              (int)kEnemySize, (int)kEnemySize, ec);
                if (e.kind == EnemyKind::Berserk && e.phase == BerserkPhase::WindingUp) {
                    float t = 1.0f - (e.phaseTimer / kBerserkWindupDuration);
                    Color rc = Fade(RED, 0.4f + 0.4f * t);
                    DrawRing(e.pos, kEnemyHalf + 4.0f, kEnemyHalf + 10.0f,
                             0.0f, 360.0f, 24, rc);
                }
            }

            if (signs.aardCastVis > 0.0f) {
                float t = signs.aardCastVis / kAardCastVisTime;
                float aimAng = std::atan2(signs.aardLastDir.y, signs.aardLastDir.x) * (180.0f / PI);
                Color cc = Fade(kPlayer, 0.4f * t);
                DrawCircleSector(playerPos, signs.aardLastRange,
                                 aimAng - kAardHalfAngleDeg, aimAng + kAardHalfAngleDeg,
                                 16, cc);
            }

            for (const Projectile& p : projectiles) {
                Color pc = p.damagesPlayer ? kEnemyKinds[(int)EnemyKind::Drowner].color
                                           : kProjectile;
                DrawRectangle((int)(p.pos.x - kProjectileHalf), (int)(p.pos.y - kProjectileHalf),
                              (int)kProjectileSize, (int)kProjectileSize, pc);
            }

            DrawRectangle((int)(playerPos.x - kPlayerHalf), (int)(playerPos.y - kPlayerHalf),
                          (int)kPlayerSize, (int)kPlayerSize, kPlayer);

            if (signs.shieldHp > 0) {
                Color rc = Fade(SKYBLUE, 0.7f);
                DrawRing(playerPos, kPlayerHalf + 4.0f, kPlayerHalf + 8.0f,
                         0.0f, 360.0f, 24, rc);
            }

            EndMode2D();

            const char* roomName = (activeRoom >= 0) ? level.dungeon.room(activeRoom).name.c_str() : "—";
            const char* stateName = "—";
            if (activeRoom >= 0) {
                switch (stateOf(waves, activeRoom)) {
                    case RoomState::Untouched:  stateName = "untouched";  break;
                    case RoomState::InProgress: stateName = "in-progress"; break;
                    case RoomState::Cleared:    stateName = "cleared";    break;
                }
            }
            DrawText(TextFormat("Level %d   Hearts: %d/3   Room: %s (%s)   HP: %d/%d   Kills: %d",
                                currentLevel, hearts, roomName, stateName,
                                playerHp, kPlayerMaxHp, kills),
                     16, 16, 24, kPlayer);

            if (exitHintTimer > 0.0f) {
                drawCentered("Clear the room first.", kHeight - 80, 24, RED);
            }
            DrawText(TextFormat("[checkpoints: %d]   Weapon: %s   [/]=cycle  U=rewind  E=letter/exit  N=debug-next  Esc=pause",
                                undoStack.size(),
                                inv.empty() ? "—" : inv.current().name.c_str()),
                     16, 44, 20, kPlayer);
            DrawText(TextFormat("Aard L%d  cd %.1fs  |  Quen %d/%d  t %.1fs  cd %.1fs  |  Q=Aard F=Quen 1/2/3=tier",
                                signs.aardLevel,
                                signs.aardCd > 0.0f ? signs.aardCd : 0.0f,
                                signs.shieldHp, kQuenShieldHp,
                                signs.shieldTimer > 0.0f ? signs.shieldTimer : 0.0f,
                                signs.quenCd > 0.0f ? signs.quenCd : 0.0f),
                     16, 68, 18, kPlayer);
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
                // Modal lives at the top so the camera-centered player stays visible
                // while moving (step-off is a dismiss trigger).
                int boxW = 900, boxH = 200;
                int boxX = (kWidth - boxW) / 2, boxY = 24;
                DrawRectangle(boxX, boxY, boxW, boxH, kBackground);
                DrawRectangleLines(boxX, boxY, boxW, boxH, kPlayer);
                LetterContent lc = letterFor(currentLevel);
                drawCentered(lc.title,  boxY + 28, 28, kPlayer);
                drawCentered(lc.line1,  boxY + 84, 20, kPlayer);
                drawCentered(lc.line2,  boxY + 116, 20, kPlayer);
                drawCentered("Step off the marker, or press E / Esc / Enter, to dismiss",
                             boxY + boxH - 32, 16, kMenuFade);
                break;
            }

            case GameState::GameOver: {
                DrawRectangle(0, 0, kWidth, kHeight, kMenuDim);
                drawCentered("GAME OVER", 180, 72, RED);
                drawCentered(TextFormat("Kills: %d   Score: %d", kills, lastScore),
                             280, 26, kPlayer);
                const char* items[2] = { "Back to Title", "View Leaderboard" };
                for (int i = 0; i < 2; i++) {
                    Color c = (i == gameOverSel) ? kPlayer : kMenuFade;
                    drawCentered(items[i], 400 + i * 56, 32, c);
                }
                break;
            }

            case GameState::Leaderboard: {
                drawCentered("LEADERBOARD", 120, 56, kPlayer);
                if (leaderboard.empty()) {
                    drawCentered("(no runs yet — die to submit a score)", 220, 22, kMenuFade);
                } else {
                    auto top = leaderboard.top(10);
                    for (int i = 0; i < (int)top.size(); i++) {
                        drawCentered(TextFormat("%2d.   %d", i + 1, top[i]),
                                     220 + i * 36, 24, kPlayer);
                    }
                }
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
