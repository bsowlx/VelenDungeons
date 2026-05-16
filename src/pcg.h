#pragma once

#include "dungeon.h"
#include "wave.h"
#include "raylib.h"
#include <cstdint>
#include <string>
#include <vector>

struct LevelSpec {
    int roomCount;
    int ghouls;
    int drowners;
    int berserks;
};

LevelSpec specFor(int level);

struct LevelData {
    std::vector<std::string> tileRows;   // kRows rows of kCols chars; '.'=floor, '#'=wall
    Dungeon dungeon;
    std::vector<std::vector<EnemySpawn>> rosters;
    Vector2 playerSpawn;
};

LevelData generateLevel(int level, std::uint32_t seed);
