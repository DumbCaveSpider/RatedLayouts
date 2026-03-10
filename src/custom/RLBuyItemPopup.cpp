#include "RLBuyItemPopup.hpp"
#include "../utils/RLNameplateItem.hpp"
#include "RLAchievements.hpp"
#include "RLShopLayer.hpp"
#include "ccTypes.h"
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <fmt/format.h>
#include <string>

using namespace geode::prelude;
using namespace ratedlayouts;

RLBuyItemPopup *RLBuyItemPopup::create(int itemId, int creatorId,
                                       const std::string &creatorUsername,
                                       const std::string &iconUrl, int value,
                                       RLShopLayer *owner) {
  auto ret = new RLBuyItemPopup();
  if (!ret)
    return nullptr;
  ret->m_itemId = itemId;
  ret->m_creatorId = creatorId;
  ret->m_creatorUsername = creatorUsername;
  ret->m_iconUrl = iconUrl;
  ret->m_value = value;
  ret->m_owner = owner;
  if (ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

bool RLBuyItemPopup::init() {
  if (!Popup::init(260.f, 190.f, "GJ_square02.png"))
    return false;

  bool owned = RLNameplateItem::isOwned(m_itemId);
  setTitle(owned
               ? "Nameplate Owned"
               : fmt::format("Buy Nameplate for {} Rubies?", m_value).c_str());

  // kill the close button
  if (auto closeBtn = this->m_closeBtn) {
    closeBtn->removeFromParent();
  }

  // status (top of popup)
  m_statusLabel =
      CCLabelBMFont::create(owned ? "Owned" : "Not owned", "bigFont.fnt");
  m_statusLabel->setScale(0.45f);
  m_statusLabel->setPosition(
      {m_mainLayer->getScaledContentSize().width / 2,
       m_mainLayer->getScaledContentSize().height / 2 + 30});
  m_statusLabel->setColor(owned ? ccGREEN : ccRED);
  m_mainLayer->addChild(m_statusLabel);

  // preview icon (banner from server)
  auto plateName = fmt::format("nameplate_{}.png", m_itemId);
  std::string url =
      "https://gdrate.arcticwoof.xyz/nameplates/banner/" + plateName;
  auto lazy = LazySprite::create(
      {m_mainLayer->getScaledContentSize() - CCSize{10, 10}}, true);
  lazy->loadFromUrl(url, CCImage::kFmtPng, true);
  lazy->setAutoResize(true);
  lazy->setPosition({m_mainLayer->getScaledContentSize().width / 2.f,
                     m_mainLayer->getScaledContentSize().height / 2.f + 6.f});
  m_mainLayer->addChild(lazy);

  // creator username (clickable if creatorId > 0)
  auto creatorLabel = CCLabelBMFont::create(
      fmt::format("By {}", m_creatorUsername).c_str(), "goldFont.fnt");
  creatorLabel->setScale(0.5f);

  if (m_creatorId > 0) {
    auto creatorMenu = CCMenu::create();
    creatorMenu->setPosition({0, 0});

    auto creatorBtn = CCMenuItemSpriteExtra::create(
        creatorLabel, this, menu_selector(RLBuyItemPopup::onProfile));
    creatorBtn->setPosition(
        {m_mainLayer->getScaledContentSize().width / 2,
         m_mainLayer->getScaledContentSize().height / 2.f - 30.f});
    creatorBtn->setTag(m_creatorId);

    creatorMenu->addChild(creatorBtn);
    m_mainLayer->addChild(creatorMenu);
  }

  // Buy / Cancel buttons
  auto buyText = owned ? "Apply" : "Buy";
  auto buySpr = ButtonSprite::create(buyText, 60, true, "goldFont.fnt",
                                     "GJ_button_01.png", 30.f, 1.f);
  // when owned the button should call onApply, otherwise onBuy
  m_buyBtn = CCMenuItemSpriteExtra::create(
      buySpr, this,
      owned ? menu_selector(RLBuyItemPopup::onApply)
            : menu_selector(RLBuyItemPopup::onBuy));
  m_buyBtn->setPosition(
      {m_mainLayer->getContentSize().width / 2.f + 60.f, 25.f});
  m_buttonMenu->addChild(m_buyBtn);

  auto cancelSpr = ButtonSprite::create("Cancel", 60, true, "goldFont.fnt",
                                        "GJ_button_06.png", 30.f, 1.f);
  m_cancelBtn = CCMenuItemSpriteExtra::create(
      cancelSpr, this, menu_selector(RLBuyItemPopup::onCancel));
  m_cancelBtn->setPosition(
      {m_mainLayer->getContentSize().width / 2.f - 60.f, 25.f});
  m_buttonMenu->addChild(m_cancelBtn);

  return true;
}

void RLBuyItemPopup::onApply(CCObject *sender) {
  // validate token/account
  auto token = Mod::get()->getSavedValue<std::string>("argon_token");
  if (token.empty()) {
    Notification::create("Argon auth missing", NotificationIcon::Warning)
        ->show();
    return;
  }

  auto upopup = UploadActionPopup::create(nullptr, "Applying Nameplate...");
  upopup->show();

  // build request body
  matjson::Value jsonBody = matjson::Value::object();
  jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
  jsonBody["argonToken"] = token;
  jsonBody["index"] = m_itemId;

  auto req = web::WebRequest();
  req.bodyJSON(jsonBody);

  Ref<RLBuyItemPopup> self = this;
  Ref<UploadActionPopup> popupRef = upopup;
  async::spawn(req.post("https://gdrate.arcticwoof.xyz/setNameplate"),
               [self, popupRef](web::WebResponse res) {
                 if (!self || !popupRef)
                   return;
                 if (!res.ok()) {
                   popupRef->showFailMessage("Failed to apply nameplate.");
                   return;
                 }
                 auto jsonRes = res.json();
                 if (!jsonRes) {
                   popupRef->showFailMessage("Invalid server response.");
                   return;
                 }
                 auto json = jsonRes.unwrap();
                 bool success = json["success"].asBool().unwrapOrDefault();
                 if (success) {
                   // persist selected nameplate locally
                   Mod::get()->setSavedValue<int>("selected_nameplate",
                                                  self->m_itemId);
                   popupRef->showSuccessMessage("Nameplate applied!");

                   self->m_owner->updateShopPage();
                   self->onClose(nullptr);

                 } else {
                   popupRef->showFailMessage("Failed to apply nameplate.");
                 }
               });
}

void RLBuyItemPopup::onBuy(CCObject *sender) {
  int current = Mod::get()->getSavedValue<int>("rubies", 0);
  if (current < m_value) {
    // gen random
    std::string response =
        "You don't have enough <cr>rubies</c> to buy this item!";
    static geode::utils::random::Generator gen = [] {
      geode::utils::random::Generator g;
      g.seed(geode::utils::random::secureU64()); // seed once
      return g;
    }();

    int v = gen.generate<int>(0, 9);
    switch (v) {
    case 1:
      response = "You can't <cg>afford</c> that <cl>item</c>!";
      break;
    case 2:
      response = "You need more <cr>rubies</c> to get this!";
      break;
    case 3:
      response = fmt::format(
          "<cg>{}</c>, you need more <cr>rubies</c> to buy this item!",
          GJAccountManager::get()->m_username);
      break;
    case 4:
      response = "You don't have enough <cr>rubies</c>! <d150><co>hah you "
                 "broke bud.</c>";
      break;
    case 5:
      response = fmt::format(
          "Yikes! You just <cr>{} rubies short</c>, can't let you have this.",
          m_value - current);
      break;
    case 6:
      response = "Go play some <cl>layouts</c> to earn more <cr>rubies</c>!";
      break;
    case 7:
      response =
          "Oh you short on <cr>rubies</c>? That's unfortunate. womp womp";
      break;
    case 8:
      response =
          fmt::format("<cr>ERROR! INSUFFICIENT RUBIES! GO HOME</c> <cg>{}</c>!",
                      GJAccountManager::get()->m_username);
    }
    DialogObject *obj = DialogObject::create("ArcticWoof", response.c_str(), 28,
                                             1.f, false, ccWHITE);
    auto dialog = DialogLayer::createDialogLayer(obj, nullptr, 2);
    dialog->addToMainScene();
    dialog->animateInRandomSide();

    dialog->m_characterSprite->setVisible(
        false); // i was gonna be fancy and use cctexturecache thingy but didnt
                // work so just did hacky way
    auto awSprite =
        CCSprite::createWithSpriteFrameName("RL_dialogIconAW.png"_spr);
    awSprite->setPosition(dialog->m_characterSprite->getPosition());
    dialog->m_mainLayer->addChild(awSprite, 1);
    dialog->m_characterSprite->removeFromParent();
    return;
  }

  Mod::get()->setSavedValue<int>("rubies", current - m_value);

  // persist ownership to owned_items.json
  if (RLNameplateItem::markOwned(m_itemId)) {
    FMODAudioEngine::sharedEngine()->playEffect("geode.loader/newNotif01.ogg");
    RLAchievements::onReward("misc_ruby");

    // update popup UI to reflect owned state
    if (m_statusLabel) {
      m_statusLabel->setString("Owned");
      m_statusLabel->setColor(ccGREEN);
    }

    // replace Buy button with Apply (now calls onApply)
    if (m_buyBtn) {
      m_buyBtn->removeFromParent();
      auto applySpr = ButtonSprite::create("Apply", 60, true, "goldFont.fnt",
                                           "GJ_button_01.png", 30.f, 1.f);
      m_buyBtn = CCMenuItemSpriteExtra::create(
          applySpr, this, menu_selector(RLBuyItemPopup::onApply));
      m_buyBtn->setPosition(
          {m_mainLayer->getContentSize().width / 2.f + 60.f, 25.f});
      m_buttonMenu->addChild(m_buyBtn);
    }

    if (m_owner) {
      m_owner->refreshRubyLabel();
      m_owner->updateShopPage();
    }
  }
}

void RLBuyItemPopup::onCancel(CCObject *sender) { this->onClose(nullptr); }

void RLBuyItemPopup::onProfile(CCObject *sender) {
  // if view profile button pressed
  if (m_creatorId > 0) {
    ProfilePage::create(m_creatorId, false)->show();
  }
}
