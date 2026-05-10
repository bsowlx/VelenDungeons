#pragma once

#include "raylib.h"
#include "enemy.h"
#include "collision.h"
#include <vector>

constexpr float kAardCooldown      = 1.5f;
constexpr float kAardKnockback     = 64.0f;   // px instant displacement on hit
constexpr int   kAardSubsteps      = 8;       // split push into substeps so the AABB resolver can't tunnel
constexpr float kAardHalfAngleDeg  = 60.0f;
constexpr float kAardCastVisTime   = 0.2f;
constexpr float kAardRangeL1       = 80.0f;
constexpr float kAardRangeL2       = 140.0f;
constexpr float kAardRangeL3       = 140.0f;
constexpr float kAardStunDuration  = 1.0f;

constexpr float kQuenCooldown = 5.0f;
constexpr int   kQuenShieldHp = 3;
constexpr float kQuenDuration = 5.0f;

// Aard cast records its last direction + range so the cone can be drawn for
// kAardCastVisTime seconds. Quen tracks shield HP and a duration timer; the
// shield drops on either hitting zero HP or the timer expiring.
struct SignState {
    float   aardCd        = 0.0f;
    int     aardLevel     = 1;
    Vector2 aardLastDir   = {1.0f, 0.0f};
    float   aardLastRange = 0.0f;
    float   aardCastVis   = 0.0f;

    float   quenCd        = 0.0f;
    int     shieldHp      = 0;
    float   shieldTimer   = 0.0f;
};

float aardRangeFor(int level);

// Cone hit: enemies in the active room within range, within half-angle of the
// aim vector. Hit enemies are displaced along aim by kAardKnockback (clamped
// by walls) and at level 3 stunned for kAardStunDuration.
void castAard(SignState& s, std::vector<Enemy>& enemies, Vector2 playerPos,
              Vector2 aimDir, int activeRoom,
              const IsWalkableFn& walkable, int cellPx);

void castQuen(SignState& s);

void tickSigns(SignState& s, float dt);
