#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>

#include "../custom/RLAchievements.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include "../player/RLDifficultyTotalPopup.hpp"
#include "../player/RLUserControl.hpp"
#include "../player/RLUserLevelControl.hpp"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/loader/Mod.hpp"
#include "Geode/ui/BasedButtonSprite.hpp"

using namespace geode::prelude;
const int DEV_ACCOUNTID = 7689052;

class $modify(RLProfilePage, ProfilePage) {
  struct Fields {
    int accountId = 0;
    bool isSupporter = false;
    bool isBooster = false;

    int m_points = 0;
    int m_planets = 0;
    int m_stars = 0;
    int m_coins = 0;
    int m_votes = 0;

    // new role flags parsed from profile JSON
    bool isClassicMod = false;
    bool isClassicAdmin = false;
    bool isLeaderboardMod = false;
    bool isPlatMod = false;
    bool isPlatAdmin = false;

    async::TaskHolder<web::WebResponse> m_profileTask;
    async::TaskHolder<Result<std::string>> m_authTask;
    bool m_profileInFlight = false;
    int m_profileForAccount = -1;

    CCMenu *m_rlButtonsMenu = nullptr;
    CCMenu *m_arrowMenu = nullptr;
    NineSlice *m_rlButtonBg = nullptr;
    CCMenu *m_rlStatsMenu = nullptr;
    CCMenuItemSpriteExtra *m_rlToggleArrow = nullptr;
    bool m_rlMenuVisible = false;

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
  
    if (!iconCallback) {
      iconBtn->setEnabled(false);
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
    if (auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu"))
      rlStatsMenu->updateLayout();
    if (auto rlButtonsMenu = getChildByIDRecursive("rl-buttons-menu"))
      rlButtonsMenu->updateLayout();
  }

  void loadPageFromUserInfo(GJUserScore *score) {
    ProfilePage::loadPageFromUserInfo(score);

    // recreate the rl button menu every time we load a profile to ensure it's
    // in a clean state and
    if (m_fields->m_rlButtonBg != nullptr) {
      m_fields->m_rlButtonBg->removeFromParent();
      m_fields->m_arrowMenu->removeFromParent();
      m_fields->m_arrowMenu = nullptr;
      m_fields->m_rlButtonBg = nullptr;
      m_fields->m_rlMenuVisible = false;
      m_fields->m_rlStatsMenu->removeFromParent();
      m_fields->m_rlStatsMenu = nullptr;
    }

    if (!Mod::get()->getSettingValue<bool>("disableRLMenu")) {

      auto winSize = CCDirector::sharedDirector()->getWinSize();
      m_fields->m_rlButtonBg = NineSlice::create("GJ_square02.png");
      m_fields->m_rlButtonBg->setContentSize({42.f, 110.f});
      m_fields->m_rlButtonBg->setPosition(
          {winSize.width + m_fields->m_rlButtonBg->getContentSize().width +
               8.f / 2.f,
           winSize.height / 2.f});
      m_fields->m_rlButtonBg->setID("rl-button-bg");

      m_fields->m_rlButtonsMenu = CCMenu::create();
      m_fields->m_rlButtonsMenu->setID("rl-buttons-menu");
      m_fields->m_rlButtonsMenu->setContentSize({32.f, 100.f});
      m_fields->m_rlButtonsMenu->setPosition(
          {m_fields->m_rlButtonBg->getContentSize().width / 2.f,
           m_fields->m_rlButtonBg->getContentSize().height / 2.f});
      m_fields->m_rlButtonsMenu->setLayout(
          ColumnLayout::create()
              ->setGap(6.f)
              ->setAxisAlignment(AxisAlignment::Center)
              ->setGrowCrossAxis(false));

      m_mainLayer->addChild(m_fields->m_rlButtonBg, 9);
      m_fields->m_rlButtonBg->addChild(m_fields->m_rlButtonsMenu, 1);

      // create toggle arrow button

      auto arrowSpr =
          CCSprite::createWithSpriteFrameName("RL_arrow_01.png"_spr);
      auto arrowBtn = CCMenuItemSpriteExtra::create(
          arrowSpr, this, menu_selector(RLProfilePage::onToggleRLMenu));
      m_fields->m_rlToggleArrow = arrowBtn;
      m_fields->m_arrowMenu = CCMenu::create(arrowBtn, nullptr);
      m_fields->m_arrowMenu->setPosition({0, 0});
      m_fields->m_arrowMenu->setID("rl-toggle-menu");
      m_mainLayer->addChild(m_fields->m_arrowMenu, 10);
      // position at right edge
      float arrowW = arrowSpr->getContentSize().width;
      arrowBtn->setPosition(
          {winSize.width - arrowW / 2 - 4, winSize.height / 2});

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
        m_fields->m_rlButtonsMenu->addChild(rlStatsBtn);
      }

      // if u are leaderboard mod show the manage button to manage your
      // leaderboard entries
      if (Mod::get()->getSavedValue<bool>("isLeaderboardMod") ||
          GJAccountManager::sharedState()->m_accountID == DEV_ACCOUNTID) {
        if (!m_fields->m_rlButtonsMenu->getChildByID("rl-manage-btn")) {
          auto modUserSpr =
              CCSprite::createWithSpriteFrameName("RL_badgelbMod01.png"_spr);
          auto modUserButton = EditorButtonSprite::create(
              modUserSpr, EditorBaseColor::LightBlue, EditorBaseSize::Normal);
          auto modUserBtnItem = CCMenuItemSpriteExtra::create(
              modUserButton, this, menu_selector(RLProfilePage::onUserManage));
          modUserBtnItem->setID("rl-manage-btn");
          m_fields->m_rlButtonsMenu->addChild(modUserBtnItem);
        }
        if (!m_fields->m_rlButtonsMenu->getChildByID("rl-manage-level-btn")) {
          auto manageLevelSpr =
              CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
          auto manageLevelButton = EditorButtonSprite::create(
              manageLevelSpr, EditorBaseColor::LightBlue,
              EditorBaseSize::Normal);
          auto manageLevelBtnItem = CCMenuItemSpriteExtra::create(
              manageLevelButton, this,
              menu_selector(RLProfilePage::onUserManageLevel));
          manageLevelBtnItem->setID("rl-manage-level-btn");
          m_fields->m_rlButtonsMenu->addChild(manageLevelBtnItem);
        }
      }
    }

    auto statsMenu = m_mainLayer->getChildByID("stats-menu");
    if (!statsMenu) {
      log::warn("stats-menu not found");
      return;
    }

    m_fields->m_rlStatsMenu = CCMenu::create();
    m_fields->m_rlStatsMenu->setID("rl-stats-menu");
    m_fields->m_rlStatsMenu->setContentSize(statsMenu->getContentSize());

    auto row = RowLayout::create();
    row->setAxisAlignment(AxisAlignment::Center);
    row->setCrossAxisAlignment(AxisAlignment::Center);
    row->setGap(4.f);
    m_fields->m_rlStatsMenu->setLayout(row);

    m_fields->m_rlStatsMenu->setAnchorPoint({0.5f, 0.5f});

    m_fields->m_rlStatsMenu->setPositionY(245.f);

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
    auto votesEntry =
        createStatEntry("rl-votes-entry", "rl-votes-label",
                        GameToolbox::pointsToString(m_fields->m_votes),
                        "RL_commVote01.png"_spr, nullptr);

    m_fields->m_rlStatsMenu->addChild(starsEntry);
    m_fields->m_rlStatsMenu->addChild(planetsEntry);
    m_fields->m_rlStatsMenu->addChild(coinsEntry);
    m_fields->m_rlStatsMenu->addChild(votesEntry);

    if (m_fields->m_points > 0) {
      auto pointsEntry =
          createStatEntry("rl-points-entry", "rl-points-label",
                          GameToolbox::pointsToString(m_fields->m_points),
                          "RL_blueprintPoint01.png"_spr,
                          menu_selector(RLProfilePage::onLayoutPointsClicked));
      m_fields->m_rlStatsMenu->addChild(pointsEntry);
    }

    m_fields->m_rlStatsMenu->setVisible(false);
    statsMenu->setVisible(true);

    m_mainLayer->addChild(m_fields->m_rlStatsMenu);

    if (score) {
      this->fetchProfileData(score->m_accountID);
    }

    if (auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu"))
      rlStatsMenu->updateLayout();

    statsMenu->updateLayout();
    if (m_fields->m_rlButtonsMenu) {
      m_fields->m_rlButtonsMenu->updateLayout();
    }
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

  void onToggleRLMenu(CCObject *sender) {
    if (!m_fields->m_rlButtonBg)
      return;
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    float bgW = m_fields->m_rlButtonBg->getContentSize().width + 8.f;
    float arrowW = 0.f;
    if (m_fields->m_rlToggleArrow) {
      arrowW =
          m_fields->m_rlToggleArrow->getNormalImage()->getContentSize().width;
    }

    bool showing = m_fields->m_rlMenuVisible;
    float targetBgX =
        showing ? winSize.width + bgW / 2 : winSize.width - bgW / 2;
    float targetArrowX = showing ? (winSize.width - arrowW / 2 - 4)
                                 : (winSize.width - bgW - arrowW / 2 - 8);

    auto moveBg = CCEaseBackOut::create(
        CCMoveTo::create(0.3f, {targetBgX, winSize.height / 2}));
    m_fields->m_rlButtonBg->runAction(moveBg);
    if (m_fields->m_rlToggleArrow) {
      auto moveArrow = CCEaseBackOut::create(
          CCMoveTo::create(0.3f, {targetArrowX, winSize.height / 2}));
      m_fields->m_rlToggleArrow->runAction(moveArrow);
      if (auto sprite = static_cast<CCSprite *>(
              m_fields->m_rlToggleArrow->getNormalImage())) {
        sprite->setFlipX(!showing);
      }
    }
    m_fields->m_rlMenuVisible = !showing;
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
            log::warn("{}: user doesn't exists in rated layouts",
                      response.code());

            if (pageRef->m_fields->m_rlToggleArrow) {
              pageRef->m_fields->m_rlToggleArrow->setEnabled(false);
              pageRef->m_fields->m_rlToggleArrow->setOpacity(100);
            }

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
          int planets = json["planets"].asInt().unwrapOrDefault();
          int votes = json["votes"].asInt().unwrapOrDefault();
          bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();
          bool isBooster = json["isBooster"].asBool().unwrapOrDefault();
          // new flags
          bool isClassicMod = json["isClassicMod"].asBool().unwrapOrDefault();
          bool isClassicAdmin =
              json["isClassicAdmin"].asBool().unwrapOrDefault();
          bool isLeaderboardMod =
              json["isLeaderboardMod"].asBool().unwrapOrDefault();
          bool isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
          bool isPlatAdmin = json["isPlatAdmin"].asBool().unwrapOrDefault();

          pageRef->m_fields->m_stars = stars;
          pageRef->m_fields->m_planets = planets;
          pageRef->m_fields->m_points = points;
          pageRef->m_fields->m_coins = coins;
          pageRef->m_fields->m_votes = votes;

          pageRef->m_fields->isSupporter = isSupporter;
          pageRef->m_fields->isBooster = isBooster;

          pageRef->m_fields->isClassicMod = isClassicMod;
          pageRef->m_fields->isClassicAdmin = isClassicAdmin;
          pageRef->m_fields->isLeaderboardMod = isLeaderboardMod;
          pageRef->m_fields->isPlatMod = isPlatMod;
          pageRef->m_fields->isPlatAdmin = isPlatAdmin;

          if (!Mod::get()->getSettingValue<bool>("disableRLMenu")) {
            // show mod button if leaderboard mod or dev
            if (Mod::get()->getSavedValue<bool>("isLeaderboardMod") ||
                Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
                Mod::get()->getSavedValue<bool>("isPlatAdmin") ||
                GJAccountManager::sharedState()->m_accountID == DEV_ACCOUNTID) {
              if (auto rlButtonsMenu =
                      pageRef->getChildByIDRecursive("rl-buttons-menu")) {
                // no recreate the manage button if it already exists
                if (!rlButtonsMenu->getChildByID("rl-manage-btn")) {
                  auto modUserSpr = CCSprite::createWithSpriteFrameName(
                      "RL_badgelbMod01.png"_spr);
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
                if (!rlButtonsMenu->getChildByID("rl-manage-level-btn") &&
                    Mod::get()->getSavedValue<bool>("isLeaderboardMod")) {
                  auto manageLevelSpr = CCSprite::createWithSpriteFrameName(
                      "RL_badgeMod01.png"_spr);
                  auto manageLevelButton = EditorButtonSprite::create(
                      manageLevelSpr, EditorBaseColor::LightBlue,
                      EditorBaseSize::Normal);
                  auto manageLevelBtnItem = CCMenuItemSpriteExtra::create(
                      manageLevelButton, pageRef,
                      menu_selector(RLProfilePage::onUserManageLevel));
                  manageLevelBtnItem->setID("rl-manage-level-btn");
                  rlButtonsMenu->addChild(manageLevelBtnItem);
                  rlButtonsMenu->updateLayout();
                }
              }
            }
          }

          // add badge to the username-menu
          CCMenu *usernameMenu = static_cast<CCMenu *>(
              pageRef->m_mainLayer->getChildByIDRecursive("username-menu"));
          if (usernameMenu) {
            auto addBadgeItem = [&](CCSprite *sprite, int tag, const char *id) {
              if (!sprite)
                return;
              auto btn = CCMenuItemSpriteExtra::create(
                  sprite, pageRef,
                  menu_selector(RLProfilePage::onBadgeClicked));
              btn->setTag(tag);
              btn->setID(id);
              usernameMenu->addChild(btn);
            };

            if (!usernameMenu->getChildByID(
                    "rl-profile-classic-admin-badge:2") &&
                pageRef->m_fields->isClassicAdmin) {
              auto adminBadgeSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeAdmin01.png"_spr);
              addBadgeItem(adminBadgeSprite, 5,
                           "rl-profile-classic-admin-badge:2");
            }
            if (!usernameMenu->getChildByID("rl-profile-classic-mod-badge:3") &&
                pageRef->m_fields->isClassicMod) {
              auto modBadgeSprite =
                  CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
              addBadgeItem(modBadgeSprite, 6, "rl-profile-classic-mod-badge:3");
            }
            if (!usernameMenu->getChildByID("rl-profile-lb-mod-badge:3") &&
                pageRef->m_fields->isLeaderboardMod) {
              auto modBadgeSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgelbMod01.png"_spr);
              addBadgeItem(modBadgeSprite, 9, "rl-profile-lb-mod-badge:3");
            }
            if (!usernameMenu->getChildByID("rl-profile-plat-admin-badge:2") &&
                pageRef->m_fields->isPlatAdmin) {
              auto adminBadgeSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgePlatAdmin01.png"_spr);
              addBadgeItem(adminBadgeSprite, 7,
                           "rl-profile-plat-admin-badge:2");
            }
            if (!usernameMenu->getChildByID("rl-profile-plat-mod-badge:3") &&
                pageRef->m_fields->isPlatMod) {
              auto modBadgeSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgePlatMod01.png"_spr);
              addBadgeItem(modBadgeSprite, 8, "rl-profile-plat-mod-badge:3");
            }
            // if user is supporter
            if (pageRef->m_fields->isSupporter &&
                !usernameMenu->getChildByID("rl-profile-supporter-badge:4")) {
              auto supporterSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeSupporter.png"_spr);
              addBadgeItem(supporterSprite, 3, "rl-profile-supporter-badge:4");
            }

            // if user is booster
            if (pageRef->m_fields->isBooster &&
                !usernameMenu->getChildByID("rl-profile-booster-badge:4")) {
              auto boosterSprite = CCSprite::createWithSpriteFrameName(
                  "RL_badgeBooster.png"_spr);
              addBadgeItem(boosterSprite, 4, "rl-profile-booster-badge:4");
            }

            // if user is arcticwoof
            if (pageRef->m_accountID == 7689052) {
              if (!usernameMenu->getChildByID("rl-profile-owner-badge:1")) {
                auto ownerBadgeSprite = CCSprite::createWithSpriteFrameName(
                    "RL_badgeOwner.png"_spr);
                addBadgeItem(ownerBadgeSprite, 10, "rl-profile-owner-badge:1");
              }
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
          pageRef->updateStatLabel(
              "rl-votes-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_votes));

          // If this is the player's own profile, check achievements for Sparks
          // and Planets
          if (pageRef->m_ownProfile) {
            log::debug(
                "checking Sparks/Planets achievements (stars={}, planets={})",
                pageRef->m_fields->m_stars, pageRef->m_fields->m_planets);
            RLAchievements::checkAll(RLAchievements::Collectable::Sparks,
                                     pageRef->m_fields->m_stars);
            RLAchievements::checkAll(RLAchievements::Collectable::Planets,
                                     pageRef->m_fields->m_planets);
            RLAchievements::checkAll(RLAchievements::Collectable::Points,
                                     pageRef->m_fields->m_points);
            RLAchievements::checkAll(RLAchievements::Collectable::Coins,
                                     pageRef->m_fields->m_coins);
            RLAchievements::checkAll(RLAchievements::Collectable::Votes,
                                     pageRef->m_fields->m_votes);
          }

          // Handle creator points
          if (auto rlStatsMenu =
                  pageRef->getChildByIDRecursive("rl-stats-menu")) {
            if (pageRef->m_fields->m_points > 0 &&
                !Mod::get()->getSettingValue<bool>("disableCreatorPoints")) {
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

  void onBadgeClicked(CCObject *sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra *>(sender);
    if (!btn)
      return;
    int tag = btn->getTag();
    switch (tag) {
    case 3: // Supporters
      FLAlertLayer::create(
          "Layout Supporter",
          "<cp>Layout Supporter</c> are those who have supported development "
          "of "
          "<cl>Rated "
          "Layouts</c> through <cp>Ko-fi</c> membership donation.",
          "OK")
          ->show();
      break;
    case 4: // Boosters
      FLAlertLayer::create(
          "Layout Booster",
          "<ca>Layout Booster</c> are those who boosted the <cl>Rated "
          "Layouts Discord server</c>, they also have the same "
          "benefits as <cp>Layout Supporter</c>.",
          "OK")
          ->show();
      break;
    case 5: // Classic Admins
      FLAlertLayer::create("Classic Layout Admin",
                           "<cr>Classic Layout Admin</c> has the ability to "
                           "rate, suggest levels "
                           "and <cg>manage Featured Layouts</c> for "
                           "<cc>classic levels</c> of <cl>Rated Layouts</c>.",
                           "OK")
          ->show();
      break;
    case 6: // Classic Mods
      FLAlertLayer::create(
          "Classic Layout Mod",
          "<cb>Classic Layout Moderator</c> can suggest levels "
          "for classic layouts to <cr>Classic Layout Admins</c>.",
          "OK")
          ->show();
      break;
    case 7: // Plat Admins
      FLAlertLayer::create(
          "Platformer Layout Admin",
          "<cr>Platformer Layout Admin</c> has the abilities to rate, suggest "
          "levels and <cg>manage Featured Layouts</c> for "
          "<cc>platformer levels</c> of <cl>Rated Layouts</c>.",
          "OK")
          ->show();
      break;
    case 8: // Plat Mods
      FLAlertLayer::create(
          "Platformer Layout Mod",
          "<cb>Platformer Layout Moderator</c> can suggest levels for "
          "<cc>platformer layouts</c> to <cr>Platformer Layout Admins</c>.",
          "OK")
          ->show();
      break;
    case 9: // Leaderboard Mods
      FLAlertLayer::create(
          "LB Layout Mod",
          "<cb>Leaderboard Layout Moderator</c> is responsible for "
          "<co>managing and "
          "moderating the leaderboard</c> section of <cl>Rated Layouts</c>.",
          "OK")
          ->show();
      break;
    case 10: // Owner
      FLAlertLayer::create(
          "Rated Layouts Owner",
          "<cf>ArcticWoof</c> is the creator and owner of "
          "<cl>Rated Layouts</c>. He is the main developer and maintainer of "
          "this <cp>Geode Mod</c> and has the ability to <cg>promote "
          "users</c>.",
          "OK")
          ->show();
      break;
    default:
      break;
    }
  }

  void onUserManage(CCObject *sender) {
    // only leaderboard moderators may manage users
    if (!Mod::get()->getSavedValue<bool>("isLeaderboardMod") &&
        GJAccountManager::sharedState()->m_accountID != DEV_ACCOUNTID) {
      Notification::create("You don't have permission to manage users.",
                           NotificationIcon::Error)
          ->show();
      return;
    }
    int accountId = m_fields->accountId;
    auto userControl = RLUserControl::create(accountId);
    userControl->show();
  }

  void onUserManageLevel(CCObject *sender) {
    // only leaderboard moderators may manage levels
    if (!Mod::get()->getSavedValue<bool>("isLeaderboardMod") &&
        GJAccountManager::sharedState()->m_accountID != DEV_ACCOUNTID) {
      Notification::create("You don't have permission to manage levels.",
                           NotificationIcon::Error)
          ->show();
      return;
    }
    int accountId = m_fields->accountId;
    auto levelControl = RLUserLevelControl::create(accountId);
    levelControl->show();
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
