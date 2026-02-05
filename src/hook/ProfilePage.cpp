#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>

#include "../custom/RLAchievements.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include "../player/RLDifficultyTotalPopup.hpp"
#include "../player/RLUserControl.hpp"
#include "BadgesAPI.hpp"

using namespace geode::prelude;

static std::unordered_map<int, std::vector<Badge>> g_pendingBadges;

$execute {
  BadgesAPI::registerBadge(
      "rl-mod-badge", "Rated Layouts Moderator",
      "This user can <cj>suggest layout levels</c> for <cl>Rated "
      "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to "
      "<co>moderate the leaderboard</c>.",
      [] {
        return CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
      },
      [](const Badge &badge, const UserInfo &user) {
        g_pendingBadges[user.accountID].push_back(badge);
      });
  BadgesAPI::registerBadge(
      "rl-admin-badge", "Rated Layouts Admin",
      "This user can <cj>rate layout levels</c> for <cl>Rated "
      "Layouts</c>. They have the same power as <cg>Moderators</c> but "
      "including the ability to change the <cy>featured ranking on the "
      "featured layout levels</c> and <cg>set event layouts</c>.",
      [] {
        return CCSprite::createWithSpriteFrameName("RL_badgeAdmin01.png"_spr);
      },
      [](const Badge &badge, const UserInfo &user) {
        g_pendingBadges[user.accountID].push_back(badge);
      });
  BadgesAPI::registerBadge(
      "rl-owner-badge", "Rated Layouts Owner",
      "<cf>ArcticWoof</c> is the <ca>Owner and Developer</c> of <cl>Rated "
      "Layouts</c> Geode Mod.\nHe controls and manages everything within "
      "<cl>Rated Layouts</c>, including updates and adding new features as "
      "well as the ability to <cg>promote users to Layout Moderators or "
      "Administrators</c>.",
      [] {
        return CCSprite::createWithSpriteFrameName("RL_badgeOwner.png"_spr);
      },
      [](const Badge &badge, const UserInfo &user) {
        g_pendingBadges[user.accountID].push_back(badge);
      });
  BadgesAPI::registerBadge(
      "rl-supporter-badge", "Rated Layouts Supporter",
      "This user is a <cp>Layout Supporter</c>! They have supported the "
      "development of <cl>Rated Layouts</c> through membership "
      "donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via "
      "<cp>Ko-Fi</c>",
      [] {
        return CCSprite::createWithSpriteFrameName("RL_badgeSupporter.png"_spr);
      },
      [](const Badge &badge, const UserInfo &user) {
        g_pendingBadges[user.accountID].push_back(badge);
      });
}

class $modify(RLProfilePage, ProfilePage) {
  struct Fields {
    int role = 0;
    int accountId = 0;
    bool isSupporter = false;

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
    if (auto leftMenu = getChildByIDRecursive("left-menu"))
      leftMenu->updateLayout();
  }

  bool init(int accountID, bool ownProfile) {
    if (!ProfilePage::init(accountID, ownProfile))
      return false;

    if (auto statsMenu = m_mainLayer->getChildByID("stats-menu")) {
      statsMenu->updateLayout();
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

    auto leftMenu = getChildByIDRecursive("left-menu");
    if (!leftMenu) {
      log::warn("left-menu not found");
      return;
    }

    auto rlStatsSpr = CCSprite::create("GJ_button_04.png");
    auto rlStatsSprOn = CCSprite::create("GJ_button_02.png");

    auto rlSprA = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
    auto rlSprB = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
    rlSprA->setPosition({20.f, 20.f});
    rlSprB->setPosition({20.f, 20.f});

    rlStatsSpr->addChild(rlSprA);
    rlStatsSprOn->addChild(rlSprB);

    rlStatsSpr->setScale(0.8f);
    rlStatsSprOn->setScale(0.8f);

    auto rlStatsBtn = CCMenuItemToggler::create(
        rlStatsSpr, rlStatsSprOn, this,
        menu_selector(RLProfilePage::onStatsSwitcher));
    rlStatsBtn->setID("rl-stats-btn");
    leftMenu->addChild(rlStatsBtn);

    auto rlStatsMenu = CCMenu::create();
    rlStatsMenu->setID("rl-stats-menu");
    rlStatsMenu->setContentSize({200.f, 20.f});

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

    rlStatsMenu->addChild(starsEntry);
    rlStatsMenu->addChild(planetsEntry);

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
    leftMenu->updateLayout();
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

          pageRef->m_fields->m_stars = stars;
          pageRef->m_fields->m_planets = planets;
          pageRef->m_fields->m_points = points;
          pageRef->m_fields->m_coins = coins;

          pageRef->m_fields->role = role;
          pageRef->m_fields->isSupporter = isSupporter;

          if (isSupporter && pageRef->m_ownProfile) {
            RLAchievements::onReward("misc_support");
          }

          if (pageRef->m_ownProfile) {
            Mod::get()->setSavedValue("role", pageRef->m_fields->role);
          }

          pageRef->updateStatLabel(
              "rl-stars-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_stars));
          pageRef->updateStatLabel(
              "rl-planets-label",
              GameToolbox::pointsToString(pageRef->m_fields->m_planets));

          // Handle creator points entry
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
