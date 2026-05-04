#pragma once

#include "raylib.h"
#include <functional>

using IsWalkableFn = std::function<bool(int, int)>;

// Discrete axis-separated AABB-vs-tile resolution. Move along one axis, then
// snap out of any non-walkable tile the AABB now overlaps. The scan range is
// at most 2x2 tiles when halfSize <= cellPx/2 (true for player and enemies).
void resolveX(Vector2& pos, float dx, float halfSize,
              const IsWalkableFn& walkable, int cellPx);
void resolveY(Vector2& pos, float dy, float halfSize,
              const IsWalkableFn& walkable, int cellPx);
