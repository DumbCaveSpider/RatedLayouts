#include "RLShopLayer.hpp"
#include "Geode/ui/General.hpp"
#include "Geode/ui/Popup.hpp"
#include "Geode/utils/random.hpp"
#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/CCCounterLabel.hpp>
#include <Geode/binding/DialogObject.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>

using namespace geode::prelude;

RLShopLayer *RLShopLayer::create() {
  auto layer = new RLShopLayer();
  if (layer && layer->init()) {
    layer->autorelease();
    return layer;
  }
  delete layer;
  return nullptr;
}

bool RLShopLayer::init() {
  if (!CCLayer::init())
    return false;

  auto winSize = CCDirector::sharedDirector()->getWinSize();

  addBackButton(this, BackButtonStyle::Pink);
  // bg
  auto shopBGSpr = CCSprite::createWithSpriteFrameName("RL_shopBG.png"_spr);
  auto bgSize = shopBGSpr->getTextureRect().size;

  shopBGSpr->setAnchorPoint({0.0f, 0.0f});
  shopBGSpr->setScaleX((winSize.width + 10.0f) / bgSize.width);
  shopBGSpr->setScaleY((winSize.height + 10.0f) / bgSize.height);
  shopBGSpr->setPosition({-5.0f, -5.0f});
  this->addChild(shopBGSpr, -2);

  // desk
  auto deckSpr = CCSprite::createWithSpriteFrameName("RL_storeDesk.png"_spr);
  deckSpr->setPosition({winSize.width / 2, 95});
  this->addChild(deckSpr);

  // ruby counter
  auto rubySpr = CCSprite::createWithSpriteFrameName("RL_rubiesIcon.png"_spr);
  rubySpr->setPosition({winSize.width - 20, winSize.height - 20});
  rubySpr->setScale(0.7f);
  this->addChild(rubySpr);

  // lyout creator menu
  auto menu = CCMenu::create();
  menu->setPosition({0, 0});
  this->addChild(menu, -1);

  // layout creator (clickable)
  auto gm = GameManager::sharedState();
  auto layoutCreator = SimplePlayer::create(275);
  layoutCreator->updatePlayerFrame(275, IconType::Cube);
  layoutCreator->setColors(gm->colorForIdx(6), gm->colorForIdx(3));
  layoutCreator->setGlowOutline(gm->colorForIdx(1));
  layoutCreator->setScale(2.5f);

  auto layoutCreatorItem = CCMenuItemSpriteExtra::create(
      layoutCreator, this, menu_selector(RLShopLayer::onLayoutCreator));
  layoutCreatorItem->setPosition(
      {winSize.width / 2 - 120, winSize.height / 2 + 60});
  layoutCreatorItem->setContentSize({80, 80});
  layoutCreatorItem->m_scaleMultiplier = 1.02;
  layoutCreator->setPosition(layoutCreatorItem->getContentSize() / 2);
  menu->addChild(layoutCreatorItem);

  // ruby counter label
  auto rubyLabel =
      CCCounterLabel::create(Mod::get()->getSavedValue<int>("rubies"),
                             "bigFont.fnt", FormatterType::Integer);
  rubyLabel->setPosition(
      {rubySpr->getPositionX() - 15, rubySpr->getPositionY()});
  rubyLabel->setAnchorPoint({1.0f, 0.5f});
  rubyLabel->setScale(0.6f);
  m_rubyLabel = rubyLabel;

  // ruby shop sign
  auto shopSignSpr =
      CCSprite::createWithSpriteFrameName("RL_shopSign_001.png"_spr);
  shopSignSpr->setPosition({winSize.width / 2 + 80, winSize.height - 45});
  shopSignSpr->setScale(1.2f);
  this->addChild(shopSignSpr);

  // button to reset
  auto resetBtnSpr = ButtonSprite::create(
      "Reset Rubies", 80, true, "goldFont.fnt", "GJ_button_06.png", 30.f, .7f);
  auto resetBtn = CCMenuItemSpriteExtra::create(
      resetBtnSpr, this, menu_selector(RLShopLayer::onResetRubies));
  resetBtn->setPosition(
      {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 30});
  menu->addChild(resetBtn);

  // no shop yet.
  auto noShopLabel =
      CCLabelBMFont::create("Shop is under construction!", "goldFont.fnt");
  noShopLabel->setPosition({winSize.width / 2, winSize.height / 2 - 60});
  noShopLabel->setScale(0.8f);
  this->addChild(noShopLabel);
  this->addChild(rubyLabel);
  this->setKeypadEnabled(true);
  return true;
}

void RLShopLayer::onLayoutCreator(CCObject *sender) {
  // gen random
  static geode::utils::random::Generator gen = [] {
    geode::utils::random::Generator g;
    g.seed(geode::utils::random::secureU64()); // seed once
    return g;
  }();

  int v = gen.generate<int>(0, 6);
  uint64_t raw = gen.next();
  DialogObject *obj1 = nullptr;
  log::debug("Random value: {}, raw: {}", v, raw);
  switch (v) {
  case 1:
    obj1 = DialogObject::create(
        "Layout Creator",
        "Go play some <cl>layouts</c>. <cr>You brokie! >:(</c>", 28, 1.f, false,
        ccWHITE);
    break;
  case 2:
    obj1 = DialogObject::create(
        "Layout Creator",
        "Come back later when you get a little more hmm <d050><cg>Richer</c>!",
        28, 1.f, false, ccWHITE);
    break;
  case 3:
    obj1 = DialogObject::create("Layout Creator",
                                "I don't have anything at the moment,<d050> my "
                                "<cr>truck broke down</c> :(",
                                28, 1.f, false, ccWHITE);
    break;
  case 4:
    obj1 = DialogObject::create(
        "Layout Creator",
        "Someone <cy>robbed all of my stuff</c>!<d050> I'm "
        "waiting for it to be delivered again.",
        28, 1.f, false, ccWHITE);
    break;
  case 5:
    obj1 = DialogObject::create(
        "Layout Creator",
        "I really need to put a better <cg>security</c> on this shop.", 28, 1.f,
        false, ccWHITE);
    break;
  default:
    obj1 = DialogObject::create("Layout Creator", "Can I help you?", 28, 1.f,
                                false, ccWHITE);
    break;
  }

  auto dialog = DialogLayer::createDialogLayer(obj1, nullptr, 2);
  dialog->addToMainScene();
  dialog->animateInRandomSide();
}

void RLShopLayer::keyBackClicked() {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}

void RLShopLayer::onResetRubies(CCObject *sender) {
  if (Mod::get()->getSavedValue<int>("rubies") <= 0) {
    Notification::create("You don't have any rubies to reset!",
                         NotificationIcon::Warning)
        ->show();
    return;
  }
  createQuickPopup(
      "Reset Rubies",
      "Are you sure you want to <cr>reset your "
      "rubies</c> and <co>all your brought cosmetics</c>?\n"
      "<cy>This will clear all your rubies but you can reclaim rubies back "
      "from any completed rated layouts.</c>",
      "No", "Yes", [this](auto, bool yes) {
        if (!yes)
          return;
        // clear the data from rubies
        auto rubyPath = dirs::getModsSaveDir() / Mod::get()->getID() /
                        "rubies_collected.json";

        if (utils::file::readString(rubyPath)) {
          auto writeRes = utils::file::writeString(rubyPath, "{}");
          if (!writeRes) {
            log::warn("Failed to clear ruby cache file: {}", rubyPath);
          }
        }

        if (Mod::get()->getSavedValue<int>("rubies") > 0) {
          Mod::get()->setSavedValue<int>("rubies", 0);
          Notification::create("Rubies have been reset!",
                               NotificationIcon::Info)
              ->show();
          FMODAudioEngine::sharedEngine()->playEffect(
              "geode.loader/newNotif02.ogg");
        }
        m_rubyLabel->setTargetCount(0);
        m_rubyLabel->updateCounter(0.5f);
      });
}