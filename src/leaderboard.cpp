#include "leaderboard.h"

void Leaderboard::submit(const RunRecord& run) {
    heap_.push(run);
}

std::vector<RunRecord> Leaderboard::top(int n) const {
    auto copy = heap_;
    std::vector<RunRecord> out;
    while (!copy.empty() && (int)out.size() < n) {
        out.push_back(copy.top());
        copy.pop();
    }
    return out;
}
