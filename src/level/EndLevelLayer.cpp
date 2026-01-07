#include <Geode/Geode.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

class $modify(EndLevelLayer) {
      void customSetup() override {
            EndLevelLayer::customSetup();  // call original method cuz if not then
                                           // it breaks lol
            auto playLayer = PlayLayer::get();
            if (!playLayer) return;

            GJGameLevel* level = playLayer->m_level;
            if (!level || level->m_levelID == 0) {
                  return;
            }

            // this is so only when you actually completed the level and detects
            // that you did then give those yum yum stars, otherwise u use safe mod
            // to do crap lmaoooo
            if (level->m_normalPercent >= 100) {
                  log::info("Level ID: {} completed for the first time!",
                            level->m_levelID);
            } else {
                  log::info("Level ID: {} not completed, skipping reward",
                            level->m_levelID);
                  return;
            }

            // Fetch rating data from server to get difficulty value
            int levelId = level->m_levelID;
            log::info("Fetching difficulty value for completed level ID: {}",
                      levelId);

            auto getReq = web::WebRequest();
            auto getTask = getReq.get(fmt::format(
                "https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

            // capture EndLevelLayer as Ref
            Ref<EndLevelLayer> endLayerRef = this;

            getTask.listen([endLayerRef, levelId](web::WebResponse* response) {
                  log::info("Received rating response for completed level ID: {}",
                            levelId);

                  // is the endlevellayer exists? idk mayeb
                  if (!endLayerRef) {
                        log::warn("EndLevelLayer has been destroyed, skipping reward");
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
                  int difficulty = json["difficulty"].asInt().unwrapOrDefault();

                  log::debug("Difficulty value: {}", difficulty);

                  // Reward stars based on difficulty value
                  int starReward = std::max(0, difficulty);
                  log::debug("Star reward amount: {}", starReward);

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
                              attemptTime = playLayer->m_level->m_attemptTime;
                              isPlat = playLayer->m_level->isPlatformer();
                              jumps = playLayer->m_level->m_jumps;
                              clicks = playLayer->m_level->m_clicks;
                        }
                        if (jumps == 0) {
                              jumps = playLayer->m_jumps;
                        }
                  }

                  log::debug("Submitting completion with attempts: {} time: {} jumps: {} clicks: {}", attempts, attemptTime, jumps, clicks);

                  matjson::Value jsonBody;
                  jsonBody["accountId"] = accountId;
                  jsonBody["argonToken"] = argonToken;
                  jsonBody["levelId"] = levelId;
                  jsonBody["attempts"] = attempts;
                  jsonBody["attemptTime"] = attemptTime;
                  jsonBody["isPlat"] = isPlat;
                  jsonBody["jumps"] = jumps;
                  jsonBody["clicks"] = clicks;

                  auto submitReq = web::WebRequest();
                  submitReq.bodyJSON(jsonBody);
                  auto submitTask =
                      submitReq.post("https://gdrate.arcticwoof.xyz/submitComplete");
                  submitTask.listen([endLayerRef, starReward, levelId, isPlat](web::WebResponse* submitResponse) {
                        log::info("Received submitComplete response for level ID: {}",
                                  levelId);

                        if (!endLayerRef) {
                              log::warn("EndLevelLayer has been destroyed");
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
                        int responseStars =
                            submitJson["stars"].asInt().unwrapOrDefault();
                        int responsePlanets = submitJson["planets"].asInt().unwrapOrDefault();

                        log::info("submitComplete success: {}, response stars: {}",
                                  success, responseStars);

                        if (success) {
                              int displayStars = isPlat ? (responsePlanets - starReward) : (responseStars - starReward);
                              if (isPlat) {
                                    log::info("Display planets: {} - {} = {}", responsePlanets, starReward, displayStars);
                                    Mod::get()->setSavedValue<int>("planets", responsePlanets);
                              } else {
                                    log::info("Display stars: {} - {} = {}", responseStars, starReward, displayStars);
                                    Mod::get()->setSavedValue<int>("stars", responseStars);
                              }

                              // choose medium icon depending on whether the level is a platformer
                              std::string medSprite = isPlat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;

                              if (!endLayerRef || !endLayerRef->m_mainLayer) {
                                    log::warn("m_mainLayer is invalid");
                                    return;
                              }

                              // make the stars reward pop when u complete the level
                              auto bigStarSprite =
                                  isPlat ? CCSprite::create("RL_planetBig.png"_spr) : CCSprite::create("RL_starBig.png"_spr);
                              bigStarSprite->setScale(1.f);
                              bigStarSprite->setPosition(
                                  {endLayerRef->m_mainLayer->getContentSize().width / 2 +
                                       135,
                                   endLayerRef->m_mainLayer->getContentSize().height /
                                       2});
                              bigStarSprite->setOpacity(0);
                              bigStarSprite->setScale(1.2f);
                              endLayerRef->m_mainLayer->addChild(bigStarSprite);

                              // star animation lol
                              auto scaleAction = CCScaleBy::create(.6f, .6f);
                              auto bounceAction = CCEaseBounceOut::create(scaleAction);
                              auto fadeAction = CCFadeIn::create(0.5f);
                              auto spawnAction =
                                  CCSpawn::createWithTwoActions(bounceAction, fadeAction);
                              bigStarSprite->runAction(spawnAction);

                              // put the difficulty value next to the big star
                              // making this look like an actual star reward in game
                              auto difficultyLabel = CCLabelBMFont::create(
                                  fmt::format("+{}", starReward).c_str(), "bigFont.fnt");
                              difficultyLabel->setPosition(
                                  {-5, bigStarSprite->getContentSize().height / 2});
                              difficultyLabel->setAnchorPoint({1.0f, 0.5f});
                              bigStarSprite->addChild(difficultyLabel);

                              // never used this before but its fancy
                              // some devices crashes from this, idk why soggify
                              if (!Mod::get()->getSettingValue<bool>("disableRewardAnimation")) {
                                    if (auto rewardLayer = CurrencyRewardLayer::create(
                                            0, isPlat ? starReward : 0, isPlat ? 0 : starReward, 0, CurrencySpriteType::Star, 0,
                                            CurrencySpriteType::Star, 0,
                                            bigStarSprite->getPosition(),
                                            CurrencyRewardType::Default, 0.0, 1.0)) {
                                          // display the calculated stars
                                          if (isPlat) {
                                                rewardLayer->m_starsLabel->setString(numToString(displayStars).c_str());
                                                rewardLayer->m_stars = displayStars;
                                          } else {
                                                rewardLayer->m_moonsLabel->setString(numToString(displayStars).c_str());
                                                rewardLayer->m_moons = displayStars;
                                          }

                                          // Replace the main display sprite
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

                                          endLayerRef->addChild(rewardLayer, 100);
                                          FMODAudioEngine::sharedEngine()->playEffect(
                                              // @geode-ignore(unknown-resource)
                                              "gold02.ogg");
                                    }
                              } else {
                                    log::info("Reward animation disabled");
                                    std::string reward = isPlat ? "planets" : "sparks";
                                    Notification::create("Received " +
                                                             numToString(starReward) + " " + reward + "!",
                                                         CCSprite::create(medSprite.c_str()), 2.f)
                                        ->show();
                                    FMODAudioEngine::sharedEngine()->playEffect(
                                        // @geode-ignore(unknown-resource)
                                        "gold02.ogg");
                                    // do the fake reward circle wave effect
                                    auto fakeCircleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                                    fakeCircleWave->m_color = ccWHITE;
                                    fakeCircleWave->setPosition(bigStarSprite->getPosition());
                                    endLayerRef->addChild(fakeCircleWave, 1);
                              }
                        } else if (!success && responseStars == 0) {
                              std::string rewards = isPlat ? "Planets" : "Sparks";
                              std::string medSprite = isPlat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;
                              Notification::create(rewards + " has already been claimed for this level!", CCSprite::create(medSprite.c_str()))->show();
                              log::info("already claimed for level ID: {}",
                                        levelId);
                        }
                  });
            });
      }
};
