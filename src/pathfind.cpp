#include "pathfind.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace {

struct PathNode {
    TileCoord t;
    float g;
    float f;
};

struct GreaterF {
    bool operator()(const PathNode& a, const PathNode& b) const { return a.f > b.f; }
};

constexpr float kCardCost = 1.0f;
const float kDiagCost = std::sqrt(2.0f);

float octile(TileCoord a, TileCoord b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    int big   = (dx > dy) ? dx : dy;
    int small = (dx < dy) ? dx : dy;
    return (float)(big - small) * kCardCost + (float)small * kDiagCost;
}

}  // namespace

std::vector<TileCoord> findPath(TileCoord start, TileCoord goal,
                                const IsWalkableFn& walkable,
                                int cols, int rows) {
    auto inBounds = [&](int x, int y) { return x >= 0 && x < cols && y >= 0 && y < rows; };
    if (!inBounds(start.x, start.y) || !inBounds(goal.x, goal.y)) return {};
    if (!walkable(start.x, start.y) || !walkable(goal.x, goal.y)) return {};

    constexpr float INF = std::numeric_limits<float>::infinity();
    std::vector<std::vector<float>> bestG(rows, std::vector<float>(cols, INF));
    std::vector<std::vector<TileCoord>> cameFrom(rows, std::vector<TileCoord>(cols, TileCoord{-1, -1}));

    std::priority_queue<PathNode, std::vector<PathNode>, GreaterF> open;
    bestG[start.y][start.x] = 0.0f;
    open.push({start, 0.0f, octile(start, goal)});

    static constexpr int dxs[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static constexpr int dys[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (!open.empty()) {
        PathNode cur = open.top();
        open.pop();
        if (cur.g > bestG[cur.t.y][cur.t.x]) continue;  // stale heap entry
        if (cur.t.x == goal.x && cur.t.y == goal.y) {
            std::vector<TileCoord> path;
            TileCoord t = goal;
            while (!(t.x == start.x && t.y == start.y)) {
                path.push_back(t);
                t = cameFrom[t.y][t.x];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }
        for (int i = 0; i < 8; i++) {
            int nx = cur.t.x + dxs[i];
            int ny = cur.t.y + dys[i];
            if (!inBounds(nx, ny)) continue;
            if (!walkable(nx, ny)) continue;
            bool diagonal = (dxs[i] != 0 && dys[i] != 0);
            if (diagonal) {
                if (!walkable(cur.t.x + dxs[i], cur.t.y) ||
                    !walkable(cur.t.x,         cur.t.y + dys[i])) {
                    continue;
                }
            }
            float stepCost = diagonal ? kDiagCost : kCardCost;
            float newG = cur.g + stepCost;
            if (newG < bestG[ny][nx]) {
                bestG[ny][nx] = newG;
                cameFrom[ny][nx] = cur.t;
                open.push({{nx, ny}, newG, newG + octile({nx, ny}, goal)});
            }
        }
    }
    return {};
}
