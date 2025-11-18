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

        // i couldve done a better way to do this but this works for now
        std::string completedKey = fmt::format("{}", level->m_levelID);
        if (Mod::get()->getSavedValue<bool>(completedKey, false)) {
            log::info("Level ID: {} was already completed before",
                      level->m_levelID);
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

        // level completed yipeee
        Mod::get()->setSavedValue<bool>(completedKey, true);

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

            matjson::Value jsonBody;
            jsonBody["accountId"] = accountId;
            jsonBody["argonToken"] = argonToken;
            jsonBody["levelId"] = levelId;

            auto submitReq = web::WebRequest();
            submitReq.bodyJSON(jsonBody);
            auto submitTask =
                submitReq.post("https://gdrate.arcticwoof.xyz/submitComplete");
            submitTask.listen([endLayerRef, starReward,
                               levelId](web::WebResponse* submitResponse) {
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

                log::info("submitComplete success: {}, response stars: {}",
                          success, responseStars);

                if (success) {
                    // response stars minus the difficulty reward
                    int displayStars = responseStars - starReward;
                    log::info("Display stars: {} - {} = {}", responseStars,
                              starReward, displayStars);
                    Mod::get()->setSavedValue<int>("stars", responseStars);

                    if (!endLayerRef || !endLayerRef->m_mainLayer) {
                        log::warn("m_mainLayer is invalid");
                        return;
                    }

                    // make the stars reward pop when u complete the level
                    auto bigStarSprite =
                        CCSprite::create("rlStarIconBig.png"_spr);
                    bigStarSprite->setScale(1.2f);
                    bigStarSprite->setPosition(
                        {endLayerRef->m_mainLayer->getContentSize().width / 2 +
                             135,
                         endLayerRef->m_mainLayer->getContentSize().height /
                             2});
                    bigStarSprite->setOpacity(0);
                    bigStarSprite->setScale(0.5f);
                    endLayerRef->m_mainLayer->addChild(bigStarSprite);

                    // star animation lol
                    auto scaleAction = CCScaleTo::create(1.f, .8f);
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
                    if (auto rewardLayer = CurrencyRewardLayer::create(
                            0, starReward, 0, 0, CurrencySpriteType::Star, 0,
                            CurrencySpriteType::Star, 0,
                            bigStarSprite->getPosition(),
                            CurrencyRewardType::Default, 0.0, 1.0)) {
                        // display the calculated stars
                        rewardLayer->m_starsLabel->setString(
                            numToString(displayStars).c_str());
                        rewardLayer->m_stars = displayStars;
                        rewardLayer->m_starsSprite =
                            CCSprite::create("rlStarIcon.png"_spr);

                        // Replace the main display sprite
                        if (auto node = rewardLayer->m_mainNode
                                            ->getChildByType<CCSprite*>(0)) {
                            node->setDisplayFrame(
                                CCSprite::create("rlStarIcon.png"_spr)
                                    ->displayFrame());
                            node->setScale(1.f);
                        }

                        endLayerRef->addChild(rewardLayer, 100);
                        FMODAudioEngine::sharedEngine()->playEffect(
                            // @geode-ignore(unknown-resource)
                            "gold02.ogg");
                    }
                } else {
                    Notification::create(
                        "Stars already claimed for this level!",
                        NotificationIcon::Warning)
                        ->show();
                }
            });
        });
    }
};
