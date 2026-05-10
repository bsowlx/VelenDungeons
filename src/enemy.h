#pragma once

#include "raylib.h"
#include "collision.h"
#include <vector>

struct Projectile;  // defined in projectile.h

enum class EnemyKind { Ghoul, Drowner };

struct EnemyKindData {
    int   maxHp;
    float speed;   // px/sec
    float size;    // px (square AABB)
    Color color;
};

constexpr EnemyKindData kEnemyKinds[] = {
    /* Ghoul   */ { 1, 16.0f, 24.0f, { 0xa0, 0x40, 0x40, 0xff } },
    /* Drowner */ { 2, 12.0f, 24.0f, { 0x6a, 0x8a, 0x40, 0xff } },
};

// Ghoul-baseline aliases. Collision sites (signs.cpp, projectile.cpp) use these;
// safe while every kind shares the same AABB size.
constexpr float kEnemySize  = kEnemyKinds[(int)EnemyKind::Ghoul].size;
constexpr float kEnemyHalf  = kEnemySize / 2.0f;
constexpr float kEnemySpeed = kEnemyKinds[(int)EnemyKind::Ghoul].speed;
constexpr int   kEnemyMaxHp = kEnemyKinds[(int)EnemyKind::Ghoul].maxHp;

constexpr float kDrownerFireRange    = 200.0f;
constexpr float kDrownerFireCooldown = 1.2f;
constexpr float kDrownerShotSpeed    = 250.0f;
constexpr int   kDrownerShotDamage   = 1;

struct Enemy {
    Vector2 pos;
    int roomId;
    int hp;
    bool alive;
    float stunTimer = 0.0f;
    EnemyKind kind = EnemyKind::Ghoul;
    float fireCooldown = 0.0f;
};

// Dispatches per-kind: ghouls chase via tile-grid A* when sharing a room with
// the player; drowners chase out of range and stop-and-shoot when in range,
// pushing into `projectiles` with damagesPlayer=true.
void updateEnemies(std::vector<Enemy>& enemies,
                   std::vector<Projectile>& projectiles,
                   Vector2 playerPos,
                   int playerActiveRoom,
                   float dt,
                   const IsWalkableFn& walkable,
                   int cellPx,
                   int cols, int rows);

bool applyDamage(Enemy& e, int damage, int& kills);
