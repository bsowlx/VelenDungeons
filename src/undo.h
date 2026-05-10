#pragma once

#include "raylib.h"
#include <optional>
#include <stack>

struct Snapshot {
    Vector2 playerPos;
    int activeRoom;
    int hp;
};

// Stack of per-room checkpoints. Push fires on entering a room (see main.cpp);
// pop is the rewind operation. Empty stack on pop returns nullopt.
class UndoStack {
public:
    void push(Snapshot s);
    std::optional<Snapshot> pop();
    bool empty() const { return stack_.empty(); }
    int size() const { return (int)stack_.size(); }

private:
    std::stack<Snapshot> stack_;
};
