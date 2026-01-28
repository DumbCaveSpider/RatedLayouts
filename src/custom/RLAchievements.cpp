#include "RLAchievements.hpp"
#include <Geode/ui/GeodeUI.hpp>
#include <mutex>

using namespace geode::prelude;

static std::vector<RLAchievements::Achievement> s_achievements;
static std::once_flag s_initFlag;

static CCSprite* makeSprite(const std::string& frameName) {
    if (frameName.empty()) return nullptr;
    auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
    if (frame) {
        return CCSprite::createWithSpriteFrameName(frameName.c_str());
    }
    CCTexture2D* tex = CCTextureCache::sharedTextureCache()->addImage(frameName.c_str(), false);
    if (tex) {
        return CCSprite::createWithTexture(tex);
    }
    return nullptr;
}

void RLAchievements::init() {
    std::call_once(s_initFlag, [](){
        // id, name, desc, type, amount, sprite
        s_achievements = {
            // Sparks (stars) achievements
            {"spark_1", "First Spark of Hope", "Collect your First Spark", RLAchievements::Collectable::Sparks, 1, "RL_starBig.png"_spr},
            {"spark_50", "Yum Yum Sparks", "Collected 50 Sparks", RLAchievements::Collectable::Sparks, 50, "RL_starBig.png"_spr},
            {"spark_100", "Lots of Sparky", "Collected 100 Sparks", RLAchievements::Collectable::Sparks, 100, "RL_starBig.png"_spr},
            {"spark_250", "250 Sta- I mean SPARKS!", "Collected 250 Sparks", RLAchievements::Collectable::Sparks, 250, "RL_starBig.png"_spr},
            {"spark_500", "Many Shiny Sparkles", "Collected 500 Sparks", RLAchievements::Collectable::Sparks, 500, "RL_starBig.png"_spr},
            {"spark_1000", "So many Sparkles...", "Collected 1000 Sparks", RLAchievements::Collectable::Sparks, 1000, "RL_starBig.png"_spr},

            // Planets achievements
            {"planet_1", "This is not Moon!", "Collect your First Planet", RLAchievements::Collectable::Planets, 1, "RL_planetBig.png"_spr},
            {"planet_250", "Outer Space", "Collected 250 Planets", RLAchievements::Collectable::Planets, 250, "RL_planetBig.png"_spr},
            {"planet_500", "Saturn Cows", "Collected 500 Planets", RLAchievements::Collectable::Planets, 500, "RL_planetBig.png"_spr},
            {"planet_750", "Exoplanet", "Collected 750 Planets", RLAchievements::Collectable::Planets, 750, "RL_planetBig.png"_spr},
            {"planet_1000", "Planetary Pro", "Collected 1000 Planets", RLAchievements::Collectable::Planets, 1000, "RL_planetBig.png"_spr},

            // Coins achievements
            {"coin_5", "Did they just paint it blue?!", "Collected 5 Blue Coin", RLAchievements::Collectable::Coins, 5, "RL_BlueCoinUI.png"_spr},
            {"coin_10", "These can come in handy", "Collected 10 Blue Coins", RLAchievements::Collectable::Coins, 10, "RL_BlueCoinUI.png"_spr},
            {"coin_25", "Nailed it!", "Collected 25 Blue Coins", RLAchievements::Collectable::Coins, 25, "RL_BlueCoinUI.png"_spr},
            {"coin_50", "High as the Blue Sky", "Collected 50 Blue Coins", RLAchievements::Collectable::Coins, 50, "RL_BlueCoinUI.png"_spr},

            // blueprint points achievements
            {"points_1", "Layouter", "Get a Rated Layout", RLAchievements::Collectable::Points, 1, "RL_blueprintPoint01.png"_spr},
            {"points_5", "Certified GLC", "Collected 5 Blueprint Points", RLAchievements::Collectable::Points, 5, "RL_blueprintPoint01.png"_spr},
            {"points_10", "Visuals are 0%", "Collected 10 Blueprint Points", RLAchievements::Collectable::Points, 10, "RL_blueprintPoint01.png"_spr},
            {"points_25", "Reached Flow-state", "Collected 25 Blueprint Points", RLAchievements::Collectable::Points, 25, "RL_blueprintPoint01.png"_spr},
            {"points_50", "Revv-olutionary!", "Collected 50 Blueprint Points", RLAchievements::Collectable::Points, 50, "RL_blueprintPoint01.png"_spr},
            {"points_75", "Noob turned Pro", "Collected 75 Blueprint Points", RLAchievements::Collectable::Points, 75, "RL_blueprintPoint01.png"_spr},
            {"points_100", "Better than y'all!", "Collected 100 Blueprint Points", RLAchievements::Collectable::Points, 100, "RL_blueprintPoint01.png"_spr},
        };
        log::info("RLAchievements initialized with {} achievements", s_achievements.size());
    });
}

static std::string saveKeyFor(const std::string& id) {
    return fmt::format("achieved_{}", id);
}

void RLAchievements::onUpdated(RLAchievements::Collectable type, int oldVal, int newVal) {
    RLAchievements::init();

    log::debug("RLAchievements::onUpdated(type={}, oldVal={}, newVal={})", static_cast<int>(type), oldVal, newVal);

    if (newVal <= oldVal) {
        log::debug("Ignoring update because newVal ({}) <= oldVal ({})", newVal, oldVal);
        return; // only consider increases
    }

    for (auto const &ach : s_achievements) {
        if (ach.type != type) continue;
        if (newVal < ach.amount) continue; // not reached yet
        int awarded = Mod::get()->getSavedValue<int>(saveKeyFor(ach.id).c_str(), 0);
        if (awarded) {
            log::debug("Achievement {} (id={}) already awarded (save={})", ach.name, ach.id, awarded);
            continue;
        }
        if (oldVal >= ach.amount) {
            log::debug("Player already had achievement {} (id={}) based on oldVal {}", ach.name, ach.id, oldVal);
            continue; // already had it
        }

        // Candidate reached
        log::info("Awarding achievement {} (id={}) amount={} (oldVal={}, newVal={})", ach.name, ach.id, ach.amount, oldVal, newVal);

        // Mark awarded
        Mod::get()->setSavedValue<int>(saveKeyFor(ach.id).c_str(), 1);
        int verify = Mod::get()->getSavedValue<int>(saveKeyFor(ach.id).c_str(), 0);
        log::debug("Saved value for {} is now {}", saveKeyFor(ach.id), verify);

        // give achievement thingy
        log::info("Notifying achievement {} via AchievementNotifier", ach.id);
        AchievementNotifier::sharedState()->notifyAchievement(ach.name.c_str(), ach.desc.c_str(), ach.sprite.c_str(), true);
    }
}

void RLAchievements::checkAll(RLAchievements::Collectable type, int currentVal) {
    RLAchievements::init();

    log::debug("RLAchievements::checkAll(type={}, currentVal={})", static_cast<int>(type), currentVal);

    for (auto const &ach : s_achievements) {
        if (ach.type != type) continue;
        if (currentVal < ach.amount) continue; // not reached yet
        int awarded = Mod::get()->getSavedValue<int>(saveKeyFor(ach.id).c_str(), 0);
        if (awarded) {
            log::debug("Achievement {} (id={}) already awarded (save={})", ach.name, ach.id, awarded);
            continue;
        }

        // Award retroactively
        log::info("Retroactively awarding achievement {} (id={}) amount={} (currentVal={})", ach.name, ach.id, ach.amount, currentVal);
        Mod::get()->setSavedValue<int>(saveKeyFor(ach.id).c_str(), 1);
        int verify = Mod::get()->getSavedValue<int>(saveKeyFor(ach.id).c_str(), 0);
        log::debug("Saved value for {} is now {}", saveKeyFor(ach.id), verify);
        log::info("Notifying achievement {} via AchievementNotifier (retro)", ach.id);
        AchievementNotifier::sharedState()->notifyAchievement(ach.name.c_str(), ach.desc.c_str(), ach.sprite.c_str(), true);
    }
}

std::vector<RLAchievements::Achievement> RLAchievements::getAll() {
    RLAchievements::init();
    return s_achievements;
}

bool RLAchievements::isAchieved(std::string const& id) {
    return Mod::get()->getSavedValue<int>(saveKeyFor(id).c_str(), 0) != 0;
}

void RLAchievements::onReward(std::string const& id, std::string const& name, std::string const& desc, std::string const& sprite) {
    RLAchievements::init();

    log::debug("RLAchievements::onReward(id={}, name={})", id, name);

    if (RLAchievements::isAchieved(id)) {
        log::debug("Misc achievement {} already awarded", id);
        return;
    }

    // Mark awarded
    Mod::get()->setSavedValue<int>(saveKeyFor(id).c_str(), 1);
    int verify = Mod::get()->getSavedValue<int>(saveKeyFor(id).c_str(), 0);
    log::info("Awarded misc achievement {} (verify={})", id, verify);

    // Notify player
    AchievementNotifier::sharedState()->notifyAchievement(name.c_str(), desc.c_str(), sprite.c_str(), true);
}
