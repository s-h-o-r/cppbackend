#pragma once

#include <vector>
#include <utility>

namespace game_obj {

template <typename Loot>
class Bag {
public:
    Bag(size_t capacity)
    : capacity_(capacity) {
        loot_.reserve(capacity_);
    }

    void AddLoot(Loot loot) {
        loot_.push_back(loot);
    }

    Loot TakeTopLoot() {
        auto loot = std::move(loot_.back());
        loot_.pop_back();
        return loot;
    }

    Loot TakeLoot(size_t id) {
        auto loot = std::move(loot_.at(id));
        loot_.erase(loot_.begin() + id);
        return loot;
    }

    const std::vector<Loot>& GetAllLoot() const {
        return loot_;
    }

    size_t GetCapacity() const {
        return capacity_;
    }

    void SetCapacity(size_t new_capacity) {
        capacity_ = new_capacity;
    }

    size_t GetSize() const {
        return loot_.size();
    }

    bool Empty() const {
        return loot_.empty();
    }

    bool Full() const {
        return loot_.size() == capacity_;
    }

private:
    std::vector<Loot> loot_;
    size_t capacity_;
};

}
