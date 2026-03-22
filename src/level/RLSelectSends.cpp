#include "RLSelectSends.hpp"
#include "../layer/RLLevelBrowserLayer.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/MDTextArea.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>

using namespace geode::prelude;

const int DEV_ACCOUNTID = 7689052;

RLSelectSends* RLSelectSends::create() {
    auto ret = new RLSelectSends();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLSelectSends::init() {
    if (!Popup::init(230.f, 220.f, "square01_001.png"))
        return false;

    // kill the close button
    if (auto closeBtn = this->m_closeBtn) {
        closeBtn->removeFromParent();
    }

    m_buttonMenu->setLayout(ColumnLayout::create()
            ->setGap(10.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAxisReverse(true));

    auto showAllBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Newest Sent", 180, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f),
        this,
        menu_selector(RLSelectSends::onAllSends));
    showAllBtn->setPosition({50.f, 50.f});
    showAllBtn->m_scaleMultiplier = 1.05f;
    m_buttonMenu->addChild(showAllBtn);

    auto mostBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Most Sents", 180, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f),
        this,
        menu_selector(RLSelectSends::onMostSents));
    mostBtn->setPosition({160.f, 80.f});
    mostBtn->m_scaleMultiplier = 1.05f;
    m_buttonMenu->addChild(mostBtn);

    // least sents button
    auto leastBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Least Sents", 180, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f),
        this,
        menu_selector(RLSelectSends::onLeastSents));
    leastBtn->setPosition({270.f, 20.f});
    leastBtn->m_scaleMultiplier = 1.05f;
    m_buttonMenu->addChild(leastBtn);

    if (Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
        Mod::get()->getSavedValue<bool>("isPlatAdmin") ||
        GJAccountManager::sharedState()->m_accountID == DEV_ACCOUNTID) {
        auto threePlusBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("3+ Sents", 180, true, "goldFont.fnt", "geode.loader/GE_button_01.png", 30.f, 1.f),
            this,
            menu_selector(RLSelectSends::onThreePlusSends));
        threePlusBtn->setPosition({160.f, 20.f});
        threePlusBtn->m_scaleMultiplier = 1.05f;
        m_buttonMenu->addChild(threePlusBtn);

        auto legendaryBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Legendary Sents", 180, true, "goldFont.fnt", "geode.loader/GE_button_01.png", 30.f, 1.f),
            this,
            menu_selector(RLSelectSends::onLegendarySends));
        legendaryBtn->setPosition({270.f, 50.f});
        legendaryBtn->m_scaleMultiplier = 1.05f;
        m_buttonMenu->addChild(legendaryBtn);
    }

    m_buttonMenu->updateLayout();

    return true;
}

void RLSelectSends::onAllSends(CCObject* sender) {
    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("type", "1");
    auto browserLayer = RLLevelBrowserLayer::create(
        RLLevelBrowserLayer::Mode::Sent, params, "Newest Sent Layouts");
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
    this->onClose(sender);
}

void RLSelectSends::onThreePlusSends(CCObject* sender) {
    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("type", "4");
    auto browserLayer = RLLevelBrowserLayer::create(
        RLLevelBrowserLayer::Mode::Sent, params, "3+ Sent Layouts");
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
    this->onClose(sender);
}

void RLSelectSends::onLegendarySends(CCObject* sender) {
    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("type", "5");
    auto browserLayer =
        RLLevelBrowserLayer::create(RLLevelBrowserLayer::Mode::LegendarySends,
            params,
            "Legendary Sent Layouts");
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
    this->onClose(sender);
}

void RLSelectSends::onMostSents(CCObject* sender) {
    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("type", "6");
    auto browserLayer = RLLevelBrowserLayer::create(
        RLLevelBrowserLayer::Mode::Sent, params, "Most Sent Layouts");
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
    this->onClose(sender);
}

void RLSelectSends::onLeastSents(CCObject* sender) {
    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("type", "7");
    auto browserLayer = RLLevelBrowserLayer::create(
        RLLevelBrowserLayer::Mode::Sent, params, "Least Sent Layouts");
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
    this->onClose(sender);
}


