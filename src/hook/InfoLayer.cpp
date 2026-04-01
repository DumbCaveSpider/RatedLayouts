#include <Geode/Geode.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/modify/InfoLayer.hpp>
#include <Geode/utils/async.hpp>

#include "../include/RLConstants.hpp"
#include "../level/RLModsNotesPopup.hpp"
#include "../level/RLReportPopup.hpp"

using namespace geode::prelude;

class $modify(RLLInfoLayer, InfoLayer) {
    struct Fields {};
    bool init(GJGameLevel* level, GJUserScore* score, GJLevelList* list) {
        if (!InfoLayer::init(level, score, list))
            return false;

        // Only create a report button if this level exists on the rated layout
        // server
        if (level && level->m_levelID != 0) {
            int levelId = level->m_levelID;

            Ref<RLLInfoLayer> layerRef = this;
            auto url = fmt::format("{}/fetch?levelId={}", std::string(rl::BASE_API_URL), levelId);
            auto req = web::WebRequest();
            async::spawn(req.get(url), [layerRef](web::WebResponse response) {
                log::info("Received /fetch response for level ID: {}",
                    layerRef && layerRef->m_level ? layerRef->m_level->m_levelID
                                                  : 0);

                if (!layerRef) {
                    log::warn("InfoLayer destroyed before /fetch completed");
                    return;
                }

                if (response.ok()) {
                    if (!response.json()) {
                        log::warn("Failed to parse /fetch JSON response");
                        return;
                    }

                    auto json = response.json().unwrap();
                    bool isRated = json["rated"].asBool().unwrapOrDefault();

                    // report button create yes
                    auto reportButtonSpr = CircleButtonSprite::create(
                        // @geode-ignore(unknown-resource)
                        CCSprite::createWithSpriteFrameName(
                            "geode.loader/exclamation-red.png"),
                        CircleBaseColor::Blue,
                        CircleBaseSize::Medium);

                    auto reportButton = CCMenuItemSpriteExtra::create(
                        reportButtonSpr, layerRef, menu_selector(RLLInfoLayer::onReportButton));

                    reportButton->setID("rated-layouts-report-button"_spr);
                    auto vanillaReportButton =
                        layerRef->getChildByIDRecursive("report-button");
                    CCPoint reportPos = vanillaReportButton->getPosition();
                    auto mainMenu = layerRef->getChildByIDRecursive("main-menu");

                    // show mod notes on level
                    auto notesButtonSpr = CircleButtonSprite::create(
                        // @geode-ignore(unknown-resource)
                        CCSprite::createWithSpriteFrameName("geode.loader/message.png"),
                        CircleBaseColor::Blue,
                        CircleBaseSize::Medium);

                    auto notesButton = CCMenuItemSpriteExtra::create(
                        notesButtonSpr, layerRef, menu_selector(RLLInfoLayer::onNotesButton));

                    notesButton->setID("rated-layouts-notes-button"_spr);
                    mainMenu->addChild(notesButton);
                    notesButton->setPosition({reportPos.x,
                        reportPos.y + 50});

                    if (vanillaReportButton && isRated) {
                        mainMenu->addChild(reportButton);
                        reportButton->setPosition(reportPos);
                        vanillaReportButton->removeFromParentAndCleanup(true);
                    }

                    if (!isRated) {
                        reportButton->removeFromParent();
                    }

                } else {
                    log::warn("failed to fetch level");
                }
            });
        }

        return true;
    };

    void onReportButton(CCObject* sender) {
        if (GJAccountManager::sharedState()->m_accountID == 0) {
            FLAlertLayer::create("Rated Layouts",
                "You must be <cg>logged in</c> to access this "
                "feature in <cl>Rated Layouts.</c>",
                "OK")
                ->show();
            return;
        }
        auto reportPopup = RLReportPopup::create(m_level->m_levelID);
        if (reportPopup)
            reportPopup->show();
    }

    void onNotesButton(CCObject* sender) {
        if (GJAccountManager::sharedState()->m_accountID == 0) {
            FLAlertLayer::create("Rated Layouts",
                "You must be <cg>logged in</c> to access this "
                "feature in <cl>Rated Layouts.</c>",
                "OK")
                ->show();
            return;
        }

        auto notesPopup = RLModsNotesPopup::create(m_level);
        if (notesPopup)
            notesPopup->show();
    }
};
