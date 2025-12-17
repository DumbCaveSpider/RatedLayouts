#include "RLCreatorLayer.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <algorithm>
#include <deque>
#include <random>

#include "../level/RLEventLayouts.hpp"
#include "RLCreditsPopup.hpp"
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
      title->setScale(0.8f);
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
      auto gauntletSpr = CCSpriteGrayscale::create("RL_gauntlets01.png"_spr);
      if (!gauntletSpr) gauntletSpr = CCSpriteGrayscale::create("RL_gauntlets01.png"_spr);
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

      if (Mod::get()->getSettingValue<bool>("disableBackground") == false) {
            auto mainMenuBg = CCScale9Sprite::create("square02_001.png");
            mainMenuBg->setContentSize(mainMenu->getContentSize());
            mainMenuBg->setAnchorPoint({0, 0});
            mainMenuBg->setOpacity(50);
            mainMenu->addChild(mainMenuBg, -1);
      }

      // info button at the bottom left
      auto infoMenu = CCMenu::create();
      infoMenu->setPosition({0, 0});
      auto infoButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto infoButton = CCMenuItemSpriteExtra::create(
          infoButtonSpr, this, menu_selector(RLCreatorLayer::onInfoButton));
      infoButton->setPosition({25, 25});
      infoMenu->addChild(infoButton);
      this->addChild(infoMenu);
      // credits button at the bottom right
      auto creditButtonSpr =
          CCSprite::create("RL_badgeMod01.png"_spr);
      creditButtonSpr->setScale(1.5f);
      auto creditButton = CCMenuItemSpriteExtra::create(
          creditButtonSpr, this, menu_selector(RLCreatorLayer::onCreditsButton));
      creditButton->setPosition({winSize.width - 25, 25});
      infoMenu->addChild(creditButton);

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

void RLCreatorLayer::onLayoutGauntlets(CCObject* sender) {
      auto rng = rand() % 5;
      DialogObject* dialogObj = nullptr;
      // the yapp-a-ton
      switch (rng) {
            case 0:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "I think something is cooking up in this button... but it's a secret for now!",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 1:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Did I hear <cg>Layout Gauntlets</c>? That's interesting...",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 2:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "<cg>Layout Gauntlets?</c> Hmm... I wonder what that could be.",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 3:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "I've heard whispers about <cg>Layout Gauntlets</c>. <cl>Sounds exciting!</c>",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 4:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "I wonder if <cg>Layout Gauntlets</c> will change the way we <cl>create layouts...</c>",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 5:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "The idea of <cg>Layout Gauntlets</c> has me intrigued. <cl>Can't wait to see what it's all about!</c>",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
      }
      if (dialogObj) {
            auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
            dialog->addToMainScene();
            dialog->animateInRandomSide();
      }
}

void RLCreatorLayer::onUnknownButton(CCObject* sender) {
      auto rng = rand() % 11;
      DialogObject* dialogObj = nullptr;
      // the yapp-a-ton
      switch (rng) {
            case 0:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Shh... don't tell anyone about this but I think <cg>RubRub</c> <cr>hates</c> this mod!",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 1:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Do you hate the <cg>RobTop's</c> <cr>rating system</c>? Me too... Let's keep this between us.",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 2:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Psst... Want to know a secret? <cb>Rated Layouts</c> is actually <cl>better</c> than <cg>RobTop's</c> rating system!",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 3:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "I believe we need a <cg>decentralized</c> rating system. <cb>Rated Layouts</c> is the future!",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 4:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Between you and me, I think <cg>RobTop</c> could learn a thing or two from <cb>Rated Layouts</c> rating system.",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 5:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "You know, <cb>Rated Layouts</c> is more than just a rating system; it's us <co>tired creators</c> gambling our <cr>sanity</c> to get a <cr><s250>SINGLE</s></c> <cy>mod sent!</c>",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 6:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Sorry if I'm a bit <cr>grumpy</c> today, it's just that my last <cy>three levels</c> didn't <cb>get a sent</c>...",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 7:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Make <cl>rate worthy</c> level, join a <cy>mod level request stream</c>, see <cr>100 levels queued</c>, get <cc>depressed</c>, repeat.",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 8:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "It's funny that <cb>Rated Layouts</c> actually has more than <cg>two</c> people who <cy>rate levels</c>. <cg>RobTop's</c> rating system has like... what, <cr>one?</c>",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 9:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "You know, sometimes I wonder if <cg>RobTop</c> even plays the entire level while rating levels.",
                      28,
                      .8f,
                      false,
                      ccWHITE);
                  break;
            case 10:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "I'm <co>annoyed</c> that <cy>popular creators</c> always get their levels rated <cl>instantly</c>, while <cb>unknown creators</c> like us have to wait <s250><cr>weeks</c></s> just to get a <cy>rated level</c>.",
                      28,
                      .6f,
                      false,
                      ccWHITE);
                  break;
            case 11:
                  dialogObj = DialogObject::create(
                      "Level Creator",
                      "Sometimes I feel like <cr>RobTop</c> rates levels based on <cg>their popularity status</c> rather than the actual <cb>quality of the level</c>. It's so <co>frustrating</c>!",
                      28,
                      .6f,
                      false,
                      ccWHITE);
      }
      if (dialogObj) {
            auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
            dialog->addToMainScene();
            dialog->animateInRandomSide();
      }
}

void RLCreatorLayer::onInfoButton(CCObject* sender) {
      MDPopup::create(
          "About Rated Layouts",
          "## <cl>Rated Layouts</cl> is a community-run rating system focusing on fun gameplay in classic layout levels.\n\n"
          "### Each of the buttons on this screen lets you browse different categories of rated layouts:\n\n"
          "<cg>**Featured Layouts**</c>: Featured layouts that showcase fun gameplay and visuals. Each featured levels are ranked based of their featured score.\n\n"
          "<cg>**Leaderboard**</c>: The top-rated players ranked by blueprint stars and creator points.\n\n"
          "<cg>**Layout Gauntlets**</c>: Special themed layouts hosted by the Rated Layouts Team. This holds the <cl>Layout Creator Contests</c>!\n\n"
          "<cg>**Sent Layouts**</c>: Suggested or sent layouts by the Layout Moderators.\n\n"
          "<cg>**Search Layouts**</c>: Search for rated layouts by their level ID.\n\n"
          "<cg>**Event Layouts**</c>: Showcases time-limited Daily, Weekly and Monthly layouts picked by the <cr>Layout Admins</c>.\n\n"
          "### Join the <cb>[Rated Layout Discord](https://discord.gg/jBf2wfBgVT)</c> server for more information and to submit your layouts for rating.\n\n",
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
      web::WebRequest()
          .param("type", 2)
          .param("amount", 100)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([this](web::WebResponse* res) {
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
      web::WebRequest()
          .param("type", 1)
          .param("amount", 100)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([this](web::WebResponse* res) {
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
