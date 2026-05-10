#pragma once

#include "collision.h"  // IsWalkableFn
#include "dungeon.h"    // TileCoord
#include <vector>

// 8-connected tile-grid A* with octile heuristic. Diagonals are blocked when
// either cardinal neighbor at the corner is non-walkable (no corner-cutting
// through walls). Returns the path inclusive [start, ..., goal]; empty vector
// if no path exists. Uses `std::priority_queue` as the min-heap.
std::vector<TileCoord> findPath(TileCoord start, TileCoord goal,
                                const IsWalkableFn& walkable,
                                int cols, int rows);
