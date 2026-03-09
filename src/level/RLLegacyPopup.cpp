#include "RLLegacyPopup.hpp"
#include "Geode/cocos/cocoa/CCObject.h"
#include "Geode/ui/MDPopup.hpp"

using namespace geode::prelude;

static int mapRatingToLevel(int rating) {
  switch (rating) {
  case 1:
    return -1;
  case 2:
    return 1;
  case 3:
    return 2;
  case 4:
  case 5:
    return 3;
  case 6:
  case 7:
    return 4;
  case 8:
  case 9:
    return 5;
  case 10:
    return 7;
  case 15:
    return 8;
  case 20:
    return 6;
  case 25:
    return 9;
  case 30:
    return 10;
  default:
    return 0;
  }
}

static std::string getResponseFailMessage(web::WebResponse const &response,
                                          std::string const &fallback) {
  auto message = response.string().unwrapOrDefault();
  if (!message.empty())
    return message;
  return fallback;
}

void RLLegacyPopup::updateFromJson(const matjson::Value &json) {
  int difficulty = json["difficulty"].asInt().unwrapOrDefault();
  int featured = json["featured"].asInt().unwrapOrDefault();
  std::string note = json["note"].asString().unwrapOrDefault();

  if (m_diffSprite) {
    int lvl = mapRatingToLevel(difficulty);
    m_diffSprite->updateDifficultyFrame(lvl, GJDifficultyName::Long);
  }

  if (m_rewardLabel) {
    m_rewardLabel->setString(numToString(difficulty).c_str());
  }
  if (m_rewardIcon) {
    bool plat = (m_level && m_level->isPlatformer());
    std::string desired = plat ? "RL_planetMed.png"_spr : "RL_starMed.png"_spr;
    if (auto frame =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                desired.c_str())) {
      m_rewardIcon->setDisplayFrame(frame);
    }
  }
  if (m_infoText) {
    if (!note.empty()) {
      m_infoText->setString(note.c_str());
    } else {
      m_infoText->setString("No notes provided for this legacy layout.");
    }
  }

  if (m_featuredRing) {
    m_featuredRing->removeFromParent();
    m_featuredRing = nullptr;
  }
  if (featured > 0 && m_diffSprite) {
    std::string frameName;
    switch (featured) {
    case 1:
      frameName = "RL_featuredCoin.png"_spr;
      break;
    case 2:
      frameName = "RL_epicFeaturedCoin.png"_spr;
      break;
    case 3:
      frameName = "RL_legendaryFeaturedCoin.png"_spr;
      break;
    default:
      break;
    }
    if (!frameName.empty()) {
      m_featuredRing =
          CCSpriteGrayscale::createWithSpriteFrameName(frameName.c_str());
      if (m_featuredRing && m_diffSprite) {
        m_featuredRing->setPosition(
            {m_diffSprite->getContentSize().width / 2,
             m_diffSprite->getContentSize().height / 2});
        m_featuredRing->setScale(m_diffSprite->getScale());
        m_featuredRing->setZOrder(-1);
        m_diffSprite->addChild(m_featuredRing);
      }
    }
  }
}

RLLegacyPopup *RLLegacyPopup::create(GJGameLevel *level) {
  auto popup = new RLLegacyPopup();
  if (popup) {
    popup->m_level = level;
    if (popup->init()) {
      popup->autorelease();
      return popup;
    }
  }
  delete popup;
  return nullptr;
}

bool RLLegacyPopup::init() {
  if (!Popup::init(380.f, 240.f, "GJ_square02.png"))
    return false;

  setTitle("Legacy Rated Layout");
  m_noElasticity = true;
  auto contentSize = m_mainLayer->getContentSize();

  m_title->setPositionY(m_title->getPositionY() + 5);

  m_diffSprite = GJDifficultySprite::create(0, GJDifficultyName::Long);
  m_diffSprite->setPosition(
      {contentSize.width / 2.f - 40.f, contentSize.height - 80.f});
  m_diffSprite->setScale(1.0f);
  m_mainLayer->addChild(m_diffSprite, 1);

  std::string rewardFrame = "RL_starMed.png"_spr;
  if (m_level && m_level->isPlatformer()) {
    rewardFrame = "RL_planetMed.png"_spr;
  }
  m_rewardIcon = CCSprite::createWithSpriteFrameName(rewardFrame.c_str());
  if (m_rewardIcon && m_diffSprite) {
    m_rewardIcon->setPosition({m_diffSprite->getPositionX() + 60.f,
                               m_diffSprite->getPositionY() + 5.f});
    m_mainLayer->addChild(m_rewardIcon, 1);
  }
  m_rewardLabel = CCLabelBMFont::create("0", "bigFont.fnt");
  if (m_rewardLabel && m_diffSprite && m_rewardIcon) {
    m_rewardLabel->setScale(0.6f);
    m_rewardLabel->setAnchorPoint({0.f, 0.5f});
    m_rewardLabel->setPosition({m_rewardIcon->getContentSize().width + 5.f,
                                m_rewardIcon->getContentSize().height / 2.f});
    m_rewardIcon->addChild(m_rewardLabel, 1);
  }

  // delete button only visible by admins
  bool isClassicAdmin = Mod::get()->getSavedValue<bool>("isClassicAdmin");
  bool isPlatAdmin = Mod::get()->getSavedValue<bool>("isPlatAdmin");
  bool isPlat = m_level ? m_level->isPlatformer() : false;
  if ((isClassicAdmin && !isPlat) ||
      (isPlatAdmin && isPlat)) {
    auto deleteSpr = ButtonSprite::create("Delete", 50, true, "goldFont.fnt",
                                          "GJ_button_06.png", 20.f, 1.f);
    auto deleteBtn = CCMenuItemSpriteExtra::create(
        deleteSpr, this, menu_selector(RLLegacyPopup::onDeleteLegacy));
    deleteBtn->setPosition({m_diffSprite->getPositionX() - 100.f,
                            m_diffSprite->getPositionY() - 20.f});
    m_buttonMenu->addChild(deleteBtn);
  }

  // bg
  auto bg = NineSlice::create("square02b_001.png", {0.0f, 0.0f, 80.0f, 80.0f});
  bg->setColor({0, 0, 0});
  bg->setOpacity(75);
  bg->setContentSize({200, 90});
  bg->setPosition({contentSize.width / 2.f, contentSize.height - 75.f});
  m_mainLayer->addChild(bg);

  m_levelLabel =
      CCLabelBMFont::create(m_level->m_levelName.c_str(), "bigFont.fnt");
  if (m_levelLabel) {
    m_levelLabel->setPosition(
        {contentSize.width / 2.f, contentSize.height - 40.f});
    m_levelLabel->setScale(0.5f);
    m_levelLabel->setAlignment(kCCTextAlignmentCenter);
    m_levelLabel->limitLabelWidth(bg->getContentSize().width - 20, 0.5f, 0.3f);
    m_mainLayer->addChild(m_levelLabel, 1);
  }

  auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoSpr->setScale(0.7f);
  m_infoButton = CCMenuItemSpriteExtra::create(
      infoSpr, this, menu_selector(RLLegacyPopup::onInfoButton));
  if (m_infoButton) {
    m_infoButton->setPosition({contentSize.width, contentSize.height});
    m_buttonMenu->addChild(m_infoButton);
  }

  m_infoText = MDTextArea::create("", {contentSize.width - 40.f, 100.f});
  m_infoText->setPosition(
      {contentSize.width / 2.f, contentSize.height / 2.f - 55.f});
  m_infoText->setAnchorPoint({0.5f, 0.5f});
  m_mainLayer->addChild(m_infoText);

  if (m_level && m_level->m_levelID != 0) {
    int lid = m_level->m_levelID;
    auto req = web::WebRequest();
    req.param("levelId", numToString(lid));
    Ref<RLLegacyPopup> self = this;
    m_fetchTask.spawn(req.get("https://gdrate.arcticwoof.xyz/getLegacy"),
                      [self](web::WebResponse response) {
                        if (!self)
                          return;
                        if (!response.ok()) {
                          log::warn("Legacy fetch returned {}",
                                    response.code());
                          return;
                        }
                        auto jres = response.json();
                        if (!jres) {
                          log::warn("Failed to parse legacy JSON");
                          return;
                        }
                        self->updateFromJson(jres.unwrap());
                      });
  }

  return true;
}

void RLLegacyPopup::onInfoButton(CCObject *sender) {
  MDPopup::create(
      "Legacy Rated Layouts",
      "<cl>**Legacy Rated Layouts**</c> is a <cg>special rating tier "
      "dedicated</c> to "
      "layouts that got <co>rated before the standards and rating system "
      "change</c> "
      "proposed by the <cr>new three classic admins</c> "
      "([Sokuto](user:9135587), [ATXM](user:10092120) & [qoid](user:13603703)) "
      "back in <cg>**February "
      "2026**</c>.\n\n"
      "This <cg>rating tier</c> is <cl>reserved for layouts</c> that are "
      "<cc>affected by the "
      "latest standards and criterias</c> required to get a rate. This feature "
      "is "
      "<cg>added in order to prevent confusion</c> or <co>false reports from "
      "new players</c> "
      "of the mod and serve as a <cf>respect for players who have previously "
      "beaten</c> the <cl>affected layouts</c>.\n\n"
      "This <cg>special rating tier</c> cannot be <co>awarded to layouts "
      "rated</c> after the change of <cc>standards and criterias</c>.\n\n"
      "These rating tier <co>does not give Blueprint Points</c> to the "
      "creator of this layout but still "
      "given the <cl>legacy rated</c> in the "
      "respect of the players who <cl>beaten these layouts</c> before the "
      "change of "
      "the <cc>rating system</c>.\n\n",
      "OK")
      ->show();
}

void RLLegacyPopup::onDeleteLegacy(CCObject *sender) {

  // check if user has admin privileges
  bool isClassicAdmin = Mod::get()->getSavedValue<bool>("isClassicAdmin");
  bool isPlatAdmin = Mod::get()->getSavedValue<bool>("isPlatAdmin");
  if (!isClassicAdmin && !isPlatAdmin) {
    Notification::create(
        "You don't have permission to delete this legacy layout",
        NotificationIcon::Error)
        ->show();
    return;
  }

  createQuickPopup(
      "Delete Legacy Layout",
      "Are you sure you want to <cr>delete</c> this <cl>legacy "
      "layout</c>?\n<cy>This action cannot be undone.</c>",
      "Cancel", "Delete", [this](auto, bool yes) {
        if (!yes)
          return;
        auto uploadPopup =
            UploadActionPopup::create(nullptr, "Deleting legacy layout...");
        uploadPopup->show();
        // send delete request
        int accountId = GJAccountManager::get()->m_accountID;
        auto argonToken = Mod::get()->getSavedValue<std::string>("argon_token");
        if (argonToken.empty()) {
          uploadPopup->showFailMessage("Missing argon token!");
          return;
        }

        matjson::Value body = matjson::Value::object();
        body["accountId"] = accountId;
        body["argonToken"] = argonToken;
        body["levelId"] = m_level ? m_level->m_levelID : 0;

        auto req = web::WebRequest();
        req.bodyJSON(body);
        Ref<RLLegacyPopup> self = this;
        self->m_deleteTask.spawn(
            req.post("https://gdrate.arcticwoof.xyz/deleteLegacy"),
            [self, uploadPopup](web::WebResponse res) {
              if (!self)
                return;
              if (!res.ok()) {
                uploadPopup->showFailMessage(getResponseFailMessage(
                    res, "Failed to delete legacy layout! Try again later."));
                return;
              }
              auto jsonRes = res.json();
              if (!jsonRes) {
                uploadPopup->showFailMessage(
                    getResponseFailMessage(res, "Invalid server response"));
                return;
              }
              auto json = jsonRes.unwrap();
              bool success = json["success"].asBool().unwrapOrDefault();
              if (success) {
                uploadPopup->showSuccessMessage("Legacy layout deleted");
              } else {
                uploadPopup->showFailMessage(
                    getResponseFailMessage(res, "Failed! Try again later."));
              }
            });
      });
}