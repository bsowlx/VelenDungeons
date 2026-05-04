#pragma once

#include "raylib.h"
#include "collision.h"
#include <vector>

constexpr float kEnemySize  = 24.0f;
constexpr float kEnemyHalf  = kEnemySize / 2.0f;
constexpr float kEnemySpeed = 80.0f;     // px/sec  =  half the player
constexpr int   kEnemyMaxHp = 1;

struct Enemy {
    Vector2 pos;
    int roomId;
    int hp;
    bool alive;
};

// Activation rule: an enemy chases the player only when it shares a room with
// them (matches the room-clear gameplay loop and previews spawn waves).
void updateEnemies(std::vector<Enemy>& enemies,
                   Vector2 playerPos,
                   int playerActiveRoom,
                   float dt,
                   const IsWalkableFn& walkable,
                   int cellPx);
