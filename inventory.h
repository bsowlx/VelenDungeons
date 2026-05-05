#pragma once

#include <list>
#include <string>

struct Weapon {
    std::string name;
    float cooldown;        // seconds between shots
    int damage;
    float projectileSpeed; // px/sec
    int spread;            // number of projectiles per trigger (1 = single)
    float spreadAngleDeg;  // total cone (only used when spread > 1)
};

// Doubly linked list of weapons. The cursor (`current_`) cycles forward and
// backward with wrap-around at the ends. `add` / `remove` give the gameplay
// layer the pickup/drop API even though no world pickups exist yet.
class Inventory {
public:
    Inventory() : current_(weapons_.end()) {}

    void add(Weapon w);
    void remove();          // erases current; cursor advances to next (wraps)
    void cycleNext();
    void cyclePrev();

    const Weapon& current() const { return *current_; }
    bool empty() const { return weapons_.empty(); }
    size_t size() const { return weapons_.size(); }

private:
    std::list<Weapon> weapons_;
    std::list<Weapon>::iterator current_;
};
