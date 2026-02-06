#include "RLAchievements.hpp"
#include <Geode/Geode.hpp>
#include <mutex>

using namespace geode::prelude;

static std::vector<RLAchievements::Achievement> s_achievements;
static std::once_flag s_initFlag;
static CCDictionary* s_achievementsDict = nullptr;

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
      std::call_once(s_initFlag, []() {
            // id, name, desc, type, amount, sprite
            s_achievements = {
                // Sparks achievements
                {"spark_1", "First Spark of Hope", "Collect your First Spark", RLAchievements::Collectable::Sparks, 1, "RL_starBig.png"_spr},
                {"spark_50", "Into the Night Sky", "Collected 50 Sparks", RLAchievements::Collectable::Sparks, 50, "RL_starBig.png"_spr},
                {"spark_100", "Getting on pace", "Collected 100 Sparks", RLAchievements::Collectable::Sparks, 100, "RL_starBig.png"_spr},
                {"spark_250", "Spark Overture", "Collected 250 Sparks", RLAchievements::Collectable::Sparks, 250, "RL_starBig.png"_spr},
                {"spark_500", "Many Shiny Sparkles", "Collected 500 Sparks", RLAchievements::Collectable::Sparks, 500, "RL_starBig.png"_spr},
                {"spark_750", "Lots Beautiful Sparkles", "Collected 750 Sparks", RLAchievements::Collectable::Sparks, 750, "RL_starBig.png"_spr},
                {"spark_1000", "So many Sparkles…", "Collected 1000 Sparks", RLAchievements::Collectable::Sparks, 1000, "RL_starBig.png"_spr},
                {"spark_2500", "Yum Yum Sparks", "Collected 2500 Sparks", RLAchievements::Collectable::Sparks, 2500, "RL_starBig.png"_spr},
                {"spark_5000", "ExefMn would be proud", "Collected 5000 Sparks", RLAchievements::Collectable::Sparks, 5000, "RL_starBig.png"_spr},
                {"spark_7500", "Sparks on the Galaxy", "Collected 7500 Sparks", RLAchievements::Collectable::Sparks, 7500, "RL_starBig.png"_spr},
                {"spark_10000", "Road to 10K", "Collected 10000 Sparks", RLAchievements::Collectable::Sparks, 10000, "RL_starBig.png"_spr},
                {"spark_12000", "More than that", "Collected 12000 Sparks", RLAchievements::Collectable::Sparks, 12000, "RL_starBig.png"_spr},
                {"spark_14000", "14K Sta- I mean SPARKS!", "Collected 14000 Sparks", RLAchievements::Collectable::Sparks, 14000, "RL_starBig.png"_spr},
                {"spark_16000", "Lots of Sparks", "Collected 16000 Sparks", RLAchievements::Collectable::Sparks, 16000, "RL_starBig.png"_spr},
                {"spark_18000", "Imagine counting all that", "Collected 18000 Sparks", RLAchievements::Collectable::Sparks, 18000, "RL_starBig.png"_spr},
                {"spark_20000", "Laniakea Supercluster", "Collected 20000 Sparks", RLAchievements::Collectable::Sparks, 20000, "RL_starBig.png"_spr},
                {"spark_25000", "End of the Spark Road", "Collected 25000 Sparks", RLAchievements::Collectable::Sparks, 25000, "RL_starBig.png"_spr},

                // Planets achievements
                {"planet_1", "That's no Moon", "Collect your First Planet", RLAchievements::Collectable::Planets, 1, "RL_planetBig.png"_spr},
                {"planet_10", "Is a planet", "Collected 10 Planets", RLAchievements::Collectable::Planets, 10, "RL_planetBig.png"_spr},
                {"planet_25", "Planet Novice", "Collected 25 Planets", RLAchievements::Collectable::Planets, 25, "RL_planetBig.png"_spr},
                {"planet_50", "A thing with rings", "Collected 50 Planets", RLAchievements::Collectable::Planets, 50, "RL_planetBig.png"_spr},
                {"planet_75", "MORE ROOMS LEVELS!", "Collected 75 Planets", RLAchievements::Collectable::Planets, 75, "RL_planetBig.png"_spr},
                {"planet_250", "Saturn Cows", "Collected 250 Planets", RLAchievements::Collectable::Planets, 250, "RL_planetBig.png"_spr},
                {"planet_500", "Uranus has rings", "Collected 500 Planets", RLAchievements::Collectable::Planets, 500, "RL_planetBig.png"_spr},
                {"planet_750", "Needling my beloved", "Collected 750 Planets", RLAchievements::Collectable::Planets, 750, "RL_planetBig.png"_spr},
                {"planet_1000", "Dragonix's Grindset", "Collected 1000 Planets", RLAchievements::Collectable::Planets, 1000, "RL_planetBig.png"_spr},
                {"planet_2500", "Need more platformers...", "Collected 2500 Planets", RLAchievements::Collectable::Planets, 2500, "RL_planetBig.png"_spr},
                {"planet_5000", "31 Jolly Planets", "Collected 5000 Planets", RLAchievements::Collectable::Planets, 5000, "RL_planetBig.png"_spr},
                {"planet_7500", "I have a small galaxy", "Collected 7500 Planets", RLAchievements::Collectable::Planets, 7500, "RL_planetBig.png"_spr},
                {"planet_10000", "Found my Dad!", "Collected 10000 Planets", RLAchievements::Collectable::Planets, 10000, "RL_planetBig.png"_spr},

                // Coins achievements
                {"coin_5", "Did they just paint it blue?!", "Collected 5 Blue Coin", RLAchievements::Collectable::Coins, 5, "RL_BlueCoinUI.png"_spr},
                {"coin_10", "These can come in handy", "Collected 10 Blue Coins", RLAchievements::Collectable::Coins, 10, "RL_BlueCoinUI.png"_spr},
                {"coin_25", "Nailed it!", "Collected 25 Blue Coins", RLAchievements::Collectable::Coins, 25, "RL_BlueCoinUI.png"_spr},
                {"coin_50", "MLG Coin Pro", "Collected 50 Blue Coins", RLAchievements::Collectable::Coins, 50, "RL_BlueCoinUI.png"_spr},
                {"coin_100", "Da wae to the Blue Coins", "Collected 100 Blue Coins", RLAchievements::Collectable::Coins, 100, "RL_BlueCoinUI.png"_spr},
                {"coin_200", "Why did you scratch this Rob", "Collected 200 Blue Coins", RLAchievements::Collectable::Coins, 200, "RL_BlueCoinUI.png"_spr},
                {"coin_300", "My coins are kinda blueless", "Collected 300 Blue Coins", RLAchievements::Collectable::Coins, 300, "RL_BlueCoinUI.png"_spr},
                {"coin_400", "I’m feeling Blue", "Collected 400 Blue Coins", RLAchievements::Collectable::Coins, 400, "RL_BlueCoinUI.png"_spr},
                {"coin_500", "High as the Blue Sky", "Collected 500 Blue Coins", RLAchievements::Collectable::Coins, 500, "RL_BlueCoinUI.png"_spr},

                // blueprint points achievements
                {"points_1", "I forgot how to deco.", "Get a Rated Layout", RLAchievements::Collectable::Points, 1, "RL_blueprintPoint01.png"_spr},
                {"points_5", "Layouter ", "Collected 5 Blueprint Points", RLAchievements::Collectable::Points, 5, "RL_blueprintPoint01.png"_spr},
                {"points_10", "Certified GLC", "Collected 10 Blueprint Points", RLAchievements::Collectable::Points, 10, "RL_blueprintPoint01.png"_spr},
                {"points_25", "Visuals are 0%", "Collected 25 Blueprint Points", RLAchievements::Collectable::Points, 25, "RL_blueprintPoint01.png"_spr},
                {"points_50", "Reached Flow-state", "Collected 50 Blueprint Points", RLAchievements::Collectable::Points, 50, "RL_blueprintPoint01.png"_spr},
                {"points_75", "Revv-olutionary!", "Collected 75 Blueprint Points", RLAchievements::Collectable::Points, 75, "RL_blueprintPoint01.png"_spr},
                {"points_100", "Noob turned Pro", "Collected 100 Blueprint Points", RLAchievements::Collectable::Points, 100, "RL_blueprintPoint01.png"_spr},

                // misc achievements
                {"misc_begin", "It Rating Out Time", "Play a Rated Layout for the first time", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_news", "Layouts Out Loud", "Check the Rated Layouts Announcement", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_credits", "Our Beautiful Team", "View the Rated Layouts Credits", RLAchievements::Collectable::Misc, 1, "RL_creditsIcon.png"_spr},
                {"misc_event", "Eventful Layouts", "Play the current Event Layouts", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_leaderboard", "Best of the Best", "Be on the Leaderboard", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_custom_bg", "Personal Stylist", "Set a Custom Background or Ground", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_creator_1", "A Fellow Creator", "Talk to the Layout Creator for the first time", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_creator_25", "Layout entrepreneur", "Talk to the Layout Creator 25 times", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_creator_50", "Nosey Creator", "Talk to the Layout Creator 50 times", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_creator_100", "Business Layout Creator", "Talk to the Layout Creator 100 times", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_discord", "Layout-Cord", "Join the Rated Layouts Discord", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_browser", "www.ratedlayouts.com", "Browse the Rated Layouts Website", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_salt", "SALT finally rated", "but is it verified?", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_moderator", "GD Mod Simulator", "Become a Layout Moderator", RLAchievements::Collectable::Misc, 1, "RL_badgeMod01.png"_spr},
                {"misc_report", "Vigilant Citizen", "Make a valid Report on a Layout", RLAchievements::Collectable::Misc, 10, "RL_bob.png"_spr},
                {"misc_arcticwoof", "Find the Woof", "Find the Rated Layouts Owner", RLAchievements::Collectable::Misc, 1, "RL_arcticwoof.png"_spr},
                {"misc_coming", "Something is coming…", "You now know about him, be careful about where you go…", RLAchievements::Collectable::Misc, 1, "RL_bob.png"_spr},
                {"misc_extreme", "TOP ONE LAYOUT LIST!", "Complete an Extreme Demon Rated Layout", RLAchievements::Collectable::Misc, 1, "diffIcon_10_btn_001.png"},

                // community vote achievements
                {"vote_1", "Civic Layout Duty", "Submit your first vote on a Layout", RLAchievements::Collectable::Votes, 1, "RL_commVote01.png"_spr},
                {"vote_10", "Democracy in Layouts", "Submit 10 votes on Layouts", RLAchievements::Collectable::Votes, 10, "RL_commVote01.png"_spr},
                {"vote_50", "Layout Pollster", "Submit 50 votes on Layouts", RLAchievements::Collectable::Votes, 50, "RL_commVote01.png"_spr},
                {"vote_100", "Holding an Election", "Submit 100 votes on Layouts", RLAchievements::Collectable::Votes, 100, "RL_commVote01.png"_spr},
                {"vote_250", "Helping out the Poor", "Submit 250 votes on Layouts", RLAchievements::Collectable::Votes, 250, "RL_commVote01.png"_spr},
                {"vote_500", "You should be a Mod!", "Submit 500 votes on Layouts", RLAchievements::Collectable::Votes, 500, "RL_commVote01.png"_spr},
                {"vote_1000", "Can I NOW rate layouts?", "Submit 1000 votes on Layouts", RLAchievements::Collectable::Votes, 1000, "RL_commVote01.png"_spr},
            };
            s_achievementsDict = CCDictionary::create();
            s_achievementsDict->retain();
            for (auto const& ach : s_achievements) {
                  auto entry = CCDictionary::create();
                  entry->setObject(CCString::create(ach.id.c_str()), "id");
                  entry->setObject(CCString::create(ach.name.c_str()), "name");
                  entry->setObject(CCString::create(ach.desc.c_str()), "desc");
                  // store type and amount as strings for compatibility
                  entry->setObject(CCString::create(numToString(static_cast<int>(ach.type)).c_str()), "type");
                  entry->setObject(CCString::create(numToString(static_cast<int>(ach.amount)).c_str()), "amount");
                  entry->setObject(CCString::create(ach.sprite.c_str()), "sprite");
                  s_achievementsDict->setObject(entry, ach.id.c_str());
            }

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
            return;  // only consider increases
      }

      for (auto const& ach : s_achievements) {
            if (ach.type != type) continue;
            if (newVal < ach.amount) continue;  // not reached yet
            int awarded = Mod::get()->getSavedValue<int>(saveKeyFor(ach.id).c_str(), 0);
            if (awarded) {
                  log::debug("Achievement {} (id={}) already awarded (save={})", ach.name, ach.id, awarded);
                  continue;
            }
            if (oldVal >= ach.amount) {
                  log::debug("Player already had achievement {} (id={}) based on oldVal {}", ach.name, ach.id, oldVal);
                  continue;  // already had it
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

      for (auto const& ach : s_achievements) {
            if (ach.type != type) continue;
            if (currentVal < ach.amount) continue;  // not reached yet
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

CCDictionary* RLAchievements::getAllAsDictionary() {
      RLAchievements::init();
      if (!s_achievementsDict) {
            s_achievementsDict = CCDictionary::create();
            s_achievementsDict->retain();
            for (auto const& ach : s_achievements) {
                  auto entry = CCDictionary::create();
                  entry->setObject(CCString::create(ach.id.c_str()), "id");
                  entry->setObject(CCString::create(ach.name.c_str()), "name");
                  entry->setObject(CCString::create(ach.desc.c_str()), "desc");
                  entry->setObject(CCString::create(numToString(static_cast<int>(ach.type)).c_str()), "type");
                  entry->setObject(CCString::create(numToString(static_cast<int>(ach.amount)).c_str()), "amount");
                  entry->setObject(CCString::create(ach.sprite.c_str()), "sprite");
                  s_achievementsDict->setObject(entry, ach.id.c_str());
            }
      }
      return s_achievementsDict;
}

CCDictionary* RLAchievements::getAchievementDictionary(std::string const& id) {
      auto dict = getAllAsDictionary();
      if (!dict) return nullptr;
      return static_cast<CCDictionary*>(dict->objectForKey(id.c_str()));
}

void RLAchievements::onReward(std::string const& id) {
      RLAchievements::init();

      // find achievement details by id
      std::string name = id;
      std::string desc = "";
      std::string sprite = "";
      bool found = false;
      for (auto const& ach : s_achievements) {
            if (ach.id == id) {
                  name = ach.name;
                  desc = ach.desc;
                  sprite = ach.sprite;
                  found = true;
                  break;
            }
      }

      log::debug("RLAchievements::onReward(id={}, name={}, found={})", id, name, found);

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
