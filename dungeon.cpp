#include "dungeon.h"

int Dungeon::addRoom(const std::string& name, Rect bounds) {
    int id = (int)rooms_.size();
    rooms_.push_back({id, name, bounds});
    adj_.emplace_back();
    nameIndex_.emplace(name, id);
    return id;
}

void Dungeon::addEdge(int a, int b, TileCoord doorAtoB, TileCoord doorBtoA) {
    adj_[a].push_back({b, doorAtoB});
    adj_[b].push_back({a, doorBtoA});
}

int Dungeon::roomIdByName(const std::string& name) const {
    auto it = nameIndex_.find(name);
    return it == nameIndex_.end() ? -1 : it->second;
}

int Dungeon::roomAt(int tileX, int tileY) const {
    for (const Room& r : rooms_) {
        if (r.bounds.contains(tileX, tileY)) return r.id;
    }
    return -1;
}

std::string Dungeon::validate(const char* const* tileMap, int rows, int cols) const {
    auto isFloor = [&](int x, int y) {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return false;
        return tileMap[y][x] == '.';
    };
    auto coordStr = [](int x, int y) {
        return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
    };

    for (const Room& r : rooms_) {
        for (int y = r.bounds.minY; y <= r.bounds.maxY; y++) {
            for (int x = r.bounds.minX; x <= r.bounds.maxX; x++) {
                if (!isFloor(x, y)) {
                    return "room '" + r.name + "' contains non-floor tile at " + coordStr(x, y);
                }
            }
        }
    }

    for (int from = 0; from < (int)adj_.size(); from++) {
        for (const Edge& e : adj_[from]) {
            if (!isFloor(e.door.x, e.door.y)) {
                return "edge " + rooms_[from].name + "->" + rooms_[e.to].name +
                       " door " + coordStr(e.door.x, e.door.y) + " is not a floor tile";
            }
            if (!rooms_[e.to].bounds.contains(e.door.x, e.door.y)) {
                return "edge " + rooms_[from].name + "->" + rooms_[e.to].name +
                       " door " + coordStr(e.door.x, e.door.y) + " is not inside destination room";
            }
        }
    }

    if (rooms_.empty()) return "";
    std::vector<bool> visited(rooms_.size(), false);
    std::vector<int> stack;
    stack.push_back(0);
    visited[0] = true;
    while (!stack.empty()) {
        int cur = stack.back();
        stack.pop_back();
        for (const Edge& e : adj_[cur]) {
            if (!visited[e.to]) {
                visited[e.to] = true;
                stack.push_back(e.to);
            }
        }
    }
    for (int i = 0; i < (int)visited.size(); i++) {
        if (!visited[i]) {
            return "room '" + rooms_[i].name + "' is not reachable from '" + rooms_[0].name + "'";
        }
    }

    return "";
}
