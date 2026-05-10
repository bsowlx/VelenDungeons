#pragma once

#include "raylib.h"
#include "enemy.h"
#include <initializer_list>
#include <queue>
#include <vector>

constexpr float kSpawnInterval = 0.5f;  // seconds between enemy spawns from queue

enum class RoomState { Untouched, InProgress, Cleared };

struct EnemySpawn {
    Vector2 pos;
    EnemyKind kind = EnemyKind::Ghoul;
};

// Per-room queue + timer + state. `updateWaves` only drains the queue belonging
// to the active room, so leaving a room mid-spawn pauses spawning until you
// return.
struct RoomWaveState {
    std::queue<EnemySpawn> queue;
    float spawnTimer = 0.0f;
    RoomState state = RoomState::Untouched;
};

struct WaveState {
    std::vector<RoomWaveState> rooms;
};

void initWaves(WaveState& w, int roomCount, std::initializer_list<int> clearedAtStart);

// On entering an Untouched room, fill its queue from the roster and mark it
// InProgress. No-op for InProgress (resume in-place) and Cleared rooms.
void onEnterRoom(WaveState& w, int roomId,
                 const std::vector<std::vector<EnemySpawn>>& rosters);

// Per frame: drain the active room's queue at kSpawnInterval; if InProgress
// and queue empty and no alive enemies in this room, transition to Cleared.
void updateWaves(WaveState& w, std::vector<Enemy>& enemies, int activeRoom, float dt);

bool isCleared(const WaveState& w, int roomId);
RoomState stateOf(const WaveState& w, int roomId);
