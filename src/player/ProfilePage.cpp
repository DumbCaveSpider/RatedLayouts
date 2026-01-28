#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/coro.hpp>
#include <argon/argon.hpp>

#include "BadgesAPI.hpp"
#include "RLDifficultyTotalPopup.hpp"
#include "RLUserControl.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include "../custom/RLAchievements.hpp"

using namespace geode::prelude;

static std::unordered_map<int, std::vector<Badge>> g_pendingBadges;

$execute {
      BadgesAPI::registerBadge(
          "rl-mod-badge",
          "Rated Layouts Moderator",
          "This user can <cj>suggest layout levels</c> for <cl>Rated "
          "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to <co>moderate the leaderboard</c>.",
          [] {
                return CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
          },
          [](const Badge& badge, const UserInfo& user) {
                g_pendingBadges[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-admin-badge",
          "Rated Layouts Admin",
          "This user can <cj>rate layout levels</c> for <cl>Rated "
          "Layouts</c>. They have the same power as <cg>Moderators</c> but including the ability to change the <cy>featured ranking on the "
          "featured layout levels</c> and <cg>set event layouts</c>.",
          [] {
                return CCSprite::createWithSpriteFrameName("RL_badgeAdmin01.png"_spr);
          },
          [](const Badge& badge, const UserInfo& user) {
                g_pendingBadges[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-owner-badge",
          "Rated Layouts Owner",
          "<cf>ArcticWoof</c> is the <ca>Owner and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
          [] {
                return CCSprite::createWithSpriteFrameName("RL_badgeOwner.png"_spr);
          },
          [](const Badge& badge, const UserInfo& user) {
                g_pendingBadges[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-supporter-badge",
          "Rated Layouts Supporter",
          "This user is a <cp>Layout Supporter</c>! They have supported the development of <cl>Rated Layouts</c> through membership donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via <cp>Ko-Fi</c>",
          [] {
                return CCSprite::createWithSpriteFrameName("RL_badgeSupporter.png"_spr);
          },
          [](const Badge& badge, const UserInfo& user) {
                g_pendingBadges[user.accountID].push_back(badge);
          });
}

static std::string getUserRoleCachePath_ProfilePage() {
      auto saveDir = dirs::getModsSaveDir();
      return geode::utils::string::pathToString(saveDir / "user_role_cache.json");
}

static void cacheUserProfile_ProfilePage(int accountId, int role, int stars, int planets, int coins) {
      auto saveDir = dirs::getModsSaveDir();
      auto createDirResult = utils::file::createDirectory(saveDir);
      if (!createDirResult) {
            log::warn("Failed to create save directory for user role cache");
            return;
      }

      auto cachePath = getUserRoleCachePath_ProfilePage();

      matjson::Value root = matjson::Value::object();
      auto existingData = utils::file::readString(cachePath);
      if (existingData) {
            auto parsed = matjson::parse(existingData.unwrap());
            if (parsed)
                  root = parsed.unwrap();
      }

      matjson::Value obj = matjson::Value::object();
      obj["role"] = role;
      obj["stars"] = stars;
      obj["planets"] = planets;
      obj["coins"] = coins;
      root[fmt::format("{}", accountId)] = obj;

      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(geode::utils::string::pathToString(cachePath), jsonString);
      if (writeResult) {
            log::debug("Cached user role {} for account ID: {} (from ProfilePage)", role, accountId);
      }
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
      };

      CCMenu* createStatEntry(
          char const* entryID,
          char const* labelID,
          std::string const& text,
          char const* iconFrameOrPath,
          SEL_MenuHandler iconCallback) {
            auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
            label->setID(labelID);

            constexpr float kLabelScale = 0.60f;
            constexpr float kMaxLabelW = 58.f;
            constexpr float kMinScale = 0.20f;

            label->setScale(kLabelScale);
            label->limitLabelWidth(kMaxLabelW, kLabelScale, kMinScale);

            CCSprite* iconSprite = nullptr;
            iconSprite = CCSprite::createWithSpriteFrameName(iconFrameOrPath);
            if (!iconSprite)
                  iconSprite = CCSprite::createWithSpriteFrameName(iconFrameOrPath);

            iconSprite->setScale(0.8f);

            auto iconBtn = CCMenuItemSpriteExtra::create(
                iconSprite,
                this,
                iconCallback);

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

      void updateStatLabel(char const* labelID, std::string const& text) {
            auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
            if (!rlStatsMenu)
                  return;

            auto label = typeinfo_cast<CCLabelBMFont*>(rlStatsMenu->getChildByIDRecursive(labelID));
            if (!label)
                  return;

            label->setString(text.c_str());

            constexpr float kLabelScale = 0.60f;
            constexpr float kMaxLabelW = 58.f;
            constexpr float kMinScale = 0.20f;
            label->limitLabelWidth(kMaxLabelW, kLabelScale, kMinScale);
      }

      bool init(int accountID, bool ownProfile) {
            if (!ProfilePage::init(accountID, ownProfile))
                  return false;

            if (auto statsMenu = m_mainLayer->getChildByID("stats-menu")) {
                  statsMenu->updateLayout();
            }
            return true;
      }

      void loadPageFromUserInfo(GJUserScore* score) {
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
                rlStatsSpr,
                rlStatsSprOn,
                this,
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
                "rl-stars-entry",
                "rl-stars-label",
                starsText,
                "RL_starMed.png"_spr,
                menu_selector(RLProfilePage::onBlueprintStars));

            auto planetsEntry = createStatEntry(
                "rl-planets-entry",
                "rl-planets-label",
                planetsText,
                "RL_planetMed.png"_spr,
                menu_selector(RLProfilePage::onPlanetsClicked));

            rlStatsMenu->addChild(starsEntry);
            rlStatsMenu->addChild(planetsEntry);

            rlStatsMenu->setVisible(false);
            statsMenu->setVisible(true);

            m_mainLayer->addChild(rlStatsMenu);

            if (score) {
                  coro::spawn << fetchProfileDataTask(score->m_accountID);
            }

            rlStatsMenu->updateLayout();

            statsMenu->updateLayout();
            leftMenu->updateLayout();
      }

      void onStatsSwitcher(CCObject* sender) {
            auto statsMenu = getChildByIDRecursive("stats-menu");
            auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
            auto switcher = typeinfo_cast<CCMenuItemToggler*>(sender);

            if (!statsMenu || !rlStatsMenu || !switcher)
                  return;

            if (!switcher->isToggled()) {
                  statsMenu->setVisible(false);
                  if (auto m = typeinfo_cast<CCMenu*>(statsMenu))
                        m->setEnabled(false);

                  rlStatsMenu->setVisible(true);
                  if (auto m = typeinfo_cast<CCMenu*>(rlStatsMenu))
                        m->setEnabled(true);
            } else {
                  statsMenu->setVisible(true);
                  if (auto m = typeinfo_cast<CCMenu*>(statsMenu))
                        m->setEnabled(true);

                  rlStatsMenu->setVisible(false);
                  if (auto m = typeinfo_cast<CCMenu*>(rlStatsMenu))
                        m->setEnabled(false);
            }
      }

      geode::Task<void> fetchProfileDataTask(int accountId) {
            log::info("Fetching profile data for account ID: {}", accountId);
            m_fields->accountId = accountId;

            std::string token;
            auto res = argon::startAuth(
                [](Result<std::string> res) {
                      if (!res) {
                            log::warn("Auth failed: {}", res.unwrapErr());
                            return;
                      }
                      auto token = std::move(res).unwrap();
                      log::debug("token obtained: {}", token);
                      Mod::get()->setSavedValue("argon_token", token);
                },
                [](argon::AuthProgress progress) {
                      log::debug("auth progress: {}", argon::authProgressToString(progress));
                });
            if (!res) {
                  log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                  co_return;
            }

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = accountId;

            auto req = web::WebRequest();
            req.bodyJSON(jsonBody);

            auto response = co_await req.post("https://gdrate.arcticwoof.xyz/profile");
            if (!response.ok()) {
                  log::warn("Server returned non-ok status");
                  co_return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  co_return;
            }

            auto json = jsonRes.unwrap();
            int points = json["points"].asInt().unwrapOrDefault();
            int stars = json["stars"].asInt().unwrapOrDefault();
            int role = json["role"].asInt().unwrapOrDefault();
            int coins = json["coins"].asInt().unwrapOrDefault();
            int planets = json["planets"].asInt().unwrapOrDefault();
            bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();

            if (m_ownProfile) {
                  // save current totals for comparison
                  int newPoints = points;
                  int newStars = stars;
                  int newPlanets = planets;
                  int newCoins = coins;

                  int oldPoints = Mod::get()->getSavedValue<int>("points", 0);
                  int oldStars = Mod::get()->getSavedValue<int>("stars", 0);
                  int oldPlanets = Mod::get()->getSavedValue<int>("planets", 0);
                  int oldCoins = Mod::get()->getSavedValue<int>("coins", 0);

                  if (points > oldPoints) {
                        RLAchievements::onUpdated(RLAchievements::Collectable::Points, oldPoints, points);
                  }
                  if (stars > oldStars) {
                        RLAchievements::onUpdated(RLAchievements::Collectable::Sparks, oldStars, stars);
                  }
                  if (planets > oldPlanets) {
                        RLAchievements::onUpdated(RLAchievements::Collectable::Planets, oldPlanets, planets);
                  }
                  if (coins > oldCoins) {
                        RLAchievements::onUpdated(RLAchievements::Collectable::Coins, oldCoins, coins);
                  }

                  // Also check current totals for any retroactive awards
                  RLAchievements::checkAll(RLAchievements::Collectable::Points, points);
                  RLAchievements::checkAll(RLAchievements::Collectable::Sparks, stars);
                  RLAchievements::checkAll(RLAchievements::Collectable::Planets, planets);
                  RLAchievements::checkAll(RLAchievements::Collectable::Coins, coins);

                  // Persist the new totals so future fetches are compared correctly
                  Mod::get()->setSavedValue<int>("points", points);
                  Mod::get()->setSavedValue<int>("stars", stars);
                  Mod::get()->setSavedValue<int>("planets", planets);
                  Mod::get()->setSavedValue<int>("coins", coins);
            }

            m_fields->m_stars = stars;
            m_fields->m_planets = planets;
            m_fields->m_points = points;
            m_fields->m_coins = coins;

            m_fields->role = role;
            m_fields->isSupporter = isSupporter;

            auto it = g_pendingBadges.find(accountId);
            if (it != g_pendingBadges.end()) {
                  for (auto const& b : it->second) {
                        if (!b.targetNode || !b.targetNode->getParent())
                              continue;

                        if (b.badgeID == "rl-owner-badge") {
                              if (accountId == 7689052)
                                    BadgesAPI::showBadge(b);
                        } else if (b.badgeID == "rl-mod-badge") {
                              if (accountId != 7689052 && role == 1)
                                    BadgesAPI::showBadge(b);
                        } else if (b.badgeID == "rl-admin-badge") {
                              if (accountId != 7689052 && role == 2)
                                    BadgesAPI::showBadge(b);
                        } else if (b.badgeID == "rl-supporter-badge") {
                              if (isSupporter)
                                    BadgesAPI::showBadge(b);
                        }
                  }
                  g_pendingBadges.erase(it);
            }

            log::info("Profile data - points: {}, stars: {}, planets: {}, supporter: {}", points, stars, planets, isSupporter);

            cacheUserProfile_ProfilePage(m_fields->accountId, role, stars, planets, coins);

            if (m_ownProfile) {
                  Mod::get()->setSavedValue("role", m_fields->role);
            }

            updateStatLabel("rl-stars-label", GameToolbox::pointsToString(m_fields->m_stars));
            updateStatLabel("rl-planets-label", GameToolbox::pointsToString(m_fields->m_planets));

            auto pointsText = GameToolbox::pointsToString(m_fields->m_points);

            auto pointsEntry = createStatEntry(
                "rl-points-entry",
                "rl-points-label",
                pointsText,
                "RL_blueprintPoint01.png"_spr,
                menu_selector(RLProfilePage::onLayoutPointsClicked));

            auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");

            if (points > 0) {
                  rlStatsMenu->addChild(pointsEntry);
            }

            if (rlStatsMenu) {
                  for (auto entryID : {"rl-stars-entry", "rl-planets-entry", "rl-points-entry"}) {
                        auto entry = typeinfo_cast<CCMenu*>(rlStatsMenu->getChildByIDRecursive(entryID));
                        if (!entry)
                              continue;

                        auto label = typeinfo_cast<CCLabelBMFont*>(entry->getChildByIDRecursive(
                            std::string(entryID) == "rl-stars-entry" ? "rl-stars-label" : std::string(entryID) == "rl-planets-entry" ? "rl-planets-label"
                                                                                                                                     : "rl-points-label"));
                        auto iconBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(entry->getChildren()->objectAtIndex(1));

                        if (!label || !iconBtn)
                              continue;

                        auto ls = label->getScaledContentSize();
                        auto is = iconBtn->getScaledContentSize();

                        constexpr float gap = 2.f;
                        constexpr float pad = 2.f;

                        float h = std::max(ls.height, is.height);
                        float w = pad + ls.width + gap + is.width + pad;

                        entry->setContentSize({w, h});

                        label->setPosition({pad, h / 2.f});
                        iconBtn->setPosition({pad + ls.width + gap, h / 2.f});
                  }

                  auto betterProgSign = getChildByIDRecursive("itzkiba.better_progression/tier-bar");
                  if (betterProgSign) {
                        rlStatsMenu->setScale(0.845f);
                        rlStatsMenu->setPosition({309.f, 248.f});
                  }

                  rlStatsMenu->updateLayout();
            }

            // add a user manage button if the user accessing it is a mod or an admin
            if (Mod::get()->getSavedValue<int>("role", 0) >= 1) {
                  auto leftMenu = static_cast<CCMenu*>(
                      m_mainLayer->getChildByIDRecursive("left-menu"));
                  if (leftMenu && !leftMenu->getChildByID("rl-user-manage")) {
                        auto hammerSprite = CCSprite::createWithSpriteFrameName("RL_blueprintPoint01.png"_spr);
                        auto circleButtonSprite = CircleButtonSprite::create(
                            hammerSprite, CircleBaseColor::DarkAqua, CircleBaseSize::Small);
                        circleButtonSprite->setScale(0.875f);
                        auto userButton = CCMenuItemSpriteExtra::create(
                            circleButtonSprite, this, menu_selector(RLProfilePage::onUserManage));
                        userButton->setID("rl-user-manage");
                        leftMenu->addChild(userButton);
                        leftMenu->updateLayout();
                  }
            }

            // only set saved data if you own the profile
            if (m_ownProfile) {
                  Mod::get()->setSavedValue("role", m_fields->role);
            }

            co_return;
      }

      void fetchProfileData(int accountId) {
            log::info("Fetching profile data for account ID: {}", accountId);
            m_fields->accountId = accountId;

            // argon my beloved <3
            std::string token;
            auto res = argon::startAuth(
                [](Result<std::string> res) {
                      if (!res) {
                            log::warn("Auth failed: {}", res.unwrapErr());
                            return;
                      }
                      auto token = std::move(res).unwrap();
                      log::debug("token obtained: {}", token);
                      Mod::get()->setSavedValue("argon_token", token);
                },
                [](argon::AuthProgress progress) {
                      log::debug("auth progress: {}", argon::authProgressToString(progress));
                });
            if (!res) {
                  log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                  return;
            }

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = accountId;

            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);

            auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");
            Ref<RLProfilePage> pageRef = this;

            postTask.listen([pageRef, this](web::WebResponse* response) {
            log::info("Received response from server");

            if (!pageRef || !pageRef->m_mainLayer) {
                log::warn("ProfilePage has been destroyed, skipping profile data update");
                return;
            }

            if (!response->ok()) {
                log::warn("Server returned non-ok status: {}", response->code());
                return;
            }

            auto jsonRes = response->json();
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

            m_fields->m_stars = stars;
            m_fields->m_planets = planets;
            m_fields->m_points = points;
            m_fields->m_coins = coins;

            pageRef->m_fields->role = role;
            pageRef->m_fields->isSupporter = isSupporter;

            cacheUserProfile_ProfilePage(pageRef->m_fields->accountId, role, stars, planets, coins);

            if (pageRef->m_ownProfile) {
                Mod::get()->setSavedValue("role", pageRef->m_fields->role);
            }

            updateStatLabel("rl-stars-label", GameToolbox::pointsToString(m_fields->m_stars));
            updateStatLabel("rl-planets-label", GameToolbox::pointsToString(m_fields->m_planets));

            if (auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu")) {
                rlStatsMenu->updateLayout();
            } });
      }

      void onUserManage(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto userControl = RLUserControl::create(accountId);
            userControl->show();
      }

      void onPlanetsClicked(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto popup = RLDifficultyTotalPopup::create(accountId, RLDifficultyTotalPopup::Mode::Planets);
            if (popup)
                  popup->show();
      }

      void onLayoutPointsClicked(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto username = GameLevelManager::sharedState()->tryGetUsername(accountId);
            if (username.empty()) username = "User";
            std::string title = fmt::format("{}'s Layouts", username);

            RLLevelBrowserLayer::ParamList params;
            params.emplace_back("accountId", numToString(accountId));
            auto browserLayer = RLLevelBrowserLayer::create(RLLevelBrowserLayer::Mode::Account, params, title);
            auto scene = CCScene::create();
            scene->addChild(browserLayer);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
      }

      void onBlueprintStars(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto popup = RLDifficultyTotalPopup::create(accountId, RLDifficultyTotalPopup::Mode::Stars);
            if (popup)
                  popup->show();
      }
};
