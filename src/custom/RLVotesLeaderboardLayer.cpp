#include "RLVotesLeaderboardLayer.hpp"
#include "../custom/RLAchievements.hpp"
#include <cue/RepeatingBackground.hpp>

bool RLVotesLeaderboardLayer::init() {
  if (!CCLayer::init())
    return false;

  auto winSize = CCDirector::sharedDirector()->getWinSize();

  // create if moving bg disabled
  if (Mod::get()->getSettingValue<bool>("disableBackground") == true) {
    auto bg = createLayerBG();
    bg->setColor(
        Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground"));
    addChild(bg, -1);
  } else {
    auto value = Mod::get()->getSettingValue<int>("backgroundType");
    std::string bgIndex = (value >= 1 && value <= 9)
                              ? ("0" + numToString(value))
                              : numToString(value);
    std::string bgName = "game_bg_" + bgIndex + "_001.png";
    auto bg = cue::RepeatingBackground::create(bgName.c_str(), 1.f,
                                               cue::RepeatMode::X);
    bg->setColor(
        Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground"));
    addChild(bg, -1);
  }

  addSideArt(this, SideArt::All, SideArtStyle::Layer, false);

  auto backMenu = CCMenu::create();
  backMenu->setPosition({0, 0});

  addBackButton(this, BackButtonStyle::Pink);

  this->fetchLeaderboard(100);

  auto listLayer = GJListLayer::create(nullptr, "Community Vote Leaderboard",
                                       {191, 114, 62, 255}, 356.f, 220.f, 0);
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
    listLayer->addChild(spinner);
    m_spinner = spinner;
  }

  this->addChild(listLayer);
  m_listLayer = listLayer;
  m_scrollLayer = scrollLayer;

  // info button at the bottom left
  auto infoMenu = CCMenu::create();
  infoMenu->setPosition({0, 0});
  auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoButtonSpr->setScale(0.7f);
  auto infoButton = CCMenuItemSpriteExtra::create(
      infoButtonSpr, this,
      menu_selector(RLVotesLeaderboardLayer::onInfoButton));
  infoButton->setPosition({25, 25});
  infoMenu->addChild(infoButton);
  this->addChild(infoMenu);

  // refresh button at the bottom right
  auto refreshSpr = CCSprite::createWithSpriteFrameName("RL_refresh01.png"_spr);
  m_refreshBtn = CCMenuItemSpriteExtra::create(
      refreshSpr, this,
      menu_selector(RLVotesLeaderboardLayer::onRefreshButton));
  m_refreshBtn->setPosition({winSize.width - 35, 35});
  infoMenu->addChild(m_refreshBtn);

  this->scheduleUpdate();
  this->setKeypadEnabled(true);

  return true;
}

void RLVotesLeaderboardLayer::onInfoButton(CCObject *sender) {
  MDPopup::create(
      "Rated Layouts Votes Leaderboard",
      "This leaderboard shows the top <cg>Community Voters</c> in "
      "<cl>Rated Layouts</c>.\n"
      "Each votes ties to <cc>each level they have voted on</c>. Although "
      "this leaderboard isn't <cr>moderated or tracked</c> as closely as "
      "the main leaderboard.\n"
      "Same punishment applies if cheating on this leaderboard as the main "
      "one, <co>which include you getting banned in the main leaderboard</c>.",
      "OK")
      ->show();
}

void RLVotesLeaderboardLayer::onAccountClicked(CCObject *sender) {
  auto button = static_cast<CCMenuItem *>(sender);
  int accountId = button->getTag();
  ProfilePage::create(accountId, false)->show();
}

void RLVotesLeaderboardLayer::onRefreshButton(CCObject *sender) {
  auto contentLayer = m_scrollLayer ? m_scrollLayer->m_contentLayer : nullptr;
  if (contentLayer) {
    // clear the list and show a spinner
    contentLayer->removeAllChildrenWithCleanup(true);
    if (m_spinner) {
      m_spinner->removeFromParent();
      m_spinner = nullptr;
    }
    auto spinner = LoadingSpinner::create(100.f);
    spinner->setPosition(m_listLayer->getContentSize() / 2);
    m_listLayer->addChild(spinner);
    m_spinner = spinner;
  }

  this->fetchLeaderboard(100);
}

void RLVotesLeaderboardLayer::fetchLeaderboard(int amount) {
  Ref<RLVotesLeaderboardLayer> self = this;
  auto request = web::WebRequest().param("amount", amount);
  async::spawn(
      request.get("https://gdrate.arcticwoof.xyz/getVotesLeaderboard"),
      [self](web::WebResponse response) {
        if (!self)
          return;
        if (!response.ok()) {
          log::warn("Server returned non-ok status: {}", response.code());
          Notification::create("Failed to fetch leaderboard",
                               NotificationIcon::Error)
              ->show();
          return;
        }

        auto jsonRes = response.json();
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

        if (json.contains("leaderboard") && json["leaderboard"].isArray()) {
          auto users = json["leaderboard"].asArray().unwrap();
          self->populateLeaderboard(users);
          if (self->m_scrollLayer)
            self->m_scrollLayer->scrollToTop();
        } else if (json.contains("users") && json["users"].isArray()) {
          auto users = json["users"].asArray().unwrap();
          self->populateLeaderboard(users);
          if (self->m_scrollLayer)
            self->m_scrollLayer->scrollToTop();
        } else {
          log::warn("No leaderboard/users array in response");
        }
      });
}

void RLVotesLeaderboardLayer::populateLeaderboard(
    const std::vector<matjson::Value> &users) {
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
  for (const auto &userValue : users) {
    if (!userValue.isObject())
      continue;

    int accountId = userValue["accountId"].asInt().unwrapOrDefault();
    int score = userValue["votes"].asInt().unwrapOrDefault();
    int nameplateId = userValue["nameplate"].asInt().unwrapOr(0);

    auto cell = TableViewCell::create();
    cell->setContentSize({356.f, 40.f});

    CCSprite *bgSprite = nullptr;
    CCSprite *namePlate = nullptr;
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
      bgSprite->setOpacity(150);
      cell->addChild(bgSprite, 0);
    }

    CCSprite *nameplate = nullptr;
    if (nameplateId != 0 &&
        !Mod::get()->getSettingValue<bool>("disableNameplate")) {
      nameplate = CCSprite::createWithSpriteFrameName(
          fmt::format("nameplate_{}.png"_spr, nameplateId).c_str());
      if (nameplate) {
        nameplate->setPosition(
            {bgSprite->getPositionX() - 10, bgSprite->getPositionY()});
        cell->addChild(nameplate, -1);
      }
    }

    // glow for top 3
    if (rank == 1) {
      auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
      glow->setPosition({100.f, 40.5f});
      glow->setRotation(90);
      glow->setAnchorPoint({0.f, 0.5f});
      glow->setScale(5.f);
      glow->setColor({255, 215, 0});
      cell->addChild(glow, 1);
    } else if (rank == 2) {
      auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
      glow->setPosition({100.f, 40.5f});
      glow->setRotation(90);
      glow->setAnchorPoint({0.f, 0.5f});
      glow->setScale(5.f);
      glow->setColor({192, 192, 192});
      cell->addChild(glow, 1);
    } else if (rank == 3) {
      auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
      glow->setPosition({100.f, 40.5f});
      glow->setRotation(90);
      glow->setAnchorPoint({0.f, 0.5f});
      glow->setScale(5.f);
      glow->setColor({205, 127, 50});
      cell->addChild(glow, 1);
    }

    // Rank label
    auto rankLabel =
        CCLabelBMFont::create(fmt::format("{}", rank).c_str(), "goldFont.fnt");
    rankLabel->setScale(0.5f);
    rankLabel->setPosition({15.f, 20.f});
    rankLabel->setAnchorPoint({0.f, 0.5f});
    cell->addChild(rankLabel, 2);

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
    if (color3 != 0) { // no color 3? no glow
      player->setGlowOutline(gm->colorForIdx(color3));
    }
    player->setPosition({55.f, 20.f});
    player->setScale(0.75f);
    cell->addChild(player, 2);

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({0, 0});

    auto accountButton = CCMenuItemSpriteExtra::create(
        accountLabel, this,
        menu_selector(RLVotesLeaderboardLayer::onAccountClicked));
    accountButton->setTag(accountId);
    accountButton->setPosition({80.f, 20.f});
    accountButton->setAnchorPoint({0.f, 0.5f});

    buttonMenu->addChild(accountButton);
    cell->addChild(buttonMenu, 2);

    auto scoreLabelText = CCLabelBMFont::create(
        fmt::format("{}", GameToolbox::pointsToString(score)).c_str(),
        "bigFont.fnt");
    scoreLabelText->setScale(0.5f);
    scoreLabelText->setPosition({320.f, 20.f});
    scoreLabelText->setAnchorPoint({1.f, 0.5f});
    cell->addChild(scoreLabelText, 2);

    const char *iconName = "RL_commVote01.png"_spr;
    auto iconSprite = CCSprite::createWithSpriteFrameName(iconName);
    iconSprite->setScale(0.65f);
    iconSprite->setPosition({325.f, 20.f});
    iconSprite->setAnchorPoint({0.f, 0.5f});
    cell->addChild(iconSprite, 2);

    contentLayer->addChild(cell);
    rank++;
  }

  // Update layout after all cells are added
  contentLayer->updateLayout();
}

RLVotesLeaderboardLayer *RLVotesLeaderboardLayer::create() {
  auto ret = new RLVotesLeaderboardLayer();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

void RLVotesLeaderboardLayer::keyBackClicked() {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}
