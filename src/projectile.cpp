#include "projectile.h"
#include <algorithm>
#include <cmath>

void fireProjectile(std::vector<Projectile>& projectiles, Vector2 origin,
                    Vector2 dir, float speed, int damage) {
    projectiles.push_back({
        origin,
        { dir.x * speed, dir.y * speed },
        damage,
        true,
    });
}

void updateProjectiles(std::vector<Projectile>& projectiles,
                       std::vector<Enemy>& enemies,
                       float dt,
                       const IsWalkableFn& walkable,
                       int cellPx,
                       int& kills) {
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
                applyDamage(e, p.damage, kills);
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
