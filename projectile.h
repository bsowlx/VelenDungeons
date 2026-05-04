#pragma once

#include "raylib.h"
#include "collision.h"
#include "enemy.h"
#include <vector>

constexpr float kProjectileSize  = 8.0f;
constexpr float kProjectileHalf  = kProjectileSize / 2.0f;
constexpr float kProjectileSpeed = 400.0f;   // px/sec
constexpr float kFireCooldown    = 0.20f;    // seconds between shots
constexpr int   kProjectileDmg   = 1;

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    int damage;
    bool alive;
};

void fireProjectile(std::vector<Projectile>& projectiles, Vector2 origin, Vector2 dir);

// Advances each projectile, kills it on a non-walkable tile, kills the enemy
// (and the projectile) on AABB overlap. Dead projectiles are erased at the end.
void updateProjectiles(std::vector<Projectile>& projectiles,
                       std::vector<Enemy>& enemies,
                       float dt,
                       const IsWalkableFn& walkable,
                       int cellPx);
