#pragma once

#include <queue>
#include <vector>

// Session-scoped max-heap of run scores. submit pushes; top(n) returns the
// best n in descending order. Empties when the program exits — there is no
// disk persistence (out of scope).
class Leaderboard {
public:
    void submit(int score);
    std::vector<int> top(int n) const;
    bool empty() const { return heap_.empty(); }
    int size() const { return (int)heap_.size(); }

private:
    std::priority_queue<int> heap_;
};
