#include "leaderboard.h"

void Leaderboard::submit(int score) {
    heap_.push(score);
}

std::vector<int> Leaderboard::top(int n) const {
    auto copy = heap_;
    std::vector<int> out;
    while (!copy.empty() && (int)out.size() < n) {
        out.push_back(copy.top());
        copy.pop();
    }
    return out;
}
