#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>

using namespace geode::prelude;

class $modify(LevelCell) {
    void loadCustomLevelCell(int levelId) {
        // Fetch rating data from server and update difficulty sprite
        log::debug("Fetching rating data for level cell ID: {}", levelId);

        auto getReq = web::WebRequest();
        auto getTask = getReq.get(fmt::format(
            "https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

        // Capture as Ref to maintain object lifetime
        Ref<LevelCell> cellRef = this;

        getTask.listen([cellRef, levelId](web::WebResponse* response) {
            log::debug(
                "Received rating response from server for level cell ID: {}",
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
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();

            log::debug("difficulty: {}, featured: {}", difficulty, featured);

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
                cellRef->m_mainLayer->getChildByID("difficulty-container");
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
            sprite->updateDifficultyFrame(difficultyLevel,
                                          GJDifficultyName::Short);

            // star icon
            auto newStarIcon = CCSprite::create("rlStarIcon.png"_spr);
            if (newStarIcon) {
                newStarIcon->setPosition(
                    {difficultySprite->getContentSize().width / 2 + 8, -8});
                newStarIcon->setScale(0.53f);
                newStarIcon->setID("rl-star-icon");
                difficultySprite->addChild(newStarIcon);

                // star value label
                auto starLabelValue = CCLabelBMFont::create(
                    numToString(difficulty).c_str(), "bigFont.fnt");
                if (starLabelValue) {
                    starLabelValue->setPosition(
                        {newStarIcon->getPositionX() - 7,
                         newStarIcon->getPositionY()});
                    starLabelValue->setScale(0.4f);
                    starLabelValue->setAnchorPoint({1.0f, 0.5f});
                    starLabelValue->setAlignment(kCCTextAlignmentRight);
                    starLabelValue->setID("rl-star-label");

                    if (GameStatsManager::sharedState()
                            ->hasCompletedOnlineLevel(
                                cellRef->m_level->m_levelID)) {
                        starLabelValue->setColor({0, 150, 255});  // cyan
                    }
                    difficultySprite->addChild(starLabelValue);
                }
            }

            // Update featured coin visibility
            if (featured == 1) {
                auto featuredCoin =
                    difficultySprite->getChildByID("featured-coin");
                if (!featuredCoin) {
                    auto newFeaturedCoin =
                        CCSprite::create("rlfeaturedCoin.png"_spr);
                    if (newFeaturedCoin) {
                        newFeaturedCoin->setPosition(
                            {difficultySprite->getContentSize().width / 2,
                             difficultySprite->getContentSize().height / 2});
                        newFeaturedCoin->setID("featured-coin");
                        difficultySprite->addChild(newFeaturedCoin, -1);
                    }
                }
            }

            // if not compacted and has at least a coin
            auto coinContainer =
                cellRef->m_mainLayer->getChildByID("difficulty-container");
            if (coinContainer && !cellRef->m_compactView) {
                auto coinIcon1 = coinContainer->getChildByID("coin-icon-1");
                auto coinIcon2 = coinContainer->getChildByID("coin-icon-2");
                auto coinIcon3 = coinContainer->getChildByID("coin-icon-3");
                if (coinIcon1 || coinIcon2 || coinIcon3) {
                    difficultySprite->setPositionY(
                        difficultySprite->getPositionY() + 10);
                }
                // doing the dumb coin move
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
        });
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        // no local levels
        if (!level || level->m_levelID == 0) {
            return;
        }

        loadCustomLevelCell(level->m_levelID);
    }
};