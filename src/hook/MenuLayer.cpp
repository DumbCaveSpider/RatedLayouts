#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "../layer/RLMenuLayer.hpp"

using namespace geode::prelude;

class $modify(RLHookMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        auto mainMenu = this->getChildByID("main-menu");
        bool isDisabled = Mod::get()->getSettingValue<bool>("disableMenuButton");

        if (mainMenu && !isDisabled) {
            // sprite
            auto sparkTop = CCSprite::createWithSpriteFrameName("RL_sparkButton.png"_spr);
            auto rlButtonSpr = CrossButtonSprite::create(sparkTop, CrossBaseColor::Green, CrossBaseSize::Small);
            auto rlButton = CCMenuItemSpriteExtra::create(
                rlButtonSpr, this, menu_selector(RLHookMenuLayer::onRatedLayoutLayer));
            rlButton->setID("rated-layouts-button"_spr);
            mainMenu->addChild(rlButton);
            mainMenu->updateLayout();
        }

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
        auto layer = RLMenuLayer::create();
        auto scene = CCScene::create();
        scene->addChild(layer);
        auto transitionFade = CCTransitionFade::create(0.5f, scene);
        CCDirector::sharedDirector()->pushScene(transitionFade);
    }
};