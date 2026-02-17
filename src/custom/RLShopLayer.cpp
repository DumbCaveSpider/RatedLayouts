#include "RLShopLayer.hpp"
#include "../utils/RLNameplateItem.hpp"
#include "Geode/ui/General.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/Popup.hpp"
#include "Geode/utils/async.hpp"
#include "Geode/utils/random.hpp"
#include "RLBuyItemPopup.hpp"
#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <fmt/format.h>

using namespace geode::prelude;
using namespace ratedlayouts;

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
  this->addChild(shopBGSpr, -3);

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
  this->addChild(rubyLabel);

  // ruby shop sign
  auto shopSignSpr =
      CCSprite::createWithSpriteFrameName("RL_shopSign_001.png"_spr);
  shopSignSpr->setPosition({winSize.width / 2 + 60, winSize.height - 45});
  shopSignSpr->setScale(1.2f);
  this->addChild(shopSignSpr, -2);

  // button to reset
  auto resetBtnSpr = ButtonSprite::create(
      "Reset Rubies", 80, true, "goldFont.fnt", "GJ_button_06.png", 25.f, .7f);
  auto resetBtn = CCMenuItemSpriteExtra::create(
      resetBtnSpr, this, menu_selector(RLShopLayer::onResetRubies));
  resetBtn->setPosition(
      {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 30});
  menu->addChild(resetBtn);

  // button to unequip nameplate
  auto unequipSpr = ButtonSprite::create("Unequip", 80, true, "goldFont.fnt",
                                         "GJ_button_06.png", 25.f, .7f);
  auto unequipBtn = CCMenuItemSpriteExtra::create(
      unequipSpr, this, menu_selector(RLShopLayer::onUnequipNameplate));
  unequipBtn->setPosition(
      {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 60});
  menu->addChild(unequipBtn);

  // open submission form
  auto submitSpr = ButtonSprite::create("Submission", 80, true, "goldFont.fnt",
                                        "GJ_button_01.png", 25.f, .7f);
  auto submitBtn = CCMenuItemSpriteExtra::create(
      submitSpr, this, menu_selector(RLShopLayer::onForm));
  submitBtn->setPosition(
      {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 90});
  menu->addChild(submitBtn);

  // shop item menu
  auto shopMenu = CCMenu::create();
  shopMenu->setPosition({deckSpr->getContentSize().width / 2,
                         deckSpr->getContentSize().height / 2});
  shopMenu->setContentSize({deckSpr->getContentSize().width - 40,
                            deckSpr->getContentSize().height - 37});
  // arrange rows vertically
  shopMenu->setLayout(ColumnLayout::create()
                          ->setGap(-15.f)
                          ->setAxisAlignment(AxisAlignment::Center)
                          ->setAxisReverse(true));

  // two horizontal row menus
  auto rowH = [&](void) {
    auto m = CCMenu::create();
    m->setPosition(
        {m->getContentSize().width / 2.f, m->getContentSize().height / 2.f});
    m->setContentSize({shopMenu->getContentSize().width,
                       (shopMenu->getContentSize().height - 8.f) / 2.f});
    m->setLayout(RowLayout::create()
                     ->setGap(40.f)
                     ->setAxisAlignment(AxisAlignment::Center)
                     ->setAxisReverse(false));
    m->updateLayout();
    return m;
  };

  m_shopRow1 = rowH();
  m_shopRow2 = rowH();

  // remind myself to add these things as it is very important
  m_shopItems = {{1, 15000, 29153807, "Artyxlr"},
                 {2, 11300, 20493315, "MajStr113gd"},
                 {3, 10000, 29960249, "DaRealSkellyGuy"},
                 {4, 10000, 14922106, "Ayeah755"},
                 {5, 15000, 4882817, "bonneville1"},
                 {6, 50000, 29153807, "Artyxlr"},
                 {7, 10000, 29960249, "DaRealSkellyGuy"},
                 {8, 12500, 11827369, "Froose"},
                 {9, 13125, 19304186, "DarkFeind"},
                 {10, 14700, 26139147, "NitzFoxcrak"},
                 {11, 12000, 13733061, "F1regek"},
                 {12, 12000, 15289357, "Landon72"},
                 {13, 20000, 3595559, "Darkore"},
                 {14, 10000, 24877069, "abbaba"},
                 {15, 10000, 24877069, "abbaba"},
                 {16, 10000, 24877069, "abbaba"},
                 {17, 10000, 24877069, "abbaba"},
                 {18, 12500, 26209086, "Niki2025"},
                 {19, 12500, 26209086, "Niki2025"},
                 {20, 27500, 36519986, "NullJuicecd"},
                 {21, 27500, 36519986, "NullJuicecd"},
                 {22, 20000, 24448008, "hexz347"},
                 {23, 57500, 7689052, "ArcticWoof"},
                 {24, 10000, 15289357, "Landon72"},
                 {25, 12340, 15289357, "Landon72"},
                 {26, 30000, 5354634, "stkyc"},
                 {27, 15000, 5354634, "stkyc"},
                 {28, 10000, 4882817, "bonneville1"},
                 {29, 15000, 16737398, "Hydraniac"},
                 {30, 29032, 21389, "Enlightenment"},
                 {31, 10000, 15289357, "Landon72"},
                 {32, 10000, 15289357, "Landon72"},
                 {33, 30000, 21389, "Enlightenment"},
                 {34, 12500, 14881095, "sebtheboi"},
                 {35, 10000, 21213401, "Flyingfish9"},
                 {36, 11000, 21213401, "Flyingfish9"},
                 {37, 12500, 21213401, "Flyingfish9"},
                 {38, 10500, 25552964, "DestructionEToH"},
                 {39, 11240, 37401841, "MunchyBob"},
                 {40, 65000, 7689052, "ArcticWoof"}};
  m_shopRow1->setAnchorPoint({0.5f, 0.5f});
  m_shopRow2->setAnchorPoint({0.5f, 0.5f});

  // compute vertical positions to match the ColumnLayout that used to be
  auto shopMenuCS = shopMenu->getContentSize();
  float rowHeight = m_shopRow1->getContentSize().height;
  const float gap = 20.f;
  const float centerY = shopMenu->getPositionY();
  const float offset = (rowHeight / 2.f) + (gap / 2.f);

  m_shopRow1->setPosition({shopMenu->getPositionX(), centerY + offset - 18});
  m_shopRow2->setPosition({shopMenu->getPositionX(), centerY - offset});
  deckSpr->addChild(m_shopRow1, 1);
  deckSpr->addChild(m_shopRow2, 1);

  // pagination controls
  const int perPage = 8;
  int totalPages = std::max(
      1, static_cast<int>((m_shopItems.size() + perPage - 1) / perPage));

  if (totalPages > 1) {
    // page indicator label (placed inside deckSpr so coords are local)
    m_pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
    if (m_pageLabel) {
      m_pageLabel->setScale(0.5f);
      m_pageLabel->setPosition({shopMenu->getPositionX(), 3.f});
      m_pageLabel->setAnchorPoint({0.5f, 0.f});
      deckSpr->addChild(m_pageLabel, 2);
    }

    // pagenation menu thingy
    auto pageMenu = CCMenu::create();
    pageMenu->setPosition({0, 0});
    pageMenu->setContentSize(deckSpr->getContentSize());
    deckSpr->addChild(pageMenu, 2);

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    if (prevSpr) {
      m_prevPageBtn = CCMenuItemSpriteExtra::create(
          prevSpr, this, menu_selector(RLShopLayer::onPrevPage));
      if (m_prevPageBtn) {
        m_prevPageBtn->setPosition(
            {-10, pageMenu->getContentSize().height / 2});
        pageMenu->addChild(m_prevPageBtn);
      }
    }

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setFlipX(true);
    if (nextSpr) {
      m_nextPageBtn = CCMenuItemSpriteExtra::create(
          nextSpr, this, menu_selector(RLShopLayer::onNextPage));
      if (m_nextPageBtn) {
        m_nextPageBtn->setPosition({pageMenu->getContentSize().width + 10,
                                    pageMenu->getContentSize().height / 2});
        pageMenu->addChild(m_nextPageBtn);
      }
    }
  }

  updateShopPage();

  shopMenu->updateLayout();
  deckSpr->addChild(shopMenu);
  this->setKeypadEnabled(true);
  return true;
}

void RLShopLayer::onForm(CCObject *sender) {
  Notification::create("Opening a new link to the browser",
                       NotificationIcon::Info)
      ->show();
  utils::web::openLinkInBrowser("https://forms.gle/3UU5JJE1XrfwPK5u7");
}

// play the dum audio lol
void RLShopLayer::onEnter() {
  CCLayer::onEnter();
  FMODAudioEngine::sharedEngine()->playMusic("rubyShop.mp3"_spr, true, 0.f, 0);
}

void RLShopLayer::onExit() {
  CCLayer::onExit();
  GameManager::sharedState()->playMenuMusic();
  GameManager::sharedState()->fadeInMenuMusic();
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
    obj1 = DialogObject::create("Layout Creator",
                                "Come back later when you get a little more "
                                "hmm... <d050><cg>Richer</c>!",
                                28, 1.f, false, ccWHITE);
    break;
  case 3:
    obj1 = DialogObject::create(
        "Layout Creator",
        "These are very <cg>high quality nameplates</c>, you know! "
        "<d050><cy>Not like those </c><cr>cheap ones</c>!",
        28, 1.f, false, ccWHITE);
    break;
  case 4:
    obj1 = DialogObject::create(
        "Layout Creator",
        "So this weird kid <cl>Darkore</c> hook up this sweet tunes on my "
        "<cr>shop</c>. <cp>I love it</c>!",
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

void RLShopLayer::onBuyItem(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  int idx = item->getTag();
  RLNameplateInfo info;
  if (!RLNameplateItem::getInfo(idx, info)) {
    log::warn("RLShopLayer: no nameplate info for index {}", idx);
    return;
  }

  // open buy popup with creator/price information
  RLBuyItemPopup::create(info.index, info.creatorId, info.creatorUsername,
                         info.value, this)
      ->show();
}

void RLShopLayer::onUnequipNameplate(CCObject *sender) {
  createQuickPopup(
      "Unequip Nameplate",
      "Are you sure you want to <cr>unequip your current "
      "nameplate</c>?\n<cy>You can "
      "re-equip it later from this shop page.</c>",
      "No", "Yes", [this](auto, bool yes) {
        if (!yes)
          return;

        // show a spinner/popup while we call the backend
        auto popupRef =
            UploadActionPopup::create(nullptr, "Unequipping nameplate...");
        popupRef->show();

        // validate token
        auto token = Mod::get()->getSavedValue<std::string>("argon_token");
        if (token.empty()) {
          popupRef->showFailMessage("Argon auth missing");
          return;
        }

        // build JSON body (same format as RLBuyItemPopup::onApply)
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
        jsonBody["argonToken"] = token;
        jsonBody["index"] = 0;

        auto req = web::WebRequest();
        req.bodyJSON(jsonBody);

        Ref<UploadActionPopup> upRef = popupRef;
        Ref<RLShopLayer> self = this;
        async::spawn(
            req.post("https://gdrate.arcticwoof.xyz/setNameplate"),
            [self, upRef](web::WebResponse res) {
              if (!upRef)
                return;
              if (!res.ok()) {
                log::warn("Failed to unequip nameplate on server: {}",
                          res.code());
                upRef->showFailMessage(
                    "Failed to unequip nameplate on server.");
                return;
              }
              auto jsonRes = res.json();
              if (!jsonRes) {
                upRef->showFailMessage("Invalid server response.");
                return;
              }
              auto json = jsonRes.unwrap();
              bool success = json["success"].asBool().unwrapOrDefault();
              if (!success) {
                upRef->showFailMessage(json["message"].asString().unwrapOr(
                    "Failed to unequip nameplate."));
                return;
              }

              Mod::get()->setSavedValue<int>("selected_nameplate", 0);
              upRef->showSuccessMessage("Nameplate unequipped!");

              if (self) {
                self->updateShopPage();
              }
            });
      });
}

void RLShopLayer::updateShopPage() {
  const int perPage = 8;
  int totalItems = static_cast<int>(m_shopItems.size());
  int totalPages =
      std::max(1, static_cast<int>((totalItems + perPage - 1) / perPage));
  if (m_shopPage < 0)
    m_shopPage = 0;
  if (m_shopPage >= totalPages)
    m_shopPage = totalPages - 1;

  // clear rows
  if (m_shopRow1)
    m_shopRow1->removeAllChildrenWithCleanup(true);
  if (m_shopRow2)
    m_shopRow2->removeAllChildrenWithCleanup(true);

  int start = m_shopPage * perPage;
  int end = std::min(start + perPage, totalItems);
  for (int i = start; i < end; ++i) {
    const auto &s = m_shopItems[i];
    auto item =
        RLNameplateItem::create(s.idx, s.price, s.creatorId, s.creatorUsername,
                                this, menu_selector(RLShopLayer::onBuyItem));
    item->setTag(s.idx);
    int localIndex = i - start;
    if (localIndex < 4) {
      m_shopRow1->addChild(item);
    } else {
      m_shopRow2->addChild(item);
    }
  }

  if (m_shopRow1)
    m_shopRow1->updateLayout();
  if (m_shopRow2)
    m_shopRow2->updateLayout();

  // update page UI
  if (m_pageLabel) {
    m_pageLabel->setString(
        fmt::format("{}/{}", m_shopPage + 1, totalPages).c_str());
  }
  if (m_prevPageBtn) {
    m_prevPageBtn->setEnabled(m_shopPage > 0);
    m_prevPageBtn->setOpacity(m_shopPage > 0 ? 255 : 120);
  }
  if (m_nextPageBtn) {
    m_nextPageBtn->setEnabled(m_shopPage < totalPages - 1);
    m_nextPageBtn->setOpacity(m_shopPage < totalPages - 1 ? 255 : 120);
  }

  // ensure parent recomputes layout
  if (m_shopRow1 && m_shopRow2) {
    if (m_shopRow1->getParent()) {
      static_cast<CCNode *>(m_shopRow1->getParent())->updateLayout();
    }
  }
}

void RLShopLayer::refreshRubyLabel() {
  if (!m_rubyLabel)
    return;
  int val = Mod::get()->getSavedValue<int>("rubies", 0);
  m_rubyLabel->setTargetCount(val);
  m_rubyLabel->updateCounter(0.25f);
}

void RLShopLayer::onPrevPage(CCObject *sender) {
  if (m_shopPage > 0) {
    m_shopPage--;
    updateShopPage();
  }
}

void RLShopLayer::onNextPage(CCObject *sender) {
  const int perPage = 8;
  int totalItems = static_cast<int>(m_shopItems.size());
  int totalPages =
      std::max(1, static_cast<int>((totalItems + perPage - 1) / perPage));
  if (m_shopPage < totalPages - 1) {
    m_shopPage++;
    updateShopPage();
  }
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

        auto ownedPath =
            dirs::getModsSaveDir() / Mod::get()->getID() / "owned_items.json";
        if (utils::file::readString(ownedPath)) {
          auto writeRes2 = utils::file::writeString(ownedPath, "[]");
          if (!writeRes2) {
            log::warn("Failed to clear owned items file: {}", ownedPath);
          }
        }
        Mod::get()->setSavedValue<int>("selected_nameplate", 0);

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

        this->updateShopPage();
      });
}