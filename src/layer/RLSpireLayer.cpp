#include <Geode/Geode.hpp>
#include "../include/RLDialogIcons.hpp"
#include "RLSpireLayer.hpp"
#include "RLSpireSelectLevelLayer.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/DialogLayer.hpp>
#include <cue/RepeatingBackground.hpp>
#include "Geode/cocos/cocoa/CCObject.h"

using namespace geode::prelude;

RLSpireLayer* RLSpireLayer::create() {
    auto layer = new RLSpireLayer();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool RLSpireLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto bg = cue::RepeatingBackground::create("game_bg_19_001.png", 1.f, cue::RepeatMode::X);
    bg->setSpeed(0.0f);
    bg->setColor({0, 70, 150});
    addChild(bg, -5);

    addBackButton(this, BackButtonStyle::Pink);

    //floating spire
    m_spireSpr = CCSprite::createWithSpriteFrameName("theSpire_01.png"_spr);
    m_spireSpr->setPosition({winSize.width / 2, winSize.height / 2 - 10});
    m_spireSpr->setScale(0.95f);
    this->addChild(m_spireSpr);

    //float up and down forever with easing
    auto moveUp = CCMoveBy::create(2.0f, {0, 5});
    auto moveDown = CCMoveBy::create(2.0f, {0, -5});
    auto easeUp = CCEaseSineInOut::create(moveUp);
    auto easeDown = CCEaseSineInOut::create(moveDown);
    auto floatSeq = CCSequence::create(easeUp, easeDown, nullptr);
    auto floatForever = CCRepeatForever::create(floatSeq);
    m_spireSpr->runAction(floatForever);

    //menu thing to enter the spire lol
    auto entryMenu = CCMenu::create();
    entryMenu->setPosition({0, 0});
    m_spireSpr->addChild(entryMenu, -1);

    auto whiteSquareSpr = CCSprite::create("geode.loader/white-square.png");
    whiteSquareSpr->setOpacity(0);
    whiteSquareSpr->setScale(2.f);
    m_enterBtn = CCMenuItemSpriteExtra::create(whiteSquareSpr, this, menu_selector(RLSpireLayer::onSpireClick));
    m_enterBtn->setPosition({50, 100});
    entryMenu->addChild(m_enterBtn);

    this->setKeypadEnabled(true);

    return true;
}

void RLSpireLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}

void RLSpireLayer::onSpireEnterComplete() {
    auto nextScene = CCScene::create();
    auto selectLayer = RLSpireSelectLevelLayer::create();
    nextScene->addChild(selectLayer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, nextScene));
    m_readyReset = true;
}

void RLSpireLayer::onEnter() {
    CCLayer::onEnter();

    // reset spire state for when return
    if (m_spireSpr && m_readyReset) {
        m_spireSpr->setScale(m_spireOriginalScale);
        m_spireSpr->setPosition(m_spireOriginalPos);
        m_enterBtn->setEnabled(true);
        m_blackout->removeFromParent();
        m_readyReset = false;
    }
}

void RLSpireLayer::onSpireClick(CCObject* sender) {
    if (!Mod::get()->getSavedValue<bool>("hasCode")) {
        DialogObject* dialogObj = nullptr;
        std::string response = "";
        switch (m_indexDia) {
            case 0:
                response = "<cf>The Spire</c> isn't opening.";
                m_indexDia++;
                break;
            case 1:
                response = "Yep... still <co>closed</c>.";
                m_indexDia++;
                break;
            case 2:
                response = "Do you think speaking to me will open it <cg>magically</c>?";
                m_indexDia++;
                break;
            case 3:
                response = "I heard <cp>The Oracle</c> is pretty <cl>knowledgeable</c>. Maybe you should ask it about this <cf>Spire</c>.";
                break;
            default:
                m_indexDia = 0;
                break;
        }

        dialogObj = DialogObject::create("ArcticWoof", response.c_str(), 1, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectCustomIcon(dialog, "RL_dialogIconAW.png"_spr);
        return;
    }

    // @geode-ignore(unknown-resource)
    FMODAudioEngine::sharedEngine()->playEffect("door01.ogg");

    // entering the spire
    if (m_spireSpr) {
        m_spireOriginalPos = m_spireSpr->getPosition();
        m_spireOriginalScale = m_spireSpr->getScale();
        m_enterBtn->setEnabled(false);

        auto scaleAction = CCScaleTo::create(1, 6.0f);
        auto moveAction = CCMoveBy::create(1, {0, 300.0f});
        auto spawnAction = CCSpawn::create(scaleAction, moveAction, nullptr);
        auto easeAction = CCEaseSineInOut::create(spawnAction);
        auto completeAction = CCCallFunc::create(this, callfunc_selector(RLSpireLayer::onSpireEnterComplete));
        m_spireSpr->runAction(CCSequence::create(easeAction, completeAction, nullptr));

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        m_blackout = CCLayerColor::create({0, 0, 0, 0}, winSize.width, winSize.height);
        m_blackout->setOpacity(0);
        this->addChild(m_blackout, 1000);
        m_blackout->runAction(CCFadeTo::create(.85, 255));
    }
}
