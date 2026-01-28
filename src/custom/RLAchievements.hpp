#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>

namespace RLAchievements {
    enum class Collectable {
        Sparks,
        Planets,
        Coins,
        Points
    };

    struct Achievement {
        std::string id;
        std::string name;
        std::string desc;
        Collectable type;
        int amount;
        std::string sprite;
    };

    void init();
    void onUpdated(Collectable type, int oldVal, int newVal); // call when an Eru value is updated
    void checkAll(Collectable type, int currentVal); // check current totals and award any missing achievements
    void onReward(std::string const& id, std::string const& name, std::string const& desc, std::string const& sprite); // reward a misc achievement once
    std::vector<Achievement> getAll(); // get all achievements
    bool isAchieved(std::string const& id); // check if achievement is achieved
} // namespace RLAchievements
