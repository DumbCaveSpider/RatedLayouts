#include <Geode/Geode.hpp>
#include <Geode/modify/GauntletSelectLayer.hpp>

#include "../custom/RLGauntletSelectLayer.hpp"

using namespace geode::prelude;

class $modify(RLHookGauntletSelectLayer, GauntletSelectLayer) {
      bool init(int unused) {
            if (!GauntletSelectLayer::init(unused)) return false;

            // layout gauntlet in vanilla gd
            if (auto topRight = static_cast<CCMenu*>(this->getChildByID("top-right-menu"))) {
                  auto gauntletSpr = CCSprite::create("RL_gauntlet-2.png"_spr);
                  auto gauntletBtnSpr = AccountButtonSprite::create(gauntletSpr, AccountBaseColor::Blue, AccountBaseSize::Normal);
                  auto gauntletBtn = CCMenuItemSpriteExtra::create(
                      gauntletBtnSpr, this, menu_selector(RLHookGauntletSelectLayer::onGauntletButtonClick));
                  gauntletBtn->setID("rated-layouts-gauntlets-button"_spr);
                  topRight->addChild(gauntletBtn);
                  topRight->updateLayout();
            }
            return true;
      }

      void onGauntletButtonClick(CCObject* sender) {
            if (GJAccountManager::sharedState()->m_accountID == 0) {
                  FLAlertLayer::create(
                      "Rated Layouts",
                      "You must be <cg>logged in</c> to access this feature in <cl>Rated Layouts.</c>",
                      "OK")
                      ->show();
                  return;
            }
            auto layer = RLGauntletSelectLayer::create();
            auto scene = CCScene::create();
            scene->addChild(layer);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
      }
};