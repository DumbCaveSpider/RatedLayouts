#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>

#include "../custom/RLAchievements.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include "../player/RLDifficultyTotalPopup.hpp"
#include "../player/RLUserControl.hpp"
#include "GUI/CCControlExtension/CCScale9Sprite.h"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/ui/BasedButtonSprite.hpp"

using namespace geode::prelude;

class $modify(RLProfilePage, ProfilePage) {
  struct Fields {
    int role = 0;
    int accountId = 0;
    bool isSupporter = false;
    bool isBooster = false;

    int m_points = 0;
    int m_planets = 0;
    int m_stars = 0;
    int m_coins = 0;

    async::TaskHolder<web::WebResponse> m_profileTask;
    async::TaskHolder<Result<std::string>> m_authTask;
    bool m_profileInFlight = false;
    int m_profileForAccount = -1;

    ~Fields() {
      m_profileTask.cancel();
      m_authTask.cancel();
    }
  };

  CCMenu *createStatEntry(char const *entryID, char const *labelID,
                          std::string const &text, char const *iconFrameOrPath,
                          SEL_MenuHandler iconCallback) {
    auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setID(labelID);

    constexpr float kLabelScale = 0.60f;
    constexpr float kMaxLabelW = 58.f;
    constexpr float kMinScale = 0.20f;

    label->setScale(kLabelScale);
    label->limitLabelWidth(kMaxLabelW, kLabelScale, kMinScale);

    CCSprite *iconSprite = nullptr;
    if (iconFrameOrPath && iconFrameOrPath[0]) {
      // try sprite frame first
      iconSprite = CCSprite::createWithSpriteFrameName(iconFrameOrPath);
      // if that fails, try loading as a file path
      if (!iconSprite) {
        iconSprite = CCSprite::create(iconFrameOrPath);
      }
    }

    // final fallback to avoid null deref
    if (!iconSprite) {
      iconSprite = CCSprite::create();
    }

    iconSprite->setScale(0.8f);

    auto iconBtn =
        CCMenuItemSpriteExtra::create(iconSprite, this, iconCallback);
    if (!iconBtn) {
      // ensure we always have some valid child so layout code doesn't crash
      auto fallbackSpr = CCSprite::create();
      fallbackSpr->setVisible(false);
      iconBtn = CCMenuItemSpriteExtra::create(fallbackSpr, this, iconCallback);
    }

    auto ls = label->getScaledContentSize();
    auto is = iconBtn->getScaledContentSize();

    constexpr float gap = 2.f;
    constexpr float pad = 2.f;

    float h = std::max(ls.height, is.height);
    float w = pad + ls.width + gap + is.width + pad;

    auto entry = CCMenu::create();
    entry->setID(entryID);
    entry->setContentSize({w, h});
    entry->setAnchorPoint({0.f, 0.5f});

    label->setAnchorPoint({0.f, 0.5f});
    label->setPosition({pad, h / 2.f});

    iconBtn->setAnchorPoint({0.f, 0.5f});
    iconBtn->setPosition({pad + ls.width + gap, h / 2.f});

    entry->addChild(label);
    entry->addChild(iconBtn);

    return entry;
  }

  void updateStatLabel(char const *labelID, std::string const &text) {
    auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
    if (!rlStatsMenu)
      return;

    auto label = typeinfo_cast<CCLabelBMFont *>(
        rlStatsMenu->getChildByIDRecursive(labelID));
    if (!label)
      return;

    label->setString(text.c_str());

    constexpr float kLabelScale = 0.60f;
    constexpr float kMaxLabelW = 58.f;
    constexpr float kMinScale = 0.20f;
    label->limitLabelWidth(kMaxLabelW, kLabelScale, kMinScale);

    // Recompute parent entry size and reposition children so layout stays
    // correct
    if (auto entry = label->getParent()) {
      auto ls = label->getScaledContentSize();

      CCNode *iconBtn = nullptr;
      for (auto child : CCArrayExt<CCNode>(entry->getChildren())) {
        if (auto btn = typeinfo_cast<CCMenuItemSpriteExtra *>(child)) {
          iconBtn = btn;
          break;
        }
      }

      CCSize is = {0.f, 0.f};
      if (iconBtn) {
        is = iconBtn->getScaledContentSize();
      }

      constexpr float gap = 2.f;
      constexpr float pad = 2.f;

      float h = std::max(ls.height, is.height);
      float w = pad + ls.width + gap + is.width + pad;

      entry->setContentSize({w, h});

      label->setAnchorPoint({0.f, 0.5f});
      label->setPosition({pad, h / 2.f});

      if (iconBtn) {
        iconBtn->setAnchorPoint({0.f, 0.5f});
        iconBtn->setPosition({pad + ls.width + gap, h / 2.f});
      }
    }

    // Ensure the menu and surrounding layout are updated when data changes
    rlStatsMenu->updateLayout();
    if (auto rlButtonsMenu = getChildByIDRecursive("rl-buttons-menu"))
      rlButtonsMenu->updateLayout();
  }

  bool init(int accountID, bool ownProfile) {
    if (!ProfilePage::init(accountID, ownProfile))
      return false;

    if (auto statsMenu = m_mainLayer->getChildByID("stats-menu")) {
      statsMenu->updateLayout();
    }

    // create the rl buttons menu at the side
    auto rlButtonsMenu = CCMenu::create();
    rlButtonsMenu->setID("rl-buttons-menu");
    rlButtonsMenu->setPosition(
        {m_mainLayer->getContentSize().width / 2.f + 250.f,
         m_mainLayer->getContentSize().height / 2.f});
    rlButtonsMenu->setContentSize({32.f, 100.f});

    // Arrange buttons vertically in a centered column
    rlButtonsMenu->setLayout(ColumnLayout::create()
                                 ->setGap(6.f)
                                 ->setAxisAlignment(AxisAlignment::Center)
                                 ->setGrowCrossAxis(false));

    m_mainLayer->addChild(rlButtonsMenu);

    if (rlButtonsMenu) {
      auto rlButtonBg = CCScale9Sprite::create("GJ_square02.png");
      rlButtonBg->setContentSize(rlButtonsMenu->getContentSize() +
                                 CCSize(10.f, 10.f));
      rlButtonBg->setPosition(rlButtonsMenu->getPosition());
      m_mainLayer->addChild(rlButtonBg, -1);
    }

    return true;
  }

  void loadPageFromUserInfo(GJUserScore *score) {
    ProfilePage::loadPageFromUserInfo(score);

    auto statsMenu = m_mainLayer->getChildByID("stats-menu");
    if (!statsMenu) {
      log::warn("stats-menu not found");
      return;
    }

    if (auto rlStatsBtnFound = getChildByIDRecursive("rl-stats-btn"))
      rlStatsBtnFound->removeFromParent();
    if (auto rlStatsMenuFound = getChildByIDRecursive("rl-stats-menu"))
      rlStatsMenuFound->removeFromParent();

    auto rlButtonsMenu = m_mainLayer->getChildByID("rl-buttons-menu");
    if (!rlButtonsMenu) {
      log::warn("rl-buttons-menu not found — recreating");
      // Recreate the buttons menu and background so the page shows correctly
      rlButtonsMenu = CCMenu::create();
      rlButtonsMenu->setID("rl-buttons-menu");
      rlButtonsMenu->setPosition(
          {m_mainLayer->getContentSize().width / 2.f + 250.f,
           m_mainLayer->getContentSize().height / 2.f});
      rlButtonsMenu->setContentSize({32.f, 100.f});
      rlButtonsMenu->setLayout(ColumnLayout::create()
                                   ->setGap(6.f)
                                   ->setAxisAlignment(AxisAlignment::Center)
                                   ->setGrowCrossAxis(false));
      m_mainLayer->addChild(rlButtonsMenu);

      // recreate background
      auto rlButtonBg = CCScale9Sprite::create("GJ_square02.png");
      rlButtonBg->setContentSize(rlButtonsMenu->getContentSize() +
                                 CCSize(10.f, 10.f));
      rlButtonBg->setPosition(rlButtonsMenu->getPosition());
      m_mainLayer->addChild(rlButtonBg, -1);
    }

    // view stats
    if (GJAccountManager::sharedState()->m_accountID != 0) {
      auto rlViewSpr =
          CCSprite::createWithSpriteFrameName("RL_starBig.png"_spr);
      auto rlStatsSprOff = EditorButtonSprite::create(
          rlViewSpr, EditorBaseColor::Gray, EditorBaseSize::Normal);
      auto rlStatsSprOn = EditorButtonSprite::create(
          rlViewSpr, EditorBaseColor::LightBlue, EditorBaseSize::Normal);

      auto rlStatsBtn = CCMenuItemToggler::create(
          rlStatsSprOff, rlStatsSprOn, this,
          menu_selector(RLProfilePage::onStatsSwitcher));
      rlStatsBtn->setID("rl-stats-btn");
      rlButtonsMenu->addChild(rlStatsBtn);
    }

    // if u are mod or admin, show manage button
    if (Mod::get()->getSavedValue<int>("role") >= 1) {
      if (!rlButtonsMenu->getChildByID("rl-manage-btn")) {
        auto modUserSpr =
            CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
        auto modUserButton = EditorButtonSprite::create(
            modUserSpr, EditorBaseColor::LightBlue, EditorBaseSize::Normal);
        auto modUserBtnItem = CCMenuItemSpriteExtra::create(
            modUserButton, this, menu_selector(RLProfilePage::onUserManage));
        modUserBtnItem->setID("rl-manage-btn");
        rlButtonsMenu->addChild(modUserBtnItem);
      }
    }

    auto rlStatsMenu = CCMenu::create();
    rlStatsMenu->setID("rl-stats-menu");
    rlStatsMenu->setContentSize(statsMenu->getContentSize());

    auto row = RowLayout::create();
    row->setAxisAlignment(AxisAlignment::Center);
    row->setCrossAxisAlignment(AxisAlignment::Center);
    row->setGap(4.f);
    rlStatsMenu->setLayout(row);

    rlStatsMenu->setAnchorPoint({0.5f, 0.5f});

    rlStatsMenu->setPositionY(245.f);

    auto starsText = GameToolbox::pointsToString(m_fields->m_stars);
    auto planetsText = GameToolbox::pointsToString(m_fields->m_planets);

    auto starsEntry = createStatEntry(
        "rl-stars-entry", "rl-stars-label", starsText, "RL_starMed.png"_spr,
        menu_selector(RLProfilePage::onBlueprintStars));

    auto planetsEntry = createStatEntry(
        "rl-planets-entry", "rl-planets-label", planetsText,
        "RL_planetMed.png"_spr, menu_selector(RLProfilePage::onPlanetsClicked));

    auto coinsEntry =
        createStatEntry("rl-coins-entry", "rl-coins-label",
                        GameToolbox::pointsToString(m_fields->m_coins),
                        "RL_BlueCoinSmall.png"_spr, nullptr);

    rlStatsMenu->addChild(starsEntry);
    rlStatsMenu->addChild(planetsEntry);
    rlStatsMenu->addChild(coinsEntry);

    if (m_fields->m_points > 0) {
      auto pointsEntry =
          createStatEntry("rl-points-entry", "rl-points-label",
                          GameToolbox::pointsToString(m_fields->m_points),
                          "RL_blueprintPoint01.png"_spr,
                          menu_selector(RLProfilePage::onLayoutPointsClicked));
      rlStatsMenu->addChild(pointsEntry);
    }

    rlStatsMenu->setVisible(false);
    statsMenu->setVisible(true);

    m_mainLayer->addChild(rlStatsMenu);

    if (score) {
      this->fetchProfileData(score->m_accountID);
    }

    rlStatsMenu->updateLayout();

    statsMenu->updateLayout();
    rlButtonsMenu->updateLayout();
  }

  void onStatsSwitcher(CCObject *sender) {
    auto statsMenu = getChildByIDRecursive("stats-menu");
    auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
    auto switcher = typeinfo_cast<CCMenuItemToggler *>(sender);

    if (!statsMenu || !rlStatsMenu || !switcher)
      return;

    if (!switcher->isToggled()) {
      statsMenu->setVisible(false);
      if (auto m = typeinfo_cast<CCMenu *>(statsMenu))
        m->setEnabled(false);

      rlStatsMenu->setVisible(true);
      if (auto m = typeinfo_cast<CCMenu *>(rlStatsMenu))
        m->setEnabled(true);
    } else {
      statsMenu->setVisible(true);
      if (auto m = typeinfo_cast<CCMenu *>(statsMenu))
        m->setEnabled(true);

      rlStatsMenu->setVisible(false);
      if (auto m = typeinfo_cast<CCMenu *>(rlStatsMenu))
        m->setEnabled(false);
    }
  }

  void fetchProfileData(int accountId) {
    log::info("Fetching profile data for account ID: {}", accountId);
    m_fields->accountId = accountId;
    if (m_fields->accountId == 7689052)
      RLAchievements::onReward("misc_arcticwoof");

    auto accountData = argon::getGameAccountData();

    m_fields->m_authTask.spawn(
        argon::startAuth(std::move(accountData)),
        [this, accountId](Result<std::string> res) {
          if (res.isOk()) {
            auto token = std::move(res).unwrap();
            log::debug("token obtained: {}", token);
            Mod::get()->setSavedValue("argon_token", token);
            this->continueProfileFetch(accountId);
            return;
          }

          auto err = res.unwrapErr();
          log::warn("Auth failed: {}", err);

          // If account data invalid, interactive auth fallback
          if (err.find("Invalid account data") != std::string::npos) {
            log::info(
                "Falling back to interactive auth due to invalid account data");
            argon::AuthOptions options;
            options.progress = [](argon::AuthProgress progress) {
              log::debug("auth progress: {}",
                         argon::authProgressToString(progress));
            };

            m_fields->m_authTask.spawn(
                argon::startAuth(std::move(options)),
                [this, accountId](Result<std::string> res2) {
                  if (res2.isOk()) {
                    auto token = std::move(res2).unwrap();
                    log::debug("token obtained (fallback): {}", token);
                    Mod::get()->setSavedValue("argon_token", token);
                    this->continueProfileFetch(accountId);
                  } else {
                    log::warn("Interactive auth also failed: {}",
                              res2.unwrapErr());
                    Notification::create(res2.unwrapErr(),
                                         NotificationIcon::Error)
                        ->show();
                    argon::clearToken();
                  }
                });
          } else {
            Notification::create(err, NotificationIcon::Error)->show();
            argon::clearToken();
          }
        });
  }

  void continueProfileFetch(int accountId) {
    std::string token = Mod::get()->getSavedValue<std::string>("argon_token");

    matjson::Value jsonBody = matjson::Value::object();
    jsonBody["argonToken"] = token;
    jsonBody["accountId"] = accountId;

    auto postReq = web::WebRequest();
    postReq.bodyJSON(jsonBody);

    if (m_fields->m_profileInFlight &&
        m_fields->m_profileForAccount == accountId) {
      log::debug("Profile request already in-flight for account {}", accountId);
      return;
    }

    m_fields->m_profileTask.cancel();
    m_fields->m_profileInFlight = true;
    m_fields->m_profileForAccount = accountId;

    Ref<RLProfilePage> pageRef = this;
    m_fields->m_profileTask.spawn(
        postReq.post("https://gdrate.arcticwoof.xyz/profile"),
        [pageRef, accountId](web::WebResponse response) {
          if (pageRef) {
            pageRef->m_fields->m_profileInFlight = false;
          } else {
            return;
          }

          log::info("Received response from server");

          if (!response.ok()) {
            log::warn("Server returned non-ok status: {}", response.code());
            return;
          }

          auto jsonRes = response.json();
          if (!jsonRes) {
            log::warn("Failed to parse JSON response");
            return;
          }

          auto json = jsonRes.unwrap();
          int points = json["points"].asInt().unwrapOrDefault();
          int stars = json["stars"].asInt().unwrapOrDefault();
          int coins = json["coins"].asInt().unwrapOrDefault();
          int role = json["role"].asInt().unwrapOrDefault();
          int planets = json["planets"].asInt().unwrapOrDefault();
          bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();
          bool isBooster = json["isBooster"].asBool().unwrapOrDefault();

          pageRef->m_fields->m_stars = stars;
          pageRef->m_fields->m_planets = planets;
          pageRef->m_fields->m_points = points;
          pageRef->m_fields->m_coins = coins;

          pageRef->m_fields->role = role;
          pageRef->m_fields->isSupporter = isSupporter;
          pageRef->m_fields->isBooster = isBooster;

          if (pageRef->m_ownProfile) {
            Mod::get()->setSavedValue("role", pageRef->m_fields->role);
          }

          // show mod button if mod or admin
          if (Mod::get()->getSavedValue<int>("role") >= 1) {
            if (auto rlButtonsMenu =
                    pageRef->getChildByIDRecursive("rl-buttons-menu")) {
              // no recreate the manage button if it already exists
              if (!rlButtonsMenu->getChildByID("rl-manage-btn")) {
                auto modUserSpr = CCSprite::createWithSpriteFrameName(
                    "RL_badgeMod01.png"_spr);
                auto modUserButton = EditorButtonSprite::create(
                    modUserSpr, EditorBaseColor::LightBlue,
                    EditorBaseSize::Normal);
                auto modUserBtnItem = CCMenuItemSpriteExtra::create(
                    modUserButton, pageRef,
                    menu_selector(RLProfilePage::onUserManage));
                modUserBtnItem->setID("rl-manage-btn");
                rlButtonsMenu->addChild(modUserBtnItem);
                rlButtonsMenu->updateLayout();
              }
            }
          }

          // add badge to the username-menu
          CCMenu *usernameMenu = typeinfo_cast<CCMenu *>(
              pageRef->m_mainLayer->getChildByIDRecursive("username-menu"));
          CCLabelBMFont *usernameLabel = typeinfo_cast<CCLabelBMFont *>(
              usernameMenu->getChildByIDRecursive("username-label"));
          bool hasBadge = false;
          if (usernameMenu) {
            // if user is arcticwoof
            if (pageRef->m_accountID == 7689052) {
              if (!usernameMenu->getChildByID("rl-profile-owner-badge")) {
                auto ownerBadgeSprite = CCSprite::createWithSpriteFrameName(
                    "RL_badgeOwner.png"_spr);
                ownerBadgeSprite->setID("rl-profile-owner-badge");
                usernameMenu->addChild(ownerBadgeSprite);
                hasBadge = true;
              }
              // if user is admin
            } else if (!usernameMenu->getChildByID("rl-profile-admin-badge") &&
                       pageRef->m_fields->role == 2) {
              auto adminBadgeSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeAdmin01.png"_spr);
              adminBadgeSprite->setID("rl-profile-admin-badge");
              usernameMenu->addChild(adminBadgeSprite);
              hasBadge = true;
              // if user is mod
            } else if (!usernameMenu->getChildByID("rl-profile-mod-badge") &&
                       pageRef->m_fields->role == 1) {
              auto modBadgeSprite =
                  CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
              modBadgeSprite->setID("rl-profile-mod-badge");
              usernameMenu->addChild(modBadgeSprite);
              hasBadge = true;
            }
            // if user is supporter
            if (pageRef->m_fields->isSupporter &&
                !usernameMenu->getChildByID("rl-profile-supporter-badge")) {
              auto supporterSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeSupporter.png"_spr);
              supporterSprite->setID("rl-profile-supporter-badge");
              usernameMenu->addChild(supporterSprite);
              hasBadge = true;
            }

            // if user is booster
            if (pageRef->m_fields->isBooster &&
                !usernameMenu->getChildByID("rl-profile-booster-badge")) {
              auto boosterSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeBooster.png"_spr);
              boosterSprite->setID("rl-profile-booster-badge");
              usernameMenu->addChild(boosterSprite);
              hasBadge = true;
            }
          }

          // if the badge exists
          if (hasBadge && usernameMenu) {
            usernameMenu->setPositionX(usernameMenu->getPositionX() - 10.f);
            // shrink the username label to fit the badges
            if (usernameLabel) {
              usernameLabel->setScale(usernameLabel->getScale() * 0.9f);
            }
            usernameMenu->updateLayout();
          }

          pageRef->updateStatLabel(
              "rl-stars-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_stars));
          pageRef->updateStatLabel(
              "rl-planets-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_planets));
          pageRef->updateStatLabel(
              "rl-coins-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_coins));

          // Handle creator points
          if (auto rlStatsMenu =
                  pageRef->getChildByIDRecursive("rl-stats-menu")) {
            if (pageRef->m_fields->m_points > 0) {
              if (!rlStatsMenu->getChildByIDRecursive("rl-points-entry")) {
                auto pointsEntry = pageRef->createStatEntry(
                    "rl-points-entry", "rl-points-label",
                    GameToolbox::pointsToString(pageRef->m_fields->m_points),
                    "RL_blueprintPoint01.png"_spr,
                    menu_selector(RLProfilePage::onLayoutPointsClicked));
                rlStatsMenu->addChild(pointsEntry);
              } else {
                pageRef->updateStatLabel(
                    "rl-points-label",
                    GameToolbox::pointsToString(pageRef->m_fields->m_points));
              }
            } else {
              if (auto creatorPoint =
                      rlStatsMenu->getChildByIDRecursive("rl-points-entry")) {
                creatorPoint->removeFromParent();
              }
            }

            rlStatsMenu->updateLayout();
          }
        });
  }

  void onUserManage(CCObject *sender) {
    int accountId = m_fields->accountId;
    auto userControl = RLUserControl::create(accountId);
    userControl->show();
  }

  void onPlanetsClicked(CCObject *sender) {
    int accountId = m_fields->accountId;
    auto popup = RLDifficultyTotalPopup::create(
        accountId, RLDifficultyTotalPopup::Mode::Planets);
    if (popup)
      popup->show();
  }

  void onLayoutPointsClicked(CCObject *sender) {
    int accountId = m_fields->accountId;
    auto username = GameLevelManager::sharedState()->tryGetUsername(accountId);
    if (username.empty())
      username = "User";
    std::string title = fmt::format("{}'s Layouts", username);

    RLLevelBrowserLayer::ParamList params;
    params.emplace_back("accountId", numToString(accountId));
    auto browserLayer = RLLevelBrowserLayer::create(
        RLLevelBrowserLayer::Mode::Account, params, title);
    auto scene = CCScene::create();
    scene->addChild(browserLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
  }

  void onBlueprintStars(CCObject *sender) {
    int accountId = m_fields->accountId;
    auto popup = RLDifficultyTotalPopup::create(
        accountId, RLDifficultyTotalPopup::Mode::Stars);
    if (popup)
      popup->show();
  }
};
