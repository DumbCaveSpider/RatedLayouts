#include <Geode/Geode.hpp>
#include <Geode/modify/InfoLayer.hpp>
#include <Geode/utils/async.hpp>

#include "../level/RLReportPopup.hpp"

using namespace geode::prelude;

class $modify(RLLInfoLayer, InfoLayer) {
      struct Fields { };
      bool init(GJGameLevel* level, GJUserScore* score, GJLevelList* list) {
            if (!InfoLayer::init(level, score, list))
                  return false;

            // Only create a report button if this level exists on the rated layout server
            if (level && level->m_levelID != 0) {
                  int levelId = level->m_levelID;

                  Ref<RLLInfoLayer> layerRef = this;
                  auto url = fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId);
                  auto req = web::WebRequest();
                  async::spawn(
                        req.get(url),
                        [layerRef](web::WebResponse response) {
                              log::info("Received /fetch response for level ID: {}",
                                        layerRef && layerRef->m_level ? layerRef->m_level->m_levelID : 0);

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

                                    bool suggested = json["isSuggested"].asBool().unwrapOrDefault();
                                    if (suggested) {
                                          log::info("Level is suggested");
                                          return;
                                    }

                                    auto reportButtonSpr = CircleButtonSprite::create(
                                          // @geode-ignore(unknown-resource)
                                          CCSprite::createWithSpriteFrameName("geode.loader/exclamation-red.png"),
                                          CircleBaseColor::Blue,
                                          CircleBaseSize::Medium);

                                    auto reportButton = CCMenuItemSpriteExtra::create(
                                        reportButtonSpr, layerRef, menu_selector(RLLInfoLayer::onReportButton));

                                    reportButton->setID("rated-layouts-report-button"_spr);
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
            if (GJAccountManager::sharedState()->m_accountID == 0) {
                  FLAlertLayer::create(
                      "Rated Layouts",
                      "You must be <cg>logged in</c> to access this feature in <cl>Rated Layouts.</c>",
                      "OK")
                      ->show();
                  return;
            }
            auto reportPopup = RLReportPopup::create(m_level->m_levelID);
            if (reportPopup) reportPopup->show();
      }
};