#include "RLLeaderboardLayer.hpp"

bool RLLeaderboardLayer::init() {
      if (!CCLayer::init())
            return false;

      auto winSize = CCDirector::sharedDirector()->getWinSize();

      auto bg = createLayerBG();
      if (bg)
            this->addChild(bg);

      addSideArt(this, SideArt::All, SideArtStyle::Layer, false);

      auto backMenu = CCMenu::create();
      backMenu->setPosition({0, 0});

      auto backButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
      auto backButton = CCMenuItemSpriteExtra::create(
          backButtonSpr, this, menu_selector(RLLeaderboardLayer::onBackButton));
      backButton->setPosition({25, winSize.height - 25});
      backMenu->addChild(backButton);
      this->addChild(backMenu);
      auto typeMenu = CCMenu::create();
      typeMenu->setPosition({0, 0});

      auto starsTab = TabButton::create(
          "Top Stars", this,
          menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
      starsTab->setTag(1);
      starsTab->toggle(true);
      starsTab->setPosition({winSize.width / 2 - 80, winSize.height - 27});
      typeMenu->addChild(starsTab);
      m_starsTab = starsTab;

      auto creatorTab = TabButton::create(
          "Top Creator", this,
          menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
      creatorTab->setTag(2);
      creatorTab->toggle(false);
      creatorTab->setPosition({winSize.width / 2 + 80, winSize.height - 27});
      typeMenu->addChild(creatorTab);
      m_creatorTab = creatorTab;

      this->addChild(typeMenu);
      this->fetchLeaderboard(1, 100);

      auto listLayer = GJListLayer::create(nullptr, nullptr, {191, 114, 62, 255},
                                           356.f, 220.f, 0);
      listLayer->setPosition(
          {winSize / 2 - listLayer->getScaledContentSize() / 2 - 5});

      auto scrollLayer = ScrollLayer::create(
          {listLayer->getContentSize().width, listLayer->getContentSize().height});
      scrollLayer->setPosition({0, 0});
      listLayer->addChild(scrollLayer);

      auto contentLayer = scrollLayer->m_contentLayer;
      if (contentLayer) {
            auto layout = ColumnLayout::create();
            contentLayer->setLayout(layout);
            layout->setGap(0.f);
            layout->setAutoGrowAxis(0.f);
            layout->setAxisReverse(true);

            auto spinner = LoadingSpinner::create(100.f);
            spinner->setPosition(contentLayer->getContentSize() / 2);
            contentLayer->addChild(spinner);
            m_spinner = spinner;
      }

      this->addChild(listLayer);
      m_listLayer = listLayer;
      m_scrollLayer = scrollLayer;

      // info button at the bottom left
      auto infoMenu = CCMenu::create();
      infoMenu->setPosition({0, 0});
      auto infoButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto infoButton = CCMenuItemSpriteExtra::create(
          infoButtonSpr, this, menu_selector(RLLeaderboardLayer::onInfoButton));
      infoButton->setPosition({25, 25});
      infoMenu->addChild(infoButton);
      this->addChild(infoMenu);

      this->setKeypadEnabled(true);

      return true;
}

void RLLeaderboardLayer::onInfoButton(CCObject* sender) {
      MDPopup::create(
          "Rated Layouts Leaderboard",
          "The leaderboard shows the top players in <cb>Rated Layouts</cb> based "
          "on <cl>Stars</cl> or <cl>Creator Points</cl>.",
          "OK")
          ->show();
}

void RLLeaderboardLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(
          0.5f, PopTransition::kPopTransitionFade);  // im such a robtop doing this
                                                     // fancy pop scene :D
}

void RLLeaderboardLayer::keyBackClicked() { this->onBackButton(nullptr); }

void RLLeaderboardLayer::onAccountClicked(CCObject* sender) {
      auto button = static_cast<CCMenuItem*>(sender);
      int accountId = button->getTag();
      ProfilePage::create(accountId, false)->show();
}

void RLLeaderboardLayer::onLeaderboardTypeButton(CCObject* sender) {
      auto button = static_cast<TabButton*>(sender);
      int type = button->getTag();

      if (type == 1 && !m_starsTab->isToggled()) {
            m_starsTab->toggle(true);
            m_creatorTab->toggle(false);
      } else if (type == 2 && !m_creatorTab->isToggled()) {
            m_starsTab->toggle(false);
            m_creatorTab->toggle(true);
      }

      auto contentLayer = m_scrollLayer->m_contentLayer;
      if (contentLayer && !m_spinner) {
            auto spinner = LoadingSpinner::create(100.f);
            spinner->setPosition(m_listLayer->getContentSize() / 2);
            m_listLayer->addChild(spinner);
            m_spinner = spinner;
      }

      this->fetchLeaderboard(type, 100);
}

void RLLeaderboardLayer::fetchLeaderboard(int type, int amount) {
      web::WebRequest()
          .param("type", type)
          .param("amount", amount)
          .get("https://gdrate.arcticwoof.xyz/getScore")
          .listen([this](web::WebResponse* response) {
                if (!response->ok()) {
                      log::warn("Server returned non-ok status: {}", response->code());
                      Notification::create("Failed to fetch leaderboard",
                                           NotificationIcon::Error)
                          ->show();
                      return;
                }

                auto jsonRes = response->json();
                if (!jsonRes) {
                      log::warn("Failed to parse JSON response");
                      Notification::create("Invalid server response",
                                           NotificationIcon::Error)
                          ->show();
                      return;
                }

                auto json = jsonRes.unwrap();
                log::info("Leaderboard data: {}", json.dump());

                bool success = json["success"].asBool().unwrapOrDefault();
                if (!success) {
                      log::warn("Server returned success: false");
                      return;
                }

                if (json.contains("users") && json["users"].isArray()) {
                      auto users = json["users"].asArray().unwrap();
                      this->populateLeaderboard(users);
                      this->m_scrollLayer->scrollToTop();
                } else {
                      log::warn("No users array in response");
                }
          });
}

void RLLeaderboardLayer::populateLeaderboard(
    const std::vector<matjson::Value>& users) {
      if (!m_scrollLayer)
            return;

      auto contentLayer = m_scrollLayer->m_contentLayer;
      if (!contentLayer)
            return;

      if (m_spinner) {
            m_spinner->removeFromParent();
            m_spinner = nullptr;
      }

      contentLayer->removeAllChildrenWithCleanup(true);

      int rank = 1;
      for (const auto& userValue : users) {
            if (!userValue.isObject())
                  continue;

            int accountId = userValue["accountId"].asInt().unwrapOrDefault();
            int score = userValue["score"].asInt().unwrapOrDefault();

            auto cell = TableViewCell::create();
            cell->setContentSize({356.f, 40.f});

            CCSprite* bgSprite = nullptr;
            int currentAccountID = GJAccountManager::sharedState()->m_accountID;

            if (accountId == currentAccountID) {
                  bgSprite = CCSprite::create();
                  bgSprite->setTextureRect(CCRectMake(0, 0, 356.f, 40.f));
                  bgSprite->setColor({230, 150, 10});
            } else if (rank % 2 == 1) {
                  bgSprite = CCSprite::create();
                  bgSprite->setTextureRect(CCRectMake(0, 0, 356.f, 40.f));
                  bgSprite->setColor({161, 88, 44});
            } else {
                  bgSprite = CCSprite::create();
                  bgSprite->setTextureRect(CCRectMake(0, 0, 356.f, 40.f));
                  bgSprite->setColor({194, 114, 62});
            }

            if (bgSprite) {
                  bgSprite->setPosition({178.f, 20.f});
                  cell->addChild(bgSprite, 0);
            }

            // Rank label
            auto rankLabel =
                CCLabelBMFont::create(fmt::format("{}", rank).c_str(), "goldFont.fnt");
            rankLabel->setScale(0.5f);
            rankLabel->setPosition({15.f, 20.f});
            rankLabel->setAnchorPoint({0.f, 0.5f});
            cell->addChild(rankLabel);

            auto username = userValue["username"].asString().unwrapOrDefault();
            auto accountLabel = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
            accountLabel->setAnchorPoint({0.f, 0.5f});
            accountLabel->setScale(0.7f);

            int iconId = userValue["iconid"].asInt().unwrapOrDefault();
            int color1 = userValue["color1"].asInt().unwrapOrDefault();
            int color2 = userValue["color2"].asInt().unwrapOrDefault();
            int color3 = userValue["color3"].asInt().unwrapOrDefault();

            auto gm = GameManager::sharedState();
            auto player = SimplePlayer::create(iconId);
            player->updatePlayerFrame(iconId, IconType::Cube);
            player->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
            if (color3 != 0) {  // no color 3? no glow
                  player->setGlowOutline(gm->colorForIdx(color3));
            }
            player->setPosition({55.f, 20.f});
            player->setScale(0.75f);
            cell->addChild(player);

            auto buttonMenu = CCMenu::create();
            buttonMenu->setPosition({0, 0});

            auto accountButton = CCMenuItemSpriteExtra::create(
                accountLabel, this,
                menu_selector(RLLeaderboardLayer::onAccountClicked));
            accountButton->setTag(accountId);
            accountButton->setPosition({80.f, 20.f});
            accountButton->setAnchorPoint({0.f, 0.5f});

            buttonMenu->addChild(accountButton);
            cell->addChild(buttonMenu);

            auto scoreLabelText =
                CCLabelBMFont::create(fmt::format("{}", score).c_str(), "bigFont.fnt");
            scoreLabelText->setScale(0.5f);
            scoreLabelText->setPosition({320.f, 20.f});
            scoreLabelText->setAnchorPoint({1.f, 0.5f});
            cell->addChild(scoreLabelText);

            const bool isStar = m_starsTab->isToggled();
            const char* iconName = isStar ? "rlStarIcon.png"_spr : "rlhammerIcon.png"_spr;
            auto iconSprite = CCSprite::create(iconName);
            float iconScale = isStar ? 1.0f : 0.65f;
            iconSprite->setScale(iconScale);
            iconSprite->setPosition({325.f, 20.f});
            iconSprite->setAnchorPoint({0.f, 0.5f});
            cell->addChild(iconSprite);

            contentLayer->addChild(cell);
            rank++;
      }

      // Update layout after all cells are added
      contentLayer->updateLayout();
}

RLLeaderboardLayer* RLLeaderboardLayer::create() {
      auto ret = new RLLeaderboardLayer();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}
