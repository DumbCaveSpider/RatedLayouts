#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>

namespace RLAchievements {
    enum class Collectable {
        Sparks,
        Planets,
        Coins,
        Points,
        Misc
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
    void onReward(std::string const& id); // reward a misc achievement once by id (looks up name/desc/sprite)
    std::vector<Achievement> getAll(); // get all achievements
    bool isAchieved(std::string const& id); // check if achievement is achieved

    cocos2d::CCDictionary* getAllAsDictionary();
    cocos2d::CCDictionary* getAchievementDictionary(std::string const& id); // individual achievement dictionary
} // namespace RLAchievements
