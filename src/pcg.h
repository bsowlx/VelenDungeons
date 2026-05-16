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
    TileCoord letterTile;                // tile inside the antechamber (room 0) that opens the letter on E
    TileCoord exitTile;                  // tile inside the last combat room that advances the level on E (when room cleared)
};

LevelData generateLevel(int level, std::uint32_t seed);
