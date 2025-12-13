#include <Geode/Geode.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <argon/argon.hpp>

#include "ModRatePopup.hpp"

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

            if (Mod::get()->getSavedValue<int>("role") == 1) {
                  // add a mod button
                  auto iconSprite = CCSprite::create("rlStarIconBig.png"_spr);
                  CCSprite* buttonSprite = nullptr;

                  if (isPlatformer || starRatings != 0) {
                        buttonSprite = CCSpriteGrayscale::create("rlStarIconBig.png"_spr);
                  } else {
                        buttonSprite = CCSprite::create("rlStarIconBig.png"_spr);
                  }

                  auto modButtonSpr = CircleButtonSprite::create(
                      buttonSprite, CircleBaseColor::Cyan, CircleBaseSize::Medium);

                  auto modButtonItem = CCMenuItemSpriteExtra::create(
                      modButtonSpr, this, menu_selector(RLLevelInfoLayer::onModButton));
                  modButtonItem->setID("mod-button");

                  leftMenu->addChild(modButtonItem);
            } else if (Mod::get()->getSavedValue<int>("role") == 2) {
                  // add an admin button
                  CCSprite* buttonSprite = nullptr;

                  if (isPlatformer || starRatings != 0) {
                        buttonSprite = CCSpriteGrayscale::create("rlStarIconBig.png"_spr);
                  } else {
                        buttonSprite = CCSprite::create("rlStarIconBig.png"_spr);
                  }

                  auto modButtonSpr = CircleButtonSprite::create(
                      buttonSprite, CircleBaseColor::Cyan, CircleBaseSize::Medium);

                  auto modButtonItem = CCMenuItemSpriteExtra::create(
                      modButtonSpr, this, menu_selector(RLLevelInfoLayer::onAdminButton));
                  modButtonItem->setID("admin-button");

                  leftMenu->addChild(modButtonItem);
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
            auto getTask = getReq.get(
                fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

            Ref<RLLevelInfoLayer> layerRef = this;
            auto difficultySprite = layerRef->getChildByID("difficulty-sprite");

            getTask.listen([layerRef, difficultySprite](web::WebResponse* response) {
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

                  layerRef->processLevelRating(json, layerRef, difficultySprite);
            });
      }
      void processLevelRating(const matjson::Value& json,
                              Ref<RLLevelInfoLayer> layerRef,
                              CCNode* difficultySprite) {
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();

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

                  matjson::Value jsonBody;
                  jsonBody["accountId"] = accountId;
                  jsonBody["argonToken"] = argonToken;
                  jsonBody["levelId"] = levelId;

                  auto submitReq = web::WebRequest();
                  submitReq.bodyJSON(jsonBody);
                  auto submitTask =
                      submitReq.post("https://gdrate.arcticwoof.xyz/submitComplete");

                  submitTask.listen([layerRef, difficulty, levelId,
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

                        log::info("submitComplete success: {}, response stars: {}", success,
                                  responseStars);

                        if (success) {
                              int displayStars = responseStars - difficulty;
                              log::info("Display stars: {} - {} = {}", responseStars, difficulty,
                                        displayStars);

                              if (!Mod::get()->getSettingValue<bool>("disableRewardAnimation")) {
                                    if (auto rewardLayer = CurrencyRewardLayer::create(
                                            0, difficulty, 0, 0, CurrencySpriteType::Star, 0,
                                            CurrencySpriteType::Star, 0, difficultySprite->getPosition(),
                                            CurrencyRewardType::Default, 0.0, 1.0)) {
                                          rewardLayer->m_starsLabel->setString(
                                              numToString(displayStars).c_str());
                                          rewardLayer->m_stars = displayStars;
                                          rewardLayer->m_starsSprite = CCSprite::create("rlStarIconMed.png"_spr);

                                          if (auto node =
                                                  rewardLayer->m_mainNode->getChildByType<CCSprite*>(0)) {
                                                node->setDisplayFrame(
                                                    CCSprite::create("rlStarIconMed.png"_spr)->displayFrame());
                                                node->setScale(1.f);
                                          }

                                          layerRef->addChild(rewardLayer, 100);
                                          // @geode-ignore(unknown-resource)
                                          FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
                                    }
                              } else {
                                    log::info("Reward animation disabled");
                                    Notification::create("Received " +
                                                             numToString(difficulty) + " stars!",
                                                         CCSprite::create("rlStarIconMed.png"_spr), 2.f)
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

                  auto existingStarIcon = difficultySprite2->getChildByID("rl-star-icon");
                  CCSprite* starIcon = nullptr;
                  bool isFirstTime = !existingStarIcon;

                  if (!existingStarIcon) {
                        // star icon
                        starIcon = CCSprite::create("rlStarIcon.png"_spr);
                        starIcon->setPosition(
                            {difficultySprite2->getContentSize().width / 2 + 7, -7});
                        starIcon->setScale(0.75f);
                        starIcon->setID("rl-star-icon");
                        difficultySprite2->addChild(starIcon);
                  } else {
                        starIcon = static_cast<CCSprite*>(existingStarIcon);
                  }

                  // star value label (update or create)
                  auto existingLabel = difficultySprite2->getChildByID("rl-star-label");
                  auto starLabelValue =
                      existingLabel ? static_cast<CCLabelBMFont*>(existingLabel)
                                    : CCLabelBMFont::create(numToString(difficulty).c_str(),
                                                            "bigFont.fnt");

                  if (existingLabel) {
                        starLabelValue->setString(numToString(difficulty).c_str());
                  } else {
                        starLabelValue->setID("rl-star-label");
                        starLabelValue->setPosition(
                            {starIcon->getPositionX() - 7, starIcon->getPositionY()});
                        starLabelValue->setScale(0.4f);
                        starLabelValue->setAnchorPoint({1.0f, 0.5f});
                        starLabelValue->setAlignment(kCCTextAlignmentRight);
                        difficultySprite2->addChild(starLabelValue);
                  }

                  if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(
                          layerRef->m_level->m_levelID)) {
                        starLabelValue->setColor({0, 150, 255});  // cyan
                  }

                  auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                  auto coinIcon2 = layerRef->getChildByID("coin-icon-2");
                  auto coinIcon3 = layerRef->getChildByID("coin-icon-3");

                  if (!layerRef->m_fields->m_difficultyOffsetApplied && coinIcon1) {
                        if (isDemon) {
                              sprite->setPositionY(sprite->getPositionY() + 15);
                        } else {
                              sprite->setPositionY(sprite->getPositionY() + 10);
                        }
                        layerRef->m_fields->m_difficultyOffsetApplied = true;

                        // time to adjust the coins as well
                        coinIcon1->setPositionY(coinIcon1->getPositionY() -
                                                (isDemon ? 6 : 4));  // very precise yesh
                        if (coinIcon2)
                              coinIcon2->setPositionY(coinIcon2->getPositionY() -
                                                      (isDemon ? 6 : 4));
                        if (coinIcon3)
                              coinIcon3->setPositionY(coinIcon3->getPositionY() -
                                                      (isDemon ? 6 : 4));
                  } else if (!layerRef->m_fields->m_difficultyOffsetApplied && !coinIcon1) {
                        // No coins, but still apply offset for levels without coins
                        if (isDemon) {
                              sprite->setPositionY(sprite->getPositionY() + 15);
                        } else {
                              sprite->setPositionY(sprite->getPositionY() + 10);
                        }
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
                              auto newFeaturedCoin = CCSprite::create("rlfeaturedCoin.png"_spr);
                              newFeaturedCoin->setPosition({difficultySprite2->getContentSize().width / 2,
                                                            difficultySprite2->getContentSize().height / 2});
                              newFeaturedCoin->setID("featured-coin");
                              difficultySprite2->addChild(newFeaturedCoin, -1);
                        }
                  } else if (featured == 2) {
                        // ensure standard is removed
                        if (featuredCoin) featuredCoin->removeFromParent();
                        if (!epicFeaturedCoin) {
                              auto newEpicCoin = CCSprite::create("rlepicFeaturedCoin.png"_spr);
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
      }

      // this shouldnt exist but it must be done to fix that positions
      void levelUpdateFinished(GJGameLevel* level, UpdateResponse response) {
            LevelInfoLayer::levelUpdateFinished(level, response);

            auto difficultySprite = this->getChildByID("difficulty-sprite");
            if (difficultySprite && m_fields->m_difficultyOffsetApplied) {
                  auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);

                  int levelId = this->m_level->m_levelID;

                  // Try to load from cache first
                  auto cachedData = getCachedLevel(levelId);

                  Ref<RLLevelInfoLayer> layerRef = this;

                  if (cachedData) {
                        log::debug("Using cached data for levelUpdateFinished");
                        processLevelUpdateWithDifficulty(cachedData.value(), layerRef);
                        return;
                  }

                  auto getReq = web::WebRequest();
                  auto getTask = getReq.get(fmt::format(
                      "https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

                  getTask.listen([layerRef](web::WebResponse* response) {
                        if (!layerRef) {
                              log::warn(
                                  "LevelInfoLayer has been destroyed, skipping level "
                                  "update");
                              return;
                        }

                        if (!response->ok() || !response->json()) {
                              return;
                        }

                        auto json = response->json().unwrap();

                        // Cache the response
                        cacheLevelData(layerRef->m_level->m_levelID, json);

                        layerRef->processLevelUpdateWithDifficulty(json, layerRef);
                  });
            }
      }

      void processLevelUpdateWithDifficulty(const matjson::Value& json,
                                            Ref<RLLevelInfoLayer> layerRef) {
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();

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

                  // AFTER frame update
                  if (isDemon) {
                        // if coin exists, reposition it
                        auto coinIcon1 = layerRef->getChildByID("coin-icon-1");
                        if (coinIcon1) {
                              sprite->setPositionY(sprite->getPositionY() + 20);  // lol
                        } else {
                              sprite->setPositionY(sprite->getPositionY() + 10);
                        }

                  } else {
                        sprite->setPositionY(sprite->getPositionY() + 10);
                  }

                  auto starIcon = CCSprite::create("rlStarIcon.png"_spr);
                  starIcon->setScale(0.75f);
                  starIcon->setID("rl-star-icon");
                  sprite->addChild(starIcon);

                  auto starLabel =
                      CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
                  starLabel->setID("rl-star-label");
                  starLabel->setScale(0.4f);
                  starLabel->setAnchorPoint({1.0f, 0.5f});
                  starLabel->setAlignment(kCCTextAlignmentRight);
                  sprite->addChild(starLabel);

                  starIcon->setPosition({sprite->getContentSize().width / 2 + 7, -7});
                  starLabel->setPosition(
                      {starIcon->getPositionX() - 7, starIcon->getPositionY()});

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
                      layerRef, callfunc_selector(RLLevelInfoLayer::repositionStars));
                  auto sequence = CCSequence::create(delayAction, callFunc, nullptr);
                  layerRef->runAction(sequence);
                  log::debug(
                      "levelUpdateFinished: repositionStars callback "
                      "scheduled");
            }
      }

      void onModButton(CCObject* sender) {
            int starRatings = this->m_level->m_stars;
            bool isPlatformer = this->m_level->isPlatformer();

            if (isPlatformer || starRatings != 0) {
                  FLAlertLayer::create("Action Unavailable",
                                       "You cannot perform this action on "
                                       "<cy>platformer or rated levels</c>.",
                                       "OK")
                      ->show();
                  return;
            }

            if (Mod::get()->getSavedValue<int>("role") != 1) {
                  requestStatus(GJAccountManager::get()->m_accountID);
            }

            if (Mod::get()->getSavedValue<int>("role") == 1) {
                  log::info("Mod button clicked!");
                  auto popup = ModRatePopup::create("Mod: Suggest Layout", this->m_level);
                  popup->show();
            } else {
                  Notification::create(
                      "You do not have the required role to perform this action",
                      NotificationIcon::Error)
                      ->show();
            }
      }

      void onAdminButton(CCObject* sender) {
            int starRatings = this->m_level->m_stars;
            bool isPlatformer = this->m_level->isPlatformer();

            if (isPlatformer || starRatings != 0) {
                  FLAlertLayer::create("Action Unavailable",
                                       "You cannot perform this action on "
                                       "<cy>platformer or rated levels</c>.",
                                       "OK")
                      ->show();
                  return;
            }

            if (Mod::get()->getSavedValue<int>("role") != 2) {
                  requestStatus(GJAccountManager::get()->m_accountID);
            }

            if (Mod::get()->getSavedValue<int>("role") == 2) {
                  log::info("Admin button clicked!");
                  auto popup = ModRatePopup::create("Admin: Rate Layout", this->m_level);
                  popup->show();
            } else {
                  Notification::create(
                      "You do not have the required role to perform this action",
                      NotificationIcon::Error)
                      ->show();
            }
      }

      // bruh
      void repositionStars() {
            log::info("repositionStars() called!");
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
                              if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(
                                      this->m_level->m_levelID)) {
                                    starLabel->setColor({0, 150, 255});  // cyan
                              }
                        }
                  }
            }
      }

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
            auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/access");

            Ref<RLLevelInfoLayer> layerRef = this;

            // handle the response
            postTask.listen([layerRef](web::WebResponse* response) {
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
            });
      }

      // refresh rating data when level is refreshed
      void onRefresh() {
            if (this->m_level && this->m_level->m_levelID != 0) {
                  log::debug("Refreshing level info for level ID: {}",
                             this->m_level->m_levelID);

                  int levelId = this->m_level->m_levelID;
                  deleteLevelFromCache(levelId);  // clear cache to force fresh fetch

                  auto getReq = web::WebRequest();
                  auto getTask = getReq.get(fmt::format(
                      "https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

                  Ref<RLLevelInfoLayer> layerRef = this;

                  getTask.listen([this, layerRef, levelId](web::WebResponse* response) {
                        log::info("Received updated rating data from server for level ID: {}",
                                  levelId);

                        if (!layerRef) {
                              log::warn("LevelInfoLayer has been destroyed");
                              return;
                        }

                        if (!response->ok() || !response->json()) {
                              return;
                        }

                        auto json = response->json().unwrap();

                        // Cache the updated response
                        cacheLevelData(levelId, json);

                        // Update the display with latest data
                        layerRef->processLevelRating(
                            json, layerRef, layerRef->getChildByID("difficulty-sprite"));
                  });
            }
      }

      // delete cache and refresh when level is updated
      void onUpdate(CCObject* sender) {
            if (this->m_level && this->m_level->m_levelID != 0 &&
                shouldDownloadLevel()) {
                  log::debug("Level updated, clearing cache for level ID: {}",
                             this->m_level->m_levelID);
                  deleteLevelFromCache(this->m_level->m_levelID);
                  this->repositionStars();
                  this->onRefresh();
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