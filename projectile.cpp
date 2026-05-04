#include "projectile.h"
#include <algorithm>
#include <cmath>

void fireProjectile(std::vector<Projectile>& projectiles, Vector2 origin, Vector2 dir) {
    projectiles.push_back({
        origin,
        { dir.x * kProjectileSpeed, dir.y * kProjectileSpeed },
        kProjectileDmg,
        true,
    });
}

void updateProjectiles(std::vector<Projectile>& projectiles,
                       std::vector<Enemy>& enemies,
                       float dt,
                       const IsWalkableFn& walkable,
                       int cellPx) {
    for (Projectile& p : projectiles) {
        if (!p.alive) continue;

        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;

        int tx = (int)std::floor(p.pos.x / (float)cellPx);
        int ty = (int)std::floor(p.pos.y / (float)cellPx);
        if (!walkable(tx, ty)) {
            p.alive = false;
            continue;
        }

        Rectangle pr = { p.pos.x - kProjectileHalf, p.pos.y - kProjectileHalf,
                         kProjectileSize, kProjectileSize };
        for (Enemy& e : enemies) {
            if (!e.alive) continue;
            Rectangle er = { e.pos.x - kEnemyHalf, e.pos.y - kEnemyHalf,
                             kEnemySize, kEnemySize };
            if (CheckCollisionRecs(pr, er)) {
                e.hp -= p.damage;
                if (e.hp <= 0) e.alive = false;
                p.alive = false;
                break;
            }
        }
    }
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
                       [](const Projectile& p) { return !p.alive; }),
        projectiles.end());
}
