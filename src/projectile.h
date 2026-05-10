#pragma once

#include "raylib.h"
#include "collision.h"
#include "enemy.h"
#include <vector>

constexpr float kProjectileSize = 8.0f;
constexpr float kProjectileHalf = kProjectileSize / 2.0f;

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    int damage;
    bool alive;
};

// Speed and damage now come from the firing weapon (see inventory.h).
void fireProjectile(std::vector<Projectile>& projectiles, Vector2 origin,
                    Vector2 dir, float speed, int damage);

// Advances each projectile, kills it on a non-walkable tile, kills the enemy
// (and the projectile) on AABB overlap. Dead projectiles are erased at the end.
// `kills` is incremented for each enemy that dies this frame.
void updateProjectiles(std::vector<Projectile>& projectiles,
                       std::vector<Enemy>& enemies,
                       float dt,
                       const IsWalkableFn& walkable,
                       int cellPx,
                       int& kills);
