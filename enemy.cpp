#include "enemy.h"
#include "pathfind.h"
#include "raymath.h"

void updateEnemies(std::vector<Enemy>& enemies,
                   Vector2 playerPos,
                   int playerActiveRoom,
                   float dt,
                   const IsWalkableFn& walkable,
                   int cellPx,
                   int cols, int rows) {
    float step = kEnemySpeed * dt;
    int playerTx = (int)(playerPos.x / (float)cellPx);
    int playerTy = (int)(playerPos.y / (float)cellPx);

    for (Enemy& e : enemies) {
        if (!e.alive) continue;
        if (e.roomId != playerActiveRoom) continue;

        int enemyTx = (int)(e.pos.x / (float)cellPx);
        int enemyTy = (int)(e.pos.y / (float)cellPx);

        // Steer toward the next tile on the A* path; fall back to direct chase
        // when same tile, no path, or path collapses to a single step.
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
        if (toTarget.x == 0.0f && toTarget.y == 0.0f) continue;
        Vector2 dir = Vector2Normalize(toTarget);
        resolveX(e.pos, dir.x * step, kEnemyHalf, walkable, cellPx);
        resolveY(e.pos, dir.y * step, kEnemyHalf, walkable, cellPx);
    }
}
