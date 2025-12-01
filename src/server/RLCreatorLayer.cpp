#include "RLCreatorLayer.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <algorithm>
#include <deque>
#include <random>

#include "LevelSearchLayer.cpp"
#include "RLCreditsPopup.hpp"
#include "RLLeaderboardLayer.hpp"

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
      auto backButton = CCMenuItemSpriteExtra::create(
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
      mainMenu->setContentSize({350.f, 240.f});
      mainMenu->setLayout(RowLayout::create()
                              ->setGap(6.f)
                              ->setGrowCrossAxis(true)
                              ->setCrossAxisOverflow(false));

      this->addChild(mainMenu);

      auto title = CCSprite::create("RL_title.png"_spr);
      title->setPosition({winSize.width / 2, winSize.height / 2 + 130});
      title->setScale(0.8f);
      this->addChild(title);

      auto featuredSpr = CCSprite::create("RL_featuredBtn.png"_spr);
      if (!featuredSpr) featuredSpr = CCSprite::create("RL_featuredBtn.png"_spr);
      auto featuredItem = CCMenuItemSpriteExtra::create(
          featuredSpr, this, menu_selector(RLCreatorLayer::onFeaturedLayouts));
      featuredItem->setID("featured-button");
      mainMenu->addChild(featuredItem);

      auto leaderboardSpr = CCSprite::create("RL_leaderboardBtn.png"_spr);
      if (!leaderboardSpr) leaderboardSpr = CCSprite::create("RL_leaderboardBtn.png"_spr);
      auto leaderboardItem = CCMenuItemSpriteExtra::create(
          leaderboardSpr, this, menu_selector(RLCreatorLayer::onLeaderboard));
      leaderboardItem->setID("leaderboard-button");
      mainMenu->addChild(leaderboardItem);

      auto newlySpr = CCSprite::create("RL_newRatedBtn.png"_spr);
      if (!newlySpr) newlySpr = CCSprite::create("RL_newRatedBtn.png"_spr);
      auto newlyItem = CCMenuItemSpriteExtra::create(
          newlySpr, this, menu_selector(RLCreatorLayer::onNewRated));
      newlyItem->setID("newly-rated-button");
      mainMenu->addChild(newlyItem);

      auto sendSpr = CCSprite::create("RL_sendLayoutsBtn.png"_spr);
      if (!sendSpr) sendSpr = CCSprite::create("RL_sendLayoutsBtn.png"_spr);
      auto sendItem = CCMenuItemSpriteExtra::create(
          sendSpr, this, menu_selector(RLCreatorLayer::onSendLayouts));
      sendItem->setID("send-layouts-button");
      mainMenu->addChild(sendItem);

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
            mainMenuBg->setOpacity(150);
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
          CCSprite::create("rlBadgeMod.png"_spr);
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
                      "Who ever made this <cl>new rating system</c> is a <cg>genius</c>! Finally, you can get the <co>recognition</c> you deserve!",
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
          "<cg>**Featured Layouts**</cg>: Featured layouts that showcase fun gameplay and visuals. Each featured levels are ranked based of their featured score.\n\n"
          "<cg>**Newly Rated**</cg>: The latest layouts that have been rated by the Layout Admins.\n\n"
          "<cg>**Sent Layouts**</cg>: Suggested or sent layouts by the Layout Moderators.\n\n"
          "You can also view the <cg>**Leaderboard**</cg> to see top-rated layout creators and players!\n\n"
          "### Join the <cb>[Rated Layout Discord](https://discord.gg/jBf2wfBgVT)</cb> server for more information and to submit your layouts for rating.\n\n",
          "OK")
          ->show();
}

void RLCreatorLayer::onCreditsButton(CCObject* sender) {
      auto creditsPopup = RLCreditsPopup::create();
      creditsPopup->show();
}

void RLCreatorLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(
          0.5f, PopTransition::kPopTransitionFade);
}

void RLCreatorLayer::onLeaderboard(CCObject* sender) {
      auto leaderboardLayer = RLLeaderboardLayer::create();
      auto scene = CCScene::create();
      scene->addChild(leaderboardLayer);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onFeaturedLayouts(CCObject* sender) {
      web::WebRequest()
          .param("type", 2)
          .param("amount", 1000)
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

void RLCreatorLayer::onNewRated(CCObject* sender) {
      web::WebRequest()
          .param("type", 3)
          .param("amount", 1000)
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
                                  Notification::create("No levels found",
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

void RLCreatorLayer::onSendLayouts(CCObject* sender) {
      web::WebRequest()
          .param("type", 1)
          .param("amount", 1000)
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
                                  Notification::create("No send layouts found",
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

RLCreatorLayer* RLCreatorLayer::create() {
      auto ret = new RLCreatorLayer();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}
