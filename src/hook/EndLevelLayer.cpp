#include "../custom/RLAchievements.hpp"
#include "../custom/RubyUtils.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

// remove UI tint from end-level coin sprites/backgrounds and animated coins
static void removeUITint(Ref<EndLevelLayer> layer) {
  if (!layer || !layer->m_mainLayer)
    return;

  for (int i = 1; i <= 3; ++i) {
    auto spriteNode =
        layer->m_mainLayer->getChildByID(fmt::format("coin-{}-sprite", i));
    auto bgNode =
        layer->m_mainLayer->getChildByID(fmt::format("coin-{}-background", i));
    if (spriteNode) {
      if (auto cs = typeinfo_cast<CCSprite *>(spriteNode)) {
        cs->setColor(ccWHITE);
      }
    }
    if (bgNode) {
      if (auto bs = typeinfo_cast<CCSprite *>(bgNode)) {
        bs->setColor(ccWHITE);
      }
    }
  }

  if (layer->m_coinsToAnimate) {
    for (auto spr : CCArrayExt<CCSprite>(layer->m_coinsToAnimate)) {
      if (spr)
        spr->setColor(ccWHITE);
    }
  }
}

class $modify(EndLevelLayer) {
  struct Fields {
    async::TaskHolder<web::WebResponse> m_getTask;
    async::TaskHolder<web::WebResponse> m_submitTask;
    ~Fields() {
      m_getTask.cancel();
      m_submitTask.cancel();
    }
  };

  void customSetup() override {
    EndLevelLayer::customSetup(); // call original method cuz if not then
                                  // it breaks lol
    auto playLayer = PlayLayer::get();
    if (!playLayer)
      return;

    GJGameLevel *level = playLayer->m_level;
    if (!level || level->m_levelID == 0) {
      return;
    }

    // this is so only when you actually completed the level and detects
    // that you did then give those yum yum stars, otherwise u use safe mod
    // to do crap lmaoooo
    if (level->m_normalPercent >= 100) {
      log::info("Level ID: {} completed for the first time!", level->m_levelID);
    } else {
      log::info("Level ID: {} not completed, skipping reward",
                level->m_levelID);
      return;
    }

    // Fetch rating data from server to get difficulty value
    int levelId = level->m_levelID;
    log::info("Fetching difficulty value for completed level ID: {}", levelId);

    auto getReq = web::WebRequest();

    // capture EndLevelLayer as Ref
    Ref<EndLevelLayer> endLayerRef = this;

    m_fields->m_getTask.spawn(
        getReq.get(fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}",
                               levelId)),
        [this, endLayerRef, levelId, level](web::WebResponse response) {
          log::info("Received rating response for completed level ID: {}",
                    levelId);

          // is the endlevellayer exists? idk mayeb
          if (!endLayerRef) {
            log::warn("EndLevelLayer has been destroyed, skipping reward");
            return;
          }

          if (!response.ok()) {
            log::warn("Server returned non-ok status: {} for level ID: {}",
                      response.code(), levelId);
            return;
          }

          auto jsonRes = response.json();
          if (!jsonRes) {
            log::warn("Failed to parse JSON response");
            return;
          }

          auto json = jsonRes.unwrap();
          int difficulty = json["difficulty"].asInt().unwrapOrDefault();

          log::debug("Difficulty value: {}", difficulty);

          // If level is not suggested, update end-level coin visuals
          bool coinVerified = json["coinVerified"].asBool().unwrapOrDefault();
          if (coinVerified) {
            log::debug(
                "Level {} coin verified; updating end-level coin visuals",
                levelId);
            endLayerRef->m_coinsVerified =
                true; // always show its verified so it shows that white coins
            removeUITint(endLayerRef);
            auto blueFrame =
                CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                    "RL_BlueCoinUI.png"_spr);
            CCTexture2D *blueTex = nullptr;
            if (!blueFrame) {
              blueTex = CCTextureCache::sharedTextureCache()->addImage(
                  "RL_BlueCoinUI.png"_spr, false);
              if (blueTex) {
                blueFrame = CCSpriteFrame::createWithTexture(
                    blueTex, {{0, 0}, blueTex->getContentSize()});
              }
            } else {
              blueTex = blueFrame->getTexture();
            }

            // prefer a frame from the spritesheet; fall back to raw texture for
            // empty coin bg
            auto emptyFrame =
                CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                    "RL_BlueCoinEmpty1.png"_spr);
            CCTexture2D *emptyTex = nullptr;
            if (!emptyFrame) {
              emptyTex = CCTextureCache::sharedTextureCache()->addImage(
                  "RL_BlueCoinEmpty1.png"_spr, false);
              if (emptyTex) {
                emptyFrame = CCSpriteFrame::createWithTexture(
                    emptyTex, {{0, 0}, emptyTex->getContentSize()});
              }
            } else {
              emptyTex = emptyFrame->getTexture();
            }

            if (endLayerRef && endLayerRef->m_mainLayer) {
              for (int i = 1; i <= 3; ++i) {
                auto spriteNode = endLayerRef->m_mainLayer->getChildByID(
                    fmt::format("coin-{}-sprite", i));
                auto bgNode = endLayerRef->m_mainLayer->getChildByID(
                    fmt::format("coin-{}-background", i));
                if (spriteNode) {
                  if (auto cs = typeinfo_cast<CCSprite *>(spriteNode)) {
                    cs->setDisplayFrame(blueFrame);
                    cs->setColor(ccWHITE);
                  }
                }
                if (bgNode) {
                  if (auto bs = typeinfo_cast<CCSprite *>(bgNode)) {
                    bs->setDisplayFrame(emptyFrame);
                  }
                }
              }
            }

            // Set animate coins to white
            if (endLayerRef->m_coinsToAnimate) {
              for (auto spr :
                   CCArrayExt<CCSprite>(endLayerRef->m_coinsToAnimate)) {
                if (spr)
                  spr->setColor(ccWHITE);
              }
            }
          }

          // Reward stars based on difficulty value
          int starReward = std::max(0, difficulty);
          log::debug("Star reward amount: {}", starReward);

          if (starReward == 30) {
            RLAchievements::onReward("misc_extreme");
          }

          int accountId = GJAccountManager::get()->m_accountID;
          std::string argonToken =
              Mod::get()->getSavedValue<std::string>("argon_token");

          // verification stuff
          int attempts = 0;
          int attemptTime = 0;
          int jumps = 0;
          int clicks = 0;
          bool isPlat = false;
          if (auto playLayer = PlayLayer::get()) {
            if (playLayer->m_level) {
              attempts = playLayer->m_level->m_attempts;
              isPlat = playLayer->m_level->isPlatformer();
              attemptTime = isPlat ? (playLayer->m_level->m_bestTime / 1000)
                                   : playLayer->m_level->m_attemptTime;
              jumps = playLayer->m_level->m_jumps;
              clicks = playLayer->m_level->m_clicks;
            }
            if (jumps == 0) {
              jumps = playLayer->m_jumps;
            }
          }

          log::debug("Submitting completion with attempts: {} time: {} jumps: "
                     "{} clicks: {}",
                     attempts, attemptTime, jumps, clicks);

          // Build comma-separated coins list based on pending user coins
          std::string coinsStr;
          if (level) {
            std::vector<int> coins;
            for (int i = 1; i <= 3; ++i) {
              auto coinKey = level->getCoinKey(i);
              if (GameStatsManager::sharedState()->hasPendingUserCoin(
                      coinKey)) {
                coins.push_back(i);
              }
            }
            for (size_t idx = 0; idx < coins.size(); ++idx) {
              if (idx)
                coinsStr += ",";
              coinsStr += std::to_string(coins[idx]);
            }
          } else {
            log::warn("Unable to build coins list: level pointer is null for "
                      "levelId {}",
                      levelId);
          }
          log::debug("Collected pending coins for level {}: {}", levelId,
                     coinsStr);

          matjson::Value jsonBody;
          jsonBody["accountId"] = accountId;
          jsonBody["argonToken"] = argonToken;
          jsonBody["levelId"] = levelId;
          jsonBody["attempts"] = attempts;
          jsonBody["attemptTime"] = attemptTime;
          jsonBody["isPlat"] = isPlat;
          jsonBody["jumps"] = jumps;
          jsonBody["clicks"] = clicks;
          if (!coinsStr.empty()) {
            jsonBody["coins"] = coinsStr;
          }

          auto submitReq = web::WebRequest();
          submitReq.bodyJSON(jsonBody);

          m_fields->m_submitTask.spawn(
              submitReq.post("https://gdrate.arcticwoof.xyz/submitComplete"),
              [endLayerRef, starReward, levelId, isPlat,
               level](web::WebResponse submitResponse) {
                log::info("Received submitComplete response for level ID: {}",
                          levelId);

                if (!endLayerRef) {
                  log::warn("EndLevelLayer has been destroyed");
                  return;
                }

                if (!submitResponse.ok()) {
                  log::warn("submitComplete returned non-ok status: {}",
                            submitResponse.code());
                  return;
                }

                auto submitJsonRes = submitResponse.json();
                if (!submitJsonRes) {
                  log::warn("Failed to parse submitComplete JSON response");
                  return;
                }

                auto submitJson = submitJsonRes.unwrap();
                bool success = submitJson["success"].asBool().unwrapOrDefault();
                int responseStars =
                    submitJson["stars"].asInt().unwrapOrDefault();
                int responsePlanets =
                    submitJson["planets"].asInt().unwrapOrDefault();
                int responseCoins =
                    submitJson["coins"].asInt().unwrapOrDefault();

                log::info("submitComplete success: {}, response stars: {}",
                          success, responseStars);

                if (success) {
                  // check for coins increases
                  int oldCoins = Mod::get()->getSavedValue<int>("coins", 0);
                  if (responseCoins > oldCoins) {
                    RLAchievements::onUpdated(
                        RLAchievements::Collectable::Coins, oldCoins,
                        responseCoins);
                  }
                  Mod::get()->setSavedValue<int>("coins", responseCoins);
                  std::string rewards = isPlat ? "Planets" : "Sparks";
                  std::string medSprite =
                      isPlat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;

                  // achievements for Sparks/Planets increment
                  int oldStars = Mod::get()->getSavedValue<int>("stars", 0);
                  int oldPlanets = Mod::get()->getSavedValue<int>("planets", 0);
                  int oldRubies = Mod::get()->getSavedValue<int>("rubies", 0);

                  int responseAmount = isPlat ? responsePlanets : responseStars;
                  int delta = responseAmount - (isPlat ? oldPlanets : oldStars);
                  int newAmount = (isPlat ? oldPlanets : oldStars) + delta;

                  if (delta > 0) {
                    if (isPlat) {
                      RLAchievements::onUpdated(
                          RLAchievements::Collectable::Planets, oldPlanets,
                          newAmount);
                    } else {
                      RLAchievements::onUpdated(
                          RLAchievements::Collectable::Sparks, oldStars,
                          newAmount);
                    }
                  }

                  // Also check current totals for retroactive awards
                  RLAchievements::checkAll(RLAchievements::Collectable::Sparks,
                                           responseStars);
                  RLAchievements::checkAll(RLAchievements::Collectable::Planets,
                                           responsePlanets);

                  if (responseStars == 0 && responsePlanets == 0) {
                    log::warn("No stars or planets rewarded, possibly already "
                              "rewarded before");
                    Notification::create(
                        rewards + " has already been claimed for this level!",
                        CCSprite::createWithSpriteFrameName(medSprite.c_str()))
                        ->show();
                    return;
                  }
                  int displayStars = isPlat ? (responsePlanets - starReward)
                                            : (responseStars - starReward);
                  if (isPlat) {
                    log::info("Display planets: {} - {} = {}", responsePlanets,
                              starReward, displayStars);
                    Mod::get()->setSavedValue<int>("planets", responsePlanets);
                  } else {
                    log::info("Display stars: {} - {} = {}", responseStars,
                              starReward, displayStars);
                    Mod::get()->setSavedValue<int>("stars", responseStars);
                  }

                  if (!endLayerRef || !endLayerRef->m_mainLayer) {
                    log::warn("m_mainLayer is invalid");
                    return;
                  }

                  // make the stars reward pop when u complete the level
                  auto bigStarSprite =
                      isPlat ? CCSprite::createWithSpriteFrameName(
                                   "RL_planetBig.png"_spr)
                             : CCSprite::createWithSpriteFrameName(
                                   "RL_starBig.png"_spr);
                  bigStarSprite->setScale(1.f);
                  bigStarSprite->setPosition(
                      {endLayerRef->m_mainLayer->getContentSize().width / 2 +
                           135,
                       endLayerRef->m_mainLayer->getContentSize().height / 2 +
                           20});
                  bigStarSprite->setOpacity(0);
                  bigStarSprite->setScale(1.2f);
                  bigStarSprite->setID(isPlat ? "rl-planet-reward-sprite"
                                              : "rl-star-reward-sprite");
                  endLayerRef->m_mainLayer->addChild(bigStarSprite);

                  // put the difficulty value next to the big star
                  // making this look like an actual star reward in game
                  auto difficultyLabel = CCLabelBMFont::create(
                      fmt::format("+{}", starReward).c_str(), "bigFont.fnt");
                  difficultyLabel->setPosition(
                      {-5, bigStarSprite->getContentSize().height / 2});
                  difficultyLabel->setAnchorPoint({1.0f, 0.5f});
                  difficultyLabel->setID(isPlat ? "rl-planet-reward-label"
                                                : "rl-star-reward-label");
                  // start the label invisible so it can fade in with the star
                  difficultyLabel->setOpacity(0);
                  bigStarSprite->addChild(difficultyLabel);

                  // star animation lol
                  auto scaleAction = CCScaleBy::create(.6f, .6f);
                  auto bounceAction = CCEaseBounceOut::create(scaleAction);
                  auto fadeAction = CCFadeIn::create(0.4f);
                  auto spawnAction =
                      CCSpawn::createWithTwoActions(bounceAction, fadeAction);
                  bigStarSprite->runAction(spawnAction);

                  // make the difficulty label fade in along with the star
                  difficultyLabel->runAction(
                      CCFadeIn::create(fadeAction->getDuration()));

                  int difficultyForRubies = starReward; // same as difficulty
                  auto rubyInfo =
                      rl::computeRubyInfo(level, difficultyForRubies);
                  int totalRuby = rubyInfo.total;
                  int collected = rubyInfo.collected;
                  int remaining = rubyInfo.remaining;
                  int calcAtPercent = rubyInfo.calcAtPercent;
                  int awardableByPercent =
                      std::max(0, calcAtPercent - collected);
                  int remainingRubies = remaining;
                  int percent = level ? level->m_normalPercent : 0;

                  log::debug(
                      "Computed rubies for end-level reward: total={}, "
                      "collected={}, percent={}, calcAtPercent={}, "
                      "awardableByPercent={}, remaining={}, remainingRubies={}",
                      totalRuby, collected, percent, calcAtPercent,
                      awardableByPercent, remaining, remainingRubies);

                  if (remainingRubies > 0) {
                    auto rubyPop = CCSprite::createWithSpriteFrameName(
                        "RL_bigRuby.png"_spr);
                    rubyPop->setID("rl-ruby-pop-sprite");
                    if (rubyPop) {
                      rubyPop->setPosition(
                          {bigStarSprite->getPositionX(),
                           bigStarSprite->getPositionY() - 40.f});
                      rubyPop->setOpacity(0);

                      // create a label for the awarded rubies and attach to the
                      // ruby pop sprite (positions to the right of the ruby)
                      auto rubyLabel = CCLabelBMFont::create(
                          (std::string("+") + numToString(remainingRubies))
                              .c_str(),
                          "bigFont.fnt");
                      if (rubyLabel) {
                        rubyLabel->setID("rl-ruby-pop-label");
                        rubyLabel->setAnchorPoint({1.f, 0.5f});
                        rubyLabel->setPosition(
                            {-14, rubyPop->getContentSize().height / 2});
                        rubyLabel->setOpacity(0);
                        rubyPop->addChild(rubyLabel, 1);
                      }

                      // add beneath the big star (one layer below)
                      endLayerRef->m_mainLayer->addChild(rubyPop);
                      rubyPop->runAction(
                          static_cast<CCAction *>(spawnAction->copy()));

                      // fade-in the label alongside the pop
                      if (rubyLabel) {
                        rubyLabel->runAction(
                            CCFadeIn::create(fadeAction->getDuration()));
                      }
                    }
                  }

                  // never used this before but its fancy
                  // some devices crashes from this, idk why soggify
                  if (!Mod::get()->getSettingValue<bool>(
                          "disableRewardAnimation")) {
                    if (auto rewardLayer = CurrencyRewardLayer::create(
                            0, isPlat ? starReward : 0, isPlat ? 0 : starReward,
                            remainingRubies, CurrencySpriteType::Star, 0,
                            CurrencySpriteType::Star, 0,
                            bigStarSprite->getPosition(),
                            CurrencyRewardType::Default, 0.0, 1.0)) {
                      // display the calculated stars
                      if (isPlat) {
                        rewardLayer->m_stars = 0;
                        rewardLayer->incrementStarsCount(displayStars);
                      } else {
                        rewardLayer->m_moons = 0;
                        rewardLayer->incrementMoonsCount(displayStars);
                      }

                      // If rubies were awarded, use the diamonds slot on the
                      // reward layer to show rubies and replace diamond
                      // sprite with ruby sprite (display shows percent total)
                      if (remainingRubies > 0) {
                        if (rewardLayer->m_diamondsLabel) {
                          rewardLayer->m_diamonds = 0;
                          rewardLayer->incrementDiamondsCount(oldRubies);
                        }

                        // persist updated collected rubies (clamp to total)
                        int newCollected = collected + remainingRubies;
                        if (newCollected > totalRuby)
                          newCollected = totalRuby;
                        bool wrote = rl::persistCollectedRubies(
                            level ? level->m_levelID : 0, totalRuby,
                            newCollected);
                        if (!wrote) {
                          log::warn("Failed to write rubies_collected.json for "
                                    "level {}",
                                    level ? level->m_levelID : 0);
                        } else {
                          log::debug("Updated rubies_collected.json: level {} "
                                     "collected -> {}",
                                     level ? level->m_levelID : 0,
                                     newCollected);
                        }

                        // increment global saved rubies
                        int oldGlobal =
                            Mod::get()->getSavedValue<int>("rubies", 0);
                        Mod::get()->setSavedValue<int>(
                            "rubies", oldGlobal + remainingRubies);
                      }

                      std::string frameName = isPlat ? "RL_planetBig.png"_spr
                                                     : "RL_starBig.png"_spr;
                      auto displayFrame =
                          CCSpriteFrameCache::sharedSpriteFrameCache()
                              ->spriteFrameByName((frameName).c_str());
                      CCTexture2D *texture = nullptr;
                      if (!displayFrame) {
                        texture =
                            CCTextureCache::sharedTextureCache()->addImage(
                                (frameName).c_str(), false);
                        if (texture) {
                          displayFrame = CCSpriteFrame::createWithTexture(
                              texture, {{0, 0}, texture->getContentSize()});
                        }
                      } else {
                        texture = displayFrame->getTexture();
                      }

                      // Setup ruby sprite frame for diamond repurposing
                      std::string rubyFrameName = "RL_bigRuby.png"_spr;
                      auto rubyDisplayFrame =
                          CCSpriteFrameCache::sharedSpriteFrameCache()
                              ->spriteFrameByName((rubyFrameName).c_str());
                      CCTexture2D *rubyTexture = nullptr;
                      if (!rubyDisplayFrame) {
                        rubyTexture =
                            CCTextureCache::sharedTextureCache()->addImage(
                                (rubyFrameName).c_str(), false);
                        if (rubyTexture) {
                          rubyDisplayFrame = CCSpriteFrame::createWithTexture(
                              rubyTexture,
                              {{0, 0}, rubyTexture->getContentSize()});
                        }
                      } else {
                        rubyTexture = rubyDisplayFrame->getTexture();
                      }

                      if (!displayFrame || !texture) {
                        log::warn("Failed to load reward frame/texture: {}",
                                  frameName);
                      } else {
                        if (isPlat) {
                          if (rewardLayer->m_starsSprite)
                            rewardLayer->m_starsSprite->setDisplayFrame(
                                displayFrame);
                        } else {
                          if (rewardLayer->m_moonsSprite)
                            rewardLayer->m_moonsSprite->setDisplayFrame(
                                displayFrame);
                        }
                        if (rewardLayer->m_currencyBatchNode)
                          rewardLayer->m_currencyBatchNode->setTexture(texture);

                        // Only set diamond->ruby frame if both exist
                        if (rewardLayer->m_diamondsSprite && rubyDisplayFrame) {
                          rewardLayer->m_diamondsSprite->setDisplayFrame(
                              rubyDisplayFrame);
                        }

                        for (auto sprite : CCArrayExt<CurrencySprite>(
                                 rewardLayer->m_objects)) {
                          if (!sprite)
                            continue;
                          if (sprite->m_burstSprite)
                            sprite->m_burstSprite->setVisible(false);
                          if (auto child = sprite->getChildByIndex(0)) {
                            child->setVisible(false);
                          }
                          if (sprite->m_spriteType ==
                              (isPlat ? CurrencySpriteType::Star
                                      : CurrencySpriteType::Moon)) {
                            sprite->setDisplayFrame(displayFrame);
                          }
                          if (sprite->m_spriteType ==
                              CurrencySpriteType::Diamond) { // repurpose
                                                             // diamond as ruby
                            if (rubyDisplayFrame) {
                              sprite->setDisplayFrame(rubyDisplayFrame);
                            }
                          }
                        }

                        endLayerRef->addChild(rewardLayer, 100);
                        FMODAudioEngine::sharedEngine()->playEffect(
                            // @geode-ignore(unknown-resource)
                            "gold02.ogg");
                      }
                    } else {
                      log::info("Reward animation disabled");
                      std::string reward = isPlat ? "planets" : "sparks";
                      Notification::create("Received " +
                                               numToString(starReward) + " " +
                                               reward + "!",
                                           CCSprite::createWithSpriteFrameName(
                                               medSprite.c_str()),
                                           2.f)
                          ->show();
                      FMODAudioEngine::sharedEngine()->playEffect(
                          // @geode-ignore(unknown-resource)
                          "gold02.ogg");
                      // do the fake reward circle wave effect
                      auto fakeCircleWave =
                          CCCircleWave::create(10.f, 110.f, 0.5f, false);
                      fakeCircleWave->m_color = ccWHITE;
                      fakeCircleWave->setPosition(bigStarSprite->getPosition());
                      endLayerRef->addChild(fakeCircleWave, 1);
                    }
                  } else if (!success &&
                             (responseStars == 0 || responsePlanets == 0)) {
                    std::string rewards = isPlat ? "Planets" : "Sparks";
                    std::string medSprite =
                        isPlat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;
                    Notification::create(
                        rewards + " has already been claimed for this level!",
                        CCSprite::createWithSpriteFrameName(medSprite.c_str()))
                        ->show();
                    log::info("already claimed for level ID: {}", levelId);
                  }
                }
              });
        });
  }
};
