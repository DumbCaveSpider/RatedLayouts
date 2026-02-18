#include "RLSearchLayer.hpp"
#include "RLLevelBrowserLayer.hpp"
#include <cue/RepeatingBackground.hpp>

using namespace geode::prelude;

// helper to map rating to difficulty level
static int mapRatingToLevel(int rating) {
  switch (rating) {
  case 1:
    return -1;
  case 2:
    return 1;
  case 3:
    return 2;
  case 4:
  case 5:
    return 3;
  case 6:
  case 7:
    return 4;
  case 8:
  case 9:
    return 5;
  case 10:
    return 7;
  case 15:
    return 8;
  case 20:
    return 6;
  case 25:
    return 9;
  case 30:
    return 10;
  default:
    return 0;
  }
}

bool RLSearchLayer::init() {
  if (!CCLayer::init())
    return false;

  auto winSize = CCDirector::sharedDirector()->getWinSize();

  addSideArt(this, SideArt::All, SideArtStyle::Layer, false);

  // create if moving bg disabled
  if (Mod::get()->getSettingValue<bool>("disableBackground") == true) {
    auto bg = createLayerBG();
    addChild(bg, -1);
  }

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

  // back menu
  auto backMenu = CCMenu::create();
  backMenu->setPosition({0, 0});

  addBackButton(this, BackButtonStyle::Pink);

  // search input stuff would go here
  m_searchInputMenu = CCMenu::create();
  m_searchInputMenu->setID("search-input-menu");
  m_searchInputMenu->setPosition({winSize.width / 2, winSize.height - 30});
  m_searchInputMenu->setContentSize({365.f, 40.f});

  auto searchInputBg = CCScale9Sprite::create("square02_small.png");
  searchInputBg->setContentSize(m_searchInputMenu->getContentSize());
  searchInputBg->setOpacity(100);
  m_searchInputMenu->addChild(searchInputBg, -1);

  m_searchInput = TextInput::create(245.f, "Search Layouts...");
  m_searchInput->setID("search-input");
  m_searchInput->setCommonFilter(CommonFilter::Name);
  m_searchInputMenu->addChild(m_searchInput);
  m_searchInput->setPosition({-50.f, 0});

  auto searchButtonSpr =
      CCSprite::createWithSpriteFrameName("GJ_longBtn06_001.png");
  auto searchButton = CCMenuItemSpriteExtra::create(
      searchButtonSpr, this, menu_selector(RLSearchLayer::onSearchButton));
  searchButton->setPosition({105.f, 0});
  m_searchInputMenu->addChild(searchButton);

  auto clearButtonSpr =
      CCSprite::createWithSpriteFrameName("GJ_longBtn07_001.png");
  auto clearButton = CCMenuItemSpriteExtra::create(
      clearButtonSpr, this, menu_selector(RLSearchLayer::onClearButton));
  clearButton->setPosition({155.f, 0});
  m_searchInputMenu->addChild(clearButton);

  this->addChild(m_searchInputMenu);

  // difficulty selection
  m_difficultyFilterMenu = CCMenu::create();
  m_difficultyFilterMenu->setID("difficulty-filter-menu");
  m_difficultyFilterMenu->setPosition(
      {winSize.width / 2, winSize.height - 110});
  m_difficultyFilterMenu->setContentSize({400.f, 80.f});
  auto difficultyLabel =
      CCLabelBMFont::create("Difficulty Filter", "bigFont.fnt");
  difficultyLabel->setScale(0.5f);
  difficultyLabel->setPosition({winSize.width / 2, winSize.height - 60});
  difficultyLabel->setAnchorPoint({0.5f, 0.5f});
  this->addChild(difficultyLabel);
  // difficulty groups
  std::vector<std::vector<int>> groups = {{1},    {2},    {3},
                                          {4, 5}, {6, 7}, {8, 9}};
  m_difficultyGroups = groups;
  m_difficultySelected.assign(static_cast<int>(groups.size()), false);

  // helper to map rating values
  auto mapRatingToLevel = [](int rating) -> int {
    switch (rating) {
    case 1:
      return -1;
    case 2:
      return 1;
    case 3:
      return 2;
    case 4:
    case 5:
      return 3;
    case 6:
    case 7:
      return 4;
    case 8:
    case 9:
      return 5;
    default:
      return 0;
    }
  };

  int groupCount = static_cast<int>(groups.size());
  float spacing = 60.0f;
  float startX = -spacing * (groupCount - 1) / 2.0f;
  for (int gi = 0; gi < groupCount; ++gi) {
    auto &group = groups[gi];
    if (group.empty())
      continue;
    int rep = group[0];
    int difficultyLevel = mapRatingToLevel(rep);

    auto difficultySprite =
        GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Long);
    if (!difficultySprite)
      continue;
    difficultySprite->updateDifficultyFrame(difficultyLevel,
                                            GJDifficultyName::Long);
    difficultySprite->setColor({125, 125, 125});

    std::string id;
    if (group.size() == 1) {
      id = "rl-difficulty-" + numToString(group[0]);
    } else {
      id = "rl-difficulty-" + numToString(group.front()) + "-" +
           numToString(group.back());
    }

    // position the menu item
    float x = startX + gi * spacing;
    difficultySprite->setPosition({x, 0});
    difficultySprite->setScale(1.0f);
    difficultySprite->setID(id.c_str());

    auto menuItem = CCMenuItemSpriteExtra::create(
        difficultySprite, this,
        menu_selector(RLSearchLayer::onDifficultyButton));
    menuItem->setPosition({x, 0});
    menuItem->setTag(gi + 1);
    menuItem->setScale(1.0f);
    menuItem->setID(id.c_str());
    m_difficultyFilterMenu->addChild(menuItem);
    m_difficultyMenuItems.push_back(menuItem);
    m_difficultySprites.push_back(difficultySprite);
  }
  auto difficultyMenuBg = CCScale9Sprite::create("square02_small.png");
  difficultyMenuBg->setContentSize(m_difficultyFilterMenu->getContentSize());
  difficultyMenuBg->setPosition(m_difficultyFilterMenu->getPosition());
  difficultyMenuBg->setOpacity(100);
  this->addChild(difficultyMenuBg, -1);

  // Create demon difficulties menu (10, 15, 20, 25, 30)
  m_demonFilterMenu = CCMenu::create();
  m_demonFilterMenu->setID("demon-filter-menu");
  m_demonFilterMenu->setPosition(m_difficultyFilterMenu->getPosition());
  m_demonFilterMenu->setContentSize(m_difficultyFilterMenu->getContentSize());

  std::vector<int> demonRatings = {10, 15, 20, 25, 30};
  m_demonSelected.assign(static_cast<int>(demonRatings.size()), false);
  float demonSpacing = 60.0f;
  float demonStartX =
      -demonSpacing * (static_cast<int>(demonRatings.size()) - 1) / 2.0f;
  for (int di = 0; di < static_cast<int>(demonRatings.size()); ++di) {
    int rating = demonRatings[di];
    int difficultyLevel = 0;
    bool isDemon = false;
    switch (rating) {
    case 10:
      difficultyLevel = 7;
      isDemon = true;
      break;
    case 15:
      difficultyLevel = 8;
      isDemon = true;
      break;
    case 20:
      difficultyLevel = 6;
      isDemon = true;
      break;
    case 25:
      difficultyLevel = 9;
      isDemon = true;
      break;
    case 30:
      difficultyLevel = 10;
      isDemon = true;
      break;
    default:
      difficultyLevel = 0;
      break;
    }
    auto demonSprite =
        GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Long);
    if (!demonSprite)
      continue;
    demonSprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);
    demonSprite->setColor({125, 125, 125});
    float x = demonStartX + di * demonSpacing;
    demonSprite->setPosition({x, 0});
    demonSprite->setScale(1.0f);
    std::string id = "rl-demon-difficulty-" + numToString(rating);
    demonSprite->setID(id.c_str());
    auto demonItem = CCMenuItemSpriteExtra::create(
        demonSprite, this,
        menu_selector(RLSearchLayer::onDemonDifficultyButton));
    demonItem->setPosition({x, 0});
    demonItem->setTag(di + 1);
    demonItem->setScale(1.0f);
    demonItem->setID(id.c_str());
    m_demonFilterMenu->addChild(demonItem);
    m_demonMenuItems.push_back(demonItem);
    m_demonSprites.push_back(demonSprite);
  }
  m_demonFilterMenu->setVisible(false);
  this->addChild(m_demonFilterMenu);

  // Demon toggle button
  auto demonOff =
      CCSpriteGrayscale::createWithSpriteFrameName("GJ_demonIcon_001.png");
  auto demonOn = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
  auto demonToggle = CCMenuItemToggler::create(
      demonOff, demonOn, this, menu_selector(RLSearchLayer::onDemonToggle));
  demonToggle->setScale(1.0f);
  demonToggle->setID("demon-toggle");

  auto toggleMenuDemon = CCMenu::create();
  toggleMenuDemon->setID("demon-toggle-menu");
  toggleMenuDemon->setPosition(m_difficultyFilterMenu->getPosition());
  demonToggle->setPosition({200, -40});
  toggleMenuDemon->addChild(demonToggle);
  this->addChild(toggleMenuDemon);

  this->addChild(m_difficultyFilterMenu);

  // options filter menu
  auto optionsMenu = CCMenu::create();
  optionsMenu->setID("options-menu");
  optionsMenu->setPosition({winSize.width / 2, 80});
  optionsMenu->setContentSize({390.f, 130.f});
  optionsMenu->setLayout(RowLayout::create()
                             ->setGap(10.f)
                             ->setGrowCrossAxis(true)
                             ->setCrossAxisOverflow(false));
  m_optionsLayout = static_cast<RowLayout *>(optionsMenu->getLayout());
  this->addChild(optionsMenu);
  auto optionsLabel = CCLabelBMFont::create("Search Options", "bigFont.fnt");
  optionsLabel->setPosition({winSize.width / 2, winSize.height - 160});
  optionsLabel->setScale(0.5f);
  this->addChild(optionsLabel);
  auto optionsMenuBg = CCScale9Sprite::create("square02_small.png");
  optionsMenuBg->setContentSize({optionsMenu->getContentSize().width + 10.f,
                                 optionsMenu->getContentSize().height + 10.f});
  optionsMenuBg->setPosition(optionsMenu->getPosition());
  optionsMenuBg->setOpacity(100);
  this->addChild(optionsMenuBg, -1);

  // awarded button toggle
  auto awardedSpr = ButtonSprite::create("Awarded", 120, true, "goldFont.fnt",
                                         "GJ_button_01.png", 30.f, 1.f);
  auto awardedItem = CCMenuItemSpriteExtra::create(
      awardedSpr, this, menu_selector(RLSearchLayer::onAwardedToggle));
  awardedItem->setScale(1.0f);
  awardedItem->setID("awarded-toggle");
  m_awardedItem = awardedItem;
  optionsMenu->addChild(awardedItem);

  // featured button toggle
  auto featuredSpr = ButtonSprite::create(
      "Featured", 120, true, "goldFont.fnt", "GJ_button_01.png", 30.f,
      1.f); // Use a single sprite button as a menu item instead
            // of a toggler
  auto featuredItem = CCMenuItemSpriteExtra::create(
      featuredSpr, this, menu_selector(RLSearchLayer::onFeaturedToggle));
  featuredItem->setScale(1.0f);
  featuredItem->setID("featured-toggle");
  m_featuredItem = featuredItem;
  optionsMenu->addChild(featuredItem);

  // epic button toggle
  auto epicSpr = ButtonSprite::create("Epic", 120, true, "goldFont.fnt",
                                      "GJ_button_01.png", 30.f, 1.f);
  auto epicItem = CCMenuItemSpriteExtra::create(
      epicSpr, this, menu_selector(RLSearchLayer::onEpicToggle));
  epicItem->setScale(1.0f);
  epicItem->setID("epic-toggle");
  m_epicItem = epicItem;
  optionsMenu->addChild(epicItem);
  // legendary button toggle
  auto legendarySpr = ButtonSprite::create(
      "Legendary", 120, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
  auto legendaryItem = CCMenuItemSpriteExtra::create(
      legendarySpr, this, menu_selector(RLSearchLayer::onLegendaryToggle));
  legendaryItem->setScale(1.0f);
  legendaryItem->setID("legendary-toggle");
  m_legendaryItem = legendaryItem;
  optionsMenu->addChild(legendaryItem);
  // platformer toggle
  auto platformerSpr = ButtonSprite::create(
      "Platformer", 120, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
  auto platformerItem = CCMenuItemSpriteExtra::create(
      platformerSpr, this, menu_selector(RLSearchLayer::onPlatformerToggle));
  platformerItem->setScale(1.0f);
  platformerItem->setID("platformer-toggle");
  m_platformerItem = platformerItem;
  optionsMenu->addChild(platformerItem);

  // classic toggle
  auto classicSpr = ButtonSprite::create("Classic", 120, true, "goldFont.fnt",
                                         "GJ_button_01.png", 30.f, 1.f);
  auto classicItem = CCMenuItemSpriteExtra::create(
      classicSpr, this, menu_selector(RLSearchLayer::onClassicToggle));
  classicItem->setScale(1.0f);
  classicItem->setID("classic-toggle");
  m_classicItem = classicItem;
  optionsMenu->addChild(classicItem);

  // username toggle
  auto usernameSpr = ButtonSprite::create("Username", 120, true, "goldFont.fnt",
                                          "GJ_button_01.png", 30.f, 1.f);
  auto usernameItem = CCMenuItemSpriteExtra::create(
      usernameSpr, this, menu_selector(RLSearchLayer::onUsernameToggle));
  usernameItem->setScale(1.0f);
  usernameItem->setID("username-toggle");
  m_usernameItem = usernameItem;
  optionsMenu->addChild(usernameItem);

  // sorting toggle - descending
  auto oldestSpr = ButtonSprite::create("Oldest", 120, true, "goldFont.fnt",
                                        "GJ_button_01.png", 30.f, 1.f);
  auto oldestItem = CCMenuItemSpriteExtra::create(
      oldestSpr, this, menu_selector(RLSearchLayer::onOldestToggle));
  oldestItem->setScale(1.0f);
  oldestItem->setID("oldest-toggle");
  m_oldestItem = oldestItem;
  optionsMenu->addChild(oldestItem);

  // completed toggle
  auto completedSpr = ButtonSprite::create(
      "Completed", 120, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
  auto completedItem = CCMenuItemSpriteExtra::create(
      completedSpr, this, menu_selector(RLSearchLayer::onCompletedToggle));
  completedItem->setScale(1.0f);
  completedItem->setID("completed-toggle");
  m_completedItem = completedItem;
  optionsMenu->addChild(completedItem);

  // uncompleted toggle
  auto uncompletedSpr = ButtonSprite::create(
      "Uncompleted", 120, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
  auto uncompletedItem = CCMenuItemSpriteExtra::create(
      uncompletedSpr, this, menu_selector(RLSearchLayer::onUncompletedToggle));
  uncompletedItem->setScale(1.0f);
  uncompletedItem->setID("uncompleted-toggle");
  m_uncompletedItem = uncompletedItem;
  optionsMenu->addChild(uncompletedItem);

  // coins verified toggle
  auto coinsVerifiedSpr =
      ButtonSprite::create("Coins Verified", 120, true, "goldFont.fnt",
                           "GJ_button_01.png", 30.f, 1.f);
  auto coinsVerifiedItem = CCMenuItemSpriteExtra::create(
      coinsVerifiedSpr, this,
      menu_selector(RLSearchLayer::onCoinsVerifiedToggle));
  coinsVerifiedItem->setScale(1.0f);
  coinsVerifiedItem->setID("coins-verified-toggle");
  m_coinsVerifiedItem = coinsVerifiedItem;
  optionsMenu->addChild(coinsVerifiedItem);

  optionsMenu->updateLayout();

  // info button yay
  auto infoMenu = CCMenu::create();
  infoMenu->setPosition({0, 0});
  auto randomSpr = CCSprite::createWithSpriteFrameName("RL_random01.png"_spr);
  randomSpr->setScale(0.7f);
  auto randomItem = CCMenuItemSpriteExtra::create(
      randomSpr, this, menu_selector(RLSearchLayer::onRandomButton));
  randomItem->setPosition({25, 65});
  randomItem->setID("random-button");
  infoMenu->addChild(randomItem);
  m_randomButton = randomItem;

  auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoButtonSpr->setScale(0.7f);
  auto infoButton = CCMenuItemSpriteExtra::create(
      infoButtonSpr, this, menu_selector(RLSearchLayer::onInfoButton));
  infoButton->setPosition({25, 25});
  infoMenu->addChild(infoButton);
  this->addChild(infoMenu);

  this->addChild(m_difficultyFilterMenu);

  this->setKeypadEnabled(true);
  this->scheduleUpdate();

  return true;
}

void RLSearchLayer::onInfoButton(CCObject *sender) {
  FLAlertLayer::create(
      "Rated Layouts Search",
      "Use the <cg>search bar</c> to find <cl>Rated Layouts Levels</c> by "
      "<co>name or keywords.</c>\n"
      "Use the <cr>difficulty filter</c> to select which "
      "<cl>layout difficulties</c> to include in the search.\n"
      "Press the <cg>Search button</c> to perform the search with "
      "the selected criteria.\n"
      "To view the <cl>recently rated layouts</c>, leave the search bar empty, "
      "then press the <cg>Search</c>.",
      "OK")
      ->show();
}

void RLSearchLayer::onRandomButton(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // prevent spamming
  item->setEnabled(false);

  Ref<RLSearchLayer> self = this;
  m_searchTask.spawn(
      web::WebRequest().get("https://gdrate.arcticwoof.xyz/getRandomLevel"),
      [self, item](web::WebResponse const &res) {
        if (!self)
          return;
        if (item)
          item->setEnabled(true);

        if (!res.ok()) {
          Notification::create("Failed to fetch random level",
                               NotificationIcon::Error)
              ->show();
          return;
        }

        auto jsonRes = res.json();
        if (!jsonRes) {
          Notification::create("Invalid response", NotificationIcon::Error)
              ->show();
          return;
        }

        auto json = jsonRes.unwrap();
        int levelId = 0;
        if (json.contains("levelId")) {
          if (auto v = json["levelId"].as<int>(); v)
            levelId = v.unwrap();
        }

        if (levelId <= 0) {
          Notification::create("No random level available",
                               NotificationIcon::Warning)
              ->show();
          return;
        }

        // Try to open immediately from cache
        auto searchObj =
            GJSearchObject::create(SearchType::Search, numToString(levelId));
        auto key = std::string(searchObj->getKey());
        auto glm = GameLevelManager::sharedState();

        auto stored = glm->getStoredOnlineLevels(key.c_str());
        if (stored && stored->count() > 0) {
          auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
          if (level && level->m_levelID == levelId) {
            auto scene = LevelInfoLayer::scene(level, false);
            auto trans = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(trans);
            return;
          }
        }

        // not cached: set pending and request via GameLevelManager (similar to
        // RLEventLayouts)
        self->m_randomPendingKey = key;
        self->m_randomPendingLevelId = levelId;
        self->m_randomPendingTimeout = 10.0f; // 10 sec timeout

        // clear external delegate to avoid interference
        if (glm && glm->m_levelManagerDelegate)
          glm->m_levelManagerDelegate = nullptr;
        glm->getOnlineLevels(searchObj);
      });
}

void RLSearchLayer::onSearchButton(CCObject *sender) {
  // disable button to prevent spamming
  auto menuItem = static_cast<CCMenuItemSpriteExtra *>(sender);
  menuItem->setEnabled(false);
  std::vector<int> selectedRatings;
  if (m_demonModeActive) {
    std::vector<int> demonRatings = {10, 15, 20, 25, 30};
    for (int i = 0; i < static_cast<int>(m_demonSelected.size()); ++i) {
      if (m_demonSelected[i] && i < static_cast<int>(demonRatings.size())) {
        selectedRatings.push_back(demonRatings[i]);
      }
    }
  } else {
    for (int gi = 0; gi < static_cast<int>(m_difficultySelected.size()); ++gi) {
      if (!m_difficultySelected[gi])
        continue;
      if (gi >= static_cast<int>(m_difficultyGroups.size()))
        continue;
      for (auto r : m_difficultyGroups[gi])
        selectedRatings.push_back(r);
    }
  }

  // make list unique & sort
  std::sort(selectedRatings.begin(), selectedRatings.end());
  selectedRatings.erase(
      std::unique(selectedRatings.begin(), selectedRatings.end()),
      selectedRatings.end());

  std::string difficultyParam;
  for (size_t i = 0; i < selectedRatings.size(); ++i) {
    if (i)
      difficultyParam += ",";
    difficultyParam += numToString(selectedRatings[i]);
  }

  int featuredParam = m_featuredActive ? 1 : 0;
  int awardedParam = m_awardedActive ? 1 : 0;
  int epicParam = m_epicActive ? 1 : 0;
  int legendaryParam = m_legendaryActive ? 1 : 0;
  int oldestParam = m_oldestActive ? 1 : 0;
  int usernameParam = m_usernameActive ? 1 : 0;
  std::string queryParam = "";
  if (m_searchInput)
    queryParam = m_searchInput->getString();

  std::vector<std::pair<std::string, std::string>> params;
  if (!queryParam.empty())
    params.emplace_back("query", queryParam);
  if (!difficultyParam.empty())
    params.emplace_back("difficulty", difficultyParam);
  if (m_platformerActive)
    params.emplace_back("platformer", "1");
  if (m_classicActive)
    params.emplace_back("classic", "1");
  if (m_featuredActive)
    params.emplace_back("featured", numToString(featuredParam));
  if (m_epicActive)
    params.emplace_back("epic", numToString(epicParam));
  if (m_legendaryActive)
    params.emplace_back("legendary", numToString(legendaryParam));
  if (m_awardedActive)
    params.emplace_back("awarded", numToString(awardedParam));
  if (m_completedActive)
    params.emplace_back("completed", "1");
  if (m_uncompletedActive)
    params.emplace_back("uncompleted", "1");
  if (m_oldestActive)
    params.emplace_back("oldest", numToString(oldestParam));
  if (m_usernameActive)
    params.emplace_back("user", numToString(usernameParam));
  if (m_coinsVerifiedActive)
    params.emplace_back("coins", "1");
  params.emplace_back("accountId",
                      numToString(GJAccountManager::get()->m_accountID));

  if (menuItem)
    menuItem->setEnabled(true);
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::Search, params, "Layouts Search");
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLSearchLayer::onCoinsVerifiedToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  m_coinsVerifiedActive = !m_coinsVerifiedActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_coinsVerifiedActive ? "GJ_button_02.png"
                                             : "GJ_button_01.png");
  }
}

void RLSearchLayer::onFeaturedToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  m_featuredActive = !m_featuredActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_featuredActive ? "GJ_button_02.png"
                                        : "GJ_button_01.png");
  }
}

void RLSearchLayer::onAwardedToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  m_awardedActive = !m_awardedActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_awardedActive ? "GJ_button_02.png"
                                       : "GJ_button_01.png");
  }
}

void RLSearchLayer::onEpicToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  m_epicActive = !m_epicActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_epicActive ? "GJ_button_02.png" : "GJ_button_01.png");
  }
}

void RLSearchLayer::onLegendaryToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  m_legendaryActive = !m_legendaryActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_legendaryActive ? "GJ_button_02.png"
                                         : "GJ_button_01.png");
  }
}

void RLSearchLayer::onPlatformerToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // toggle the platformer filter
  m_platformerActive = !m_platformerActive;
  // ensure mutual exclusivity with Classic
  if (m_platformerActive && m_classicActive) {
    m_classicActive = false;
    if (m_classicItem) {
      auto classicNormal =
          static_cast<ButtonSprite *>(m_classicItem->getNormalImage());
      if (classicNormal)
        classicNormal->updateBGImage("GJ_button_01.png");
    }
  }
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_platformerActive ? "GJ_button_02.png"
                                          : "GJ_button_01.png");
  }
}

void RLSearchLayer::onClassicToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // toggle the classic filter
  m_classicActive = !m_classicActive;
  // ensure mutual exclusivity with Platformer
  if (m_classicActive && m_platformerActive) {
    m_platformerActive = false;
    if (m_platformerItem) {
      auto platNormal =
          static_cast<ButtonSprite *>(m_platformerItem->getNormalImage());
      if (platNormal)
        platNormal->updateBGImage("GJ_button_01.png");
    }
  }
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_classicActive ? "GJ_button_02.png"
                                       : "GJ_button_01.png");
  }
}

void RLSearchLayer::onUsernameToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // toggle the username filter
  m_usernameActive = !m_usernameActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_usernameActive ? "GJ_button_02.png"
                                        : "GJ_button_01.png");
  }
}

void RLSearchLayer::onOldestToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // Toggle oldest
  m_oldestActive = !m_oldestActive;
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_oldestActive ? "GJ_button_02.png"
                                      : "GJ_button_01.png");
  }
}

void RLSearchLayer::onCompletedToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // mutual exclusivity with Uncompleted
  m_completedActive = !m_completedActive;
  if (m_completedActive && m_uncompletedActive) {
    m_uncompletedActive = false;
    if (m_uncompletedItem) {
      auto unNode =
          static_cast<ButtonSprite *>(m_uncompletedItem->getNormalImage());
      if (unNode)
        unNode->updateBGImage("GJ_button_01.png");
    }
  }
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_completedActive ? "GJ_button_02.png"
                                         : "GJ_button_01.png");
  }
}

void RLSearchLayer::onUncompletedToggle(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  // mutual exclusivity with Completed
  m_uncompletedActive = !m_uncompletedActive;
  if (m_uncompletedActive && m_completedActive) {
    m_completedActive = false;
    if (m_completedItem) {
      auto cNode =
          static_cast<ButtonSprite *>(m_completedItem->getNormalImage());
      if (cNode)
        cNode->updateBGImage("GJ_button_01.png");
    }
  }
  auto normalNode = item->getNormalImage();
  auto btn = static_cast<ButtonSprite *>(normalNode);
  if (btn) {
    btn->updateBGImage(m_uncompletedActive ? "GJ_button_02.png"
                                           : "GJ_button_01.png");
  }
}

void RLSearchLayer::onDemonToggle(CCObject *sender) {
  auto toggler = static_cast<CCMenuItemToggler *>(sender);
  if (!toggler)
    return;
  // toggle state
  m_demonModeActive = !m_demonModeActive;
  // show/hide menus
  if (m_demonModeActive) {
    m_demonFilterMenu->setVisible(true);
    m_difficultyFilterMenu->setVisible(false);
    // clear normal selection state entirely
    for (size_t _i = 0; _i < m_difficultySelected.size(); ++_i)
      m_difficultySelected[_i] = false;
    for (size_t _i = 0; _i < m_difficultySprites.size(); ++_i) {
      auto sprite = m_difficultySprites[_i];
      auto mi = (_i < m_difficultyMenuItems.size() ? m_difficultyMenuItems[_i]
                                                   : nullptr);
      if (sprite) {
        sprite->setColor({125, 125, 125});
      }
    }
  } else {
    m_demonFilterMenu->setVisible(false);
    m_difficultyFilterMenu->setVisible(true);
    // clear demon selection state entirely
    for (size_t _i = 0; _i < m_demonSelected.size(); ++_i)
      m_demonSelected[_i] = false;
    for (size_t _i = 0; _i < m_demonSprites.size(); ++_i) {
      auto sprite = m_demonSprites[_i];
      if (sprite) {
        sprite->setColor({125, 125, 125});
      }
    }
  }
  // if switching back to normal, update the normal sprites to long
  if (!m_demonModeActive) {
    for (int gi = 0; gi < static_cast<int>(m_difficultySprites.size()); ++gi) {
      auto sprite = m_difficultySprites[gi];
      if (!sprite)
        continue;
      auto &group = m_difficultyGroups[gi];
      if (group.empty())
        continue;
      int rep = group[0];
      int difficultyLevel = mapRatingToLevel(rep);
      sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);
      sprite->setColor({125, 125, 125});
    }
  } else {
    // when switching to demon we might want to clear normal selection
    for (int gi = 0; gi < static_cast<int>(m_difficultySprites.size()); ++gi) {
      auto sprite = m_difficultySprites[gi];
      if (!sprite)
        continue;
      // revert to short or leave as-is
      auto &group = m_difficultyGroups[gi];
      if (group.empty())
        continue;
      int rep = group[0];
      int difficultyLevel = mapRatingToLevel(rep);
      sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Short);
      sprite->setColor({125, 125, 125});
    }
  }
}

void RLSearchLayer::onDemonDifficultyButton(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  int tag = item->getTag();
  if (tag < 1 || tag > static_cast<int>(m_demonSelected.size()))
    return;
  int idx = tag - 1;
  m_demonSelected[idx] = !m_demonSelected[idx];
  auto sprite = m_demonSprites[idx];
  if (!sprite)
    return;
  if (m_demonSelected[idx]) {
    sprite->setColor({255, 255, 255});
  } else {
    sprite->setColor({125, 125, 125});
  }
}

void RLSearchLayer::onDifficultyButton(CCObject *sender) {
  auto item = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!item)
    return;
  int tag = item->getTag();
  if (tag < 1 || tag > static_cast<int>(m_difficultySelected.size()))
    return;
  int idx = tag - 1;
  // toggle state
  m_difficultySelected[idx] = !m_difficultySelected[idx];
  auto sprite = m_difficultySprites[idx];
  if (!sprite)
    return;
  if (m_difficultySelected[idx]) {
    // selected, highlight
    sprite->setColor({255, 255, 255});
  } else {
    // unselected, gray out
    sprite->setColor({125, 125, 125});
  }
}

void RLSearchLayer::onClearButton(CCObject *sender) {
  // clear the text input
  if (m_searchInput) {
    m_searchInput->setString("");
  }
}

void RLSearchLayer::update(float dt) {
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

  // handle pending random level fetch
  if (m_randomPendingLevelId > 0) {
    auto glm = GameLevelManager::sharedState();
    if (m_randomPendingTimeout > 0.0f) {
      m_randomPendingTimeout -= dt;
    }
    if (glm) {
      auto stored = glm->getStoredOnlineLevels(m_randomPendingKey.c_str());
      if (stored && stored->count() > 0) {
        for (size_t i = 0; i < stored->count(); ++i) {
          auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(i));
          if (level && level->m_levelID == m_randomPendingLevelId) {
            auto scene = LevelInfoLayer::scene(level, false);
            auto trans = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(trans);
            m_randomPendingLevelId = -1;
            m_randomPendingKey.clear();
            m_randomPendingTimeout = 0.0f;
            if (m_randomButton)
              m_randomButton->setEnabled(true);
            return;
          }
        }
      }
    }

    if (m_randomPendingTimeout <= 0.0f) {
      Notification::create("Failed to fetch random level",
                           NotificationIcon::Error)
          ->show();
      m_randomPendingLevelId = -1;
      m_randomPendingKey.clear();
      m_randomPendingTimeout = 0.0f;
      if (m_randomButton)
        m_randomButton->setEnabled(true);
    }
  }
}

RLSearchLayer *RLSearchLayer::create() {
  RLSearchLayer *layer = new RLSearchLayer();
  if (layer && layer->init()) {
    layer->autorelease();
    return layer;
  }
  delete layer;
  return nullptr;
}

void RLSearchLayer::keyBackClicked() {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}