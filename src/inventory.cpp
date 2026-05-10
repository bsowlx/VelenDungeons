#include "inventory.h"
#include <iterator>
#include <utility>

void Inventory::add(Weapon w) {
    weapons_.push_back(std::move(w));
    if (weapons_.size() == 1) current_ = weapons_.begin();
}

void Inventory::remove() {
    if (weapons_.empty()) return;
    current_ = weapons_.erase(current_);
    if (current_ == weapons_.end() && !weapons_.empty()) current_ = weapons_.begin();
}

void Inventory::cycleNext() {
    if (weapons_.empty()) return;
    ++current_;
    if (current_ == weapons_.end()) current_ = weapons_.begin();
}

void Inventory::cyclePrev() {
    if (weapons_.empty()) return;
    if (current_ == weapons_.begin()) current_ = std::prev(weapons_.end());
    else --current_;
}
