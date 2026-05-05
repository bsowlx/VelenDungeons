#include "wave.h"

void initWaves(WaveState& w, int roomCount, std::initializer_list<int> clearedAtStart) {
    w.rooms.clear();
    w.rooms.resize(roomCount);
    for (int id : clearedAtStart) {
        if (id >= 0 && id < roomCount) w.rooms[id].state = RoomState::Cleared;
    }
}

void onEnterRoom(WaveState& w, int roomId,
                 const std::vector<std::vector<EnemySpawn>>& rosters) {
    if (roomId < 0 || roomId >= (int)w.rooms.size()) return;
    RoomWaveState& r = w.rooms[roomId];
    if (r.state != RoomState::Untouched) return;

    if (roomId < (int)rosters.size()) {
        for (const EnemySpawn& s : rosters[roomId]) r.queue.push(s);
    }
    r.spawnTimer = 0.0f;  // first enemy spawns on the next update tick
    r.state = RoomState::InProgress;
}

void updateWaves(WaveState& w, std::vector<Enemy>& enemies, int activeRoom, float dt) {
    if (activeRoom < 0 || activeRoom >= (int)w.rooms.size()) return;
    RoomWaveState& r = w.rooms[activeRoom];
    if (r.state != RoomState::InProgress) return;

    if (!r.queue.empty()) {
        r.spawnTimer -= dt;
        while (r.spawnTimer <= 0.0f && !r.queue.empty()) {
            EnemySpawn s = r.queue.front();
            r.queue.pop();
            enemies.push_back({ s.pos, activeRoom, kEnemyMaxHp, true });
            r.spawnTimer += kSpawnInterval;
        }
    }

    if (r.queue.empty()) {
        bool anyAlive = false;
        for (const Enemy& e : enemies) {
            if (e.alive && e.roomId == activeRoom) { anyAlive = true; break; }
        }
        if (!anyAlive) r.state = RoomState::Cleared;
    }
}

bool isCleared(const WaveState& w, int roomId) {
    if (roomId < 0 || roomId >= (int)w.rooms.size()) return false;
    return w.rooms[roomId].state == RoomState::Cleared;
}

RoomState stateOf(const WaveState& w, int roomId) {
    if (roomId < 0 || roomId >= (int)w.rooms.size()) return RoomState::Untouched;
    return w.rooms[roomId].state;
}
