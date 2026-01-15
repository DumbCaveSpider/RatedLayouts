#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>

#include "../custom/RLCreatorLayer.hpp"

using namespace geode::prelude;

class $modify(RLLevelSearchLayer, LevelSearchLayer) {
      bool init(int type) {
            if (!LevelSearchLayer::init(type))
                  return false;

            // add a menu on the left side of the layer
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            auto bottomLeftMenu =
                static_cast<CCMenu*>(this->getChildByID("bottom-left-menu"));
            auto lbButtonSpr = CCSprite::create("RL_btn01.png"_spr);
            lbButtonSpr->setScale(1.2f);
            auto lbButton = CCMenuItemSpriteExtra::create(
                lbButtonSpr, this, menu_selector(RLLevelSearchLayer::onRatedLayoutLayer));
            bottomLeftMenu->addChild(lbButton);
            bottomLeftMenu->updateLayout();
            return true;
      }
      void onRatedLayoutLayer(CCObject* sender) {
            if (GJAccountManager::sharedState()->m_accountID == 0) {
                  FLAlertLayer::create(
                      "Rated Layouts",
                      "You must be <cg>logged in</c> to access this feature in <cl>Rated Layouts.</c>",
                      "OK")
                      ->show();
                  return;
            }
            auto layer = RLCreatorLayer::create();
            auto scene = CCScene::create();
            scene->addChild(layer);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
      }
};