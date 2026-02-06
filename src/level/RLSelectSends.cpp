#include "RLSelectSends.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include "Geode/ui/MDTextArea.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>

using namespace geode::prelude;

RLSelectSends *RLSelectSends::create() {
  auto ret = new RLSelectSends();

  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }

  delete ret;
  return nullptr;
};

bool RLSelectSends::init() {
  if (!Popup::init(380.f, 180.f, "square01_001.png"))
    return false;
  setTitle("View Sent Layouts");
  m_title->setPositionY(m_title->getPositionY() - 10.f);
  m_title->setScale(0.9f);

  auto text = MDTextArea::create("Select a category of sent layouts to view the sent layouts.",
                                 {m_mainLayer->getContentSize().width - 30.f, 80.f});
  text->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 90.f});
  m_mainLayer->addChild(text);

  auto buttonMenu = CCMenu::create();
  buttonMenu->setContentWidth(m_mainLayer->getContentWidth() - 20.f);
  buttonMenu->setPosition({m_mainLayer->getContentSize().width / 2, 30});
  buttonMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(
      AxisAlignment::Center));

  auto showAllBtn =
      CCMenuItemSpriteExtra::create(ButtonSprite::create("All Sent"), this,
                                    menu_selector(RLSelectSends::onAllSends));
  showAllBtn->setPosition({50.f, 50.f});
  buttonMenu->addChild(showAllBtn);
  auto threePlusBtn = CCMenuItemSpriteExtra::create(
      ButtonSprite::create("3+ Sents"), this,
      menu_selector(RLSelectSends::onThreePlusSends));
  threePlusBtn->setPosition({160.f, 20.f});
  buttonMenu->addChild(threePlusBtn);

  auto legendaryBtn = CCMenuItemSpriteExtra::create(
      ButtonSprite::create("Legendary Sents"), this,
      menu_selector(RLSelectSends::onLegendarySends));
  legendaryBtn->setPosition({270.f, 50.f});
  buttonMenu->addChild(legendaryBtn);

  m_mainLayer->addChild(buttonMenu);
  buttonMenu->updateLayout();

  m_buttonMenu = nullptr; // no need xd

  return true;
}

void RLSelectSends::onAllSends(CCObject *sender) {
  RLLevelBrowserLayer::ParamList params;
  params.emplace_back("type", "1");
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::Sent, params, "Sent Layouts");
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
  this->onClose(sender);
}

void RLSelectSends::onThreePlusSends(CCObject *sender) {
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

void RLSelectSends::onLegendarySends(CCObject *sender) {
  RLLevelBrowserLayer::ParamList params;
  params.emplace_back("type", "5");
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::LegendarySends, params, "Legendary Sent Layouts");
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
  this->onClose(sender);
}