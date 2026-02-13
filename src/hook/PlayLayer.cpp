#include "Geode/utils/general.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

class $modify(RLPlayLayer, PlayLayer) {
  struct Fields {
    bool m_isRatedLayout = false;
    int m_levelDifficulty = 0;
  };

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    log::debug(
        "PlayLayer init called, levelId={}, useReplay={}, dontCreateObjects={}",
        level ? static_cast<int>(level->m_levelID) : 0, useReplay,
        dontCreateObjects);

    // asynchronously check rated status via /fetch endpoint
    if (level && level->m_levelID != 0) {
      Ref<RLPlayLayer> self = this;
      auto req = web::WebRequest();
      std::string url =
          fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}",
                      static_cast<int>(level->m_levelID));
      async::spawn(req.get(url), [self,
                                  lvlId = static_cast<int>(level->m_levelID)](
                                     web::WebResponse resp) {
        if (!self)
          return;
        if (!resp.ok()) {
          log::debug("fetch (play) returned non-ok for level {}: {}", lvlId,
                     resp.code());
          self->m_fields->m_isRatedLayout = false;
          return;
        }
        // server returned 200 -> rated layout data exists
        auto jsonRes = resp.json();
        if (jsonRes) {
          log::info("Level {} identified as a rated layout (fetch OK)", lvlId);
          self->m_fields->m_isRatedLayout = true;
        }
        auto json = jsonRes.unwrap();
        auto difficulty = json["difficulty"].asInt().unwrapOr(0);
        self->m_fields->m_levelDifficulty = difficulty;

        auto savePath = dirs::getModsSaveDir() / Mod::get()->getID() /
                        "rubies_collected.json";
        auto existing =
            utils::file::readString(utils::string::pathToString(savePath));
        matjson::Value root = matjson::Value::object();
        if (existing) {
          auto parsed = matjson::parse(existing.unwrap());
          if (parsed && parsed.unwrap().isObject()) {
            root = parsed.unwrap();
          }
        }

        std::string levelKey = fmt::format("{}", lvlId);
        if (!root[levelKey].isObject()) {
          auto getTotalRubies = [](int difficulty) {
            switch (difficulty) {
            case 1:
              return 0;
            case 2:
              return 50;
            case 3:
              return 100;
            case 4:
              return 175;
            case 5:
              return 175;
            case 6:
              return 250;
            case 7:
              return 250;
            case 8:
              return 350;
            case 9:
              return 350;
            case 10:
              return 500;
            case 15:
              return 625;
            case 20:
              return 750;
            case 25:
              return 875;
            case 30:
              return 1000;
            default:
              return 0;
            }
          };

          int totalRuby = getTotalRubies(difficulty);
          root[levelKey] = matjson::Value::object();
          root[levelKey]["totalRubies"] = totalRuby;
          root[levelKey]["collectedRubies"] = 0;

          auto writeRes = utils::file::writeString(
              utils::string::pathToString(savePath), root.dump());
          if (!writeRes) {
            log::warn("Failed to create rubies_collected.json entry for "
                      "level {}: {}",
                      lvlId, writeRes.unwrapErr());
          } else {
            log::debug("Created rubies_collected.json entry for level {} -> {}",
                       lvlId, totalRuby);
          }
        }
      });
    } else {
      m_fields->m_isRatedLayout = false;
    }

    return true;
  }

  void showNewBest(bool newReward, int orbs, int diamonds, bool demonKey,
                   bool noRetry, bool noTitle) {
    // Prefer demonKey as the trigger for ruby rewards when this is a rated
    // layout
    if (m_fields->m_isRatedLayout && this->m_level) {
      // read collected rubies from mod save JSON
      auto savePath = dirs::getModsSaveDir() / Mod::get()->getID() /
                      "rubies_collected.json";
      auto existing =
          utils::file::readString(utils::string::pathToString(savePath));
      // load or create JSON root from savePath (structure: { "<levelid>": {
      // "totalRubies": N, "collectedRubies": M } })
      matjson::Value root = matjson::Value::object();
      if (existing) {
        auto parsed = matjson::parse(existing.unwrap());
        if (parsed && parsed.unwrap().isObject()) {
          root = parsed.unwrap();
        }
      }

      std::string levelKey =
          fmt::format("{}", static_cast<int>(this->m_level->m_levelID));
      matjson::Value entry = root[levelKey];
      int collected = 0;
      if (entry.isObject()) {
        collected = entry["collectedRubies"].asInt().unwrapOr(0);
        log::debug("rubies_collected.json: level {} already collected {} "
                   "rubies (file)",
                   this->m_level->m_levelID, collected);
      } else {
        // no existing entry — start at 0
        collected = 0;
      }

      // compute total rubies for this level's difficulty (fallback to stored
      // difficulty)
      int difficulty = m_fields->m_levelDifficulty;
      auto getTotalRubies = [](int difficulty) {
        switch (difficulty) {
        case 1:
          return 0;
        case 2:
          return 50;
        case 3:
          return 100;
        case 4:
          return 175;
        case 5:
          return 175;
        case 6:
          return 250;
        case 7:
          return 250;
        case 8:
          return 350;
        case 9:
          return 350;
        case 10:
          return 500;
        case 15:
          return 625;
        case 20:
          return 750;
        case 25:
          return 875;
        case 30:
          return 1000;
        default:
          return 0;
        }
      };
      int totalRuby = getTotalRubies(difficulty);

      // remaining rubies available to award
      int remaining = std::max(0, totalRuby - collected);

      // compute awarded rubies based on player's current percent in this play
      int percent = this->getCurrentPercent();
      int calcAtPercent;
      if (percent >= 100) {
        // full completion grants the total ruby amount
        calcAtPercent = totalRuby;
      } else {
        // Formula: Total x 0.8 x Percent / 100, rounded to nearest whole number
        double calcD = static_cast<double>(totalRuby) * 0.8 *
                       (static_cast<double>(percent) / 100.0);
        calcAtPercent = static_cast<int>(std::lround(calcD));
        if (calcAtPercent > totalRuby)
          calcAtPercent = totalRuby;
      }

      // amount the player should have at this percent minus already collected
      int awardableByPercent = std::max(0, calcAtPercent - collected);

      // When demonKey is used for rubies, desired amount comes from
      // awardableByPercent
      int desired = awardableByPercent;
      int adjustedRubies =
          std::min(desired, std::min(remaining, awardableByPercent));

      log::debug(
          "Adjusting reward (demonKey->ruby) for rated layout: total={}, "
          "collected={}, percent={}, calcAtPercent={}, awardableByPercent={}, "
          "remaining={}, requestedOrbs={}, adjustedRubies={}",
          totalRuby, collected, percent, calcAtPercent, awardableByPercent,
          remaining, orbs, adjustedRubies);

      // show reward as orbs (pass adjustedRubies to base)
      PlayLayer::showNewBest(newReward, orbs, adjustedRubies, demonKey, noRetry,
                             noTitle);

      // override the displayed orbs to show the percent-derived collected
      // amount
      auto children = this->getChildren();
      if (children) {
        for (unsigned int i = 0; i < children->count(); ++i) {
          auto child = static_cast<CCNode *>(children->objectAtIndex(i));
          if (!child)
            continue;
          auto rewardLayer = typeinfo_cast<CurrencyRewardLayer *>(child);
          if (!rewardLayer)
            continue; // skip non-reward children

          if (rewardLayer->m_keysLabel) {
            rewardLayer->m_keysLabel->setString(
                numToString(calcAtPercent).c_str());
            rewardLayer->m_keys = calcAtPercent;
          }

          // replace displayed sprite with ruby sprite
          std::string frameName = "RL_rubiesIcon.png"_spr;
          auto displayFrame =
              CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                  frameName.c_str());
          CCTexture2D *texture = nullptr;
          if (!displayFrame) {
            texture = CCTextureCache::sharedTextureCache()->addImage(
                frameName.c_str(), false);
            if (texture) {
              displayFrame = CCSpriteFrame::createWithTexture(
                  texture, {{0, 0}, texture->getContentSize()});
            }
          } else {
            texture = displayFrame->getTexture();
          }

          if (displayFrame && texture) {
            if (rewardLayer->m_diamondsSprite)
              rewardLayer->m_diamondsSprite->setDisplayFrame(displayFrame);
            if (rewardLayer->m_currencyBatchNode)
              rewardLayer->m_currencyBatchNode->setTexture(texture);
            if (rewardLayer->m_objects) {
              for (auto sprite :
                   CCArrayExt<CurrencySprite>(rewardLayer->m_objects)) {
                if (!sprite)
                  continue;
                if (sprite->m_burstSprite)
                  sprite->m_burstSprite->setVisible(false);
                if (auto childNode = sprite->getChildByIndex(0))
                  childNode->setVisible(false);
                sprite->setDisplayFrame(displayFrame);
              }
            }

            // does new best seriously doesnt have an id? bruh
            auto _allChildren = this->getChildren();
            if (_allChildren) {
              for (unsigned int j = 0; j < _allChildren->count(); ++j) {
                auto node =
                    static_cast<CCNode *>(_allChildren->objectAtIndex(j));
                if (!node)
                  continue;
                auto nodeChildren = node->getChildren();
                if (!nodeChildren)
                  continue;
                if (nodeChildren->count() ==
                    4) { // its always 4 children on the new best node so ye
                  auto obj3 = nodeChildren->objectAtIndex(3);
                  if (!obj3)
                    continue;
                  if (auto spr3 = typeinfo_cast<CCSprite *>(obj3)) {
                    spr3->setDisplayFrame(displayFrame);
                  }
                }
              }
            }
          }

          if (adjustedRubies > 0) {
            int oldRubies = Mod::get()->getSavedValue<int>("rubies");
            int newRubies = oldRubies + adjustedRubies;
            Mod::get()->setSavedValue<int>("rubies", newRubies);
            log::debug("Awarded {} rubies: {} -> {}", adjustedRubies, oldRubies,
                       newRubies);

            rewardLayer->m_diamonds = 0;
            if (rewardLayer->m_diamondsLabel) {
              rewardLayer->incrementDiamondsCount(oldRubies);
            }
          }

          break;
        }
      }

      // persist updated collected rubies (clamp to total)
      int newCollected = collected + adjustedRubies;
      if (newCollected > totalRuby)
        newCollected = totalRuby;
      root[levelKey] = matjson::Value::object();
      root[levelKey]["totalRubies"] = totalRuby;
      root[levelKey]["collectedRubies"] = newCollected;
      auto writeRes = utils::file::writeString(
          utils::string::pathToString(savePath), root.dump());
      if (!writeRes) {
        log::warn("Failed to write rubies_collected.json: {}",
                  writeRes.unwrapErr());
      } else {
        log::debug("Updated rubies_collected.json: level {} collected -> {}",
                   this->m_level->m_levelID, newCollected);
      }

      return; // done
    }

    PlayLayer::showNewBest(newReward, orbs, diamonds, demonKey, noRetry,
                           noTitle);
  }
};