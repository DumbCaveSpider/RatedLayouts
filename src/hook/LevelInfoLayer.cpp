#include <Geode/Geode.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <argon/argon.hpp>

#include "../level/RLCommunityVotePopup.hpp"
#include "../level/RLModRatePopup.hpp"

using namespace geode::prelude;

// helper functions all about caching level rating data
// get the cache file path
static std::string getCachePath() {
      auto saveDir = dirs::getModsSaveDir();
      return utils::string::pathToString(saveDir / "level_ratings_cache.json");  // stored outside the save dir, idk why
}

// load cached data for a level
static std::optional<matjson::Value> getCachedLevel(int levelId) {
      auto cachePath = getCachePath();
      auto data = utils::file::readString(cachePath);
      if (!data)
            return std::nullopt;

      auto json = matjson::parse(data.unwrap());
      if (!json)
            return std::nullopt;

      auto root = json.unwrap();
      if (root.isObject() && root.contains(fmt::format("{}", levelId))) {
            return root[fmt::format("{}", levelId)];
      }

      return std::nullopt;
}

// delete level from cache
static void deleteLevelFromCache(int levelId) {
      auto cachePath = getCachePath();
      auto existingData = utils::file::readString(cachePath);
      if (existingData) {
            auto parsed = matjson::parse(existingData.unwrap());
            if (parsed) {
                  auto root = parsed.unwrap();
                  if (root.isObject()) {
                        std::string key = fmt::format("{}", levelId);
                        auto result = root.erase(key);
                  }
                  auto jsonString = root.dump();
                  auto writeResult = utils::file::writeString(cachePath, jsonString);
                  log::debug("Deleted level ID {} from cache", levelId);
            }
      }
}

// save level cache
static void cacheLevelData(int levelId, const matjson::Value& data) {
      auto saveDir = dirs::getModsSaveDir();
      auto createDirResult = utils::file::createDirectory(saveDir);
      if (!createDirResult) {
            log::warn("Failed to create save directory for cache");
            return;
      }

      auto cachePath = getCachePath();

      // load existing cache
      matjson::Value root = matjson::Value::object();
      auto existingData = utils::file::readString(cachePath);
      if (existingData) {
            auto parsed = matjson::parse(existingData.unwrap());
            if (parsed) {
                  root = parsed.unwrap();
            }
      }

      // update data
      root[fmt::format("{}", levelId)] = data;

      // write to file
      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(cachePath, jsonString);
      if (writeResult) {
            log::debug("Cached level rating data for level ID: {}", levelId);
      }
}

// most of the code here are just repositioning the stars and coins to fit the
// new difficulty icon its very messy, yes but it just works do please clean up
// my messy code pls
class $modify(RLLevelInfoLayer, LevelInfoLayer) {
      struct Fields {
            bool m_difficultyOffsetApplied = false;  // this is ass. very trash, hacky
                                                     // way to fix this refresh bug
            bool m_originalYSaved = false;
            float m_originalDifficultySpriteY = 0.0f;
            bool m_lastAppliedIsDemon = false;
            // coins original positions
            bool m_originalCoinsSaved = false;
            float m_originalCoin1Y = 0.0f;
            float m_originalCoin2Y = 0.0f;
            float m_originalCoin3Y = 0.0f;
            utils::web::WebTask m_fetchTask;
            utils::web::WebTask m_submitTask;
            utils::web::WebTask m_accessTask;
            ~Fields() {
                  m_fetchTask.cancel();
                  m_submitTask.cancel();
                  m_accessTask.cancel();
            }
      };

      bool init(GJGameLevel* level, bool challenge) {
            if (!LevelInfoLayer::init(level, challenge))
                  return false;

            log::debug("LevelInfoLayer: entering for level id {}", level->m_levelID);

            int starRatings = level->m_stars;
            bool legitCompleted = level->m_isCompletionLegitimate;
            auto leftMenu = this->getChildByID("left-side-menu");
            bool isPlatformer = this->m_level->isPlatformer();

            log::debug("isPlatformer = {}, starRatings = {}, legitCompleted = {}",
                       isPlatformer, starRatings, legitCompleted);

            int userRole = Mod::get()->getSavedValue<int>("role");
            if (userRole == 1 || userRole == 2) {
                  // add a role button for send/rate
                  auto iconSprite = CCSprite::createWithSpriteFrameName("RL_starBig.png"_spr);
                  CCSprite* buttonSprite = nullptr;

                  if (starRatings != 0) {
                        buttonSprite = CCSpriteGrayscale::createWithSpriteFrameName("RL_starBig.png"_spr);
                        auto roleButtonSpr = CircleButtonSprite::create(buttonSprite, CircleBaseColor::Gray, CircleBaseSize::Medium);
                        auto roleButtonItem = CCMenuItemSpriteExtra::create(
                            roleButtonSpr, this, menu_selector(RLLevelInfoLayer::onRoleButton));
                        roleButtonItem->setID("role-button");

                        leftMenu->addChild(roleButtonItem);
                  } else {
                        buttonSprite = CCSprite::createWithSpriteFrameName("RL_starBig.png"_spr);
                        auto roleButtonSpr = CircleButtonSprite::create(buttonSprite, CircleBaseColor::Cyan, CircleBaseSize::Medium);
                        auto roleButtonItem = CCMenuItemSpriteExtra::create(
                            roleButtonSpr, this, menu_selector(RLLevelInfoLayer::onRoleButton));
                        roleButtonItem->setID("role-button");

                        leftMenu->addChild(roleButtonItem);
                  }
            }

            leftMenu->updateLayout();

            // If the level is already downloaded
            if (this->m_level && !this->m_level->m_levelString.empty()) {
                  this->levelDownloadFinished(this->m_level);
            }

            return true;
      };

      void levelDownloadFinished(GJGameLevel* level) {
            LevelInfoLayer::levelDownloadFinished(level);

            log::info("Level download finished, fetching rating data...");

            // Fetch rating data from server
            int levelId = this->m_level->m_levelID;
            deleteLevelFromCache(levelId);  // delete cache
            log::info("Fetching rating data for level ID: {}", levelId);

            // Try to load from cache first
            auto cachedData = getCachedLevel(levelId);

            if (cachedData) {
                  log::debug("Loading cached rating data for level ID: {}", levelId);
                  Ref<RLLevelInfoLayer> layerRef = this;
                  auto difficultySprite = layerRef->getChildByID("difficulty-sprite");
                  processLevelRating(cachedData.value(), layerRef, difficultySprite);
                  return;
            }

            auto getReq = web::WebRequest();
            m_fields->m_fetchTask = getReq.get(
                fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

            Ref<RLLevelInfoLayer> layerRef = this;
            auto difficultySprite = layerRef->getChildByID("difficulty-sprite");

            m_fields->m_fetchTask.listen([layerRef, difficultySprite](web::WebResponse* response) {
                  log::info("Received rating response from server");

                  if (!layerRef) {
                        log::warn("LevelInfoLayer has been destroyed");
                        return;
                  }

                  if (!response->ok()) {
                        log::warn("Server returned non-ok status: {}", response->code());
                        return;
                  }

                  auto jsonRes = response->json();
                  if (!jsonRes) {
                        log::warn("Failed to parse JSON response");
                        return;
                  }

                  auto json = jsonRes.unwrap();

                  // Cache the response
                  cacheLevelData(layerRef->m_level->m_levelID, json);

                  layerRef->processLevelRating(json, layerRef, difficultySprite, true);
            });
      }
      void processLevelRating(const matjson::Value& json,
                              Ref<RLLevelInfoLayer> layerRef,
                              CCNode* difficultySprite,
                              bool forceShow = false) {
            if (!layerRef) return;
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();
            bool showCommunity = forceShow;

            // helper to remove existing button
            auto removeExistingCommunityBtn = [layerRef]() {
                  auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                  if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                        auto existing = static_cast<CCMenu*>(rightMenuNode)->getChildByID("rl-community-vote");
                        if (existing) {
                              existing->removeFromParent();
                              static_cast<CCMenu*>(rightMenuNode)->updateLayout();
                        }
                  }
            };

            if (showCommunity) {
                  // create button if not already present
                  bool exists = false;
                  auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                  if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                        if (static_cast<CCMenu*>(rightMenuNode)->getChildByID("rl-community-vote"))
                              exists = true;
                  }

                  if (!exists) {
                        // determine whether we can use the level's percentage fields
                        int normalPct = -1;
                        int practicePct = -1;
                        bool hasPctFields = false;

                        if (layerRef && layerRef->m_level) {
                              hasPctFields = true;
                              normalPct = layerRef->m_level->m_normalPercent;
                              practicePct = layerRef->m_level->m_practicePercent;
                        }

                        bool shouldDisable = true;
                        if (hasPctFields) {
                              shouldDisable = !(normalPct >= 20 || practicePct >= 80);
                        }

                        // Mods/Admins can always vote regardless of percentages
                        int userRole = Mod::get()->getSavedValue<int>("role");
                        if (userRole == 1 || userRole == 2) {
                              shouldDisable = false;
                              log::debug("Community vote enabled due to role override (role={})", userRole);
                        }

                        auto commSpriteGray = CCSpriteGrayscale::createWithSpriteFrameName("RL_commVote01.png"_spr);
                        if (commSpriteGray) {
                              auto commBtnSpr = CircleButtonSprite::create(commSpriteGray, CircleBaseColor::Gray, CircleBaseSize::Medium);
                              auto commBtnItem = CCMenuItemSpriteExtra::create(commBtnSpr, layerRef, menu_selector(RLLevelInfoLayer::onCommunityVote));
                              commBtnItem->setID("rl-community-vote");

                              auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                              if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                                    static_cast<CCMenu*>(rightMenuNode)->addChild(commBtnItem);
                                    // ensure layout updates so the button is placed correctly
                                    static_cast<CCMenu*>(rightMenuNode)->updateLayout();

                                    // Reactivate immediately if it should be enabled according to data
                                    if (!shouldDisable) {
                                          // swap to colored sprite
                                          auto commSpriteColor = CCSprite::createWithSpriteFrameName("RL_commVote01.png"_spr);
                                          if (commSpriteColor) {
                                                auto coloredBtnSpr = CircleButtonSprite::create(commSpriteColor, CircleBaseColor::Green, CircleBaseSize::Medium);
                                                commBtnItem->setNormalImage(coloredBtnSpr);
                                          }
                                          commBtnItem->setEnabled(true);
                                          commBtnItem->setOpacity(255);
                                    }
                              } else {
                                    log::warn("Right-side-menu not found; community vote button not added");
                              }
                        }
                  } else {
                        // update existing button state based on current data
                        int normalPct = -1;
                        int practicePct = -1;
                        bool hasPctFields = false;

                        if (layerRef && layerRef->m_level) {
                              hasPctFields = true;
                              normalPct = layerRef->m_level->m_normalPercent;
                              practicePct = layerRef->m_level->m_practicePercent;
                        }

                        bool shouldDisable = true;
                        if (hasPctFields) {
                              shouldDisable = !(normalPct >= 20 || practicePct >= 80);
                        }

                        int userRole = Mod::get()->getSavedValue<int>("role");
                        if (userRole == 1 || userRole == 2) {
                              shouldDisable = false;
                              log::debug("Community vote enabled due to role override (role={})", userRole);
                        }

                        auto rightMenuNode2 = layerRef->getChildByID("right-side-menu");
                        if (rightMenuNode2 && typeinfo_cast<CCMenu*>(rightMenuNode2)) {
                              auto existing = static_cast<CCMenu*>(rightMenuNode2)->getChildByID("rl-community-vote");
                              if (existing) {
                                    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(existing);
                                    if (menuItem) {
                                          menuItem->setEnabled(!shouldDisable);
                                          menuItem->setOpacity(shouldDisable ? 100 : 255);
                                    }
                              }
                        }
                  }
            } else {
                  // already rated / not suggested
                  removeExistingCommunityBtn();
            }

            // If no difficulty rating, remove from cache
            if (difficulty == 0) {
                  auto cachePath = getCachePath();
                  auto existingData = utils::file::readString(cachePath);
                  if (existingData) {
                        auto parsed = matjson::parse(existingData.unwrap());
                        if (parsed) {
                              auto root = parsed.unwrap();
                              if (root.isObject()) {
                                    std::string key = fmt::format("{}", layerRef->m_level->m_levelID);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult = utils::file::writeString(cachePath, jsonString);
                              log::debug("Removed level ID {} from cache (no difficulty)",
                                         layerRef->m_level->m_levelID);
                        }
                  }
                  return;
            }

            log::info("difficulty: {}, featured: {}", difficulty, featured);

            // check if the level needs to submit completion reward if completed
            if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(
                    layerRef->m_level->m_levelID)) {
                  log::info("Level is completed, submitting completion reward");

                  int levelId = layerRef->m_level->m_levelID;
                  int accountId = GJAccountManager::get()->m_accountID;
                  std::string argonToken =
                      Mod::get()->getSavedValue<std::string>("argon_token");

                  // include attempt data for analytics / verification
                  int attempts = 0;
                  int attemptTime = 0;
                  int jumps = 0;
                  int clicks = 0;
                  bool isPlat = false;
                  if (layerRef && layerRef->m_level) {
                        attempts = layerRef->m_level->m_attempts;
                        isPlat = layerRef->m_level->isPlatformer();
                        attemptTime = isPlat ? (layerRef->m_level->m_bestTime / 1000) : layerRef->m_level->m_attemptTime;
                        jumps = layerRef->m_level->m_jumps;
                        clicks = layerRef->m_level->m_clicks;
                  }

                  log::debug("Submitting completion with attempts: {} time: {} jumps: {} clicks: {}", attempts, attemptTime, jumps, clicks);

                  // Build comma-separated coins list based on pending user coins
                  std::string coinsStr;
                  if (layerRef && layerRef->m_level) {
                        std::vector<int> coins;
                        for (int i = 1; i <= 3; ++i) {
                              auto coinKey = layerRef->m_level->getCoinKey(i);
                              if (GameStatsManager::sharedState()->hasPendingUserCoin(coinKey)) {
                                    coins.push_back(i);
                              }
                        }
                        for (size_t idx = 0; idx < coins.size(); ++idx) {
                              if (idx) coinsStr += ",";
                              coinsStr += std::to_string(coins[idx]);
                        }
                  } else {
                        log::warn("Unable to build coins list: level pointer is null for levelId {}", levelId);
                  }
                  log::debug("Collected pending coins for level {}: {}", levelId, coinsStr);

                  matjson::Value jsonBody;
                  jsonBody["accountId"] = accountId;
                  jsonBody["argonToken"] = argonToken;
                  jsonBody["levelId"] = levelId;
                  jsonBody["attempts"] = attempts;
                  jsonBody["attemptTime"] = attemptTime;
                  jsonBody["jumps"] = jumps;
                  jsonBody["clicks"] = clicks;
                  jsonBody["isPlat"] = isPlat;
                  if (!coinsStr.empty()) {
                        jsonBody["coins"] = coinsStr;
                  }

                  auto submitReq = web::WebRequest();
                  submitReq.bodyJSON(jsonBody);
                  m_fields->m_submitTask =
                      submitReq.post("https://gdrate.arcticwoof.xyz/submitComplete");

                  m_fields->m_submitTask.listen([layerRef, difficulty, levelId,
                                                 difficultySprite](web::WebResponse* submitResponse) {
                        log::info("Received submitComplete response for level ID: {}", levelId);

                        if (!layerRef) {
                              log::warn("LevelInfoLayer has been destroyed");
                              return;
                        }

                        if (!submitResponse->ok()) {
                              log::warn("submitComplete returned non-ok status: {}",
                                        submitResponse->code());
                              return;
                        }

                        auto submitJsonRes = submitResponse->json();
                        if (!submitJsonRes) {
                              log::warn("Failed to parse submitComplete JSON response");
                              return;
                        }

                        auto submitJson = submitJsonRes.unwrap();
                        bool success = submitJson["success"].asBool().unwrapOrDefault();
                        int responseStars = submitJson["stars"].asInt().unwrapOrDefault();
                        int responsePlanets = submitJson["planets"].asInt().unwrapOrDefault();
                        log::info("submitComplete success: {}, response stars: {}", success,
                                  responseStars);

                        if (success) {
                              if (responseStars == 0 && responsePlanets == 0) {
                                    log::warn("No stars or planets rewarded, possibly already rewarded before");
                                    return;
                              }
                              bool isPlat = false;
                              if (layerRef && layerRef->m_level) isPlat = layerRef->m_level->isPlatformer();
                              log::info("submitComplete values - stars: {}, planets: {}", responseStars, responsePlanets);
                              int displayReward = (isPlat ? (responsePlanets - difficulty) : (responseStars - difficulty));

                              // persist returned totals
                              if (isPlat) {
                                    Mod::get()->setSavedValue<int>("planets", responsePlanets);
                              } else {
                                    Mod::get()->setSavedValue<int>("stars", responseStars);
                              }

                              std::string medSprite = isPlat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;
                              std::string reward = isPlat ? "planets" : "sparks";

                              if (!Mod::get()->getSettingValue<bool>("disableRewardAnimation")) {
                                    if (auto rewardLayer = CurrencyRewardLayer::create(
                                            0, isPlat ? difficulty : 0, isPlat ? 0 : difficulty, 0, CurrencySpriteType::Star, 0,
                                            CurrencySpriteType::Star, 0, difficultySprite->getPosition(),
                                            CurrencyRewardType::Default, 0.0, 1.0)) {
                                          if (isPlat) {
                                                rewardLayer->m_starsLabel->setString(numToString(displayReward).c_str());
                                                rewardLayer->m_stars = displayReward;
                                          } else {
                                                rewardLayer->m_moonsLabel->setString(numToString(displayReward).c_str());
                                                rewardLayer->m_moons = displayReward;
                                          }

                                          auto texture = CCTextureCache::sharedTextureCache()->addImage(
                                              isPlat ? "RL_planetBig.png"_spr : "RL_starBig.png"_spr, false);
                                          auto displayFrame = CCSpriteFrame::createWithTexture(texture, {{0, 0}, texture->getContentSize()});

                                          if (isPlat) {
                                                rewardLayer->m_starsSprite->setDisplayFrame(displayFrame);
                                          } else {
                                                rewardLayer->m_moonsSprite->setDisplayFrame(displayFrame);
                                          }
                                          rewardLayer->m_currencyBatchNode->setTexture(texture);

                                          for (auto sprite : CCArrayExt<CurrencySprite>(rewardLayer->m_objects)) {
                                                sprite->m_burstSprite->setVisible(false);
                                                if (auto child = sprite->getChildByIndex(0)) {
                                                      child->setVisible(false);
                                                }
                                                sprite->setDisplayFrame(displayFrame);
                                          }

                                          layerRef->addChild(rewardLayer, 100);
                                          // @geode-ignore(unknown-resource)
                                          FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
                                    }
                              } else {
                                    log::info("Reward animation disabled");
                                    Notification::create("Received " + numToString(difficulty) + " " + reward + "!",
                                                         CCSprite::createWithSpriteFrameName(medSprite.c_str()), 2.f)
                                        ->show();
                                    FMODAudioEngine::sharedEngine()->playEffect(
                                        // @geode-ignore(unknown-resource)
                                        "gold02.ogg");
                                    // do the fake reward circle wave effect
                                    auto fakeCircleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                                    fakeCircleWave->m_color = ccWHITE;
                                    fakeCircleWave->setPosition(difficultySprite->getPosition());
                                    layerRef->addChild(fakeCircleWave, 1);
                              }
                        } else {
                              log::warn("level already completed and rewarded beforehand");
                        }
                  });
            }

            // Map difficulty to difficultyLevel
            int difficultyLevel = 0;
            bool isDemon = false;
            switch (difficulty) {
                  case 1:
                        difficultyLevel = -1;
                        break;
                  case 2:
                        difficultyLevel = 1;
                        break;
                  case 3:
                        difficultyLevel = 2;
                        break;
                  case 4:
                  case 5:
                        difficultyLevel = 3;
                        break;
                  case 6:
                  case 7:
                        difficultyLevel = 4;
                        break;
                  case 8:
                  case 9:
                        difficultyLevel = 5;
                        break;
                  case 10:
                        difficultyLevel = 7;
                        isDemon = true;
                        break;
                  case 15:
                        difficultyLevel = 8;
                        isDemon = true;
                        break;
                  case 20:
                        difficultyLevel = 6;
                        isDemon = true;
                        break;
                  case 25:
                        difficultyLevel = 9;
                        isDemon = true;
                        break;
                  case 30:
                        difficultyLevel = 10;
                        isDemon = true;
                        break;
                  default:
                        difficultyLevel = 0;
                        break;
            }

            // Update the existing difficulty sprite
            auto difficultySprite2 = layerRef->getChildByID("difficulty-sprite");
            if (difficultySprite2) {
                  auto sprite = static_cast<GJDifficultySprite*>(difficultySprite2);
                  sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);

                  if (auto moreDifficultiesSpr = getChildByID("uproxide.more_difficulties/more-difficulties-spr")) {
                        moreDifficultiesSpr->setVisible(false);
                        sprite->setOpacity(255);
                  }

                  // Remove existing icon
                  auto existingStarIcon = difficultySprite2->getChildByID("rl-star-icon");
                  if (existingStarIcon) {
                        existingStarIcon->removeFromParent();
                  }

                  CCSprite* starIcon = nullptr;
                  // Choose icon based on platformer flag: planets for platformer levels
                  if (layerRef && layerRef->m_level && layerRef->m_level->isPlatformer()) {
                        starIcon = CCSprite::createWithSpriteFrameName("RL_planetSmall.png"_spr);
                        if (!starIcon) starIcon = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
                  }
                  if (!starIcon) starIcon = CCSprite::createWithSpriteFrameName("RL_starSmall.png"_spr);
                  if (starIcon) {
                        starIcon->setPosition({difficultySprite2->getContentSize().width / 2 + 7, -7});
                        starIcon->setScale(0.75f);
                        starIcon->setID("rl-star-icon");
                        difficultySprite2->addChild(starIcon);
                  } else {
                        log::warn("Failed to create star icon for level {}", layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);
                  }

                  // star value label (update or create)
                  auto existingLabel = difficultySprite2->getChildByID("rl-star-label");
                  auto starLabelValue =
                      existingLabel ? static_cast<CCLabelBMFont*>(existingLabel)
                                    : CCLabelBMFont::create(numToString(difficulty).c_str(),
                                                            "bigFont.fnt");

                  if (existingLabel) {
                        if (starLabelValue) starLabelValue->setString(numToString(difficulty).c_str());
                  } else if (starLabelValue) {
                        starLabelValue->setID("rl-star-label");
                        if (starIcon) {
                              starLabelValue->setPosition({starIcon->getPositionX() - 7, starIcon->getPositionY()});
                        }
                        starLabelValue->setScale(0.4f);
                        starLabelValue->setAnchorPoint({1.0f, 0.5f});
                        starLabelValue->setAlignment(kCCTextAlignmentRight);
                        difficultySprite2->addChild(starLabelValue);
                  } else {
                        log::warn("Failed to create star label for level {}", layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);
                  }

                  if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(layerRef->m_level->m_levelID)) {
                        if (layerRef && layerRef->m_level && layerRef->m_level->isPlatformer()) {
                              starLabelValue->setColor({255, 160, 0});  // orange for planets
                        } else {
                              starLabelValue->setColor({0, 150, 255});  // cyan for stars
                        }
                  }

                  auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                  auto coinIcon2 = layerRef->getChildByID("coin-icon-2");
                  auto coinIcon3 = layerRef->getChildByID("coin-icon-3");

                  if (!layerRef->m_fields->m_difficultyOffsetApplied && coinIcon1) {
                        // save original Y so we can revert on refresh
                        if (!layerRef->m_fields->m_originalYSaved) {
                              layerRef->m_fields->m_originalDifficultySpriteY = sprite->getPositionY();
                              layerRef->m_fields->m_originalYSaved = true;
                        }
                        // save original coin Y positions once
                        if (!layerRef->m_fields->m_originalCoinsSaved) {
                              layerRef->m_fields->m_originalCoin1Y = coinIcon1->getPositionY();
                              if (coinIcon2) layerRef->m_fields->m_originalCoin2Y = coinIcon2->getPositionY();
                              if (coinIcon3) layerRef->m_fields->m_originalCoin3Y = coinIcon3->getPositionY();
                              layerRef->m_fields->m_originalCoinsSaved = true;
                        }

                        layerRef->m_fields->m_lastAppliedIsDemon = isDemon;

                        // compute absolute base and set absolute Y to avoid drift
                        float baseY = layerRef->m_fields->m_originalDifficultySpriteY;
                        bool coinPresent = coinIcon1 != nullptr;
                        log::debug("Applying difficulty offset (processLevelRating): baseY={}, isDemon={}, coinPresent={}", baseY, isDemon, coinPresent);
                        float newY = baseY + (isDemon ? 15.f : 10.f);
                        sprite->setPositionY(newY);
                        log::debug("Sprite Y after apply (processLevelRating): {}", sprite->getPositionY());
                        layerRef->m_fields->m_difficultyOffsetApplied = true;

                        // set coin absolute positions based on saved originals
                        int delta = isDemon ? 6 : 4;
                        coinIcon1->setPositionY(layerRef->m_fields->m_originalCoin1Y - delta);
                        if (coinIcon2) coinIcon2->setPositionY(layerRef->m_fields->m_originalCoin2Y - delta);
                        if (coinIcon3) coinIcon3->setPositionY(layerRef->m_fields->m_originalCoin3Y - delta);
                  } else if (!layerRef->m_fields->m_difficultyOffsetApplied && !coinIcon1) {
                        // No coins, but still apply offset for levels without coins
                        // save original Y so we can revert on refresh
                        if (!layerRef->m_fields->m_originalYSaved) {
                              float currentY = sprite->getPositionY();
                              log::debug("Saving originalDifficultySpriteY (no coins): {}", currentY);
                              layerRef->m_fields->m_originalDifficultySpriteY = currentY;
                              layerRef->m_fields->m_originalYSaved = true;
                        }
                        layerRef->m_fields->m_lastAppliedIsDemon = isDemon;
                        float baseY = layerRef->m_fields->m_originalDifficultySpriteY;
                        log::debug("Applying difficulty offset (no coins): baseY={}, isDemon={}", baseY, isDemon);
                        float newY = baseY + (isDemon ? 15.f : 10.f);
                        sprite->setPositionY(newY);
                        log::debug("Sprite Y after apply (no coins): {}", sprite->getPositionY());
                        layerRef->m_fields->m_difficultyOffsetApplied = true;
                  }
            }

            // Update featured coin visibility
            if (difficultySprite2) {
                  auto featuredCoin = difficultySprite2->getChildByID("featured-coin");
                  auto epicFeaturedCoin = difficultySprite2->getChildByID("epic-featured-coin");
                  if (featured == 1) {
                        // ensure epic is removed
                        if (epicFeaturedCoin) epicFeaturedCoin->removeFromParent();
                        if (!featuredCoin) {
                              auto newFeaturedCoin = CCSprite::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
                              newFeaturedCoin->setPosition({difficultySprite2->getContentSize().width / 2,
                                                            difficultySprite2->getContentSize().height / 2});
                              newFeaturedCoin->setID("featured-coin");
                              difficultySprite2->addChild(newFeaturedCoin, -1);
                        }
                  } else if (featured == 2) {
                        // ensure standard is removed
                        if (featuredCoin) featuredCoin->removeFromParent();
                        if (!epicFeaturedCoin) {
                              auto newEpicCoin = CCSprite::createWithSpriteFrameName("RL_epicFeaturedCoin.png"_spr);
                              newEpicCoin->setPosition({difficultySprite2->getContentSize().width / 2,
                                                        difficultySprite2->getContentSize().height / 2});
                              newEpicCoin->setID("epic-featured-coin");
                              difficultySprite2->addChild(newEpicCoin, -1);
                        }
                  } else {
                        if (featuredCoin) featuredCoin->removeFromParent();
                        if (epicFeaturedCoin) epicFeaturedCoin->removeFromParent();
                  }
            }

            // Replace coin sprites if level is not suggested
            bool isSuggested = json["isSuggested"].asBool().unwrapOrDefault();
            if (!isSuggested) {
                  auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                  auto coinIcon2 = layerRef->getChildByID("coin-icon-2");
                  auto coinIcon3 = layerRef->getChildByID("coin-icon-3");

                  auto replaceCoinSprite = [layerRef](CCNode* coinNode, int coinIndex) {
                        if (!coinNode || !layerRef || !layerRef->m_level) return;
                        auto coinSprite = typeinfo_cast<CCSprite*>(coinNode);
                        if (!coinSprite) return;

                        // Check if coin exists/collected using getCoinKey
                        std::string coinKey = layerRef->m_level->getCoinKey(coinIndex);
                        log::debug("Checking coin collected status for coin {}: key={}", coinIndex, coinKey);
                        bool coinCollected = GameStatsManager::sharedState()->hasPendingUserCoin(coinKey.c_str());
                        auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName("RL_BlueCoinSmall.png"_spr);
                        CCTexture2D* blueCoinTexture = nullptr;
                        if (!frame) {
                              blueCoinTexture = CCTextureCache::sharedTextureCache()->addImage("RL_BlueCoinSmall.png"_spr, false);
                              if (blueCoinTexture) {
                                    frame = CCSpriteFrame::createWithTexture(blueCoinTexture, {{0, 0}, blueCoinTexture->getContentSize()});
                              }
                        } else {
                              blueCoinTexture = frame->getTexture();
                        }

                        if (!frame || !blueCoinTexture) {
                              log::warn("Failed to load blue coin frame/texture, skipping coin replacement for coin {}", coinIndex);
                              return;
                        }

                        if (coinCollected) {
                              coinSprite->setDisplayFrame(frame);
                              coinSprite->setColor({255, 255, 255});
                              coinSprite->setScale(0.6f);
                              log::debug("Replaced coin {} sprite with blue coin", coinIndex);
                        } else {
                              coinSprite->setDisplayFrame(frame);
                              coinSprite->setColor({120, 120, 120});
                              coinSprite->setScale(0.6f);
                              log::debug("Darkened coin {} because not collected", coinIndex);
                        }
                  };

                  replaceCoinSprite(coinIcon1, 1);
                  replaceCoinSprite(coinIcon2, 2);
                  replaceCoinSprite(coinIcon3, 3);
            }

            // delayed reposition to ensure proper star label placement after frame updates
            // ts is stupid cuz of the levelUpdate function shenanigans
            auto delayAction = CCDelayTime::create(0.01f);
            auto callFunc = CCCallFunc::create(
                layerRef, callfunc_selector(RLLevelInfoLayer::repositionRLStars));
            auto sequence = CCSequence::create(delayAction, callFunc, nullptr);
            layerRef->runAction(sequence);
            log::debug("repositionRLStars callback scheduled");
      }

      void processLevelUpdateWithDifficulty(const matjson::Value& json,
                                            Ref<RLLevelInfoLayer> layerRef,
                                            bool forceShow = false) {
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();

            // handle community vote button visibility when level updates are fetched
            bool showCommunity = forceShow;

            auto removeExistingCommunityBtn = [layerRef]() {
                  auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                  if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                        auto existing = static_cast<CCMenu*>(rightMenuNode)->getChildByID("rl-community-vote");
                        if (existing) {
                              existing->removeFromParent();
                              static_cast<CCMenu*>(rightMenuNode)->updateLayout();
                        }
                  }
            };

            if (showCommunity) {
                  bool exists = false;
                  auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                  if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                        if (static_cast<CCMenu*>(rightMenuNode)->getChildByID("rl-community-vote")) exists = true;
                  }
                  if (!exists) {
                        // determine whether we can use the level's percentage fields
                        int normalPct = -1;
                        int practicePct = -1;
                        bool hasPctFields = false;

                        if (layerRef && layerRef->m_level) {
                              hasPctFields = true;
                              normalPct = layerRef->m_level->m_normalPercent;
                              practicePct = layerRef->m_level->m_practicePercent;
                        }

                        bool shouldDisable = true;
                        if (hasPctFields) {
                              shouldDisable = !(normalPct >= 20 || practicePct >= 80);
                        }

                        // Mods/Admins can always vote regardless of percentages
                        int userRole = Mod::get()->getSavedValue<int>("role");
                        if (userRole == 1 || userRole == 2) {
                              shouldDisable = false;
                              log::debug("Community vote enabled due to role override (role={})", userRole);
                        }

                        auto commSpriteGray = CCSpriteGrayscale::createWithSpriteFrameName("RL_commVote01.png"_spr);
                        if (commSpriteGray) {
                              auto commBtnSpr = CircleButtonSprite::create(commSpriteGray, CircleBaseColor::Gray, CircleBaseSize::Medium);
                              auto commBtnItem = CCMenuItemSpriteExtra::create(commBtnSpr, layerRef, menu_selector(RLLevelInfoLayer::onCommunityVote));
                              commBtnItem->setID("rl-community-vote");

                              // Initially disabled and visually grayed
                              commBtnItem->setEnabled(false);
                              commBtnItem->setOpacity(100);

                              auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                              if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                                    static_cast<CCMenu*>(rightMenuNode)->addChild(commBtnItem);
                                    static_cast<CCMenu*>(rightMenuNode)->updateLayout();

                                    // Reactivate immediately if it should be enabled according to data
                                    if (!shouldDisable) {
                                          auto commSpriteColor = CCSprite::createWithSpriteFrameName("RL_commVote01.png"_spr);
                                          if (commSpriteColor) {
                                                auto coloredBtnSpr = CircleButtonSprite::create(commSpriteColor, CircleBaseColor::Green, CircleBaseSize::Medium);
                                                commBtnItem->setNormalImage(coloredBtnSpr);
                                          }
                                          commBtnItem->setEnabled(true);
                                          commBtnItem->setOpacity(255);
                                    }
                              } else {
                                    log::warn("Right-side-menu not found; community vote button not added");
                              }
                        }
                  } else {
                        // update existing button state based on current data
                        int normalPct = -1;
                        int practicePct = -1;
                        bool hasPctFields = false;

                        if (layerRef && layerRef->m_level) {
                              hasPctFields = true;
                              normalPct = layerRef->m_level->m_normalPercent;
                              practicePct = layerRef->m_level->m_practicePercent;
                        }

                        bool shouldDisable = true;
                        if (hasPctFields) {
                              shouldDisable = !(normalPct >= 20 || practicePct >= 80);
                        }

                        int userRole = Mod::get()->getSavedValue<int>("role");
                        if (userRole == 1 || userRole == 2) {
                              shouldDisable = false;
                              log::debug("Community vote enabled due to role override (role={})", userRole);
                        }

                        auto rightMenuNode2 = layerRef->getChildByID("right-side-menu");
                        if (rightMenuNode2 && typeinfo_cast<CCMenu*>(rightMenuNode2)) {
                              auto existing = static_cast<CCMenu*>(rightMenuNode2)->getChildByID("rl-community-vote");
                              if (existing) {
                                    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(existing);
                                    if (menuItem) {
                                          menuItem->setEnabled(!shouldDisable);
                                          menuItem->setOpacity(shouldDisable ? 100 : 255);
                                    }
                              }
                        }
                  }
            } else {
                  removeExistingCommunityBtn();
            }

            // If no difficulty rating, remove from cache
            if (difficulty == 0) {
                  auto cachePath = getCachePath();
                  auto existingData = utils::file::readString(cachePath);
                  if (existingData) {
                        auto parsed = matjson::parse(existingData.unwrap());
                        if (parsed) {
                              auto root = parsed.unwrap();
                              if (root.isObject()) {
                                    std::string key = fmt::format("{}", layerRef->m_level->m_levelID);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult = utils::file::writeString(cachePath, jsonString);
                              log::debug("Removed level ID {} from cache (no difficulty)",
                                         layerRef->m_level->m_levelID);
                        }
                  }
                  return;
            }

            auto difficultySprite = layerRef->getChildByID("difficulty-sprite");
            if (difficultySprite) {
                  auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);

                  bool isDemon = false;
                  switch (difficulty) {
                        case 10:
                        case 15:
                        case 20:
                        case 25:
                        case 30:
                              isDemon = true;
                              break;
                  }

                  int difficultyLevel = 0;
                  switch (difficulty) {
                        case 1:
                              difficultyLevel = -1;
                              break;
                        case 2:
                              difficultyLevel = 1;
                              break;
                        case 3:
                              difficultyLevel = 2;
                              break;
                        case 4:
                        case 5:
                              difficultyLevel = 3;
                              break;
                        case 6:
                        case 7:
                              difficultyLevel = 4;
                              break;
                        case 8:
                        case 9:
                              difficultyLevel = 5;
                              break;
                        case 10:
                              difficultyLevel = 7;
                              break;
                        case 15:
                              difficultyLevel = 8;
                              break;
                        case 20:
                              difficultyLevel = 6;
                              break;
                        case 25:
                              difficultyLevel = 9;
                              break;
                        case 30:
                              difficultyLevel = 10;
                              break;
                        default:
                              difficultyLevel = 0;
                              break;
                  }

                  // RESET: Remove all star icons and labels FIRST
                  auto existingStarIcon = sprite->getChildByID("rl-star-icon");
                  auto existingStarLabel = sprite->getChildByID("rl-star-label");

                  if (existingStarIcon) {
                        existingStarIcon->removeFromParent();
                  }

                  if (existingStarLabel) {
                        existingStarLabel->removeFromParent();
                  }

                  // update difficulty frame
                  sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);

                  if (auto moreDifficultiesSpr = getChildByID("uproxide.more_difficulties/more-difficulties-spr")) {
                        moreDifficultiesSpr->setVisible(false);
                        sprite->setOpacity(255);
                  }

                  // AFTER frame update - apply offsets using saved absolute positions to avoid cumulative drift
                  // save original Y and coin positions if not already saved
                  if (!layerRef->m_fields->m_originalYSaved) {
                        layerRef->m_fields->m_originalDifficultySpriteY = sprite->getPositionY();
                        layerRef->m_fields->m_originalYSaved = true;
                  }

                  auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                  auto coinIcon2 = layerRef->getChildByID("coin-icon-2");
                  auto coinIcon3 = layerRef->getChildByID("coin-icon-3");
                  if (!layerRef->m_fields->m_originalCoinsSaved && coinIcon1) {
                        layerRef->m_fields->m_originalCoin1Y = coinIcon1->getPositionY();
                        if (coinIcon2) layerRef->m_fields->m_originalCoin2Y = coinIcon2->getPositionY();
                        if (coinIcon3) layerRef->m_fields->m_originalCoin3Y = coinIcon3->getPositionY();
                        layerRef->m_fields->m_originalCoinsSaved = true;
                  }

                  layerRef->m_fields->m_lastAppliedIsDemon = isDemon;
                  float baseY = layerRef->m_fields->m_originalDifficultySpriteY;
                  log::debug("Applying difficulty offset (processLevelUpdateWithDifficulty): originalYSaved={}, baseY={}, isDemon={}", layerRef->m_fields->m_originalYSaved, baseY, isDemon);
                  float newY = baseY + (isDemon ? 15.f : 10.f);
                  sprite->setPositionY(newY);
                  log::debug("Sprite Y after apply (processLevelUpdateWithDifficulty): {}", sprite->getPositionY());

                  if (coinIcon1) {
                        int delta = isDemon ? 6 : 4;
                        coinIcon1->setPositionY(layerRef->m_fields->m_originalCoin1Y - delta);
                        if (coinIcon2) coinIcon2->setPositionY(layerRef->m_fields->m_originalCoin2Y - delta);
                        if (coinIcon3) coinIcon3->setPositionY(layerRef->m_fields->m_originalCoin3Y - delta);
                  }

                  // Choose icon based on platformer flag: planets for platformer levels
                  CCSprite* starIcon = nullptr;
                  if (layerRef && layerRef->m_level && layerRef->m_level->isPlatformer()) {
                        starIcon = CCSprite::createWithSpriteFrameName("RL_planetSmall.png"_spr);
                        if (!starIcon) starIcon = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
                  }
                  if (!starIcon) starIcon = CCSprite::createWithSpriteFrameName("RL_starSmall.png"_spr);
                  if (starIcon) {
                        starIcon->setScale(0.75f);
                        starIcon->setID("rl-star-icon");
                        sprite->addChild(starIcon);
                  } else {
                        log::warn("Failed to create star icon (update) for level {}", layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);
                  }

                  auto starLabel = CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
                  if (starLabel) {
                        starLabel->setID("rl-star-label");
                        starLabel->setScale(0.4f);
                        starLabel->setAnchorPoint({1.0f, 0.5f});
                        starLabel->setAlignment(kCCTextAlignmentRight);
                        sprite->addChild(starLabel);
                        if (starIcon) {
                              starIcon->setPosition({sprite->getContentSize().width / 2 + 7, -7});
                              starLabel->setPosition({starIcon->getPositionX() - 7, starIcon->getPositionY()});
                        }
                  } else {
                        log::warn("Failed to create star label (update) for level {}", layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);
                  }

                  // Update featured coin position
                  auto featureCoin = sprite->getChildByID("featured-coin");
                  auto epicFeatureCoin = sprite->getChildByID("epic-featured-coin");
                  if (featureCoin) {
                        featureCoin->setPosition({difficultySprite->getContentSize().width / 2,
                                                  difficultySprite->getContentSize().height / 2});
                  }
                  if (epicFeatureCoin) {
                        epicFeatureCoin->setPosition({difficultySprite->getContentSize().width / 2,
                                                      difficultySprite->getContentSize().height / 2});
                  }

                  // delayed reposition for stars after frame update to ensure
                  // proper positioning
                  auto delayAction = CCDelayTime::create(0.15f);
                  auto callFunc = CCCallFunc::create(
                      layerRef, callfunc_selector(RLLevelInfoLayer::repositionRLStars));
                  auto sequence = CCSequence::create(delayAction, callFunc, nullptr);
                  layerRef->runAction(sequence);
                  log::debug(
                      "levelUpdateFinished: repositionRLStars callback "
                      "scheduled");
            }
      }
      void onRoleButton(CCObject* sender) {
            int starRatings = this->m_level->m_stars;
            bool isPlatformer = this->m_level->isPlatformer();

            if (starRatings != 0) {
                  FLAlertLayer::create("Action Unavailable",
                                       "You cannot perform this action on "
                                       "<cy>officially rated levels</c>.",
                                       "OK")
                      ->show();
                  return;
            }

            int userRole = Mod::get()->getSavedValue<int>("role");
            if (userRole != 1 && userRole != 2) {
                  requestStatus(GJAccountManager::get()->m_accountID);
                  return;
            }

            if (userRole == 1) {
                  log::info("Role button clicked as Mod");
                  auto popup = RLModRatePopup::create(RLModRatePopup::PopupRole::Mod, "Mod: Suggest Layout", this->m_level);
                  if (popup) popup->show();
            } else if (userRole == 2) {
                  log::info("Role button clicked as Admin");
                  auto popup = RLModRatePopup::create(RLModRatePopup::PopupRole::Admin, "Admin: Rate Layout", this->m_level);
                  if (popup) popup->show();
            }
      }

      void onCommunityVote(CCObject* sender) {
            int normalPct = this->m_level->m_normalPercent;
            int practicePct = this->m_level->m_practicePercent;
            bool shouldDisable = true;
            shouldDisable = !(normalPct >= 20 || practicePct >= 80);

            int userRole = Mod::get()->getSavedValue<int>("role");
            if (userRole == 1 || userRole == 2) {
                  shouldDisable = false;
                  log::debug("Community vote enabled due to role override (role={})", userRole);
            }

            if (shouldDisable) {
                  log::info("Community vote button clicked!");
                  FLAlertLayer::create(
                      "Insufficient Completion",
                      "You need to have <co>completed</c> at least <cg>20% in </c>"
                      "<cg>normal mode</c> or <cf>80% in practice mode</c> to access "
                      "the <cb>Community Vote.</c>",
                      "OK")
                      ->show();
                  return;
            }
            int levelId = 0;
            if (this->m_level) levelId = this->m_level->m_levelID;
            auto popup = RLCommunityVotePopup::create(levelId);
            if (popup) popup->show();
      }

      // bruh
      void repositionRLStars() {
            log::info("repositionRLStars() called!");
            auto difficultySprite = this->getChildByID("difficulty-sprite");
            if (difficultySprite) {
                  auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);
                  auto starIcon = static_cast<CCSprite*>(
                      difficultySprite->getChildByID("rl-star-icon"));
                  auto starLabel = static_cast<CCLabelBMFont*>(
                      difficultySprite->getChildByID("rl-star-label"));

                  if (starIcon && sprite) {
                        starIcon->setPosition({sprite->getContentSize().width / 2 + 7, -7});

                        if (starLabel) {
                              float labelX = starIcon->getPositionX() - 7;
                              float labelY = starIcon->getPositionY();
                              log::info("Repositioning star label to ({}, {})", labelX, labelY);
                              starLabel->setPosition({labelX, labelY});
                              if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(this->m_level->m_levelID)) {
                                    if (this->m_level && this->m_level->isPlatformer()) {
                                          starLabel->setColor({255, 160, 0});  // orange
                                    } else {
                                          starLabel->setColor({0, 150, 255});  // cyan
                                    }
                              }
                        }
                  }
            }
      };

      void requestStatus(int accountId) {
            // argon my beloved <3
            std::string token;
            auto res = argon::startAuth(
                [](Result<std::string> res) {
                      if (!res) {
                            log::warn("Auth failed: {}", res.unwrapErr());
                            Notification::create(res.unwrapErr(), NotificationIcon::Error)
                                ->show();
                      }
                      auto token = std::move(res).unwrap();
                      log::debug("token obtained: {}", token);
                      Mod::get()->setSavedValue("argon_token", token);
                },
                [](argon::AuthProgress progress) {
                      log::debug("auth progress: {}",
                                 argon::authProgressToString(progress));
                });
            if (!res) {
                  log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                  Notification::create(res.unwrapErr(), NotificationIcon::Error)->show();
            }

            // json boody crap
            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] =
                Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = accountId;

            // verify the user's role
            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            m_fields->m_accessTask = postReq.post("https://gdrate.arcticwoof.xyz/access");

            Ref<RLLevelInfoLayer> layerRef = this;

            // handle the response
            m_fields->m_accessTask.listen([layerRef](web::WebResponse* response) {
                  log::info("Received response from server");

                  if (!layerRef) {
                        log::warn("LevelInfoLayer has been destroyed, skipping role update");
                        return;
                  }

                  if (!response->ok()) {
                        log::warn("Server returned non-ok status: {}", response->code());
                        return;
                  }

                  auto jsonRes = response->json();
                  if (!jsonRes) {
                        log::warn("Failed to parse JSON response");
                        return;
                  }

                  auto json = jsonRes.unwrap();
                  int role = json["role"].asInt().unwrapOrDefault();

                  // role check lol
                  if (role == 1) {
                        log::info("User has mod role");
                        Mod::get()->setSavedValue<int>("role", role);
                  } else if (role == 2) {
                        log::info("User has admin role. Enable featured layouts");
                        Mod::get()->setSavedValue<int>("role", role);
                  } else {
                        Mod::get()->setSavedValue<int>("role", 0);
                  }

                  // Refresh the community vote button immediately so role overrides take effect
                  auto cached = getCachedLevel(layerRef->m_level->m_levelID);
                  if (cached) {
                        auto rightMenuNode = layerRef->getChildByID("right-side-menu");
                        if (rightMenuNode && typeinfo_cast<CCMenu*>(rightMenuNode)) {
                              auto existing = static_cast<CCMenu*>(rightMenuNode)->getChildByID("rl-community-vote");
                              if (existing) existing->removeFromParent();
                              static_cast<CCMenu*>(rightMenuNode)->updateLayout();
                        }

                        // Re-run processing with cached data to recreate the button with the correct state
                        layerRef->processLevelRating(cached.value(), layerRef, layerRef->getChildByID("difficulty-sprite"));
                  }
            });
      }

      void likedItem(LikeItemType type, int id, bool liked) override {
            LevelInfoLayer::likedItem(type, id, liked);
            this->updateRLLevelInfo();
      }

      // this shouldnt exist but it must be done to fix that positions
      void levelUpdateFinished(GJGameLevel* level, UpdateResponse response) override {
            LevelInfoLayer::levelUpdateFinished(level, response);
            this->updateRLLevelInfo();
      }

      void updateRLLevelInfo() {
            auto difficultySpriteNode = this->getChildByID("difficulty-sprite");
            if (difficultySpriteNode) {
                  // remove star icon and label if present
                  auto starIcon = difficultySpriteNode->getChildByID("rl-star-icon");
                  if (starIcon) starIcon->removeFromParent();
                  auto starLabel = difficultySpriteNode->getChildByID("rl-star-label");
                  if (starLabel) starLabel->removeFromParent();

                  // remove featured/epic coins if present
                  auto featuredCoin = difficultySpriteNode->getChildByID("featured-coin");
                  if (featuredCoin) featuredCoin->removeFromParent();
                  auto epicFeaturedCoin = difficultySpriteNode->getChildByID("epic-featured-coin");
                  if (epicFeaturedCoin) epicFeaturedCoin->removeFromParent();

                  // reset difficulty frame and revert any applied offsets
                  auto sprite = static_cast<GJDifficultySprite*>(difficultySpriteNode);
                  // sprite->updateDifficultyFrame(0, GJDifficultyName::Long); // this breaks on non-rated layouts

                  if (this->m_fields->m_originalYSaved) {
                        log::debug("restoring original Y (originalYSaved={}, originalY={})", this->m_fields->m_originalYSaved, this->m_fields->m_originalDifficultySpriteY);
                        sprite->setPositionY(this->m_fields->m_originalDifficultySpriteY);
                        auto coinIcon1 = this->getChildByID("coin-icon-1");
                        auto coinIcon2 = this->getChildByID("coin-icon-2");
                        auto coinIcon3 = this->getChildByID("coin-icon-3");
                        if (this->m_fields->m_originalCoinsSaved) {
                              if (coinIcon1) coinIcon1->setPositionY(this->m_fields->m_originalCoin1Y);
                              if (coinIcon2) coinIcon2->setPositionY(this->m_fields->m_originalCoin2Y);
                              if (coinIcon3) coinIcon3->setPositionY(this->m_fields->m_originalCoin3Y);
                        }

                        // reset flags so next fetch can apply offsets cleanly
                        this->m_fields->m_difficultyOffsetApplied = false;
                        this->m_fields->m_originalYSaved = false;
                        this->m_fields->m_lastAppliedIsDemon = false;
                        this->m_fields->m_originalCoinsSaved = false;
                  }
            }

            // reposition (cleared state) and then fetch fresh data to re-apply rating
            this->repositionRLStars();
            this->fetchRLLevelInfo();
      }

      void fetchRLLevelInfo() {
            if (this->m_level && this->m_level->m_levelID != 0) {
                  log::debug("Refreshing level info for level ID: {}",
                             this->m_level->m_levelID);

                  int levelId = this->m_level->m_levelID;
                  deleteLevelFromCache(levelId);  // clear cache to force fresh fetch

                  auto getReq = web::WebRequest();
                  m_fields->m_fetchTask = getReq.get(fmt::format(
                      "https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

                  Ref<RLLevelInfoLayer> layerRef = this;

                  m_fields->m_fetchTask.listen([layerRef, levelId](web::WebResponse* response) {
                        log::info("Received updated rating data from server for level ID: {}",
                                  levelId);

                        if (!layerRef) {
                              log::warn("LevelInfoLayer has been destroyed");
                              return;
                        }

                        // If server returned non-ok, its okay, just remove existing data... sob
                        if (!response->ok()) {
                              if (response->code() == 404) {
                                    auto difficultySprite = layerRef->getChildByID("difficulty-sprite");
                                    if (difficultySprite) {
                                          auto starIcon = difficultySprite->getChildByID("rl-star-icon");
                                          if (starIcon) starIcon->removeFromParent();
                                          auto starLabel = difficultySprite->getChildByID("rl-star-label");
                                          if (starLabel) starLabel->removeFromParent();
                                    }
                              }
                              return;
                        }

                        if (!response->json()) {
                              return;
                        }

                        auto json = response->json().unwrap();

                        int difficulty = json["difficulty"].asInt().unwrapOrDefault();
                        if (difficulty == 0) {
                              // remove label and icon if present
                              auto difficultySprite = layerRef->getChildByID("difficulty-sprite");
                              if (difficultySprite) {
                                    auto starIcon = difficultySprite->getChildByID("rl-star-icon");
                                    if (starIcon) starIcon->removeFromParent();
                                    auto starLabel = difficultySprite->getChildByID("rl-star-label");
                                    if (starLabel) starLabel->removeFromParent();

                                    // revert any applied difficulty Y offset and coin shifts
                                    if (layerRef->m_fields->m_originalYSaved) {
                                          auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);
                                          sprite->setPositionY(layerRef->m_fields->m_originalDifficultySpriteY);

                                          auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                                          auto coinIcon2 = layerRef->getChildByID("coin-icon-2");
                                          auto coinIcon3 = layerRef->getChildByID("coin-icon-3");
                                          // restore coins to original saved positions when available
                                          if (layerRef->m_fields->m_originalCoinsSaved) {
                                                if (coinIcon1) coinIcon1->setPositionY(layerRef->m_fields->m_originalCoin1Y);
                                                if (coinIcon2) coinIcon2->setPositionY(layerRef->m_fields->m_originalCoin2Y);
                                                if (coinIcon3) coinIcon3->setPositionY(layerRef->m_fields->m_originalCoin3Y);
                                          } else {
                                                int delta = layerRef->m_fields->m_lastAppliedIsDemon ? 6 : 4;
                                                if (coinIcon1) coinIcon1->setPositionY(coinIcon1->getPositionY() + delta);
                                                if (coinIcon2) coinIcon2->setPositionY(coinIcon2->getPositionY() + delta);
                                                if (coinIcon3) coinIcon3->setPositionY(coinIcon3->getPositionY() + delta);
                                          }

                                          // reset flags
                                          layerRef->m_fields->m_difficultyOffsetApplied = false;
                                          layerRef->m_fields->m_originalYSaved = false;
                                          layerRef->m_fields->m_lastAppliedIsDemon = false;
                                          layerRef->m_fields->m_originalCoinsSaved = false;
                                    }
                              }

                              // also remove from cache to match behavior elsewhere
                              auto cachePath = getCachePath();
                              auto existingData = utils::file::readString(cachePath);
                              if (existingData) {
                                    auto parsed = matjson::parse(existingData.unwrap());
                                    if (parsed) {
                                          auto root = parsed.unwrap();
                                          if (root.isObject()) {
                                                std::string key = fmt::format("{}", levelId);
                                                auto result = root.erase(key);
                                          }
                                          auto jsonString = root.dump();
                                          auto writeResult = utils::file::writeString(cachePath, jsonString);
                                          log::debug("Removed level ID {} from cache (no difficulty) on refresh", levelId);
                                    }
                              }

                              return;
                        }

                        // Cache the updated response
                        cacheLevelData(levelId, json);

                        // Update the display with latest data
                        layerRef->processLevelRating(
                            json, layerRef, layerRef->getChildByID("difficulty-sprite"), true);
                  });
            }
      }

      // delete cache and refresh when level is updated
      void onUpdate(CCObject* sender) {
            if (this->m_level && this->m_level->m_levelID != 0 && shouldDownloadLevel()) {
                  log::debug("Level updated, clearing cache for level ID: {}",
                             this->m_level->m_levelID);
                  deleteLevelFromCache(this->m_level->m_levelID);
                  this->repositionRLStars();
            }
            LevelInfoLayer::onUpdate(sender);
      }
};

// delete cache when deleting level
class $modify(RLGameLevelManager, GameLevelManager) {
      void deleteLevel(GJGameLevel* level) {
            if (level && level->m_levelID != 0) {
                  log::info("Deleting level ID: {} from cache", level->m_levelID);
                  deleteLevelFromCache(level->m_levelID);
            }
            GameLevelManager::deleteLevel(level);
      }
};