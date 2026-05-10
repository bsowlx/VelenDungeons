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
    bool damagesPlayer;
};

// Speed and damage now come from the firing weapon (see inventory.h).
// damagesPlayer=true marks enemy-fired projectiles; their player-collision is
// resolved in main.cpp so the i-frame/shield path stays in one place.
void fireProjectile(std::vector<Projectile>& projectiles, Vector2 origin,
                    Vector2 dir, float speed, int damage,
                    bool damagesPlayer = false);

// Advances each projectile, kills it on a non-walkable tile. Player-fired
// projectiles also damage enemies on AABB overlap. Player-damaging projectiles
// skip the enemy pass; main.cpp handles the player hit. Dead projectiles are
// erased at the end. `kills` is incremented for each enemy that dies this frame.
void updateProjectiles(std::vector<Projectile>& projectiles,
                       std::vector<Enemy>& enemies,
                       float dt,
                       const IsWalkableFn& walkable,
                       int cellPx,
                       int& kills);
