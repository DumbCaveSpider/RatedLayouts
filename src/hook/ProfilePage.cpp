#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/coro.hpp>
#include <argon/argon.hpp>

#include "../player/RLDifficultyTotalPopup.hpp"
#include "../player/RLUserControl.hpp"

using namespace geode::prelude;

static std::string getUserRoleCachePath_ProfilePage() {
      auto saveDir = dirs::getModsSaveDir();
      return geode::utils::string::pathToString(saveDir / "user_role_cache.json");
}

static void cacheUserProfile_ProfilePage(int accountId, int role, int stars, int planets) {
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
            if (parsed) root = parsed.unwrap();
      }

      matjson::Value obj = matjson::Value::object();
      obj["role"] = role;
      obj["stars"] = stars;
      obj["planets"] = planets;
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
            bool isBooster = false;

            int m_points = 0;
            int m_planets = 0;
            int m_stars = 0;
            int m_coins = 0;

            utils::web::WebTask m_profileTask;
            int m_profileForAccount = -1;
            bool m_profileInFlight = false;

            utils::web::WebTask m_accountLevelsTask;
            int m_accountLevelsForAccount = -1;
            bool m_accountLevelsInFlight = false;

            ~Fields() {
                  m_profileTask.cancel();
                  m_accountLevelsTask.cancel();
            }
      };

      CCLayer* createStatEntry(
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

            CCSprite* iconSprite = CCSprite::create(iconFrameOrPath);
            auto iconBtn = CCMenuItemSpriteExtra::create(iconSprite, this, iconCallback);
            iconBtn->setID(fmt::format("{}-icon-btn", entryID).c_str());

            auto ls = label->getScaledContentSize();
            auto is = iconBtn->getScaledContentSize();

            constexpr float gap = 2.f;
            constexpr float pad = 2.f;

            float h = std::max(ls.height, is.height);
            float w = pad + ls.width + gap + is.width + pad;

            auto entry = CCLayer::create();
            entry->setID(entryID);
            entry->setContentSize({w, h});
            entry->setAnchorPoint({0.f, 0.5f});

            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({pad, h / 2.f});
            entry->addChild(label);

            auto iconMenu = CCMenu::create();
            iconMenu->setPosition({12.f, 0.f});  // offset ts
            iconMenu->setContentSize({0, 0});
            iconMenu->setID(fmt::format("{}-icon-menu", entryID).c_str());
            // iconBtn->setAnchorPoint({0.f, 0.5f});
            iconBtn->setPosition({pad + ls.width + gap, h / 2.f});
            iconMenu->addChild(iconBtn);
            entry->addChild(iconMenu);

            return entry;
      }

      void badgeFixer(float dt) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto usernameMenu = m_mainLayer->getChildByID("username-menu");
        if (usernameMenu->getChildrenCount() > 1) {
            usernameMenu->setPositionX(winSize.width / 2.1);
        }
    }

      void updateStatLabel(char const* labelID, std::string const& text) {
            auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
            if (!rlStatsMenu) return;

            auto label = typeinfo_cast<CCLabelBMFont*>(rlStatsMenu->getChildByIDRecursive(labelID));
            if (!label) return;

            label->setString(text.c_str());

            constexpr float kLabelScale = 0.60f;
            constexpr float kMaxLabelW = 58.f;
            constexpr float kMinScale = 0.20f;
            label->limitLabelWidth(kMaxLabelW, kLabelScale, kMinScale);

            auto entry = typeinfo_cast<CCLayer*>(label->getParent());
            if (!entry) return;

            auto iconBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(entry->getChildByIDRecursive(
                fmt::format("{}-icon-btn", entry->getID()).c_str()));

            auto ls = label->getScaledContentSize();
            float isw = 0.f, ish = 0.f;
            if (iconBtn) {
                  auto is = iconBtn->getScaledContentSize();
                  isw = is.width;
                  ish = is.height;
            }

            constexpr float gap = 2.f;
            constexpr float pad = 2.f;

            float h = std::max(ls.height, ish);
            float w = pad + ls.width + gap + isw + pad;

            entry->setContentSize({w, h});

            label->setPosition({pad, h / 2.f});
            if (iconBtn) iconBtn->setPosition({pad + ls.width + gap, h / 2.f});
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

            if (Mod::get()->getSettingValue<bool>("badgesFixer")) {
                  schedule(schedule_selector(RLProfilePage::badgeFixer), 0.0f);
            }

            if (auto rlMenuBtnSpr = getChildByIDRecursive("rl-menu-btn-spr")) rlMenuBtnSpr->removeFromParent();
            if (auto rlMenuBtn = getChildByIDRecursive("rl-btn-menu")) rlMenuBtn->removeFromParent();

            auto statsMenu = m_mainLayer->getChildByID("stats-menu");
            if (!statsMenu) {
                  log::warn("stats-menu not found");
                  return;
            }

            if (auto rlStatsBtnFound = getChildByIDRecursive("rl-stats-btn")) rlStatsBtnFound->removeFromParent();
            if (auto rlStatsMenuFound = getChildByIDRecursive("rl-stats-menu")) rlStatsMenuFound->removeFromParent();

            auto leftMenu = getChildByIDRecursive("left-menu");
            if (!leftMenu) {
                  log::warn("left-menu not found");
                  return;
            }

            auto winSize = CCDirector::sharedDirector()->getWinSize();

            auto rlBtnMenuSpr = CCScale9Sprite::create("GJ_square01.png");
            rlBtnMenuSpr->setContentSize({ 40.f, 40.f });
            rlBtnMenuSpr->setID("rl-menu-btn-spr");
            auto rlBtnMenu = CCMenu::create();
            auto layout = ColumnLayout::create();
            layout->setAutoGrowAxis(true);
            rlBtnMenu->setLayout(layout);
            rlBtnMenu->setContentSize({ 0.f, 0.f });
            rlBtnMenu->setPosition(winSize.width / 1.07, winSize.height / 2);
            rlBtnMenuSpr->setPosition(winSize.width / 1.07, winSize.height / 2);
            rlBtnMenu->setID("rl-btn-menu");
            m_mainLayer->addChild(rlBtnMenuSpr);
            m_mainLayer->addChild(rlBtnMenu);
            rlBtnMenu->updateLayout();
            rlBtnMenu->updateLayout();

            auto planetSpr = CCSprite::create("RL_planetMed.png"_spr);
            planetSpr->setScale(0.8f);
            auto rlStatsSpr = EditorButtonSprite::create(planetSpr, EditorBaseColor::Gray, EditorBaseSize::Normal);
            auto rlStatsSprOn = EditorButtonSprite::create(planetSpr, EditorBaseColor::Cyan, EditorBaseSize::Normal);

            auto rlStatsBtn = CCMenuItemToggler::create(
                rlStatsSpr,
                rlStatsSprOn,
                this,
                menu_selector(RLProfilePage::onStatsSwitcher));
            rlStatsBtn->setID("rl-stats-btn");
            rlStatsBtn->setScale(0.8f);
            rlBtnMenu->addChild(rlStatsBtn);

            auto rlStatsMenu = CCMenu::create();
            rlStatsMenu->setID("rl-stats-menu");
            rlStatsMenu->setContentSize(statsMenu->getContentSize());
            rlStatsMenu->setPosition(statsMenu->getPositionX(), statsMenu->getPositionY());
            rlStatsMenu->setScale(statsMenu->getScale());

            auto row = RowLayout::create();
            row->setAxisAlignment(AxisAlignment::Center);
            row->setCrossAxisAlignment(AxisAlignment::Center);
            row->setGap(4.f);
            rlStatsMenu->setLayout(row);

            rlStatsMenu->setAnchorPoint({0.5f, 0.5f});

            auto starsText = GameToolbox::pointsToString(m_fields->m_stars);
            auto planetsText = GameToolbox::pointsToString(m_fields->m_planets);
            auto coinsText = GameToolbox::pointsToString(m_fields->m_coins);

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

            auto coinsEntry = createStatEntry(
                "rl-coins-entry",
                "rl-coins-label",
                coinsText,
                "RL_BlueCoinSmall.png"_spr,
                nullptr);

            rlStatsMenu->addChild(starsEntry);
            rlStatsMenu->addChild(planetsEntry);
            rlStatsMenu->addChild(coinsEntry);

            rlStatsMenu->setVisible(false);
            statsMenu->setVisible(true);

            m_mainLayer->addChild(rlStatsMenu);

            if (score) {
                  fetchProfileData(score->m_accountID);
            }

            rlStatsMenu->updateLayout();

            statsMenu->updateLayout();
            leftMenu->updateLayout();
      }

      void onStatsSwitcher(CCObject* sender) {
            auto statsMenu = getChildByIDRecursive("stats-menu");
            auto rlStatsMenu = getChildByIDRecursive("rl-stats-menu");
            auto switcher = typeinfo_cast<CCMenuItemToggler*>(sender);

            if (!statsMenu || !rlStatsMenu || !switcher) return;

            if (!switcher->isToggled()) {
                  statsMenu->setVisible(false);
                  if (auto m = typeinfo_cast<CCMenu*>(statsMenu)) m->setEnabled(false);

                  rlStatsMenu->setVisible(true);
                  if (auto m = typeinfo_cast<CCMenu*>(rlStatsMenu)) m->setEnabled(true);
            } else {
                  statsMenu->setVisible(true);
                  if (auto m = typeinfo_cast<CCMenu*>(statsMenu)) m->setEnabled(true);

                  rlStatsMenu->setVisible(false);
                  if (auto m = typeinfo_cast<CCMenu*>(rlStatsMenu)) m->setEnabled(false);
            }
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

            if (m_fields->m_profileInFlight && m_fields->m_profileForAccount == accountId) {
                  log::debug("Profile request already in-flight for account {}", accountId);
                  return;
            }

            m_fields->m_profileTask.cancel();
            m_fields->m_profileInFlight = true;
            m_fields->m_profileForAccount = accountId;

            m_fields->m_profileTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");

            Ref<RLProfilePage> pageRef = this;
            m_fields->m_profileTask.listen([pageRef, accountId](web::WebResponse* response) {
                  if (pageRef) {
                        pageRef->m_fields->m_profileInFlight = false;
                        pageRef->m_fields->m_profileForAccount = -1;
                  }
                  if (!pageRef) {
                        log::warn("skipping profile data update");
                        return;
                  }
                  log::info("Received response from server");

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
                  int role = json["role"].asInt().unwrapOrDefault();
                  int coins = json["coins"].asInt().unwrapOrDefault();
                  int planets = json["planets"].asInt().unwrapOrDefault();
                  bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();
                  bool isBooster = json["isBooster"].asBool().unwrapOrDefault();

                  cacheUserProfile_ProfilePage(accountId, role, stars, planets);

                  if (!pageRef->m_mainLayer) {
                        log::debug("ProfilePage UI destroyed; cached profile data updated only");
                        if (pageRef->m_ownProfile) {
                              Mod::get()->setSavedValue("role", role);
                        }
                        return;
                  }

                  auto page = pageRef;
                  page->m_fields->m_stars = stars;
                  page->m_fields->m_planets = planets;
                  page->m_fields->m_points = points;
                  page->m_fields->m_coins = coins;

                  page->m_fields->role = role;
                  page->m_fields->isSupporter = isSupporter;
                  page->m_fields->isBooster = isBooster;

                  if (page->m_ownProfile && page->m_fields->role == 0) {
                        Mod::get()->setSavedValue("role", page->m_fields->role);
                        log::info("removing layout mod access");
                  }

                  if (Mod::get()->getSavedValue<int>("role") >= 1 && !page->getChildByIDRecursive("rl-user-manage-btn")) {
                        auto userManageSpr = CCSprite::create("RL_badgeAdmin01.png"_spr);
                        auto rlStatsSpr = EditorButtonSprite::create(userManageSpr, EditorBaseColor::Gray, EditorBaseSize::Normal);

                        auto userManageButton = CCMenuItemSpriteExtra::create(
                            rlStatsSpr,
                            pageRef,
                            menu_selector(RLProfilePage::onUserManage));
                        userManageButton->setID("rl-user-manage-btn");
                        if (auto rlBtnMenu = pageRef->getChildByIDRecursive("rl-btn-menu")) {
                              auto rlBtnMenuSpr = pageRef->getChildByIDRecursive("rl-menu-btn-spr");
                              rlBtnMenu->addChild(userManageButton);
                              rlBtnMenuSpr->setContentHeight(85.f);
                              rlBtnMenu->updateLayout();
                        }
                  }

                  page->updateStatLabel("rl-stars-label", GameToolbox::pointsToString(page->m_fields->m_stars));
                  page->updateStatLabel("rl-planets-label", GameToolbox::pointsToString(page->m_fields->m_planets));
                  page->updateStatLabel("rl-points-label", GameToolbox::pointsToString(page->m_fields->m_points));
                  page->updateStatLabel("rl-coins-label", GameToolbox::pointsToString(page->m_fields->m_coins));
                  {
                        auto pointsText = GameToolbox::pointsToString(page->m_fields->m_points);
                        auto rlStatsMenu = page->getChildByIDRecursive("rl-stats-menu");
                        auto statsMenu = page->m_mainLayer->getChildByID("stats-menu");

                        if (!Mod::get()->getSettingValue<bool>("disableCreatorPoints")) {
                              if (rlStatsMenu && !rlStatsMenu->getChildByID("rl-points-entry")) {
                                    auto pointsEntry = page->createStatEntry(
                                        "rl-points-entry",
                                        "rl-points-label",
                                        pointsText,
                                        "RL_blueprintPoint01.png"_spr,
                                        menu_selector(RLProfilePage::onLayoutPointsClicked));

                                    if (page->m_fields->m_points > 0) {
                                          log::info("Adding rl-points-entry (points={})", page->m_fields->m_points);
                                          rlStatsMenu->addChild(pointsEntry);
                                    } else {
                                          log::info("Not adding rl-points-entry because points=={}", page->m_fields->m_points);
                                    }
                              }

                              if (rlStatsMenu && statsMenu) {
                                    auto betterProgSign = page->getChildByIDRecursive("itzkiba.better_progression/tier-bar");
                                    if (betterProgSign) {
                                          rlStatsMenu->setContentSize(statsMenu->getContentSize());
                                          rlStatsMenu->setPosition(statsMenu->getPositionX(), statsMenu->getPositionY());
                                          rlStatsMenu->setScale(statsMenu->getScale());
                                    }
                              }
                        }

                        rlStatsMenu->updateLayout();
                  }

                  page->loadBadgeFromUserInfo();
            });
      }

      void
      onUserManage(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto userControl = RLUserControl::create(accountId);
            userControl->show();
      }

      void onPlanetsClicked(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto popup = RLDifficultyTotalPopup::create(accountId, RLDifficultyTotalPopup::Mode::Planets);
            if (popup) popup->show();
      }

      void onLayoutPointsClicked(CCObject* sender) {
            // disable button when clicked
            auto menuItem = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
            if (menuItem) menuItem->setEnabled(false);

            int accountId = m_fields->accountId;
            log::info("Fetching account levels for account ID: {}", accountId);

            if (m_fields->m_accountLevelsInFlight && m_fields->m_accountLevelsForAccount == accountId) {
                  log::debug("Account levels request already in-flight for account {}", accountId);
                  if (menuItem) menuItem->setEnabled(true);
                  return;
            }

            m_fields->m_accountLevelsTask.cancel();
            m_fields->m_accountLevelsInFlight = true;
            m_fields->m_accountLevelsForAccount = accountId;

            m_fields->m_accountLevelsTask = web::WebRequest()
                                                .param("accountId", accountId)
                                                .get("https://gdrate.arcticwoof.xyz/getAccountLevels");

            Ref<RLProfilePage> pageRef = this;

            m_fields->m_accountLevelsTask.listen([pageRef, menuItem, accountId](web::WebResponse* res) {
                  if (pageRef) {
                        pageRef->m_fields->m_accountLevelsInFlight = false;
                        pageRef->m_fields->m_accountLevelsForAccount = -1;
                  }
                  if (!pageRef) return;
                  if (!res || !res->ok()) {
                        Notification::create("Failed to fetch account levels", NotificationIcon::Error)->show();
                        if (menuItem) menuItem->setEnabled(true);
                        return;
                  }

                  auto jsonRes = res->json();
                  if (!jsonRes) {
                        Notification::create("Invalid server response", NotificationIcon::Error)->show();
                        if (menuItem) menuItem->setEnabled(true);
                        return;
                  }

                  auto json = jsonRes.unwrap();
                  std::string levelIDs;
                  bool first = true;

                  if (json.contains("levels") && json["levels"].isArray()) {
                        log::info("getAccountLevels: using 'levels' array");
                        auto arr = json["levels"].asArray().unwrap();
                        for (auto v : arr) {
                              auto id = v.as<int>();
                              if (!id) continue;
                              if (!first) levelIDs += ",";
                              levelIDs += numToString(id.unwrap());
                              first = false;
                        }
                  } else if (json.contains("levelIds") && json["levelIds"].isArray()) {
                        log::info("getAccountLevels: using 'levelIds' array");
                        auto arr = json["levelIds"].asArray().unwrap();
                        for (auto v : arr) {
                              auto id = v.as<int>();
                              if (!id) continue;
                              if (!first) levelIDs += ",";
                              levelIDs += numToString(id.unwrap());
                              first = false;
                        }
                  }

                  if (!levelIDs.empty()) {
                        if (menuItem) menuItem->setEnabled(true);
                        auto searchObject = GJSearchObject::create(SearchType::Type19, levelIDs);
                        auto browserLayer = LevelBrowserLayer::create(searchObject);
                        auto scene = CCScene::create();
                        scene->addChild(browserLayer);
                        auto transitionFade = CCTransitionFade::create(0.5f, scene);
                        CCDirector::sharedDirector()->pushScene(transitionFade);
                  } else {
                        Notification::create("No levels found for this account", NotificationIcon::Warning)->show();
                        if (menuItem) menuItem->setEnabled(true);
                  }
            });
      }

      void onBlueprintStars(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto popup = RLDifficultyTotalPopup::create(accountId, RLDifficultyTotalPopup::Mode::Stars);
            if (popup) popup->show();
      }

      void onModBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Moderator",
                "This user can <cj>suggest layout levels</c> for <cl>Rated "
                "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to <co>moderate the leaderboard</c>.",
                "OK")
                ->show();
      }

      void onAdminBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Administrator",
                "This user can <cj>rate layout levels</c> for <cl>Rated "
                "Layouts</c>. They have the same power as <cg>Moderators</c> but including the ability to change the <cy>featured ranking on the "
                "featured layout levels</c> and <cg>set event layouts</c>.",
                "OK")
                ->show();
      }
      void onOwnerBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Rated Layouts Owner",
                "<cf>ArcticWoof</c> is the <ca>Owner and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
                "OK")
                ->show();
      }
      void onSupporterBadge(CCObject* sender) {
            geode::createQuickPopup(
                "Layout Supporter",
                "This user is a <cp>Layout Supporter</c>! They have supported the development of <cl>Rated Layouts</c> through membership donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via <cp>Ko-Fi</c>",
                "OK",
                "Ko-Fi", [this](auto, bool yes) {
                      if (!yes) return;
                      utils::web::openLinkInBrowser("https://ko-fi.com/arcticwoof");
                });
      }
      void onBoosterBadge(CCObject* sender) {
            geode::createQuickPopup(
                  "Layout Server Booster",
                  "This user is <cp>Layout Server Booster</c>! They have supported our <cy>Discord Server</c> through boosting it.\n\nYou can become a <cp>Layout Server Booster</c> by joining our server and giving a boost",
                  "OK",
                  "Discord", [this](auto, bool btn2) {
                        if (!btn2) return;
                        utils::web::openLinkInBrowser("https://discord.gg/jBf2wfBgVT");
                  }
            );
      }

      void loadBadgeFromUserInfo() {
            if (!m_mainLayer) {
                  log::warn("main layer is null, cannot load badges from user info");
                  return;
            }
            auto userNameMenu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByIDRecursive("username-menu"));
            if (!userNameMenu) {
                  log::warn("username-menu not found");
                  return;
            }

            // supporter badge (show for any profile that is a supporter)
            if (m_fields->isSupporter) {
                  if (!userNameMenu->getChildByID("rl-supporter-badge")) {
                        auto supporterSprite = CCSprite::create("RL_badgeSupporter.png"_spr);
                        auto supporterButton = CCMenuItemSpriteExtra::create(
                            supporterSprite,
                            this,
                            menu_selector(RLProfilePage::onSupporterBadge));
                        supporterButton->setID("rl-supporter-badge");
                        userNameMenu->addChild(supporterButton);
                  }
            }

            if (m_fields->isBooster) {
                  if (userNameMenu->getChildByID("rl-booster-badge")) {
                        log::info("Booster badge already exists, skipping creation");
                        return;
                  }
                  if (auto booster = userNameMenu->getChildByID("rl-booster-badge")) booster->removeFromParent();

                  auto boosterBadgeSpr = CCSprite::create("RL_badgeBooster.png"_spr);
                  auto boosterBadgeBtn = CCMenuItemSpriteExtra::create(
                        boosterBadgeSpr,
                        this,
                        menu_selector(RLProfilePage::onBoosterBadge)
                  );
                  boosterBadgeBtn->setID("rl-booster-badge");
                  userNameMenu->addChild(boosterBadgeBtn);
                  userNameMenu->updateLayout();
            }

            if (m_accountID == 7689052) {  // ArcticWoof exception
                  if (userNameMenu->getChildByID("rl-owner-badge")) {
                        return;
                  }
                  if (auto mod = userNameMenu->getChildByID("rl-mod-badge")) mod->removeFromParent();
                  if (auto admin = userNameMenu->getChildByID("rl-admin-badge")) admin->removeFromParent();

                  auto ownerBadgeSprite = CCSprite::create("RL_badgeOwner.png"_spr);
                  auto ownerBadgeButton = CCMenuItemSpriteExtra::create(
                      ownerBadgeSprite,
                      this,
                      menu_selector(RLProfilePage::onOwnerBadge));
                  ownerBadgeButton->setID("rl-owner-badge");
                  userNameMenu->addChild(ownerBadgeButton);
            } else if (m_fields->role == 1) {
                  if (userNameMenu->getChildByID("rl-mod-badge")) {
                        return;
                  }
                  if (auto owner = userNameMenu->getChildByID("rl-owner-badge")) owner->removeFromParent();
                  if (auto admin = userNameMenu->getChildByID("rl-admin-badge")) admin->removeFromParent();

                  auto modBadgeSprite = CCSprite::create("RL_badgeMod01.png"_spr);
                  auto modBadgeButton = CCMenuItemSpriteExtra::create(
                      modBadgeSprite,
                      this,
                      menu_selector(RLProfilePage::onModBadge));
                  modBadgeButton->setID("rl-mod-badge");
                  userNameMenu->addChild(modBadgeButton);
                  userNameMenu->updateLayout();
            } else if (m_fields->role == 2) {
                  if (userNameMenu->getChildByID("rl-admin-badge")) {
                        log::info("Admin badge already exists, skipping creation");
                        return;
                  }
                  if (auto owner = userNameMenu->getChildByID("rl-owner-badge")) owner->removeFromParent();
                  if (auto mod = userNameMenu->getChildByID("rl-mod-badge")) mod->removeFromParent();

                  auto adminBadgeSprite = CCSprite::create("RL_badgeAdmin01.png"_spr);
                  auto adminBadgeButton = CCMenuItemSpriteExtra::create(
                      adminBadgeSprite,
                      this,
                      menu_selector(RLProfilePage::onAdminBadge));
                  adminBadgeButton->setID("rl-admin-badge");
                  userNameMenu->addChild(adminBadgeButton);
                  userNameMenu->updateLayout();
            } else {
                  // remove role badges if any
                  if (auto owner = userNameMenu->getChildByID("rl-owner-badge")) owner->removeFromParent();
                  if (auto mod = userNameMenu->getChildByID("rl-mod-badge")) mod->removeFromParent();
                  if (auto admin = userNameMenu->getChildByID("rl-admin-badge")) admin->removeFromParent();
            }

            userNameMenu->updateLayout();
      }
};
