#pragma once

#include <queue>
#include <string>
#include <vector>

// One finished run: the name the player entered plus the stats shown on the
// board. Ordered by score.
struct RunRecord {
    std::string name;
    int score;
    int floor;
    int kills;
};

// Session-scoped max-heap of run records, keyed on score. submit pushes;
// top(n) returns the best n in descending score order. Empties when the
// program exits — there is no disk persistence (out of scope).
class Leaderboard {
public:
    void submit(const RunRecord& run);
    std::vector<RunRecord> top(int n) const;
    bool empty() const { return heap_.empty(); }
    int size() const { return (int)heap_.size(); }

private:
    struct ByScore {
        bool operator()(const RunRecord& a, const RunRecord& b) const {
            return a.score < b.score;
        }
    };
    std::priority_queue<RunRecord, std::vector<RunRecord>, ByScore> heap_;
};
