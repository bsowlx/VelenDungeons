#include "signs.h"
#include "raymath.h"
#include <cmath>

float aardRangeFor(int level) {
    switch (level) {
        case 2: return kAardRangeL2;
        case 3: return kAardRangeL3;
        default: return kAardRangeL1;
    }
}

void castAard(SignState& s, std::vector<Enemy>& enemies, Vector2 playerPos,
              Vector2 aimDir, int activeRoom,
              const IsWalkableFn& walkable, int cellPx) {
    if (s.aardCd > 0.0f) return;

    s.aardCd        = kAardCooldown;
    s.aardLastDir   = aimDir;
    s.aardLastRange = aardRangeFor(s.aardLevel);
    s.aardCastVis   = kAardCastVisTime;

    const float range    = s.aardLastRange;
    const float cosHalf  = std::cos(kAardHalfAngleDeg * (PI / 180.0f));
    const Vector2 disp   = { aimDir.x * kAardKnockback, aimDir.y * kAardKnockback };

    for (Enemy& e : enemies) {
        if (!e.alive)               continue;
        if (e.roomId != activeRoom) continue;

        Vector2 toE = { e.pos.x - playerPos.x, e.pos.y - playerPos.y };
        float dist = std::sqrt(toE.x * toE.x + toE.y * toE.y);
        if (dist > range || dist < 0.0001f) continue;

        Vector2 dirE = { toE.x / dist, toE.y / dist };
        float cosAng = aimDir.x * dirE.x + aimDir.y * dirE.y;
        if (cosAng < cosHalf) continue;

        resolveX(e.pos, disp.x, kEnemyHalf, walkable, cellPx);
        resolveY(e.pos, disp.y, kEnemyHalf, walkable, cellPx);

        if (s.aardLevel >= 3) {
            e.stunTimer = kAardStunDuration;
        }
    }
}

void castQuen(SignState& s) {
    if (s.quenCd > 0.0f) return;
    s.quenCd      = kQuenCooldown;
    s.shieldHp    = kQuenShieldHp;
    s.shieldTimer = kQuenDuration;
}

void tickSigns(SignState& s, float dt) {
    if (s.aardCd > 0.0f)      s.aardCd      -= dt;
    if (s.aardCastVis > 0.0f) s.aardCastVis -= dt;
    if (s.quenCd > 0.0f)      s.quenCd      -= dt;
    if (s.shieldTimer > 0.0f) {
        s.shieldTimer -= dt;
        if (s.shieldTimer <= 0.0f) {
            s.shieldTimer = 0.0f;
            s.shieldHp    = 0;
        }
    }
}
