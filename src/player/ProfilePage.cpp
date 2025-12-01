#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <argon/argon.hpp>

#include "RLUserControl.hpp"

using namespace geode::prelude;

class $modify(RLProfilePage, ProfilePage) {
      struct Fields {
            CCLabelBMFont* blueprintStarsCount = nullptr;
            CCLabelBMFont* layoutPointsCount = nullptr;
            int role = 0;
            int accountId = 0;
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

            auto blueprintStarsContainer =
                statsMenu->getChildByID("blueprint-stars-container");
            auto layoutPointsContainer =
                statsMenu->getChildByID("layout-points-container");
            auto blueprintStars = statsMenu->getChildByID("blueprint-stars-icon");
            auto layoutPoints =
                statsMenu->getChildByID("layout-points-icon");

            if (blueprintStarsContainer || layoutPointsContainer) {
                  if (blueprintStarsContainer)
                        blueprintStarsContainer->removeFromParent();
                  if (blueprintStars) blueprintStars->removeFromParent();
                  if (layoutPointsContainer)
                        layoutPointsContainer->removeFromParent();
                  if (layoutPoints) layoutPoints->removeFromParent();
                  log::info("recreating blueprint stars and layout points containers");
            }

            log::info("adding the blueprint stars stats");

            auto blueprintStarsCount =
                CCLabelBMFont::create(numToString(0).c_str(), "bigFont.fnt");
            blueprintStarsCount->setID("label");
            blueprintStarsCount->setAnchorPoint({1.f, .5f});

            limitNodeSize(blueprintStarsCount, {blueprintStarsCount->getContentSize()},
                          .7f,
                          .1f);  // geode limit node size thingy blep

            auto container = CCNode::create();
            float bsWidth = blueprintStarsCount->getContentSize().width * blueprintStarsCount->getScaleX();
            if (bsWidth > 50.f) bsWidth = 50.f;
            container->setContentSize({bsWidth, 19.5f});
            container->setID("blueprint-stars-container");

            container->addChildAtPosition(blueprintStarsCount, Anchor::Right);
            container->setAnchorPoint({1.f, .5f});
            statsMenu->addChild(container);

            if (!blueprintStars) {
                  blueprintStars = CCSprite::create("rlStarIconMed.png"_spr);
                  if (blueprintStars) {
                        blueprintStars->setID("blueprint-stars-icon");
                        blueprintStars->setScale(1.0f);
                        statsMenu->addChild(blueprintStars);
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
            postTask.listen([pageRef, statsMenu](web::WebResponse* response) {
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

                  log::info("Profile data - points: {}, stars: {}", points, stars);

                  // store the values into the saved value
                  pageRef->m_fields->role = role;

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
                      CCLabelBMFont::create(numToString(stars).c_str(), "bigFont.fnt");
                  blueprintStarsCount->setID("label");
                  blueprintStarsCount->setAnchorPoint({1.f, .5f});
                  limitNodeSize(blueprintStarsCount,
                                {blueprintStarsCount->getContentSize()}, .7f, .1f);

                  auto container = CCNode::create();
                  float bsWidth = blueprintStarsCount->getContentSize().width * blueprintStarsCount->getScaleX();
                  if (bsWidth > 50.f) bsWidth = 50.f;
                  container->setContentSize({bsWidth, 19.5f});
                  container->setID("blueprint-stars-container");
                  container->addChildAtPosition(blueprintStarsCount, Anchor::Right);
                  container->setAnchorPoint({1.f, .5f});
                  statsMenu->addChild(container);

                  auto blueprintStars = CCSprite::create("rlStarIconMed.png"_spr);
                  if (blueprintStars) {
                        blueprintStars->setID("blueprint-stars-icon");
                        blueprintStars->setScale(1.0f);
                        statsMenu->addChild(blueprintStars);
                  }

                  if (points > 0) {
                        auto layoutPointsCount =
                            CCLabelBMFont::create(numToString(points).c_str(), "bigFont.fnt");
                        layoutPointsCount->setID("label");
                        layoutPointsCount->setAnchorPoint({1.f, .5f});
                        limitNodeSize(layoutPointsCount, {layoutPointsCount->getContentSize()},
                                      .7f, .1f);

                        auto layoutContainer = CCNode::create();
                        float lpWidth = layoutPointsCount->getContentSize().width * layoutPointsCount->getScaleX();
                        if (lpWidth > 50.f) lpWidth = 50.f;
                        layoutContainer->setContentSize({lpWidth, 19.5f});
                        layoutContainer->setID("layout-points-container");
                        layoutContainer->addChildAtPosition(layoutPointsCount, Anchor::Right);
                        layoutContainer->setAnchorPoint({1.f, .5f});
                        statsMenu->addChild(layoutContainer);

                        auto layoutIcon = CCSprite::create("rlhammerIcon.png"_spr);
                        if (layoutIcon) {
                              layoutIcon->setID("layout-points-icon");
                              statsMenu->addChild(layoutIcon);
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
                              auto hammerSprite = CCSprite::create("rlhammerIcon.png"_spr);
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

            if (m_fields->role == 1) {
                  if (userNameMenu->getChildByID("rl-mod-badge")) {
                        return;
                  }

                  auto modBadgeSprite = CCSprite::create("rlBadgeMod.png"_spr);
                  auto modBadgeButton = CCMenuItemSpriteExtra::create(
                      modBadgeSprite, this, menu_selector(RLProfilePage::onModBadge));

                  modBadgeButton->setID("rl-mod-badge");
                  userNameMenu->addChild(modBadgeButton);
                  userNameMenu->updateLayout();
            } else if (m_fields->role == 2) {
                  // Check if badge already exists
                  if (userNameMenu->getChildByID("rl-admin-badge")) {
                        log::info("Admin badge already exists, skipping creation");
                        return;
                  }

                  auto adminBadgeSprite = CCSprite::create("rlBadgeAdmin.png"_spr);
                  auto adminBadgeButton = CCMenuItemSpriteExtra::create(
                      adminBadgeSprite, this, menu_selector(RLProfilePage::onAdminBadge));

                  adminBadgeButton->setID("rl-admin-badge");
                  userNameMenu->addChild(adminBadgeButton);
                  userNameMenu->updateLayout();
            }
      }

      void onModBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Moderator",
                "This user can <cj>suggest layout levels</c> for <cl>Rated "
                "Layouts</c> to the <cr>Layout Admins</c>.",
                "OK")
                ->show();
      }

      void onAdminBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Administrator",
                "This user can <cj>rate layout levels</c> for <cl>Rated "
                "Layouts</c>. They can change the <cy>featured ranking on the "
                "featured layout levels.</c>",
                "OK")
                ->show();
      }
};