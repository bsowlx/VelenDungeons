#include "enemy.h"
#include "raymath.h"

void updateEnemies(std::vector<Enemy>& enemies,
                   Vector2 playerPos,
                   int playerActiveRoom,
                   float dt,
                   const IsWalkableFn& walkable,
                   int cellPx) {
    float step = kEnemySpeed * dt;
    for (Enemy& e : enemies) {
        if (!e.alive) continue;
        if (e.roomId != playerActiveRoom) continue;
        Vector2 toPlayer = { playerPos.x - e.pos.x, playerPos.y - e.pos.y };
        if (toPlayer.x == 0.0f && toPlayer.y == 0.0f) continue;
        Vector2 dir = Vector2Normalize(toPlayer);
        resolveX(e.pos, dir.x * step, kEnemyHalf, walkable, cellPx);
        resolveY(e.pos, dir.y * step, kEnemyHalf, walkable, cellPx);
    }
}
