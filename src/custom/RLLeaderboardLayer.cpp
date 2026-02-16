#include "RLLeaderboardLayer.hpp"
#include "../custom/RLAchievements.hpp"
#include <cue/RepeatingBackground.hpp>

bool RLLeaderboardLayer::init() {
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
    listLayer->addChild(spinner);
    m_spinner = spinner;
  }

  this->addChild(listLayer);
  m_listLayer = listLayer;
  m_scrollLayer = scrollLayer;

  auto typeMenu = CCMenu::create();
  typeMenu->setPosition({0, 0});
  typeMenu->setContentSize(listLayer->getContentSize());

  auto starsTab = TabButton::create(
      TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Sparks",
      this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
  float centerX = listLayer->getContentSize().width / 2.f;
  const float tabY = 247.f;
  const float spacing = 100.f;

  starsTab->setTag(1);
  starsTab->toggle(true);
  starsTab->setPosition({centerX - 1.5f * spacing, tabY});
  typeMenu->addChild(starsTab);
  m_starsTab = starsTab;

  auto planetsTab = TabButton::create(
      TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Planets",
      this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
  planetsTab->setTag(3);
  planetsTab->toggle(false);
  planetsTab->setPosition({centerX - 0.5f * spacing, tabY});
  typeMenu->addChild(planetsTab);
  m_planetsTab = planetsTab;

  auto creatorTab = TabButton::create(
      TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Creator",
      this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
  creatorTab->setTag(2);
  creatorTab->toggle(false);
  creatorTab->setPosition({centerX + 0.5f * spacing, tabY});
  typeMenu->addChild(creatorTab);
  m_creatorTab = creatorTab;

  // Top Coins tab (type 4)
  auto coinsTab = TabButton::create(
      TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Coins", this,
      menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
  coinsTab->setTag(4);
  coinsTab->toggle(false);
  coinsTab->setPosition({centerX + 1.5f * spacing, tabY});
  typeMenu->addChild(coinsTab);
  m_coinsTab = coinsTab;

  if (Mod::get()->getSettingValue<bool>("disableCreatorPoints") == true) {
    if (m_creatorTab) {
      m_creatorTab->setEnabled(false);
      m_creatorTab->setVisible(false);
    }
  }

  listLayer->addChild(typeMenu);

  // info button at the bottom left
  auto infoMenu = CCMenu::create();
  infoMenu->setPosition({0, 0});
  auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoButtonSpr->setScale(0.7f);
  auto infoButton = CCMenuItemSpriteExtra::create(
      infoButtonSpr, this, menu_selector(RLLeaderboardLayer::onInfoButton));
  infoButton->setPosition({25, 25});
  infoMenu->addChild(infoButton);
  this->addChild(infoMenu);

  // refresh button at the bottom right
  auto refreshSpr = CCSprite::createWithSpriteFrameName("RL_refresh01.png"_spr);
  m_refreshBtn = CCMenuItemSpriteExtra::create(
      refreshSpr, this, menu_selector(RLLeaderboardLayer::onRefreshButton));
  m_refreshBtn->setPosition({winSize.width - 35, 35});
  infoMenu->addChild(m_refreshBtn);

  this->scheduleUpdate();
  this->setKeypadEnabled(true);

  return true;
}

void RLLeaderboardLayer::onInfoButton(CCObject *sender) {
  MDPopup::create(
      "Rated Layouts Leaderboard",
      "The leaderboard shows the top players in <cb>Rated Layouts</c> based "
      "on <cl>Sparks</c>, <co>Planets</c>, <cb>Blue Coins</c> or <cf>Blueprint "
      "Points</c>. You can view each category by selecting the tabs.\n\n"
      "- <cl>Sparks</c> are earned by completing a <cb>Classic Rated "
      "Layouts</c> level and are only counted when beaten legitimately.\n"
      "- <co>Planets</c> are earned by completing a <cb>Platformer Rated "
      "Layouts</c> level and are only counted when beaten legitimately.\n"
      "- <cb>Blue Coins</c> are earned by collecting them while playing in "
      "Rated Layouts levels.\n"
      "- <cf>Blueprint Points</c> are earned based on the how many rated "
      "layouts levels you have in your account and users who are excluded "
      "won't be affected from this leaderboard.\n\n"
      "Getting a <cs>Normal Rated Layout</c> earn you 1 point, <cg>Featured "
      "Rated Layouts</c> level earns you 2 points, <cp>Epic Rated Layout</c> "
      "levels earn you 3 points and <cd>Legendary Rated Layout</c> levels earn "
      "you 4 points\n\n"
      "<cd>Legendary</c> layouts are only awarded by <cf>**ArcticWoof**</c> "
      "himself, <cr>Layout Admins can not set new Legendary Layouts</c>.\n\n"
      "### Any <cr>unfair</c> means of obtaining these stats <cy>(eg. instant "
      "complete, noclipping, secret way)</c> will result in an <cr>exclusion "
      "from the leaderboard and there will be NO APPEALS!</c> Each completion "
      "are <co>publicly logged</c> for this purpose.\n\n",
      "OK")
      ->show();
}

void RLLeaderboardLayer::update(float dt) {
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
}

void RLLeaderboardLayer::onAccountClicked(CCObject *sender) {
  auto button = static_cast<CCMenuItem *>(sender);
  int accountId = button->getTag();
  ProfilePage::create(accountId, false)->show();
}

void RLLeaderboardLayer::onRefreshButton(CCObject *sender) {
  // determine active tab type
  int type = 1;
  if (m_starsTab && m_starsTab->isToggled()) {
    type = 1;
  } else if (m_planetsTab && m_planetsTab->isToggled()) {
    type = 3;
  } else if (m_creatorTab && m_creatorTab->isToggled()) {
    type = 2;
  } else if (m_coinsTab && m_coinsTab->isToggled()) {
    type = 4;
  }

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

  this->fetchLeaderboard(type, 100);
}

void RLLeaderboardLayer::onLeaderboardTypeButton(CCObject *sender) {
  auto button = static_cast<TabButton *>(sender);
  int type = button->getTag();

  if (type == 1 && !m_starsTab->isToggled()) {
    m_starsTab->toggle(true);
    m_planetsTab->toggle(false);
    m_creatorTab->toggle(false);
    if (m_coinsTab)
      m_coinsTab->toggle(false);
  } else if (type == 3 && !m_planetsTab->isToggled()) {
    m_starsTab->toggle(false);
    m_planetsTab->toggle(true);
    m_creatorTab->toggle(false);
    if (m_coinsTab)
      m_coinsTab->toggle(false);
  } else if (type == 2 && !m_creatorTab->isToggled()) {
    m_starsTab->toggle(false);
    m_planetsTab->toggle(false);
    m_creatorTab->toggle(true);
    if (m_coinsTab)
      m_coinsTab->toggle(false);
  } else if (type == 4 && m_coinsTab && !m_coinsTab->isToggled()) {
    m_starsTab->toggle(false);
    m_planetsTab->toggle(false);
    m_creatorTab->toggle(false);
    m_coinsTab->toggle(true);
  }

  auto contentLayer = m_scrollLayer->m_contentLayer;
  if (contentLayer) {
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

  this->fetchLeaderboard(type, 100);
}

void RLLeaderboardLayer::fetchLeaderboard(int type, int amount) {
  Ref<RLLeaderboardLayer> self = this;
  auto request = web::WebRequest().param("type", type).param("amount", amount);
  async::spawn(request.get("https://gdrate.arcticwoof.xyz/getScore"),
               [self](web::WebResponse response) {
                 if (!self)
                   return;
                 if (!response.ok()) {
                   log::warn("Server returned non-ok status: {}",
                             response.code());
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

                 if (json.contains("users") && json["users"].isArray()) {
                   auto users = json["users"].asArray().unwrap();
                   self->populateLeaderboard(users);
                   if (self->m_scrollLayer)
                     self->m_scrollLayer->scrollToTop();
                 } else {
                   log::warn("No users array in response");
                 }
               });
}

void RLLeaderboardLayer::populateLeaderboard(
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
    int score = userValue["score"].asInt().unwrapOrDefault();
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
    if (nameplateId != 0 && !Mod::get()->getSettingValue<bool>("disableNameplate")) {
      nameplate = CCSprite::createWithSpriteFrameName(
          fmt::format("nameplate_{}.png"_spr, nameplateId).c_str());
      if (nameplate) {
        nameplate->setPosition({bgSprite->getPositionX() - 10, bgSprite->getPositionY()});
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

    // award achievement for getting on the leaderboard for the first time :)
    if (accountId == currentAccountID) {
      RLAchievements::onReward("misc_leaderboard"); // gg
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
        menu_selector(RLLeaderboardLayer::onAccountClicked));
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

    const bool isStar = m_starsTab->isToggled();
    const bool isPlanets = m_planetsTab && m_planetsTab->isToggled();
    const bool isCoins = m_coinsTab && m_coinsTab->isToggled();
    const char *iconName =
        isStar ? "RL_starMed.png"_spr
               : (isPlanets ? "RL_planetMed.png"_spr
                            : (isCoins ? "RL_BlueCoinSmall.png"_spr
                                       : "RL_blueprintPoint01.png"_spr));
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

RLLeaderboardLayer *RLLeaderboardLayer::create() {
  auto ret = new RLLeaderboardLayer();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

void RLLeaderboardLayer::keyBackClicked() {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}