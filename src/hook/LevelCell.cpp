#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

extern const std::string legendaryPString;
extern const std::string mythicPString;
extern const std::string epicPString;


static std::unordered_map<LevelCell *, int> g_deferredPendingLevels;
static std::unordered_map<LevelCell *, matjson::Value> g_deferredPendingJsons;





class $modify(LevelCell) {
  struct Fields {
    std::optional<matjson::Value> m_pendingJson;
    int m_pendingLevelId = 0;
    async::TaskHolder<web::WebResponse> m_fetchTask;
    int m_waitRetries = 0; // used for waiting for level data to arrive
    ~Fields() { m_fetchTask.cancel(); }
  };





  void applyRatingToCell(const matjson::Value &json, int levelId) {
    if (!this->m_mainLayer || !this->m_level) {
      log::warn("applyRatingToCell called but LevelCell or main layer missing "
                "for level ID: {}",
                levelId);
      return;
    }
    int difficulty = json["difficulty"].asInt().unwrapOrDefault();
    int featured = json["featured"].asInt().unwrapOrDefault();

    log::debug("difficulty: {}, featured: {}", difficulty, featured);

    // If no difficulty rating, nothing to apply
    if (difficulty == 0) {
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
      log::warn("difficulty-container not found"); // womp womp
      return;
    }

    auto difficultySprite =
        difficultyContainer->getChildByID("difficulty-sprite");
    if (!difficultySprite) {
      log::warn("difficulty-sprite not found");
      return;
    }

    difficultySprite->setPositionY(5);
    auto sprite = static_cast<GJDifficultySprite *>(difficultySprite);
    sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Short);

    if (auto moreDifficultiesSpr = difficultyContainer->getChildByID(
            "uproxide.more_difficulties/more-difficulties-spr")) {
      moreDifficultiesSpr->setVisible(false);
      sprite->setOpacity(255);
    }

    // star or planet icon (planet for platformer levels)
    // remove existing to avoid duplicate icons on repeated updates
    if (auto existingIcon = difficultySprite->getChildByID("rl-star-icon")) {
      existingIcon->removeFromParent();
    }
    if (auto existingRewardLabel =
            difficultySprite->getChildByID("rl-reward-label")) {
      existingRewardLabel->removeFromParent();
    }
    CCSprite *newStarIcon = nullptr;
    if (this->m_level && this->m_level->isPlatformer()) {
      newStarIcon =
          CCSprite::createWithSpriteFrameName("RL_planetSmall.png"_spr);
      if (!newStarIcon)
        newStarIcon = CCSprite::create("RL_planetMed.png"_spr);
    }
    if (!newStarIcon)
      newStarIcon = CCSprite::createWithSpriteFrameName("RL_starSmall.png"_spr);
    if (newStarIcon) {
      newStarIcon->setPosition(
          {difficultySprite->getContentSize().width / 2 + 8, -8});
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

        if (this->m_level &&
            GameStatsManager::sharedState()->hasCompletedOnlineLevel(
                this->m_level->m_levelID)) {
          if (this->m_level->isPlatformer()) {
            rewardLabelValue->setColor({255, 160, 0}); // orange for planets
          } else {
            rewardLabelValue->setColor({0, 150, 255}); // cyan for stars
          }
        }
      }

      // Update featured coin visibility (support featured types: 1=featured,
      // 2=epic, 3=legendary, 4=mythic)
      {
        auto featuredCoin = difficultySprite->getChildByID("featured-coin");
        auto epicFeaturedCoin =
            difficultySprite->getChildByID("epic-featured-coin");
        auto legendaryFeaturedCoin =
            difficultySprite->getChildByID("legendary-featured-coin");
        auto mythicFeaturedCoin =
            difficultySprite->getChildByID("mythic-featured-coin");

        if (featured == 1) {
          // remove other types
          if (epicFeaturedCoin)
            epicFeaturedCoin->removeFromParent();
          if (legendaryFeaturedCoin)
            legendaryFeaturedCoin->removeFromParent();
          if (mythicFeaturedCoin)
            mythicFeaturedCoin->removeFromParent();
          if (!featuredCoin) {
            auto newFeaturedCoin =
                CCSprite::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
            if (newFeaturedCoin) {
              newFeaturedCoin->setPosition(
                  {difficultySprite->getContentSize().width / 2,
                   difficultySprite->getContentSize().height / 2});
              newFeaturedCoin->setID("featured-coin");
              difficultySprite->addChild(newFeaturedCoin, -1);
            }
          }
        } else if (featured == 2) {
          // ensure only epic present
          if (featuredCoin)
            featuredCoin->removeFromParent();
          if (legendaryFeaturedCoin)
            legendaryFeaturedCoin->removeFromParent();
          if (mythicFeaturedCoin)
            mythicFeaturedCoin->removeFromParent();
          if (!epicFeaturedCoin) {
            auto newEpicCoin = CCSprite::createWithSpriteFrameName(
                "RL_epicFeaturedCoin.png"_spr);
            if (newEpicCoin) {
              newEpicCoin->setPosition(
                  {difficultySprite->getContentSize().width / 2,
                   difficultySprite->getContentSize().height / 2});
              newEpicCoin->setID("epic-featured-coin");
              difficultySprite->addChild(newEpicCoin, -1);

              // add particle (if configured) on top of epic ring
              const std::string &pString = epicPString;
              if (!pString.empty()) {
                if (auto existingP =
                        newEpicCoin->getChildByID("rating-particles")) {
                  existingP->removeFromParent();
                }
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  newEpicCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(newEpicCoin->getContentSize() / 2.f);
                  particle->setID("rating-particles"_spr);
                }
              }
            }
          }
        } else if (featured == 3) {
          // legendary
          if (featuredCoin)
            featuredCoin->removeFromParent();
          if (epicFeaturedCoin)
            epicFeaturedCoin->removeFromParent();
          if (mythicFeaturedCoin)
            mythicFeaturedCoin->removeFromParent();
          if (!legendaryFeaturedCoin) {
            auto newLegendaryCoin = CCSprite::createWithSpriteFrameName(
                "RL_legendaryFeaturedCoin.png"_spr);
            if (newLegendaryCoin) {
              newLegendaryCoin->setPosition(
                  {difficultySprite->getContentSize().width / 2,
                   difficultySprite->getContentSize().height / 2});
              newLegendaryCoin->setID("legendary-featured-coin");
              difficultySprite->addChild(newLegendaryCoin, -1);

              // particle legendary ring
              const std::string &pString = legendaryPString;
              if (!pString.empty()) {
                // remove any existing particles on this coin to avoid dupes
                if (auto existingP =
                        newLegendaryCoin->getChildByID("rating-particles")) {
                  existingP->removeFromParent();
                }
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  newLegendaryCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(newLegendaryCoin->getContentSize() /
                                        2.f);
                  particle->setID("rating-particles"_spr);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                }
              }
            }
          }
        } else if (featured == 4) {
          // mythic
          if (featuredCoin)
            featuredCoin->removeFromParent();
          if (epicFeaturedCoin)
            epicFeaturedCoin->removeFromParent();
          if (legendaryFeaturedCoin)
            legendaryFeaturedCoin->removeFromParent();
          if (!mythicFeaturedCoin) {
            auto newMythicCoin = CCSprite::createWithSpriteFrameName(
                "RL_mythicFeaturedCoin.png"_spr);
            if (newMythicCoin) {
              newMythicCoin->setPosition(
                  {difficultySprite->getContentSize().width / 2,
                   difficultySprite->getContentSize().height / 2});
              newMythicCoin->setID("mythic-featured-coin");
              difficultySprite->addChild(newMythicCoin, -1);

              // particle mythic ring
              const std::string &pString = mythicPString;
              if (!pString.empty()) {
                if (auto existingP =
                        newMythicCoin->getChildByID("rating-particles")) {
                  existingP->removeFromParent();
                }
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  newMythicCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(newMythicCoin->getContentSize() / 2.f);
                  particle->setID("rating-particles"_spr);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                }
              }
            }
          }
        } else {
          if (featuredCoin)
            featuredCoin->removeFromParent();
          if (epicFeaturedCoin)
            epicFeaturedCoin->removeFromParent();
          if (legendaryFeaturedCoin)
            legendaryFeaturedCoin->removeFromParent();
          if (mythicFeaturedCoin)
            mythicFeaturedCoin->removeFromParent();
        }

        // ensure particles exist for existing epic/legendary/mythic coins (if
        // configured)
        if (featured == 2) {
          if (epicFeaturedCoin) {
            if (!epicFeaturedCoin->getChildByID("rating-particles")) {
              const std::string &pString = epicPString;
              if (!pString.empty()) {
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  epicFeaturedCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(epicFeaturedCoin->getContentSize() /
                                        2.f);
                  particle->setID("rating-particles"_spr);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                }
              }
            }
          }
        } else if (featured == 3) {
          if (legendaryFeaturedCoin) {
            if (!legendaryFeaturedCoin->getChildByID("rating-particles")) {
              const std::string &pString = legendaryPString;
              if (!pString.empty()) {
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  legendaryFeaturedCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(
                      legendaryFeaturedCoin->getContentSize() / 2.f);
                  particle->setID("rating-particles"_spr);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                }
              }
            }
          }
        } else if (featured == 4) {
          if (mythicFeaturedCoin) {
            if (!mythicFeaturedCoin->getChildByID("rating-particles")) {
              const std::string &pString = mythicPString;
              if (!pString.empty()) {
                ParticleStruct pStruct;
                GameToolbox::particleStringToStruct(pString, pStruct);
                CCParticleSystemQuad *particle =
                    GameToolbox::particleFromStruct(pStruct, nullptr, false);
                if (particle) {
                  mythicFeaturedCoin->addChild(particle, 1);
                  particle->resetSystem();
                  particle->setPosition(mythicFeaturedCoin->getContentSize() /
                                        2.f);
                  particle->setID("rating-particles"_spr);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                  particle->update(0.15f);
                }
              }
            }
          }
        }

        // handle coin icons (if compact view, fetch coin nodes directly from
        // m_mainLayer)
        auto coinContainer = m_mainLayer->getChildByID("difficulty-container");
        if (coinContainer) {
          CCNode *coinIcon1 = nullptr;
          CCNode *coinIcon2 = nullptr;
          CCNode *coinIcon3 = nullptr;

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
            difficultySprite->setPositionY(difficultySprite->getPositionY() +
                                           10);
          }

          // Replace or darken coins when level is not suggested
          bool coinVerified = json["coinVerified"].asBool().unwrapOrDefault();
          if (coinVerified) {
            // try to obtain a GJGameLevel for coin keys
            GJGameLevel *levelPtr = this->m_level;
            if (!levelPtr) {
              auto glm = GameLevelManager::sharedState();
              auto stored = glm->getStoredOnlineLevels(
                  fmt::format("{}", levelId).c_str());
              if (stored && stored->count() > 0) {
                levelPtr = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
              }
            }

            auto replaceCoinSprite = [levelPtr, this](CCNode *coinNode,
                                                      int coinIndex) {
              auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()
                               ->spriteFrameByName("RL_BlueCoinSmall.png"_spr);
              CCTexture2D *blueCoinTexture = nullptr;
              if (!frame) {
                blueCoinTexture =
                    CCTextureCache::sharedTextureCache()->addImage(
                        "RL_BlueCoinSmall.png"_spr, true);
                if (blueCoinTexture) {
                  frame = CCSpriteFrame::createWithTexture(
                      blueCoinTexture,
                      {{0, 0}, blueCoinTexture->getContentSize()});
                }
              } else {
                blueCoinTexture = frame->getTexture();
              }

              if (!frame || !blueCoinTexture) {
                log::warn("failed to load blue coin frame/texture");
                return;
              }
              if (!coinNode)
                return;
              auto coinSprite = typeinfo_cast<CCSprite *>(coinNode);
              if (!coinSprite)
                return;

              bool coinCollected = false;
              if (levelPtr) {
                std::string coinKey = levelPtr->getCoinKey(coinIndex);
                log::debug("LevelCell: checking coin {} key={}", coinIndex,
                           coinKey);
                coinCollected =
                    GameStatsManager::sharedState()->hasPendingUserCoin(
                        coinKey.c_str());
              }

              if (coinCollected) {
                coinSprite->setDisplayFrame(frame);
                coinSprite->setColor({255, 255, 255});
                coinSprite->setOpacity(255);
                coinSprite->setScale(0.6f);
                log::debug("LevelCell: replaced coin {} with blue sprite",
                           coinIndex);
              } else {
                coinSprite->setDisplayFrame(frame);
                coinSprite->setColor({120, 120, 120});
                coinSprite->setScale(0.6f);
                log::debug("LevelCell: darkened coin {} (not present)",
                           coinIndex);
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

          // Handle pulsing level name for legendary/mythic featured
          {
            CCNode *nameNode = nullptr;
            if (m_mainLayer) {
              nameNode = m_mainLayer->getChildByID("level-name");
              if (!nameNode)
                nameNode = m_mainLayer->getChildByID("level-name-label");
              if (!nameNode)
                nameNode = m_mainLayer->getChildByID("level-label");
            }

            auto nameLabel = typeinfo_cast<CCLabelBMFont *>(nameNode);
            if (featured == 3) {
              if (nameLabel) {
                nameLabel->stopActionByTag(0xF00D);
                auto tintUp = CCTintTo::create(1.f, 150, 220, 255);
                auto tintDown = CCTintTo::create(1.f, 200, 200, 255);
                auto seq = CCSequence::create(tintUp, tintDown, nullptr);
                auto repeat = CCRepeatForever::create(seq);
                repeat->setTag(0xF00D);
                nameLabel->runAction(repeat);
              }
            } else if (featured == 4) {
              if (nameLabel) {
                nameLabel->stopActionByTag(0xF00D);
                auto tintUp = CCTintTo::create(1.f, 255, 220, 150);
                auto tintDown = CCTintTo::create(1.f, 255, 255, 200);
                auto seq = CCSequence::create(tintUp, tintDown, nullptr);
                auto repeat = CCRepeatForever::create(seq);
                repeat->setTag(0xF00D);
                nameLabel->runAction(repeat);
              }
            } else {
              if (nameLabel)
                nameLabel->stopActionByTag(0xF00D);
            }
          }
        }
      }
    }
  }

  void loadFromLevel(GJGameLevel *level) {

    LevelCell::loadFromLevel(level);
    log::debug("LevelCell loadFromLevel called for level ID: {}",
               level ? level->m_levelID : 0);
    // If no level or a local level
    if (!level || level->m_levelID == 0) {
      return;
    }

    int levelId = static_cast<int>(level->m_levelID);

    // We don't use cached data for LevelCell. If compatibilityMode is
    // enabled, mark pending and return; otherwise proceed to fetch.
    if (Mod::get()->getSettingValue<bool>("compatibilityMode")) {
      m_fields->m_pendingLevelId = levelId;
      return;
    }

    // fetch directly here and apply or store on callback
    Ref<LevelCell> cellRef = this;
    auto req = web::WebRequest();
    log::debug("Fetching rating data for level cell ID: {}", levelId);
    async::spawn(
        req.get(fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId)),
        [cellRef, levelId, this](web::WebResponse const &response) {
          log::debug("Received rating response from server for level ID: {}", levelId);

          if (!response.ok()) {
            log::warn("Server returned non-ok status: {} for level ID: {}", response.code(), levelId);
            return;
          }

          auto jsonRes = response.json();
          if (!jsonRes) {
            log::warn("Failed to parse JSON response");
            return;
          }

          auto json = jsonRes.unwrap();

          if (!cellRef) return;

          LevelCell *ptr = cellRef.operator->();
          // If the cell's UI is present and it references the same level, apply
          // the fetched rating immediately on the main thread using our own
          // instance pointer (we captured 'this').
          if (this->m_mainLayer && this->m_level && this->m_level->m_levelID == levelId) {
            log::debug("Applying fetched rating data immediately for level ID: {}", levelId);
            this->applyRatingToCell(json, levelId);
          } else {
            // Otherwise defer to the main-thread update path
            g_deferredPendingJsons[ptr] = json;
            g_deferredPendingLevels[ptr] = levelId;
            ptr->scheduleUpdate();
          }
        });

    // ensure update waits for level data to arrive (use globals to avoid
    // touching m_fields here)
    g_deferredPendingLevels[this] = levelId;
    this->scheduleUpdate();
    return;
  }

  void onEnter() {
    LevelCell::onEnter();


    if (m_fields->m_pendingJson && this->m_level &&
        this->m_level->m_levelID == m_fields->m_pendingLevelId) {
      // Apply pending JSON immediately if the cell references the same level.
      this->applyRatingToCell(m_fields->m_pendingJson.value(),
                              m_fields->m_pendingLevelId);
      m_fields->m_pendingJson = std::nullopt;
      m_fields->m_pendingLevelId = 0;
    }

    if (m_fields->m_pendingLevelId && this->m_level &&
        this->m_level->m_levelID == m_fields->m_pendingLevelId) {
      if (!Mod::get()->getSettingValue<bool>("compatibilityMode")) {
        int pending = m_fields->m_pendingLevelId;
        m_fields->m_pendingLevelId = 0;
        if (this->m_level) {
          loadFromLevel(this->m_level);
        }
      } else {
        m_fields->m_pendingLevelId = 0;
      }
    }
  }

};
