#include "pcg.h"
#include "grid.h"
#include "enemy.h"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kMinRoomW       = 11;
constexpr int kMaxRoomW       = 16;
constexpr int kMinRoomH       = 8;
constexpr int kMaxRoomH       = 12;
constexpr int kRoomPad        = 2;
constexpr int kBorder         = 1;
constexpr int kMaxAttempts    = 200;
constexpr int kMaxRestarts    = 50;
constexpr int kMaxCorridorLen = 15;  // Manhattan distance between consecutive room centers

using TileGrid = std::vector<std::string>;

inline bool inBounds(int x, int y) {
    return x >= 0 && x < kCols && y >= 0 && y < kRows;
}
inline void carve(TileGrid& g, int x, int y) {
    if (inBounds(x, y)) g[y][x] = '.';
}
inline bool isFloor(const TileGrid& g, int x, int y) {
    return inBounds(x, y) && g[y][x] == '.';
}

bool overlap(const Rect& a, const Rect& b, int pad) {
    return !(a.maxX + pad < b.minX || b.maxX + pad < a.minX ||
             a.maxY + pad < b.minY || b.maxY + pad < a.minY);
}

TileCoord roomCenter(const Rect& r) {
    return { (r.minX + r.maxX) / 2, (r.minY + r.maxY) / 2 };
}

bool packRooms(int n, std::mt19937& rng, std::vector<Rect>& out) {
    out.clear();
    for (int i = 0; i < n; i++) {
        bool placed = false;
        for (int attempt = 0; attempt < kMaxAttempts && !placed; attempt++) {
            std::uniform_int_distribution<int> dw(kMinRoomW, kMaxRoomW);
            std::uniform_int_distribution<int> dh(kMinRoomH, kMaxRoomH);
            int w = dw(rng);
            int h = dh(rng);
            std::uniform_int_distribution<int> dx(kBorder + 1, kCols - kBorder - 1 - w);
            std::uniform_int_distribution<int> dy(kBorder + 1, kRows - kBorder - 1 - h);
            int x = dx(rng);
            int y = dy(rng);
            Rect r{ x, y, x + w - 1, y + h - 1 };
            bool clash = false;
            for (const Rect& other : out) {
                if (overlap(r, other, kRoomPad)) { clash = true; break; }
            }
            if (!clash && !out.empty()) {
                TileCoord pc = roomCenter(out.back());
                TileCoord rc = roomCenter(r);
                if (std::abs(pc.x - rc.x) + std::abs(pc.y - rc.y) > kMaxCorridorLen) {
                    clash = true;
                }
            }
            if (!clash) { out.push_back(r); placed = true; }
        }
        if (!placed) return false;
    }
    return true;
}

void carveRoom(TileGrid& g, const Rect& r) {
    for (int y = r.minY; y <= r.maxY; y++)
        for (int x = r.minX; x <= r.maxX; x++)
            carve(g, x, y);
}

struct CorridorDoors {
    TileCoord doorAtoB;
    TileCoord doorBtoA;
};

CorridorDoors carveCorridor(TileGrid& g, const Rect& a, const Rect& b, std::mt19937& rng) {
    TileCoord ac = roomCenter(a);
    TileCoord bc = roomCenter(b);

    std::uniform_int_distribution<int> coin(0, 1);
    bool xFirst = coin(rng) == 0;

    std::vector<TileCoord> path;
    auto walkX = [&](int x0, int x1, int y) {
        int step = (x1 >= x0) ? 1 : -1;
        for (int x = x0; x != x1; x += step) path.push_back({x, y});
    };
    auto walkY = [&](int y0, int y1, int x) {
        int step = (y1 >= y0) ? 1 : -1;
        for (int y = y0; y != y1; y += step) path.push_back({x, y});
    };

    if (xFirst) {
        walkX(ac.x, bc.x, ac.y);
        walkY(ac.y, bc.y, bc.x);
    } else {
        walkY(ac.y, bc.y, ac.x);
        walkX(ac.x, bc.x, bc.y);
    }
    path.push_back(bc);

    for (const TileCoord& t : path) carve(g, t.x, t.y);

    CorridorDoors out{ bc, ac };
    for (const TileCoord& t : path) {
        if (b.contains(t.x, t.y)) { out.doorAtoB = t; break; }
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (a.contains(it->x, it->y)) { out.doorBtoA = *it; break; }
    }
    return out;
}

bool nearDoor(int x, int y, const std::vector<TileCoord>& doors, int dist) {
    for (const TileCoord& d : doors) {
        if (std::abs(x - d.x) <= dist && std::abs(y - d.y) <= dist) return true;
    }
    return false;
}

void scatterPillars(TileGrid& g, const Rect& r,
                    const std::vector<TileCoord>& doors,
                    std::mt19937& rng) {
    int w = r.maxX - r.minX + 1;
    int h = r.maxY - r.minY + 1;
    int target = (w * h) / 9;
    int attempts = target * 5;
    std::uniform_int_distribution<int> dx(r.minX + 1, r.maxX - 1);
    std::uniform_int_distribution<int> dy(r.minY + 1, r.maxY - 1);
    int placed = 0;
    while (attempts-- > 0 && placed < target) {
        int x = dx(rng);
        int y = dy(rng);
        if (g[y][x] == '#') continue;
        if (nearDoor(x, y, doors, 1)) continue;
        g[y][x] = '#';
        placed++;
    }
}

void distributeEnemies(std::vector<std::vector<EnemySpawn>>& rosters,
                       const std::vector<Rect>& rects,
                       const LevelSpec& spec,
                       const TileGrid& g,
                       Vector2 playerSpawn,
                       std::mt19937& rng) {
    int n = (int)rects.size();
    rosters.assign(n, {});

    auto split = [&](int total) {
        std::vector<int> out(n, 0);
        for (int i = 0; i < total; i++) out[i % n]++;
        return out;
    };
    auto ghoulPer   = split(spec.ghouls);
    auto drownerPer = split(spec.drowners);
    auto berserkPer = split(spec.berserks);

    const float kMinSpawnDist = 3.0f * kCellPx;
    const float kMinSpawnDistSq = kMinSpawnDist * kMinSpawnDist;

    for (int i = 0; i < n; i++) {
        std::vector<TileCoord> used;
        std::uniform_int_distribution<int> dx(rects[i].minX, rects[i].maxX);
        std::uniform_int_distribution<int> dy(rects[i].minY, rects[i].maxY);

        auto place = [&](EnemyKind kind, int count) {
            for (int k = 0; k < count; k++) {
                for (int tries = 0; tries < 200; tries++) {
                    int x = dx(rng);
                    int y = dy(rng);
                    if (!isFloor(g, x, y)) continue;
                    float wx = (x + 0.5f) * kCellPx;
                    float wy = (y + 0.5f) * kCellPx;
                    float ddx = wx - playerSpawn.x;
                    float ddy = wy - playerSpawn.y;
                    if (ddx * ddx + ddy * ddy < kMinSpawnDistSq) continue;
                    bool clash = false;
                    for (const TileCoord& u : used) {
                        if (u.x == x && u.y == y) { clash = true; break; }
                    }
                    if (clash) continue;
                    used.push_back({x, y});
                    rosters[i].push_back({ {wx, wy}, kind });
                    break;
                }
            }
        };
        place(EnemyKind::Ghoul,   ghoulPer[i]);
        place(EnemyKind::Drowner, drownerPer[i]);
        place(EnemyKind::Berserk, berserkPer[i]);
    }
}

}  // namespace

LevelSpec specFor(int level) {
    if (level <= 1) return { 1, 3, 0, 0 };
    if (level == 2) return { 1, 2, 1, 0 };
    if (level == 3) return { 2, 4, 2, 0 };
    if (level == 4) return { 2, 4, 2, 2 };
    if (level == 5) return { 3, 6, 4, 2 };
    int extra = level - 5;
    return { 3, 6 + extra, 4 + extra, 2 + extra };
}

LevelData generateLevel(int level, std::uint32_t seed) {
    LevelData data;
    LevelSpec spec = specFor(level);

    std::mt19937 rng(seed);

    std::vector<Rect> rects;
    bool packed = false;
    for (int r = 0; r < kMaxRestarts && !packed; r++) {
        packed = packRooms(spec.roomCount, rng, rects);
    }
    if (!packed) {
        rects = { Rect{ 4, 4, 4 + kMinRoomW - 1, 4 + kMinRoomH - 1 } };
    }

    data.tileRows.assign(kRows, std::string(kCols, '#'));
    for (const Rect& r : rects) carveRoom(data.tileRows, r);

    std::vector<std::pair<TileCoord, TileCoord>> corridors;
    std::vector<TileCoord> allDoors;
    for (int i = 0; i + 1 < (int)rects.size(); i++) {
        CorridorDoors cd = carveCorridor(data.tileRows, rects[i], rects[i + 1], rng);
        corridors.push_back({ cd.doorAtoB, cd.doorBtoA });
        allDoors.push_back(cd.doorAtoB);
        allDoors.push_back(cd.doorBtoA);
    }

    for (const Rect& r : rects) scatterPillars(data.tileRows, r, allDoors, rng);

    for (int i = 0; i < (int)rects.size(); i++) {
        data.dungeon.addRoom("R" + std::to_string(i + 1), rects[i]);
    }
    for (int i = 0; i + 1 < (int)rects.size(); i++) {
        data.dungeon.addEdge(i, i + 1, corridors[i].first, corridors[i].second);
    }

    TileCoord spawn = roomCenter(rects[0]);
    data.playerSpawn = { (spawn.x + 0.5f) * kCellPx, (spawn.y + 0.5f) * kCellPx };

    distributeEnemies(data.rosters, rects, spec, data.tileRows, data.playerSpawn, rng);
    return data;
}
