#include "collision.h"
#include <cmath>

void resolveX(Vector2& pos, float dx, float halfSize,
              const IsWalkableFn& walkable, int cellPx) {
    if (dx == 0.0f) return;
    pos.x += dx;
    int minX = (int)std::floor((pos.x - halfSize) / (float)cellPx);
    int maxX = (int)std::floor((pos.x + halfSize - 0.0001f) / (float)cellPx);
    int minY = (int)std::floor((pos.y - halfSize) / (float)cellPx);
    int maxY = (int)std::floor((pos.y + halfSize - 0.0001f) / (float)cellPx);
    if (dx > 0) {
        for (int tx = minX; tx <= maxX; tx++)
            for (int ty = minY; ty <= maxY; ty++)
                if (!walkable(tx, ty)) { pos.x = tx * (float)cellPx - halfSize; return; }
    } else {
        for (int tx = maxX; tx >= minX; tx--)
            for (int ty = minY; ty <= maxY; ty++)
                if (!walkable(tx, ty)) { pos.x = (tx + 1) * (float)cellPx + halfSize; return; }
    }
}

void resolveY(Vector2& pos, float dy, float halfSize,
              const IsWalkableFn& walkable, int cellPx) {
    if (dy == 0.0f) return;
    pos.y += dy;
    int minX = (int)std::floor((pos.x - halfSize) / (float)cellPx);
    int maxX = (int)std::floor((pos.x + halfSize - 0.0001f) / (float)cellPx);
    int minY = (int)std::floor((pos.y - halfSize) / (float)cellPx);
    int maxY = (int)std::floor((pos.y + halfSize - 0.0001f) / (float)cellPx);
    if (dy > 0) {
        for (int ty = minY; ty <= maxY; ty++)
            for (int tx = minX; tx <= maxX; tx++)
                if (!walkable(tx, ty)) { pos.y = ty * (float)cellPx - halfSize; return; }
    } else {
        for (int ty = maxY; ty >= minY; ty--)
            for (int tx = minX; tx <= maxX; tx++)
                if (!walkable(tx, ty)) { pos.y = (ty + 1) * (float)cellPx + halfSize; return; }
    }
}
