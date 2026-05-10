#include "enemy.h"
#include "projectile.h"
#include "pathfind.h"
#include "raymath.h"
#include <cmath>

static bool hasLineOfSight(Vector2 from, Vector2 to,
                           const IsWalkableFn& walkable, int cellPx) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.001f) return true;
    int steps = (int)(dist / ((float)cellPx * 0.25f)) + 1;
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        int tx = (int)((from.x + dx * t) / (float)cellPx);
        int ty = (int)((from.y + dy * t) / (float)cellPx);
        if (!walkable(tx, ty)) return false;
    }
    return true;
}

static void chaseTowards(Enemy& e, float step, Vector2 playerPos,
                         const IsWalkableFn& walkable, int cellPx,
                         int cols, int rows) {
    int playerTx = (int)(playerPos.x / (float)cellPx);
    int playerTy = (int)(playerPos.y / (float)cellPx);
    int enemyTx  = (int)(e.pos.x   / (float)cellPx);
    int enemyTy  = (int)(e.pos.y   / (float)cellPx);

    Vector2 target = playerPos;
    if (enemyTx != playerTx || enemyTy != playerTy) {
        std::vector<TileCoord> path = findPath({enemyTx, enemyTy},
                                               {playerTx, playerTy},
                                               walkable, cols, rows);
        if (path.size() >= 2) {
            TileCoord next = path[1];
            target = { (next.x + 0.5f) * (float)cellPx,
                       (next.y + 0.5f) * (float)cellPx };
        }
    }

    Vector2 toTarget = { target.x - e.pos.x, target.y - e.pos.y };
    if (toTarget.x == 0.0f && toTarget.y == 0.0f) return;
    Vector2 dir = Vector2Normalize(toTarget);
    resolveX(e.pos, dir.x * step, kEnemyHalf, walkable, cellPx);
    resolveY(e.pos, dir.y * step, kEnemyHalf, walkable, cellPx);
}

static void updateGhoul(Enemy& e, Vector2 playerPos, float dt,
                        const IsWalkableFn& walkable, int cellPx,
                        int cols, int rows) {
    float step = kEnemyKinds[(int)EnemyKind::Ghoul].speed * dt;
    chaseTowards(e, step, playerPos, walkable, cellPx, cols, rows);
}

static void updateDrowner(Enemy& e, std::vector<Projectile>& projectiles,
                          Vector2 playerPos, float dt,
                          const IsWalkableFn& walkable, int cellPx,
                          int cols, int rows) {
    if (e.fireCooldown > 0.0f) e.fireCooldown -= dt;

    Vector2 toPlayer = { playerPos.x - e.pos.x, playerPos.y - e.pos.y };
    float dist = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

    bool hasLOS = dist <= kDrownerFireRange
                  && hasLineOfSight(e.pos, playerPos, walkable, cellPx);

    if (hasLOS) {
        if (e.fireCooldown <= 0.0f && dist > 0.001f) {
            Vector2 dir = { toPlayer.x / dist, toPlayer.y / dist };
            fireProjectile(projectiles, e.pos, dir,
                           kDrownerShotSpeed, kDrownerShotDamage, true);
            e.fireCooldown = kDrownerFireCooldown;
        }
    } else {
        float step = kEnemyKinds[(int)EnemyKind::Drowner].speed * dt;
        chaseTowards(e, step, playerPos, walkable, cellPx, cols, rows);
    }
}

void updateEnemies(std::vector<Enemy>& enemies,
                   std::vector<Projectile>& projectiles,
                   Vector2 playerPos,
                   int playerActiveRoom,
                   float dt,
                   const IsWalkableFn& walkable,
                   int cellPx,
                   int cols, int rows) {
    for (Enemy& e : enemies) {
        if (!e.alive) continue;
        if (e.roomId != playerActiveRoom) continue;
        if (e.stunTimer > 0.0f) { e.stunTimer -= dt; continue; }

        switch (e.kind) {
            case EnemyKind::Ghoul:
                updateGhoul(e, playerPos, dt, walkable, cellPx, cols, rows);
                break;
            case EnemyKind::Drowner:
                updateDrowner(e, projectiles, playerPos, dt, walkable, cellPx, cols, rows);
                break;
        }
    }
}

bool applyDamage(Enemy& e, int damage, int& kills) {
    if (!e.alive) return false;
    e.hp -= damage;
    if (e.hp <= 0) {
        e.alive = false;
        ++kills;
        return true;
    }
    return false;
}
