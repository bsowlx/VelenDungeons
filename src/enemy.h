#pragma once

#include "raylib.h"
#include "collision.h"
#include <vector>

struct Projectile;  // defined in projectile.h

enum class EnemyKind { Ghoul, Drowner, Berserk };

struct EnemyKindData {
    int   maxHp;
    float speed;   // px/sec
    float size;    // px (square AABB)
    Color color;
};

constexpr EnemyKindData kEnemyKinds[] = {
    /* Ghoul   */ { 1, 140.0f, 24.0f, { 0xa0, 0x40, 0x40, 0xff } },
    /* Drowner */ { 2, 140.0f, 24.0f, { 0x6a, 0x8a, 0x40, 0xff } },
    /* Berserk */ { 4, 140.0f, 24.0f, { 0x60, 0x18, 0x18, 0xff } },
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

constexpr float kBerserkChargeRange    = 320.0f;  // 10 tiles — bull commits from far
constexpr float kBerserkChargeCooldown = 2.0f;    // chases slowly between charges
constexpr float kBerserkWindupDuration = 0.8f;
constexpr float kBerserkStrikeDuration = 3.0f;    // safety cap; wall stops strike in practice
constexpr float kBerserkSkidDuration   = 0.5f;    // decelerate over ~2 tiles after strike ends
constexpr float kBerserkRecoveryTime   = 0.7f;
constexpr float kBerserkStrikeSpeed    = 240.0f;
constexpr int   kBerserkStrikeDamage   = 2;

enum class BerserkPhase { Chasing, WindingUp, Striking, Skidding, Recovery };

struct Enemy {
    Vector2 pos;
    int roomId;
    int hp;
    bool alive;
    float stunTimer = 0.0f;
    EnemyKind kind = EnemyKind::Ghoul;
    float fireCooldown = 0.0f;
    BerserkPhase phase = BerserkPhase::Chasing;
    float phaseTimer = 0.0f;
    Vector2 strikeDir = {0.0f, 0.0f};
    float chargeCd = 0.0f;
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

// Per-enemy contact damage. Berserk mid-Strike hits for kBerserkStrikeDamage;
// every other case hits for 1.
int contactDamage(const Enemy& e);
