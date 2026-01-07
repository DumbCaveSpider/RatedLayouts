#include <Geode/Geode.hpp>
#include <Geode/modify/InfoLayer.hpp>

#include "../level/RLReportPopup.hpp"

using namespace geode::prelude;

class $modify(RLLInfoLayer, InfoLayer) {
      bool init(GJGameLevel* level, GJUserScore* score, GJLevelList* list) {
            if (!InfoLayer::init(level, score, list))
                  return false;

            // Only create a report button if this level exists on the rated layout server
            if (level && level->m_levelID != 0) {
                  int levelId = level->m_levelID;

                  auto getReq = web::WebRequest();
                  auto getTask = getReq.get(fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

                  Ref<RLLInfoLayer> layerRef = this;
                  getTask.listen([layerRef](web::WebResponse* response) {
                        log::info("Received /fetch response for level ID: {}",
                                  layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);

                        if (!layerRef) {
                              log::warn("InfoLayer destroyed before /fetch completed");
                              return;
                        }

                        if (response->ok()) {
                              if (!response->json()) {
                                    log::warn("Failed to parse /fetch JSON response");
                                    return;
                              }

                              auto json = response->json().unwrap();

                              bool suggested = json["isSuggested"].asBool().unwrapOrDefault();
                              if (suggested) {
                                    log::info("Level is suggested");
                                    return;
                              }

                              auto reportButtonSpr = CircleButtonSprite::create(
                                  CCSprite::createWithSpriteFrameName("exMark_001.png"),
                                  CircleBaseColor::Blue,
                                  CircleBaseSize::Medium);

                              auto reportButton = CCMenuItemSpriteExtra::create(
                                  reportButtonSpr, layerRef, menu_selector(RLLInfoLayer::onReportButton));

                              auto vanillaReportButton = layerRef->getChildByIDRecursive("report-button");
                              auto mainMenu = layerRef->getChildByIDRecursive("main-menu");
                              if (vanillaReportButton) {
                                    mainMenu->addChild(reportButton);
                                    reportButton->setPosition(vanillaReportButton->getPosition());
                                    vanillaReportButton->removeFromParentAndCleanup(true);
                              }
                        } else {
                              log::warn("failed to fetch level");
                        }
                  });
            }

            return true;
      };

      void onReportButton(CCObject* sender) {
            auto reportPopup = RLReportPopup::create(m_level->m_levelID);
            if (reportPopup) reportPopup->show();
      }
};