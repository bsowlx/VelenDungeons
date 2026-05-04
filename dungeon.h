#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct TileCoord {
    int x;
    int y;
};

struct Rect {
    int minX;
    int minY;
    int maxX;
    int maxY;

    bool contains(int x, int y) const {
        return x >= minX && x <= maxX && y >= minY && y <= maxY;
    }
};

struct Room {
    int id;
    std::string name;
    Rect bounds;
};

// Edge.door = first tile inside the destination room along the corridor.
// Stepping onto that tile means "arrived in destination" — used to fire
// room-transition events (future: spawn waves).
struct Edge {
    int to;
    TileCoord door;
};

class Dungeon {
public:
    int addRoom(const std::string& name, Rect bounds);

    // Adds A->B (with door inside B) AND B->A (with door inside A).
    void addEdge(int a, int b, TileCoord doorAtoB, TileCoord doorBtoA);

    int roomCount() const { return (int)rooms_.size(); }
    const Room& room(int id) const { return rooms_[id]; }
    int roomIdByName(const std::string& name) const;
    int roomAt(int tileX, int tileY) const;
    const std::vector<Edge>& edgesFrom(int id) const { return adj_[id]; }

    // tileMap: rows strings of length cols. '#' = wall, '.' = floor.
    // Checks: (1) every tile inside every room rect is floor;
    //         (2) every edge's door tile is floor AND inside the destination room;
    //         (3) DFS from room 0 reaches every other room.
    // Returns empty string on success; an error message on failure.
    std::string validate(const char* const* tileMap, int rows, int cols) const;

private:
    std::vector<Room> rooms_;
    std::vector<std::vector<Edge>> adj_;
    std::unordered_map<std::string, int> nameIndex_;
};
