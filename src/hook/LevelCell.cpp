#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>

using namespace geode::prelude;

// get the cache file path
static std::string getCachePath() {
      auto saveDir = dirs::getModsSaveDir();
      return utils::string::pathToString(saveDir / "level_ratings_cache.json");
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

class $modify(LevelCell) {
      struct Fields {
            utils::web::WebTask m_fetchTask;
            ~Fields() { m_fetchTask.cancel(); }
      };

      void loadCustomLevelCell(int levelId) {
            // load from cache first
            auto cachedData = getCachedLevel(levelId);

            if (cachedData) {
                  log::debug("Loading cached rating data for level cell ID: {}", levelId);
                  this->applyRatingToCell(cachedData.value(), levelId);
                  return;
            }

            if (Mod::get()->getSettingValue<bool>("compatibilityMode")) {
                  // log::debug("skipping /fetch request for level ID: {}", levelId);
                  return;
            }

            log::debug("Fetching rating data for level cell ID: {}", levelId);

            auto getReq = web::WebRequest();
            m_fields->m_fetchTask = getReq.get(
                fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

            Ref<LevelCell> cellRef = this;

            m_fields->m_fetchTask.listen([cellRef, levelId, this](web::WebResponse* response) {
                  log::debug("Received rating response from server for level cell ID: {}",
                             levelId);

                  // Validate that the cell still exists
                  if (!cellRef || !cellRef->m_mainLayer) {
                        log::warn(
                            "LevelCell has been destroyed, skipping update for level "
                            "ID: {}",
                            levelId);
                        return;
                  }

                  if (!response->ok()) {
                        log::warn("Server returned non-ok status: {} for level ID: {}",
                                  response->code(), levelId);
                        return;
                  }

                  auto jsonRes = response->json();
                  if (!jsonRes) {
                        log::warn("Failed to parse JSON response");
                        return;
                  }

                  auto json = jsonRes.unwrap();

                  // Cache the response
                  cacheLevelData(levelId, json);

                  this->applyRatingToCell(json, levelId);
            });
      }

      void applyRatingToCell(const matjson::Value& json, int levelId) {
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();

            log::debug("difficulty: {}, featured: {}", difficulty, featured);

            // If no difficulty rating, remove from cache
            if (difficulty == 0) {
                  auto cachePath = getCachePath();
                  auto existingData = utils::file::readString(cachePath);
                  if (existingData) {
                        auto parsed = matjson::parse(existingData.unwrap());
                        if (parsed) {
                              auto root = parsed.unwrap();
                              if (root.isObject()) {
                                    std::string key = fmt::format("{}", levelId);
                                    root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult = utils::file::writeString(cachePath, jsonString);
                              log::debug("Removed level ID {} from cache (no difficulty)", levelId);
                        }
                  }
                  return;
            }

            // Map difficulty
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

            // update  difficulty sprite
            auto difficultyContainer =
                m_mainLayer->getChildByID("difficulty-container");
            if (!difficultyContainer) {
                  log::warn("difficulty-container not found");  // womp womp
                  return;
            }

            auto difficultySprite =
                difficultyContainer->getChildByID("difficulty-sprite");
            if (!difficultySprite) {
                  log::warn("difficulty-sprite not found");
                  return;
            }

            difficultySprite->setPositionY(5);
            auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);
            sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Short);

            if (auto moreDifficultiesSpr = difficultyContainer->getChildByID("uproxide.more_difficulties/more-difficulties-spr")) {
                  moreDifficultiesSpr->setVisible(false);
                  sprite->setOpacity(255);
            }

            // star or planet icon (planet for platformer levels)
            CCSprite* newStarIcon = nullptr;
            if (this->m_level && this->m_level->isPlatformer()) {
                  newStarIcon = CCSprite::create("RL_planetSmall.png"_spr);
                  if (!newStarIcon) newStarIcon = CCSprite::create("RL_planetMed.png"_spr);
            }
            if (!newStarIcon) newStarIcon = CCSprite::create("RL_starSmall.png"_spr);
            if (newStarIcon) {
                  newStarIcon->setPosition({difficultySprite->getContentSize().width / 2 + 8, -8});
                  newStarIcon->setScale(0.75f);
                  newStarIcon->setID("rl-star-icon");
                  difficultySprite->addChild(newStarIcon);

                  // reward value label
                  auto rewardLabelValue =
                      CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
                  if (rewardLabelValue) {
                        rewardLabelValue->setPosition(
                            {newStarIcon->getPositionX() - 7, newStarIcon->getPositionY()});
                        rewardLabelValue->setScale(0.4f);
                        rewardLabelValue->setAnchorPoint({1.0f, 0.5f});
                        rewardLabelValue->setAlignment(kCCTextAlignmentRight);
                        rewardLabelValue->setID("rl-reward-label");
                        difficultySprite->addChild(rewardLabelValue);

                        if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(m_level->m_levelID)) {
                              if (this->m_level && this->m_level->isPlatformer()) {
                                    rewardLabelValue->setColor({255, 160, 0});  // orange for planets
                              } else {
                                    rewardLabelValue->setColor({0, 150, 255});  // cyan for stars
                              }
                        }
                  }

                  // Update featured coin visibility
                  {
                        auto featuredCoin = difficultySprite->getChildByID("featured-coin");
                        auto epicFeaturedCoin = difficultySprite->getChildByID("epic-featured-coin");
                        if (featured == 1) {
                              if (epicFeaturedCoin) epicFeaturedCoin->removeFromParent();
                              if (!featuredCoin) {
                                    auto newFeaturedCoin = CCSprite::create("RL_featuredCoin.png"_spr);
                                    if (newFeaturedCoin) {
                                          newFeaturedCoin->setPosition({difficultySprite->getContentSize().width / 2,
                                                                        difficultySprite->getContentSize().height / 2});
                                          newFeaturedCoin->setID("featured-coin");
                                          difficultySprite->addChild(newFeaturedCoin, -1);
                                    }
                              }
                        } else if (featured == 2) {
                              if (featuredCoin) featuredCoin->removeFromParent();
                              if (!epicFeaturedCoin) {
                                    auto newEpicCoin = CCSprite::create("RL_epicFeaturedCoin.png"_spr);
                                    if (newEpicCoin) {
                                          newEpicCoin->setPosition({difficultySprite->getContentSize().width / 2,
                                                                    difficultySprite->getContentSize().height / 2});
                                          newEpicCoin->setID("epic-featured-coin");
                                          difficultySprite->addChild(newEpicCoin, -1);
                                    }
                              }
                        } else {
                              if (featuredCoin) featuredCoin->removeFromParent();
                              if (epicFeaturedCoin) epicFeaturedCoin->removeFromParent();
                        }
                  }

                  // handle coin icons (if compact view, fetch coin nodes directly from m_mainLayer)
                  auto coinContainer = m_mainLayer->getChildByID("difficulty-container");
                  if (coinContainer) {
                        CCNode* coinIcon1 = nullptr;
                        CCNode* coinIcon2 = nullptr;
                        CCNode* coinIcon3 = nullptr;

                        if (!m_compactView) {
                              coinIcon1 = coinContainer->getChildByID("coin-icon-1");
                              coinIcon2 = coinContainer->getChildByID("coin-icon-2");
                              coinIcon3 = coinContainer->getChildByID("coin-icon-3");
                        } else {
                              // compact view: coin icons live on the main layer
                              coinIcon1 = m_mainLayer->getChildByID("coin-icon-1");
                              coinIcon2 = m_mainLayer->getChildByID("coin-icon-2");
                              coinIcon3 = m_mainLayer->getChildByID("coin-icon-3");
                        }

                        // push difficulty sprite down if coins exist in non-compact view
                        if ((coinIcon1 || coinIcon2 || coinIcon3) && !m_compactView) {
                              difficultySprite->setPositionY(difficultySprite->getPositionY() + 10);
                        }

                        // Replace or darken coins when level is not suggested
                        bool isSuggested = json["isSuggested"].asBool().unwrapOrDefault();
                        if (!isSuggested) {
                              // try to obtain a GJGameLevel for coin keys
                              GJGameLevel* levelPtr = this->m_level;
                              if (!levelPtr) {
                                    auto glm = GameLevelManager::sharedState();
                                    auto stored = glm->getStoredOnlineLevels(fmt::format("{}", levelId).c_str());
                                    if (stored && stored->count() > 0) {
                                          levelPtr = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
                                    }
                              }

                              auto replaceCoinSprite = [levelPtr, this](CCNode* coinNode, int coinIndex) {
                                    auto blueCoinTexture = CCTextureCache::sharedTextureCache()->addImage("RL_BlueCoinSmall.png"_spr, false);
                                    auto displayFrame = CCSpriteFrame::createWithTexture(blueCoinTexture, {{0, 0}, blueCoinTexture->getContentSize()});
                                    if (!coinNode) return;
                                    auto coinSprite = typeinfo_cast<CCSprite*>(coinNode);
                                    if (!coinSprite) return;

                                    bool coinCollected = false;
                                    if (levelPtr) {
                                          std::string coinKey = levelPtr->getCoinKey(coinIndex);
                                          log::debug("LevelCell: checking coin {} key={}", coinIndex, coinKey);
                                          coinCollected = GameStatsManager::sharedState()->hasPendingUserCoin(coinKey.c_str());
                                    }

                                    if (coinCollected) {
                                          coinSprite->setDisplayFrame(displayFrame);
                                          coinSprite->setColor({255, 255, 255});
                                          coinSprite->setOpacity(255);
                                          coinSprite->setScale(0.6f);
                                          log::debug("LevelCell: replaced coin {} with blue sprite", coinIndex);
                                    } else {
                                          coinSprite->setDisplayFrame(displayFrame);
                                          coinSprite->setColor({120, 120, 120});
                                          coinSprite->setScale(0.6f);
                                          log::debug("LevelCell: darkened coin {} (not present)", coinIndex);
                                    }

                                    if (m_compactView) {
                                          coinSprite->setScale(0.4f);
                                    }
                              };

                              replaceCoinSprite(coinIcon1, 1);
                              replaceCoinSprite(coinIcon2, 2);
                              replaceCoinSprite(coinIcon3, 3);
                        }

                        // doing the dumb coin move
                        if (!m_compactView) {
                              if (coinIcon1) {
                                    coinIcon1->setPositionY(coinIcon1->getPositionY() - 5);
                              }
                              if (coinIcon2) {
                                    coinIcon2->setPositionY(coinIcon2->getPositionY() - 5);
                              }
                              if (coinIcon3) {
                                    coinIcon3->setPositionY(coinIcon3->getPositionY() - 5);
                              }
                        }
                  }
            }
      }

      void
      loadFromLevel(GJGameLevel* level) {
            LevelCell::loadFromLevel(level);

            // no local levels
            if (!level || level->m_levelID == 0) {
                  return;
            }

            loadCustomLevelCell(level->m_levelID);
      }

      void onEnter() { LevelCell::onEnter(); }

      void refreshLevelCell() {
            if (this->m_level && this->m_level->m_levelID != 0) {
                  log::debug("Refreshing level cell ID: {}", this->m_level->m_levelID);
                  loadCustomLevelCell(this->m_level->m_levelID);
            }
      }
};