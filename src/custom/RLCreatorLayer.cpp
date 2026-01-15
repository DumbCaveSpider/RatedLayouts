#include "RLCreatorLayer.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <algorithm>
#include <deque>
#include <random>

#include "../level/RLEventLayouts.hpp"
#include "RLAddDialogue.hpp"
#include "RLAnnoucementPopup.hpp"
#include "RLCreditsPopup.hpp"
#include "RLDonationPopup.hpp"
#include "RLGauntletSelectLayer.hpp"
#include "RLLeaderboardLayer.hpp"
#include "RLSearchLayer.hpp"

using namespace geode::prelude;

bool RLCreatorLayer::init() {
      if (!CCLayer::init())
            return false;

      auto winSize = CCDirector::sharedDirector()->getWinSize();

      // create if moving bg disabled
      if (Mod::get()->getSettingValue<bool>("disableBackground") == true) {
            auto bg = createLayerBG();
            addChild(bg, -1);
      }

      addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

      auto backMenu = CCMenu::create();
      backMenu->setPosition({0, 0});

      auto backButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
      CCMenuItemSpriteExtra* backButton = CCMenuItemSpriteExtra::create(
          backButtonSpr, this, menu_selector(RLCreatorLayer::onBackButton));
      backButton->setPosition({25, winSize.height - 25});
      backMenu->addChild(backButton);

      auto modSettingsBtnSprite = CircleButtonSprite::createWithSpriteFrameName(
          // @geode-ignore(unknown-resource)
          "geode.loader/settings.png",
          1.f,
          CircleBaseColor::Green,
          CircleBaseSize::Medium);
      modSettingsBtnSprite->setScale(0.75f);
      auto settingsButton = CCMenuItemSpriteExtra::create(
          modSettingsBtnSprite, this, menu_selector(RLCreatorLayer::onSettingsButton));
      settingsButton->setPosition({winSize.width - 25, winSize.height - 25});
      backMenu->addChild(settingsButton);
      this->addChild(backMenu);

      auto mainMenu = CCMenu::create();
      mainMenu->setPosition({winSize.width / 2, winSize.height / 2 - 10});
      mainMenu->setContentSize({400.f, 240.f});
      mainMenu->setLayout(RowLayout::create()
                              ->setGap(10.f)
                              ->setGrowCrossAxis(true)
                              ->setCrossAxisOverflow(false));

      this->addChild(mainMenu);

      auto title = CCSprite::create("RL_title.png"_spr);
      title->setPosition({winSize.width / 2, winSize.height / 2 + 130});
      title->setScale(1.4f);
      this->addChild(title);

      auto featuredSpr = CCSprite::create("RL_featured01.png"_spr);
      if (!featuredSpr) featuredSpr = CCSprite::create("RL_featured01.png"_spr);
      auto featuredItem = CCMenuItemSpriteExtra::create(
          featuredSpr, this, menu_selector(RLCreatorLayer::onFeaturedLayouts));
      featuredItem->setID("featured-button");
      mainMenu->addChild(featuredItem);

      auto leaderboardSpr = CCSprite::create("RL_leaderboard01.png"_spr);
      if (!leaderboardSpr) leaderboardSpr = CCSprite::create("RL_leaderboard01.png"_spr);
      auto leaderboardItem = CCMenuItemSpriteExtra::create(
          leaderboardSpr, this, menu_selector(RLCreatorLayer::onLeaderboard));
      leaderboardItem->setID("leaderboard-button");
      mainMenu->addChild(leaderboardItem);

      // gauntlet coming soon
      auto gauntletSpr = CCSprite::create("RL_gauntlets01.png"_spr);
      if (!gauntletSpr) gauntletSpr = CCSprite::create("RL_gauntlets01.png"_spr);
      auto gauntletItem = CCMenuItemSpriteExtra::create(
          gauntletSpr, this, menu_selector(RLCreatorLayer::onLayoutGauntlets));
      gauntletItem->setID("gauntlet-button");
      mainMenu->addChild(gauntletItem);

      auto sentSpr = CCSprite::create("RL_sent01.png"_spr);
      if (!sentSpr) sentSpr = CCSprite::create("RL_sent01.png"_spr);
      auto sentItem = CCMenuItemSpriteExtra::create(
          sentSpr, this, menu_selector(RLCreatorLayer::onSentLayouts));
      sentItem->setID("sent-layouts-button");
      mainMenu->addChild(sentItem);

      auto searchSpr = CCSprite::create("RL_search01.png"_spr);
      if (!searchSpr) searchSpr = CCSprite::create("RL_search01.png"_spr);
      auto searchItem = CCMenuItemSpriteExtra::create(
          searchSpr, this, menu_selector(RLCreatorLayer::onSearchLayouts));
      searchItem->setID("search-layouts-button");
      mainMenu->addChild(searchItem);

      auto dailySpr = CCSprite::create("RL_daily01.png"_spr);
      if (!dailySpr) dailySpr = CCSprite::create("RL_daily01.png"_spr);
      auto dailyItem = CCMenuItemSpriteExtra::create(
          dailySpr, this, menu_selector(RLCreatorLayer::onDailyLayouts));
      dailyItem->setID("daily-layouts-button");
      mainMenu->addChild(dailyItem);

      auto weeklySpr = CCSprite::create("RL_weekly01.png"_spr);
      if (!weeklySpr) weeklySpr = CCSprite::create("RL_weekly01.png"_spr);
      auto weeklyItem = CCMenuItemSpriteExtra::create(
          weeklySpr, this, menu_selector(RLCreatorLayer::onWeeklyLayouts));
      weeklyItem->setID("weekly-layouts-button");
      mainMenu->addChild(weeklyItem);

      auto monthlySpr = CCSprite::create("RL_monthly01.png"_spr);
      if (!monthlySpr) monthlySpr = CCSprite::create("RL_monthly01.png"_spr);
      auto monthlyItem = CCMenuItemSpriteExtra::create(
          monthlySpr, this, menu_selector(RLCreatorLayer::onMonthlyLayouts));
      monthlyItem->setID("monthly-layouts-button");
      mainMenu->addChild(monthlyItem);

      // Try to use a grayscale sprite where available, but fallback to a regular sprite
      cocos2d::CCSprite* unknownSpr = CCSpriteGrayscale::create("RL_unknownBtn.png"_spr);
      if (!unknownSpr) unknownSpr = CCSpriteGrayscale::create("RL_unknownBtn.png"_spr);
      auto unknownItem = CCMenuItemSpriteExtra::create(
          unknownSpr, this, menu_selector(RLCreatorLayer::onUnknownButton));
      unknownItem->setID("unknown-button");
      mainMenu->addChild(unknownItem);

      mainMenu->updateLayout();

      // info button at the bottom left
      auto infoMenu = CCMenu::create();
      infoMenu->setPosition({0, 0});
      auto infoButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto infoButton = CCMenuItemSpriteExtra::create(
          infoButtonSpr, this, menu_selector(RLCreatorLayer::onInfoButton));
      infoButton->setPosition({25, 25});
      infoMenu->addChild(infoButton);
      // discord thingy
      auto discordIconSpr = CCSprite::createWithSpriteFrameName("gj_discordIcon_001.png");
      auto discordIconBtn = CCMenuItemSpriteExtra::create(discordIconSpr, this, menu_selector(RLCreatorLayer::onDiscordButton));
      discordIconBtn->setPosition({infoButton->getPositionX(), infoButton->getPositionY() + 40});
      infoMenu->addChild(discordIconBtn);

      // news button above discord
      // @geode-ignore(unknown-resource)
      auto annouceSpr = AccountButtonSprite::createWithSpriteFrameName("geode.loader/news.png", 1.f, AccountBaseColor::Gray, AccountBaseSize::Normal);
      annouceSpr->setScale(0.7f);
      auto annouceBtn = CCMenuItemSpriteExtra::create(annouceSpr, this, menu_selector(RLCreatorLayer::onAnnoucementButton));
      annouceBtn->setPosition({infoButton->getPositionX(), infoButton->getPositionY() + 80});
      infoMenu->addChild(annouceBtn);
      m_newsIconBtn = annouceBtn;

      // @geode-ignore(unknown-resource)
      auto badgeSpr = CCSprite::createWithSpriteFrameName("geode.loader/updates-failed.png");
      if (badgeSpr) {
            // position top-right of the icon
            auto size = annouceSpr->getContentSize();
            badgeSpr->setScale(0.5f);
            badgeSpr->setPosition({size.width - 15, size.height - 15});
            badgeSpr->setVisible(false);
            annouceBtn->addChild(badgeSpr, 10);
            m_newsBadge = badgeSpr;
      }

      // check server announcement id and set badge visibility
      Ref<RLCreatorLayer> self = this;
      web::WebRequest()
          .get("https://gdrate.arcticwoof.xyz/getAnnoucement")
          .listen([self](web::WebResponse* res) {
                if (!self) return;
                if (!res || !res->ok()) return;
                auto jsonRes = res->json();
                if (!jsonRes) return;
                auto json = jsonRes.unwrap();
                int id = 0;
                if (json.contains("id")) {
                      if (auto i = json["id"].as<int>(); i) id = i.unwrap();
                }
                int saved = Mod::get()->getSavedValue<int>("annoucementId");
                if (id && id != saved) {
                      if (self->m_newsBadge) self->m_newsBadge->setVisible(true);
                } else {
                      if (self->m_newsBadge) self->m_newsBadge->setVisible(false);
                }
          });

      this->addChild(infoMenu);
      // credits button at the bottom right
      auto creditButtonSpr =
          CCSprite::create("RL_badgeMod01.png"_spr);
      creditButtonSpr->setScale(1.5f);
      auto creditButton = CCMenuItemSpriteExtra::create(
          creditButtonSpr, this, menu_selector(RLCreatorLayer::onCreditsButton));
      creditButton->setPosition({winSize.width - 25, 25});
      infoMenu->addChild(creditButton);

      // supporter button left side of the credits
      auto supportButtonSpr = CCSprite::create("RL_badgeSupporter.png"_spr);
      supportButtonSpr->setScale(1.5f);
      auto supportButton = CCMenuItemSpriteExtra::create(
          supportButtonSpr, this, menu_selector(RLCreatorLayer::onSupporterButton));
      supportButton->setPosition({creditButton->getPositionX(), creditButton->getPositionY() + 40});
      infoMenu->addChild(supportButton);

      // button
      if (Mod::get()->getSavedValue<int>("role") >= 1) {
            auto addDiagloueBtnSpr = CCSprite::createWithSpriteFrameName("GJ_secretLock4_small_001.png");
            addDiagloueBtnSpr->setOpacity(100);
            auto addDialogueBtn = CCMenuItemSpriteExtra::create(
                addDiagloueBtnSpr, this, menu_selector(RLCreatorLayer::onSecretDialogueButton));
            addDialogueBtn->setPosition({creditButton->getPositionX(), creditButton->getPositionY() + 80});
            infoMenu->addChild(addDialogueBtn);
      }

      // test the ground moving thingy :o
      // idk how gd actually does it correctly but this is close enough i guess
      m_bgContainer = CCNode::create();
      m_bgContainer->setContentSize(winSize);
      this->addChild(m_bgContainer, -7);

      if (Mod::get()->getSettingValue<bool>("disableBackground") == false) {
            std::string bgName = "game_bg_01_001.png";
            auto testBg = CCSprite::create(bgName.c_str());
            if (!testBg) {
                  testBg = CCSprite::create("game_bg_01_001.png");
            }
            if (testBg) {
                  float tileW = testBg->getContentSize().width;
                  int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
                  for (int i = 0; i < tiles; ++i) {
                        auto bgSpr = CCSprite::create(bgName.c_str());
                        if (!bgSpr) bgSpr = CCSprite::create("game_bg_01_001.png");
                        if (!bgSpr) continue;
                        bgSpr->setAnchorPoint({0.f, 0.f});
                        bgSpr->setPosition({i * tileW, 0.f});
                        bgSpr->setColor({40, 125, 255});
                        m_bgContainer->addChild(bgSpr);
                        m_bgTiles.push_back(bgSpr);
                  }
            }

            m_groundContainer = CCNode::create();
            m_groundContainer->setContentSize(winSize);
            this->addChild(m_groundContainer, -5);

            std::string groundName = "groundSquare_01_001.png";
            auto testGround = CCSprite::create(groundName.c_str());
            if (!testGround) testGround = CCSprite::create("groundSquare_01_001.png");
            if (testGround) {
                  float tileW = testGround->getContentSize().width;
                  int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
                  for (int i = 0; i < tiles; ++i) {
                        auto gSpr = CCSprite::create(groundName.c_str());
                        if (!gSpr) gSpr = CCSprite::create("groundSquare_01_001.png");
                        if (!gSpr) continue;
                        gSpr->setAnchorPoint({0.f, 0.f});
                        gSpr->setPosition({i * tileW, -70.f});
                        gSpr->setColor({0, 102, 255});
                        m_groundContainer->addChild(gSpr);
                        m_groundTiles.push_back(gSpr);
                  }
            }

            auto floorLineSpr = CCSprite::createWithSpriteFrameName("floorLine_01_001.png");
            floorLineSpr->setPosition({winSize.width / 2, 58});
            m_groundContainer->addChild(floorLineSpr, -4);
      }

      this->scheduleUpdate();
      this->setKeypadEnabled(true);
      return true;
}

void RLCreatorLayer::onSettingsButton(CCObject* sender) {
      openSettingsPopup(getMod());
}

void RLCreatorLayer::onDiscordButton(CCObject* sender) {
      utils::web::openLinkInBrowser("https://discord.gg/jBf2wfBgVT");
}

void RLCreatorLayer::onLayoutGauntlets(CCObject* sender) {
      auto gauntletSelect = RLGauntletSelectLayer::create();
      auto scene = CCScene::create();
      scene->addChild(gauntletSelect);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onSupporterButton(CCObject* sender) {
      auto donationPopup = RLDonationPopup::create();
      donationPopup->show();
}

void RLCreatorLayer::onSecretDialogueButton(CCObject* sender) {
      if (Mod::get()->getSavedValue<int>("role") < 1) {
            return;
      }
      auto dialogue = RLAddDialogue::create();
      dialogue->show();
}

void RLCreatorLayer::onAnnoucementButton(CCObject* sender) {
      // disable the button if provided to avoid spamming
      auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (menuItem) menuItem->setEnabled(false);

      Ref<RLCreatorLayer> self = this;
      web::WebRequest()
          .get("https://gdrate.arcticwoof.xyz/getAnnoucement")
          .listen([self, menuItem](web::WebResponse* res) {
                if (!self) return;
                if (!res || !res->ok()) {
                      Notification::create("Failed to fetch announcement", NotificationIcon::Error)->show();
                      if (menuItem) menuItem->setEnabled(true);
                      return;
                }

                auto jsonRes = res->json();
                if (!jsonRes) {
                      Notification::create("Invalid announcement response", NotificationIcon::Error)->show();
                      if (menuItem) menuItem->setEnabled(true);
                      return;
                }

                auto json = jsonRes.unwrap();
                std::string body = "";
                int id = 0;
                if (json.contains("body")) {
                      if (auto s = json["body"].asString(); s) body = s.unwrap();
                }
                if (json.contains("id")) {
                      if (auto i = json["id"].as<int>(); i) id = i.unwrap();
                }

                if (!body.empty()) {
                      MDPopup::create("Rated Layouts Annoucement", body.c_str(), "OK")->show();
                      if (id) {
                            Mod::get()->setSavedValue<int>("annoucementId", id);
                            // hide badge since the user just viewed the announcement
                            if (self->m_newsBadge) self->m_newsBadge->setVisible(false);
                      }
                } else {
                      Notification::create("No announcement available", NotificationIcon::Warning)->show();
                }

                if (self->m_newsBadge) {
                      self->m_newsBadge->setVisible(false);
                }

                if (menuItem) menuItem->setEnabled(true);
          });
}

void RLCreatorLayer::onUnknownButton(CCObject* sender) {
      // disable the button first to prevent spamming
      auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
      menuItem->setEnabled(false);
      // fetch dialogue from server and show it in a dialog
      Ref<RLCreatorLayer> self = this;
      web::WebRequest()
          .get("https://gdrate.arcticwoof.xyz/getDialogue")
          .listen([self, menuItem](web::WebResponse* res) {
                if (!self) return;
                std::string text = "...";  // default text
                if (res && res->ok()) {
                      auto jsonRes = res->json();
                      if (jsonRes) {
                            auto json = jsonRes.unwrap();
                            if (auto diag = json["dialogue"].asString(); diag) {
                                  text = diag.unwrap();
                            }
                      } else {
                            log::error("Failed to parse getDialogue response");
                            if (menuItem) menuItem->setEnabled(true);
                      }
                } else {
                      log::error("Failed to fetch dialogue");
                      if (menuItem) menuItem->setEnabled(true);
                }

                DialogObject* dialogObj = DialogObject::create(
                    "Layout Creator",
                    text.c_str(),
                    28,
                    1.f,
                    false,
                    ccWHITE);
                if (dialogObj) {
                      auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
                      dialog->addToMainScene();
                      dialog->animateInRandomSide();
                }
                menuItem->setEnabled(true);
          });
}

void RLCreatorLayer::onInfoButton(CCObject* sender) {
      MDPopup::create(
          "About Rated Layouts",
          "## <cl>Rated Layouts</cl> is a community-run rating system focusing on gameplay in layout levels.\n\n"
          "### Each of the buttons on this screen lets you browse different categories of rated layouts:\n\n"
          "<cg>**Featured Layouts**</c>: Featured layouts that showcase fun gameplay and visuals. Each featured levels are ranked based of their featured score.\n\n"
          "<cg>**Leaderboard**</c>: The top-rated players ranked by blueprint stars and creator points.\n\n"
          "<cg>**Layout Gauntlets**</c>: Special themed layouts hosted by the Rated Layouts Team. This holds the <cl>Layout Creator Contests</c>!\n\n"
          "<cg>**Sent Layouts**</c>: Suggested or sent layouts by the Layout Moderators. The community can vote on these layouts based of their Design, Difficulty and Gameplay. <co>(Only enabled if you have at least 20% in Normal Mode or 80% in Practice Mode)</c>\n\n"
          "<cg>**Search Layouts**</c>: Search for rated layouts by their level name/ID.\n\n"
          "<cg>**Event Layouts**</c>: Showcases time-limited Daily, Weekly and Monthly layouts picked by the <cr>Layout Admins</c>.\n\n"
          "### Join the <cb>[Rated Layouts Discord](https://discord.gg/jBf2wfBgVT)</c> server for more information and to submit your layouts for rating.\n\n",
          "OK")
          ->show();
}

void RLCreatorLayer::onCreditsButton(CCObject* sender) {
      auto creditsPopup = RLCreditsPopup::create();
      creditsPopup->show();
}

void RLCreatorLayer::onDailyLayouts(CCObject* sender) {
      auto dailyPopup = RLEventLayouts::create(RLEventLayouts::EventType::Daily);
      dailyPopup->show();
}

void RLCreatorLayer::onWeeklyLayouts(CCObject* sender) {
      auto weeklyPopup = RLEventLayouts::create(RLEventLayouts::EventType::Weekly);
      weeklyPopup->show();
}

void RLCreatorLayer::onMonthlyLayouts(CCObject* sender) {
      auto monthlyPopup = RLEventLayouts::create(RLEventLayouts::EventType::Monthly);
      monthlyPopup->show();
}

void RLCreatorLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(
          0.5f, PopTransition::kPopTransitionFade);
}

void RLCreatorLayer::onFeaturedLayouts(CCObject* sender) {
      Ref<RLCreatorLayer> self = this;
      web::WebRequest()
          .param("type", 2)
          .param("amount", 100)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([self](web::WebResponse* res) {
                if (!self) return;
                if (res && res->ok()) {
                      auto jsonResult = res->json();

                      if (jsonResult) {
                            auto json = jsonResult.unwrap();
                            std::string levelIDs;
                            bool first = true;

                            if (json.contains("levelIds")) {
                                  auto levelsArr = json["levelIds"];

                                  // iterate
                                  for (auto levelIDValue : levelsArr) {
                                        auto levelID = levelIDValue.as<int>();
                                        if (levelID) {
                                              if (!first)
                                                    levelIDs += ",";
                                              levelIDs += numToString(levelID.unwrap());
                                              first = false;
                                        }
                                  }
                            }

                            if (!levelIDs.empty()) {
                                  auto searchObject =
                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                  auto scene = CCScene::create();
                                  scene->addChild(browserLayer);
                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                            } else {
                                  log::warn("No levels found in response");
                                  Notification::create("No featured levels found",
                                                       NotificationIcon::Warning)
                                      ->show();
                            }
                      } else {
                            log::error("Failed to parse response JSON");
                      }
                } else {
                      log::error("Failed to fetch levels from server");
                      Notification::create("Failed to fetch levels from server",
                                           NotificationIcon::Error)
                          ->show();
                }
          });
}

void RLCreatorLayer::onSentLayouts(CCObject* sender) {
      // if the user is admin (role == 2), show a quickpopup to choose between type 1 or type 4
      if (Mod::get()->getSavedValue<int>("role") >= 2) {
            Ref<RLCreatorLayer> self = this;
            geode::createQuickPopup(
                "View Sent Layouts",
                "Choose which sent layouts to view:\n\n<cg>All Sent Layouts</c> or <co>Sent layouts with 3+ sends</c>.",
                "All",
                "3+ Sends",
                [self](auto, bool yes) {
                      if (!self) return;
                      int type = yes ? 4 : 1;

                      web::WebRequest()
                          .param("type", type)
                          .param("amount", 100)
                          .get("https://gdrate.arcticwoof.xyz/getLevels")
                          .listen([self](web::WebResponse* res) {
                                if (!self) return;
                                if (res && res->ok()) {
                                      auto jsonResult = res->json();

                                      if (jsonResult) {
                                            auto json = jsonResult.unwrap();
                                            std::string levelIDs;
                                            bool first = true;
                                            if (json.contains("levelIds")) {
                                                  auto levelsArr = json["levelIds"];

                                                  // iterate
                                                  for (auto levelIDValue : levelsArr) {
                                                        auto levelID = levelIDValue.as<int>();
                                                        if (levelID) {
                                                              if (!first)
                                                                    levelIDs += ",";
                                                              levelIDs += numToString(levelID.unwrap());
                                                              first = false;
                                                        }
                                                  }
                                            }

                                            if (!levelIDs.empty()) {
                                                  auto searchObject =
                                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                                  auto scene = CCScene::create();
                                                  scene->addChild(browserLayer);
                                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                                            } else {
                                                  log::warn("No levels found in response");
                                                  Notification::create("No sent layouts found",
                                                                       NotificationIcon::Warning)
                                                      ->show();
                                            }
                                      } else {
                                            log::error("Failed to parse response JSON");
                                      }
                                } else {
                                      log::error("Failed to fetch levels from server");
                                      Notification::create("Failed to fetch levels from server",
                                                           NotificationIcon::Error)
                                          ->show();
                                }
                          });
                });

            return;
      }
      // normal user, just show type 1
      Ref<RLCreatorLayer> self = this;
      web::WebRequest()
          .param("type", 1)
          .param("amount", 100)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([self](web::WebResponse* res) {
                if (!self) return;
                if (res && res->ok()) {
                      auto jsonResult = res->json();

                      if (jsonResult) {
                            auto json = jsonResult.unwrap();
                            std::string levelIDs;
                            bool first = true;
                            if (json.contains("levelIds")) {
                                  auto levelsArr = json["levelIds"];

                                  // iterate
                                  for (auto levelIDValue : levelsArr) {
                                        auto levelID = levelIDValue.as<int>();
                                        if (levelID) {
                                              if (!first)
                                                    levelIDs += ",";
                                              levelIDs += numToString(levelID.unwrap());
                                              first = false;
                                        }
                                  }
                            }

                            if (!levelIDs.empty()) {
                                  auto searchObject =
                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                  auto scene = CCScene::create();
                                  scene->addChild(browserLayer);
                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                            } else {
                                  log::warn("No levels found in response");
                                  Notification::create("No sent layouts found",
                                                       NotificationIcon::Warning)
                                      ->show();
                            }
                      } else {
                            log::error("Failed to parse response JSON");
                      }
                } else {
                      log::error("Failed to fetch levels from server");
                      Notification::create("Failed to fetch levels from server",
                                           NotificationIcon::Error)
                          ->show();
                }
          });
}

void RLCreatorLayer::keyBackClicked() { this->onBackButton(nullptr); }

void RLCreatorLayer::update(float dt) {
      // scroll background tiles
      if (m_bgTiles.size()) {
            float move = m_bgSpeed * dt;
            int num = static_cast<int>(m_bgTiles.size());
            for (auto spr : m_bgTiles) {
                  if (!spr)
                        continue;
                  float tileW = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -tileW) {
                        x += tileW * num;
                  }
                  spr->setPositionX(x);
            }
      }

      // scroll ground tiles
      if (m_groundTiles.size()) {
            float move = m_groundSpeed * dt;
            int num = static_cast<int>(m_groundTiles.size());
            for (auto spr : m_groundTiles) {
                  if (!spr)
                        continue;
                  float tileW = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -tileW) {
                        x += tileW * num;
                  }
                  spr->setPositionX(x);
            }
      }

      // move background decorations (same speed as ground)
      if (!m_bgDecorations.empty()) {
            float move = m_groundSpeed * dt;
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            for (auto spr : m_bgDecorations) {
                  if (!spr) continue;
                  float w = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -w) {
                        // wrap by columns so sprite stays in grid alignment
                        int cols = m_decoCols ? m_decoCols : static_cast<int>(ceil(winSize.width / m_decoGridX)) + 2;
                        x += cols * m_decoGridX;
                  }
                  spr->setPositionX(x);
            }
      }
}

void RLCreatorLayer::onLeaderboard(CCObject* sender) {
      auto leaderboardLayer = RLLeaderboardLayer::create();
      auto scene = CCScene::create();
      scene->addChild(leaderboardLayer);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onSearchLayouts(CCObject* sender) {
      auto searchLayer = RLSearchLayer::create();
      auto scene = CCScene::create();
      scene->addChild(searchLayer);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}

RLCreatorLayer* RLCreatorLayer::create() {
      auto ret = new RLCreatorLayer();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}
