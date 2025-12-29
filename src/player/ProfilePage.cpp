#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <argon/argon.hpp>

#include "RLDifficultyTotalPopup.hpp"
#include "RLUserControl.hpp"

using namespace geode::prelude;

// helper functions for caching user roles (local copy to keep file self-contained)
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
            CCLabelBMFont* blueprintStarsCount = nullptr;
            CCLabelBMFont* layoutPointsCount = nullptr;
            CCLabelBMFont* planetsCount = nullptr;
            int role = 0;
            int accountId = 0;
            bool isSupporter = false;
      };

      bool init(int accountID, bool ownProfile) {
            if (!ProfilePage::init(accountID, ownProfile))
                  return false;

            auto statsMenu = m_mainLayer->getChildByID("stats-menu");
            statsMenu->updateLayout();

            return true;
      }

      void getUserInfoFinished(GJUserScore* score) {
            ProfilePage::getUserInfoFinished(score);

            // find the stats menu
            // im trying to recreate how robtop adds the stats in the menu
            // very hacky way to do it but oh well
            auto statsMenu = m_mainLayer->getChildByID("stats-menu");
            if (!statsMenu) {
                  log::warn("stats-menu not found");
                  return;
            }

            auto blueprintStarsContainer = statsMenu->getChildByID("blueprint-stars-container");
            auto planetsContainer = statsMenu->getChildByID("planets-container");
            auto layoutPointsContainer = statsMenu->getChildByID("layout-points-container");
            auto blueprintStars = statsMenu->getChildByID("blueprint-stars-icon");
            auto planetsIcon = statsMenu->getChildByID("planets-icon");
            auto layoutPoints = statsMenu->getChildByID("layout-points-icon");

            if (blueprintStarsContainer || layoutPointsContainer || planetsContainer) {
                  if (blueprintStarsContainer) blueprintStarsContainer->removeFromParent();
                  if (planetsContainer) planetsContainer->removeFromParent();
                  if (planetsIcon) planetsIcon->removeFromParent();
                  if (blueprintStars) blueprintStars->removeFromParent();
                  if (layoutPointsContainer) layoutPointsContainer->removeFromParent();
                  if (layoutPoints) layoutPoints->removeFromParent();
                  log::info("recreating containers");
            }

            auto blueprintStarsCount =
                CCLabelBMFont::create(numToString(0).c_str(), "bigFont.fnt");
            blueprintStarsCount->setID("label");
            blueprintStarsCount->setAnchorPoint({1.f, .5f});

            limitNodeSize(blueprintStarsCount, {blueprintStarsCount->getContentSize()},
                          .7f,
                          .1f);  // geode limit node size thingy blep

            // Ensure label fits inside max width (50.f)
            const float maxWidth = 50.f;
            float origWidth = blueprintStarsCount->getContentSize().width;
            float currentScale = blueprintStarsCount->getScaleX();
            float desiredScale = std::min(currentScale, maxWidth / std::max(1.0f, origWidth));
            blueprintStarsCount->setScale(desiredScale);

            auto container = CCNode::create();
            float bsWidth = origWidth * blueprintStarsCount->getScaleX();
            if (bsWidth > maxWidth) bsWidth = maxWidth;
            container->setContentSize({bsWidth, 19.5f});
            container->setID("blueprint-stars-container");

            container->addChildAtPosition(blueprintStarsCount, Anchor::Right);
            container->setAnchorPoint({1.f, .5f});
            statsMenu->addChild(container);

            if (!blueprintStars) {
                  auto blueprintSprite = CCSprite::create("RL_starMed.png"_spr);
                  if (blueprintSprite) {
                        blueprintSprite->setScale(1.0f);
                        auto blueprintButton = CCMenuItemSpriteExtra::create(
                            blueprintSprite, this, menu_selector(RLProfilePage::onBlueprintStars));
                        blueprintButton->setID("blueprint-stars-icon");
                        if (auto statsMenuAsMenu = typeinfo_cast<CCMenu*>(statsMenu)) {
                              statsMenuAsMenu->addChild(blueprintButton);
                        } else {
                              auto btnMenu = CCMenu::create();
                              btnMenu->setPosition({0, 0});
                              btnMenu->addChild(blueprintButton);
                              statsMenu->addChild(btnMenu);
                        }
                  }
            }

            m_fields->blueprintStarsCount = nullptr;
            m_fields->layoutPointsCount = nullptr;

            if (score) {
                  fetchProfileData(score->m_accountID);
            }

            statsMenu->updateLayout();
      }

      void fetchProfileData(int accountId) {
            log::info("Fetching profile data for account ID: {}", accountId);
            m_fields->accountId = accountId;

            auto statsMenu = m_mainLayer->getChildByID("stats-menu");

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
                      log::debug("auth progress: {}",
                                 argon::authProgressToString(progress));
                });
            if (!res) {
                  log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                  return;
            }

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] =
                Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = accountId;

            // web request
            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");

            Ref<RLProfilePage> pageRef = this;

            // Handle the response
            postTask.listen([pageRef, statsMenu, this](web::WebResponse* response) {
                  log::info("Received response from server");

                  if (!pageRef || !pageRef->m_mainLayer) {
                        log::warn(
                            "ProfilePage has been destroyed, skipping profile data "
                            "update");
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
                  int role = json["role"].asInt().unwrapOrDefault();
                  int planets = json["planets"].asInt().unwrapOrDefault();
                  bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();

                  log::info("Profile data - points: {}, stars: {} supporter: {}", points, stars, isSupporter);

                  // store the values into the saved value
                  pageRef->m_fields->role = role;
                  pageRef->m_fields->isSupporter = isSupporter;

                  cacheUserProfile_ProfilePage(pageRef->m_fields->accountId, role, stars, planets);

                  // existing stats containers, this is so hacky but wanted to keep it
                  // at the right side
                  auto blueprintStarsContainer =
                      statsMenu->getChildByID("blueprint-stars-container");
                  auto blueprintStarsIcon = statsMenu->getChildByID("blueprint-stars-icon");
                  auto layoutPointsContainer =
                      statsMenu->getChildByID("layout-points-container");
                  auto layoutPointsIcon = statsMenu->getChildByID("layout-points-icon");

                  if (blueprintStarsContainer)
                        blueprintStarsContainer->removeFromParent();
                  if (blueprintStarsIcon)
                        blueprintStarsIcon->removeFromParent();
                  if (layoutPointsContainer)
                        layoutPointsContainer->removeFromParent();
                  if (layoutPointsIcon)
                        layoutPointsIcon->removeFromParent();

                  // label stuff
                  auto blueprintStarsCount =
                      CCLabelBMFont::create(numToString(GameToolbox::pointsToString(stars)).c_str(), "bigFont.fnt");
                  blueprintStarsCount->setID("label");
                  blueprintStarsCount->setAnchorPoint({1.f, .5f});
                  limitNodeSize(blueprintStarsCount,
                                {blueprintStarsCount->getContentSize()}, .7f, .1f);

                  // Ensure label fits inside max width (50.f)
                  const float maxWidth = 50.f;
                  float origWidth = blueprintStarsCount->getContentSize().width;
                  float currentScale = blueprintStarsCount->getScaleX();
                  float desiredScale = std::min(currentScale, maxWidth / std::max(1.0f, origWidth));
                  blueprintStarsCount->setScale(desiredScale);

                  auto container = CCNode::create();
                  float bsWidth = origWidth * blueprintStarsCount->getScaleX();
                  if (bsWidth > maxWidth) bsWidth = maxWidth;
                  container->setContentSize({bsWidth, 19.5f});
                  container->setID("blueprint-stars-container");
                  container->addChildAtPosition(blueprintStarsCount, Anchor::Right);
                  container->setAnchorPoint({1.f, .5f});
                  statsMenu->addChild(container);

                  auto blueprintStars = CCSprite::create("RL_starMed.png"_spr);
                  if (blueprintStars) {
                        blueprintStars->setScale(1.0f);
                        auto blueprintButton = CCMenuItemSpriteExtra::create(
                            blueprintStars, pageRef, menu_selector(RLProfilePage::onBlueprintStars));
                        blueprintButton->setID("blueprint-stars-icon");
                        if (auto statsMenuAsMenu = typeinfo_cast<CCMenu*>(statsMenu)) {
                              statsMenuAsMenu->addChild(blueprintButton);
                        } else {
                              auto btnMenu = CCMenu::create();
                              btnMenu->setPosition({0, 0});
                              btnMenu->addChild(blueprintButton);
                              statsMenu->addChild(btnMenu);
                        }
                  }

                  // planets
                  auto planetsCount = CCLabelBMFont::create(numToString(GameToolbox::pointsToString(planets)).c_str(), "bigFont.fnt");
                  planetsCount->setID("label");
                  planetsCount->setAnchorPoint({1.f, .5f});
                  limitNodeSize(planetsCount, {planetsCount->getContentSize()}, .7f, .1f);

                  // Ensure label fits inside max width (50.f)
                  const float maxWidthPlanets = 50.f;
                  float origPWidth = planetsCount->getContentSize().width;
                  float currentPScale = planetsCount->getScaleX();
                  float desiredPScale = std::min(currentPScale, maxWidthPlanets / std::max(1.0f, origPWidth));
                  planetsCount->setScale(desiredPScale);

                  auto planetsContainer = CCNode::create();
                  float pWidth = origPWidth * planetsCount->getScaleX();
                  if (pWidth > maxWidthPlanets) pWidth = maxWidthPlanets;
                  planetsContainer->setContentSize({pWidth, 19.5f});
                  planetsContainer->setID("planets-container");
                  planetsContainer->addChildAtPosition(planetsCount, Anchor::Right);
                  planetsContainer->setAnchorPoint({1.f, .5f});
                  statsMenu->addChild(planetsContainer);

                  // planets icon
                  auto planetsSprite = CCSprite::create("RL_planetMed.png"_spr);
                  if (planetsSprite) {
                        planetsSprite->setScale(1.0f);
                        auto planetsButton = CCMenuItemSpriteExtra::create(planetsSprite, pageRef, menu_selector(RLProfilePage::onPlanetsClicked));
                        planetsButton->setID("planets-icon");
                        if (auto statsMenuAsMenu = typeinfo_cast<CCMenu*>(statsMenu)) {
                              statsMenuAsMenu->addChild(planetsButton);
                        } else {
                              auto btnMenu = CCMenu::create();
                              btnMenu->setPosition({0, 0});
                              btnMenu->addChild(planetsButton);
                              statsMenu->addChild(btnMenu);
                        }
                  }

                  pageRef->m_fields->planetsCount = planetsCount;

                  bool disableCreatorPoints = Mod::get()->getSettingValue<bool>("disableCreatorPoints");
                  if (!disableCreatorPoints && points > 0) {
                        auto layoutPointsCount =
                            CCLabelBMFont::create(numToString(GameToolbox::pointsToString(points)).c_str(), "bigFont.fnt");
                        layoutPointsCount->setID("label");
                        layoutPointsCount->setAnchorPoint({1.f, .5f});
                        limitNodeSize(layoutPointsCount, {layoutPointsCount->getContentSize()},
                                      .7f, .1f);

                        // Ensure layout points label fits inside max width (50.f)
                        const float maxWidth = 50.f;
                        float origLpWidth = layoutPointsCount->getContentSize().width;
                        float currentLpScale = layoutPointsCount->getScaleX();
                        float desiredLpScale = std::min(currentLpScale, maxWidth / std::max(1.0f, origLpWidth));
                        layoutPointsCount->setScale(desiredLpScale);

                        auto layoutContainer = CCNode::create();
                        float lpWidth = origLpWidth * layoutPointsCount->getScaleX();
                        if (lpWidth > maxWidth) lpWidth = maxWidth;
                        layoutContainer->setContentSize({lpWidth, 19.5f});
                        layoutContainer->setID("layout-points-container");
                        layoutContainer->addChildAtPosition(layoutPointsCount, Anchor::Right);
                        layoutContainer->setAnchorPoint({1.f, .5f});
                        statsMenu->addChild(layoutContainer);

                        auto layoutIcon = CCSprite::create("RL_blueprintPoint01.png"_spr);
                        if (layoutIcon) {
                              layoutIcon->setScale(1.0f);
                              auto layoutButton = CCMenuItemSpriteExtra::create(layoutIcon, pageRef, menu_selector(RLProfilePage::onLayoutPointsClicked));
                              layoutButton->setID("layout-points-icon");
                              if (auto statsMenuAsMenu = typeinfo_cast<CCMenu*>(statsMenu)) {
                                    statsMenuAsMenu->addChild(layoutButton);
                              } else {
                                    auto btnMenu = CCMenu::create();
                                    btnMenu->setPosition({0, 0});
                                    btnMenu->addChild(layoutButton);
                                    statsMenu->addChild(btnMenu);
                              }
                        }

                        pageRef->m_fields->layoutPointsCount = layoutPointsCount;
                  } else {
                        pageRef->m_fields->layoutPointsCount = nullptr;
                  }

                  statsMenu->updateLayout();

                  // add a user manage button if the user accessing it is a mod or an admin
                  if (Mod::get()->getSavedValue<int>("role", 0) >= 1) {
                        auto leftMenu = static_cast<CCMenu*>(
                            pageRef->m_mainLayer->getChildByIDRecursive("left-menu"));
                        if (leftMenu && !leftMenu->getChildByID("rl-user-manage")) {
                              auto hammerSprite = CCSprite::create("RL_blueprintPoint01.png"_spr);
                              auto circleButtonSprite = CircleButtonSprite::create(
                                  hammerSprite, CircleBaseColor::DarkAqua, CircleBaseSize::Small);
                              circleButtonSprite->setScale(0.875f);
                              auto userButton = CCMenuItemSpriteExtra::create(
                                  circleButtonSprite, pageRef, menu_selector(RLProfilePage::onUserManage));
                              userButton->setID("rl-user-manage");
                              leftMenu->addChild(userButton);
                              leftMenu->updateLayout();
                        }
                  }

                  // only set saved data if you own the profile
                  if (pageRef->m_ownProfile) {
                        Mod::get()->setSavedValue("role", pageRef->m_fields->role);
                  }

                  pageRef->loadBadgeFromUserInfo();
            });
      }
      void onUserManage(CCObject* sender) {
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

            auto task = web::WebRequest().param("accountId", accountId).get("https://gdrate.arcticwoof.xyz/getAccountLevels");

            Ref<RLProfilePage> pageRef = this;

            task.listen([pageRef, menuItem](web::WebResponse* res) {
                  if (!pageRef) return;
                  if (!res || !res->ok()) {
                        Notification::create("Failed to fetch account levels", NotificationIcon::Error)->show();
                        menuItem->setEnabled(true);
                        return;
                  }

                  auto jsonRes = res->json();
                  if (!jsonRes) {
                        Notification::create("Invalid server response", NotificationIcon::Error)->show();
                        menuItem->setEnabled(true);
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
                        menuItem->setEnabled(true);
                        auto searchObject = GJSearchObject::create(SearchType::Type19, levelIDs);
                        auto browserLayer = LevelBrowserLayer::create(searchObject);
                        auto scene = CCScene::create();
                        scene->addChild(browserLayer);
                        auto transitionFade = CCTransitionFade::create(0.5f, scene);
                        CCDirector::sharedDirector()->pushScene(transitionFade);
                  } else {
                        Notification::create("No levels found for this account", NotificationIcon::Warning)->show();
                        menuItem->setEnabled(true);
                  }
            });
      }

      // badge
      void loadPageFromUserInfo(GJUserScore* a2) {
            ProfilePage::loadPageFromUserInfo(a2);
      }

      void loadBadgeFromUserInfo() {
            auto userNameMenu = typeinfo_cast<CCMenu*>(
                m_mainLayer->getChildByIDRecursive("username-menu"));
            if (!userNameMenu) {
                  log::warn("username-menu not found");
                  return;
            }

            // supporter badge (show for any profile that is a supporter)
            if (m_fields->isSupporter) {
                  if (!userNameMenu->getChildByID("rl-supporter-badge")) {
                        auto supporterSprite = CCSprite::create("RL_badgeSupporter.png"_spr);
                        auto supporterButton = CCMenuItemSpriteExtra::create(supporterSprite, this, menu_selector(RLProfilePage::onSupporterBadge));
                        supporterButton->setID("rl-supporter-badge");
                        userNameMenu->addChild(supporterButton);
                  }
            }

            if (m_accountID == 7689052) {  // ArcticWoof exception
                  if (userNameMenu->getChildByID("rl-owner-badge")) {
                        return;
                  }
                  if (auto mod = userNameMenu->getChildByID("rl-mod-badge")) mod->removeFromParent();
                  if (auto admin = userNameMenu->getChildByID("rl-admin-badge")) admin->removeFromParent();

                  auto ownerBadgeSprite = CCSprite::create("RL_badgeOwner.png"_spr);
                  auto ownerBadgeButton = CCMenuItemSpriteExtra::create(
                      ownerBadgeSprite, this, menu_selector(RLProfilePage::onOwnerBadge));
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
                      modBadgeSprite, this, menu_selector(RLProfilePage::onModBadge));

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
                      adminBadgeSprite, this, menu_selector(RLProfilePage::onAdminBadge));
                  adminBadgeButton->setID("rl-admin-badge");
                  userNameMenu->addChild(adminBadgeButton);
                  userNameMenu->updateLayout();
            }
            userNameMenu->updateLayout();
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
                "Rated Layouts Creator",
                "<cf>ArcticWoof</c> is the <ca>Creator and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
                "OK")
                ->show();
      }
      void onSupporterBadge(CCObject* sender) {
            geode::createQuickPopup(
                "Supporter Badge",
                "This user is a <cp>Layout Supporter</c>! They have supported the development of <cl>Rated Layouts</c> through membership donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via <cp>Ko-Fi</c>",
                "OK",
                "Ko-Fi", [this](auto, bool yes) {
                      if (!yes) return;
                      utils::web::openLinkInBrowser("https://ko-fi.com/arcticwoof");
                });
      }
      void onBlueprintStars(CCObject* sender) {
            int accountId = m_fields->accountId;
            auto popup = RLDifficultyTotalPopup::create(accountId);
            if (popup) popup->show();
      }
};