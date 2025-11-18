#include <alphalaneous.pages_api/include/PageMenu.h>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>

using namespace geode::prelude;

class $modify(RLLevelSearchLayer, LevelSearchLayer) {
    struct Fields {
        web::WebTask m_webTask;
    };

    bool init(int type) {
        if (!LevelSearchLayer::init(type)) {
            return false;
        }

        auto quickSearchMenu = this->getChildByID("quick-search-menu");
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (quickSearchMenu) {
            auto ratedButtonSprite =
                SearchButton::create("GJ_longBtn04_001.png", "Feat. LOs", 0.5f,
                                     "GJ_sFollowedIcon_001.png");
            auto ratedTabButton = CCMenuItemSpriteExtra::create(
                ratedButtonSprite, this,
                menu_selector(RLLevelSearchLayer::onRatedLevelsButton));
            quickSearchMenu->addChild(ratedTabButton);

            auto allRatedButtonSprite =
                SearchButton::create("GJ_longBtn04_001.png", "New LOs", 0.5f,
                                     "GJ_sFollowedIcon_001.png");
            auto allRatedTabButton = CCMenuItemSpriteExtra::create(
                allRatedButtonSprite, this,
                menu_selector(RLLevelSearchLayer::onAllRatedLevelsButton));
            quickSearchMenu->addChild(allRatedTabButton);

            auto buttonSprite =
                SearchButton::create("GJ_longBtn04_001.png", "Sent LOs", 0.5f,
                                     "GJ_sFollowedIcon_001.png");
            auto tabButton = CCMenuItemSpriteExtra::create(
                buttonSprite, this,
                menu_selector(RLLevelSearchLayer::onSuggestedLevelsButton));
            quickSearchMenu->addChild(tabButton);

            // check if layouts from other mods exists
            RowLayout* layout = nullptr;
            if (!layout) {
                layout = RowLayout::create();
                layout->setGrowCrossAxis(true);
                layout->setCrossAxisOverflow(false);
                layout->setAxisAlignment(AxisAlignment::Center);
                layout->setCrossAxisAlignment(AxisAlignment::Center);
                layout->ignoreInvisibleChildren(true);

                quickSearchMenu->setContentSize({365, 116});
                quickSearchMenu->ignoreAnchorPointForPosition(false);
                quickSearchMenu->setPosition({quickSearchMenu->getPosition().x,
                                              winSize.height / 2 + 28});
                quickSearchMenu->setLayout(layout);

                static_cast<PageMenu*>(quickSearchMenu)
                    ->setPaged(9, PageOrientation::HORIZONTAL, 422);
            }

            // hacky way to use custom icons (inspired by random tab snippet
            // lol)
            auto oldSprite = typeinfo_cast<CCSprite*>(
                buttonSprite->getChildren()->objectAtIndex(1));
            if (oldSprite) {
                auto newSprite = CCSprite::create("rlBadgeMod.png"_spr);
                oldSprite->setVisible(false);
                newSprite->setPosition(oldSprite->getPosition());
                newSprite->setScale(0.8f);
                buttonSprite->addChild(newSprite);
            }
            auto oldSprite2 = typeinfo_cast<CCSprite*>(
                ratedButtonSprite->getChildren()->objectAtIndex(1));
            if (oldSprite2) {
                auto newSprite = CCSprite::create("rlhammerIcon.png"_spr);
                oldSprite2->setVisible(false);
                newSprite->setPosition(oldSprite2->getPosition());
                newSprite->setScale(0.8f);
                ratedButtonSprite->addChild(newSprite);
            }
            auto oldSprite3 = typeinfo_cast<CCSprite*>(
                allRatedButtonSprite->getChildren()->objectAtIndex(1));
            if (oldSprite3) {
                auto newSprite = CCSprite::create("rlStarIcon.png"_spr);
                oldSprite3->setVisible(false);
                newSprite->setPosition(oldSprite3->getPosition());
                newSprite->setScale(0.8f);
                allRatedButtonSprite->addChild(newSprite);
            }

            quickSearchMenu->updateLayout();
        }
        return true;
    }

    void onEnterTransitionDidFinish() override {
        if (!Mod::get()->getSavedValue<bool>("isFirstTime")) {
            FLAlertLayer::create(
                "Where to find Rated Layouts?",
                "Rated Layouts introduced a <cy>new rating system</c> for "
                "<cb>layout levels</c>! You can find these layout levels at "
                "the <cl>Quick Menu</c> on the next page! ",
                "OK")
                ->show();
            // hello for the first time, so that way people stop complaining
            // about where the new layout levels are >:)
            Mod::get()->setSavedValue<bool>("isFirstTime", true);
        }
        LevelSearchLayer::onEnterTransitionDidFinish();
    }

    // these were absolute trash to do, legit had to go find snippets from other
    // repos just to get this working lol but i actually did it, so poggers
    void onSuggestedLevelsButton(CCObject* sender) {
        m_fields->m_webTask.cancel();

        web::WebRequest()
            .param("type", 1)
            .param("amount", 1000)
            .get("https://gdrate.arcticwoof.xyz/getLevels")
            .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                    auto jsonResult = res->json();

                    if (jsonResult) {
                        auto json = jsonResult.unwrap();
                        std::string levelIDs;
                        bool first = true;
                        if (json.contains("levelIds")) {
                            auto levelsArr = json["levelIds"];

                            // iterate
                            for (auto levelIDValue : levelsArr) {
                                auto levelID = levelIDValue.as<int>();
                                if (levelID) {
                                    if (!first) levelIDs += ",";
                                    levelIDs += numToString(levelID.unwrap());
                                    first = false;
                                }
                            }
                        }

                        if (!levelIDs.empty()) {
                            auto searchObject = GJSearchObject::create(
                                SearchType::Type19, levelIDs);
                            auto browserLayer =
                                LevelBrowserLayer::create(searchObject);
                            auto scene = CCScene::create();
                            scene->addChild(browserLayer);
                            auto transitionFade =
                                CCTransitionFade::create(0.5f, scene);
                            CCDirector::sharedDirector()->pushScene(
                                transitionFade);
                        } else {
                            log::warn("No levels found in response");
                        }
                    } else {
                        log::error("Failed to parse response JSON");
                    }
                } else {
                    log::error("Failed to fetch levels from server");
                }
            });
    }

    void onRatedLevelsButton(CCObject* sender) {
        m_fields->m_webTask.cancel();

        web::WebRequest()
            .param("type", 2)
            .param("amount", 1000)
            .get("https://gdrate.arcticwoof.xyz/getLevels")
            .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                    auto jsonResult = res->json();

                    if (jsonResult) {
                        auto json = jsonResult.unwrap();
                        std::string levelIDs;
                        bool first = true;

                        if (json.contains("levelIds")) {
                            auto levelsArr = json["levelIds"];

                            // iterate
                            for (auto levelIDValue : levelsArr) {
                                auto levelID = levelIDValue.as<int>();
                                if (levelID) {
                                    if (!first) levelIDs += ",";
                                    levelIDs += numToString(levelID.unwrap());
                                    first = false;
                                }
                            }
                        }

                        if (!levelIDs.empty()) {
                            auto searchObject = GJSearchObject::create(
                                SearchType::Type19, levelIDs);
                            auto browserLayer =
                                LevelBrowserLayer::create(searchObject);
                            auto scene = CCScene::create();
                            scene->addChild(browserLayer);
                            auto transitionFade =
                                CCTransitionFade::create(0.5f, scene);
                            CCDirector::sharedDirector()->pushScene(
                                transitionFade);
                        } else {
                            log::warn("No levels found in response");
                        }
                    } else {
                        log::error("Failed to parse response JSON");
                    }
                } else {
                    log::error("Failed to fetch levels from server");
                }
            });
    }

    void onAllRatedLevelsButton(CCObject* sender) {
        m_fields->m_webTask.cancel();

        web::WebRequest()
            .param("type", 3)
            .param("amount", 1000)
            .get("https://gdrate.arcticwoof.xyz/getLevels")
            .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                    auto jsonResult = res->json();

                    if (jsonResult) {
                        auto json = jsonResult.unwrap();
                        std::string levelIDs;
                        bool first = true;

                        if (json.contains("levelIds")) {
                            auto levelsArr = json["levelIds"];

                            // iterate
                            for (auto levelIDValue : levelsArr) {
                                auto levelID = levelIDValue.as<int>();
                                if (levelID) {
                                    if (!first) levelIDs += ",";
                                    levelIDs += numToString(levelID.unwrap());
                                    first = false;
                                }
                            }
                        }

                        if (!levelIDs.empty()) {
                            auto searchObject = GJSearchObject::create(
                                SearchType::Type19, levelIDs);
                            auto browserLayer =
                                LevelBrowserLayer::create(searchObject);
                            auto scene = CCScene::create();
                            scene->addChild(browserLayer);
                            auto transitionFade =
                                CCTransitionFade::create(0.5f, scene);
                            CCDirector::sharedDirector()->pushScene(
                                transitionFade);
                        } else {
                            log::warn("No levels found in response");
                            Notification::create("No levels found",
                                                 NotificationIcon::Warning)
                                ->show();
                        }
                    } else {
                        log::error("Failed to parse response JSON");
                    }
                } else {
                    log::error("Failed to fetch levels from server");
                    Notification::create("Failed to fetch levels from server",
                                         NotificationIcon::Error)
                        ->show();
                }
            });
    }
};