#include "RLEventLayouts.hpp"

#include "../custom/RLAchievements.hpp"
#include "../custom/RLLevelBrowserLayer.hpp"
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

extern const std::string epicPString;
extern const std::string legendaryPString;
extern const std::string mythicPString;

// helper prototypes
static std::string formatTime(long seconds);
static int getDifficulty(int numerator);

RLEventLayouts *RLEventLayouts::create(EventType type) {
  auto ret = new RLEventLayouts();
  ret->m_eventType = type;

  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }

  delete (ret);
  return nullptr;
};

bool RLEventLayouts::init() {
  if (!Popup::init(420.f, 280.f))
    return false;
  // register instance
  addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

  auto contentSize = m_mainLayer->getContentSize();

  m_eventMenu = CCLayer::create();
  m_eventMenu->setPosition({contentSize.width / 2, contentSize.height / 2});
  m_eventMenu->setAnchorPoint({0.5f, 0.5f});

  float startY = contentSize.height - 87.f;
  float rowSpacing = 90.f;

  // info button on main layer
  auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoSpr->setScale(0.7f);
  auto infoBtn = CCMenuItemSpriteExtra::create(
      infoSpr, this, menu_selector(RLEventLayouts::onInfo));
  infoBtn->setPosition({contentSize.width - 25.f, contentSize.height - 25.f});
  m_buttonMenu->addChild(infoBtn);

  // Create a single centered container that represents the chosen EventType
  std::vector<std::string> labels = {"Daily", "Weekly", "Monthly"};
  int idx = static_cast<int>(this->m_eventType);
  if (idx < 0 || idx >= 3)
    idx = 0;

  // container layer for the chosen event (main: classic layouts)
  auto container = CCLayer::create();
  // event container size set to 380x84
  container->setContentSize({380.f, 84.f});
  container->setAnchorPoint({0.5f, 0.5f});

  const char *bgTex = (idx == 0)   ? "GJ_square03.png"
                      : (idx == 1) ? "GJ_square05.png"
                                   : "GJ_square04.png";
  auto bgSprite = CCScale9Sprite::create(bgTex);
  if (bgSprite) {
    bgSprite->setContentSize({380.f, 84.f});
    bgSprite->setAnchorPoint({0.5f, 0.5f});
    bgSprite->setPosition({container->getContentSize().width / 2.f,
                           container->getContentSize().height / 2.f});
    container->addChild(bgSprite, -1);
  }

  m_sections[idx].container = container;
  const float containerTopY = 145.f;
  container->setPosition({22, containerTopY});
  m_mainLayer->addChild(container);

  auto cellSize = container->getContentSize();
  auto levelCell = new LevelCell("RLLevelCell", 420, 94);
  levelCell->setScale(.85f);
  levelCell->setPosition(-18.f, -4.f);
  levelCell->autorelease();
  levelCell->setContentSize(cellSize);
  levelCell->setAnchorPoint({0.5f, 0.5f});
  container->addChild(levelCell, 1);
  m_sections[idx].levelCell = levelCell;

  // main play LoadingSpinner (centered in container)
  if (auto spinner = LoadingSpinner::create(25.f)) {
    spinner->setPosition({cellSize.width / 2.f, cellSize.height / 2.f});
    spinner->setVisible(false);
    container->addChild(spinner, 2);
    m_sections[idx].playSpinner = spinner;
  }

  auto platContainer = CCLayer::create();
  platContainer->setContentSize({380.f, 84.f});
  platContainer->setAnchorPoint({0.5f, 0.5f});
  auto platBg = CCScale9Sprite::create(bgTex);
  if (platBg) {
    platBg->setContentSize({380.f, 84.f});
    platBg->setAnchorPoint({0.5f, 0.5f});
    platBg->setPosition({platContainer->getContentSize().width / 2.f,
                         platContainer->getContentSize().height / 2.f});
    platContainer->addChild(platBg, -1);
  }
  m_sections[idx].platContainer = platContainer;

  auto platCell = new LevelCell("RLLevelCell", 420, 94);
  platCell->setScale(.85f);
  platCell->setPosition(-18.f, -4.f);
  platCell->autorelease();
  platCell->setContentSize(cellSize);
  platCell->setAnchorPoint({0.5f, 0.5f});
  platContainer->addChild(platCell, 1);
  m_sections[idx].platLevelCell = platCell;

  if (auto platSpinner = LoadingSpinner::create(25.f)) {
    platSpinner->setPosition({cellSize.width / 2.f, cellSize.height / 2.f});
    platSpinner->setVisible(false);
    platContainer->addChild(platSpinner, 2);
    m_sections[idx].platPlaySpinner = platSpinner;
  }

  float platY =
      containerTopY - (container->getContentSize().height / 2.f +
                       platContainer->getContentSize().height / 2.f + 6.f);
  platContainer->setPosition({22, platY});
  m_mainLayer->addChild(platContainer);

  // header label
  const char *headerTex = (idx == 0)   ? "RL_dailyLabelSilly.png"_spr
                          : (idx == 1) ? "RL_weeklyLabelSilly.png"_spr
                                       : "RL_monthlyLabelSilly.png"_spr;
  auto headerLabel = CCSprite::createWithSpriteFrameName(headerTex);
  headerLabel->setAnchorPoint({0.5f, 1.f});
  headerLabel->setPosition({m_mainLayer->getContentSize().width / 2.f,
                            m_mainLayer->getContentSize().height - 15.f});
  m_mainLayer->addChild(headerLabel, 3);

  // timer label
  std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ",
                                            "Next Monthly in "};
  auto timerLabel = CCLabelBMFont::create(
      (timerPrefixes[idx] + "--:--:--:--").c_str(), "bigFont.fnt");
  timerLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, 15.f});
  timerLabel->setScale(0.4f);
  m_mainLayer->addChild(timerLabel);
  m_sections[idx].timerLabel = timerLabel;

  // safe button
  auto safeMenu = CCMenu::create();
  safeMenu->setPosition({0, 0});
  auto safeSprite = CCSprite::createWithSpriteFrameName("GJ_safeBtn_001.png");
  if (safeSprite) {
    safeSprite->setScale(0.9f);
    auto safeButton = CCMenuItemSpriteExtra::create(
        safeSprite, this, menu_selector(RLEventLayouts::onSafeButton));
    safeButton->setPosition({25.f, 25.f});
    safeMenu->addChild(safeButton);
    m_mainLayer->addChild(safeMenu, 3);
  }

  this->scheduleUpdate();

  // Fetch event info from server
  {
    Ref<RLEventLayouts> self = this;
    self->m_eventTask.spawn(
        web::WebRequest().get("https://gdrate.arcticwoof.xyz/getEvent"),
        [self](web::WebResponse res) {
          if (!self)
            return;
          if (!res.ok()) {
            Notification::create("Failed to fetch event info",
                                 NotificationIcon::Error)
                ->show();
            return;
          }
          auto jsonResult = res.json();
          if (!jsonResult) {
            Notification::create("Invalid event JSON",
                                 NotificationIcon::Warning)
                ->show();
            return;
          }
          auto json = jsonResult.unwrap();

          std::vector<std::string> keys = {"daily", "weekly", "monthly"};
          int idx = static_cast<int>(self->m_eventType);
          if (idx < 0 || idx >= 3)
            return;
          const auto &key = keys[idx];
          auto sec = &self->m_sections[idx];

          if (!json.contains(key)) {
            return;
          }
          auto obj = json[key];
          int levelId = -1;
          auto levelIdValue = obj["levelId"].as<int>();
          if (levelIdValue) {
            levelId = levelIdValue.unwrap();
          }

          self->m_sections[idx].levelId = levelId;
          self->m_sections[idx].secondsLeft =
              obj["secondsLeft"].as<int>().unwrapOrDefault();

          // if we already have the full GJGameLevel cached, populate the
          if (auto glm = GameLevelManager::sharedState()) {
            // main-level search object/key
            auto mainSearchObj = GJSearchObject::create(
                SearchType::Search, fmt::format("{}", levelId));
            auto mainKey = std::string(mainSearchObj->getKey());
            std::string platJsonKey = mainKey + "Plat";
            int platLevelId = -1;
            if (json.contains(platJsonKey)) {
              auto pval = json[platJsonKey]["levelId"].as<int>();
              if (pval) {
                platLevelId = pval.unwrap();
              }
            }

            if (platLevelId > 0) {
              auto combinedSearchObj = GJSearchObject::create(
                  SearchType::Search,
                  fmt::format("{},{}", levelId, platLevelId));
              auto combinedKey = std::string(combinedSearchObj->getKey());

              // check cache for combined key
              auto storedLvls = glm->getStoredOnlineLevels(combinedKey.c_str());
              if (storedLvls && storedLvls->count() > 0) {
                // iterate through returned levels and populate matching cells
                for (unsigned int si = 0; si < storedLvls->count(); ++si) {
                  if (auto lvl = static_cast<GJGameLevel *>(
                          storedLvls->objectAtIndex(si))) {
                    if (lvl->m_levelID == levelId &&
                        self->m_sections[idx].levelCell) {
                      self->m_sections[idx].levelCell->loadFromLevel(lvl);
                      if (self->m_sections[idx].playSpinner)
                        self->m_sections[idx].playSpinner->setVisible(false);
                    } else if (lvl->m_levelID == platLevelId &&
                               self->m_sections[idx].platLevelCell) {
                      self->m_sections[idx].platLevelCell->loadFromLevel(lvl);
                      if (self->m_sections[idx].platPlaySpinner)
                        self->m_sections[idx].platPlaySpinner->setVisible(
                            false);
                    }
                  }
                }
              } else {
                self->m_sections[idx].pendingKey = combinedKey;
                self->m_sections[idx].pendingLevelId = levelId;
                self->m_sections[idx].pendingPlatKey = combinedKey;
                self->m_sections[idx].pendingPlatLevelId = platLevelId;
                if (self->m_sections[idx].playSpinner)
                  self->m_sections[idx].playSpinner->setVisible(true);
                if (self->m_sections[idx].platPlaySpinner)
                  self->m_sections[idx].platPlaySpinner->setVisible(true);
                glm->getOnlineLevels(combinedSearchObj);
              }
            } else {
              auto storedLvls = glm->getStoredOnlineLevels(mainKey.c_str());
              if (storedLvls && storedLvls->count() > 0) {
                if (auto lvl = static_cast<GJGameLevel *>(
                        storedLvls->objectAtIndex(0))) {
                  if (self->m_sections[idx].levelCell) {
                    self->m_sections[idx].levelCell->loadFromLevel(lvl);
                    if (self->m_sections[idx].playSpinner)
                      self->m_sections[idx].playSpinner->setVisible(false);
                    if (self->m_sections[idx].levelCell->m_mainMenu) {
                      self->m_sections[idx].levelCell->m_mainMenu->setPosition(
                          {0, 0});
                      auto viewButton =
                          self->m_sections[idx]
                              .levelCell->m_mainMenu->getChildByID(
                                  "view-button");
                      auto creatorButton =
                          self->m_sections[idx]
                              .levelCell->m_mainMenu->getChildByID(
                                  "creator-name");
                      creatorButton->setPosition({50, 54});
                      creatorButton->setAnchorPoint({0.f, 0.5f});
                      viewButton->setPosition(
                          {self->m_sections[idx]
                               .levelCell->getContentSize()
                               .width,
                           self->m_sections[idx]
                                   .levelCell->getContentSize()
                                   .height /
                               2.f});
                    }
                  }
                }
              } else {
                // Level not cached, request it from server
                self->m_sections[idx].pendingKey = mainKey;
                self->m_sections[idx].pendingLevelId = levelId;
                if (self->m_sections[idx].playSpinner)
                  self->m_sections[idx].playSpinner->setVisible(true);
                glm->getOnlineLevels(mainSearchObj);
              }
            }
          }

          if (!sec || !sec->container)
            return;

          // platformer support: check for dailyPlat/weeklyPlat/monthlyPlat
          // variant
          std::string platKey = key + "Plat";
          if (json.contains(platKey)) {
            auto pobj = json[platKey];
            auto platLevelVal = pobj["levelId"].as<int>();
            if (platLevelVal) {
              int platLevelId = platLevelVal.unwrap();
              sec->platLevelId = platLevelId;
              sec->platSecondsLeft =
                  pobj["secondsLeft"].as<int>().unwrapOrDefault();

              // populate platform LevelCell from cache if available
              if (auto glm = GameLevelManager::sharedState()) {
                auto platSearchObj = GJSearchObject::create(
                    SearchType::Search, fmt::format("{}", platLevelId));
                auto platKey = std::string(platSearchObj->getKey());
                auto storedLvls = glm->getStoredOnlineLevels(platKey.c_str());
                // try to find the exact plat level in the stored results (may
                // contain multiple entries)
                GJGameLevel *platMatch = nullptr;
                if (storedLvls && storedLvls->count() > 0) {
                  for (unsigned int si = 0; si < storedLvls->count(); ++si) {
                    if (auto cand = static_cast<GJGameLevel *>(
                            storedLvls->objectAtIndex(si))) {
                      if (cand->m_levelID == platLevelId) {
                        platMatch = cand;
                        break;
                      }
                    }
                  }
                }

                if (platMatch) {
                  if (sec->platLevelCell) {
                    sec->platLevelCell->loadFromLevel(platMatch);
                    // ensure platformer spinner is hidden after load
                    if (sec->platPlaySpinner)
                      sec->platPlaySpinner->setVisible(false);
                    // Position m_mainMenu after level is loaded
                    if (sec->platLevelCell->m_mainMenu) {
                      sec->platLevelCell->m_mainMenu->setPosition({0, 0});
                      auto viewButton =
                          sec->platLevelCell->m_mainMenu->getChildByID(
                              "view-button");
                      auto creatorButton =
                          sec->platLevelCell->m_mainMenu->getChildByID(
                              "creator-name");
                      creatorButton->setPosition({50, 54});
                      creatorButton->setAnchorPoint({0.f, 0.5f});
                      viewButton->setPosition(
                          {sec->platLevelCell->getContentSize().width,
                           sec->platLevelCell->getContentSize().height / 2.f});
                    }
                  }
                } else {
                  // not cached — request and show spinner
                  sec->pendingPlatKey = platKey;
                  sec->pendingPlatLevelId = platLevelId;
                  if (sec->platPlaySpinner)
                    sec->platPlaySpinner->setVisible(true);
                  glm->getOnlineLevels(platSearchObj);
                }
              }
            }
            std::vector<std::string> timerPrefixes = {
                "Next Daily in ", "Next Weekly in ", "Next Monthly in "};
            if (sec->timerLabel)
              sec->timerLabel->setString(
                  (timerPrefixes[idx] +
                   formatTime(static_cast<long>(sec->secondsLeft)))
                      .c_str());
          };
        });
    return true;
  }
}

void RLEventLayouts::onInfo(CCObject *sender) {
  MDPopup::create(
      "Event Layouts",
      "Play <cg>daily</c>, <cy>weekly</c>, and <cp>monthly</c> rated layouts "
      "curated by the <cr>Layout Admins.</c>\n\n"
      "Each layout features a <cb>unique selection</c> of levels handpicked "
      "for their <co>gameplay and layout design!</c>\n\n"
      "If you want to play <cg>Past Event Layouts</c>, click the "
      "<co>Safe</c> "
      "button at the bottom-left of the popup to view previously event "
      "layouts.\n\n"
      "### <co>Daily Layouts</c> refresh every 24 hours, <cy>Weekly "
      "Layouts</c> every 7 days, and <cp>Monthly Layouts</c> every 30 "
      "days.\n\n"
      "*<cy>All event layouts are set manually so sometimes layouts might be "
      "there for long than expected</c>*\n\n"
      "\r\n\r\n---\r\n\r\n"
      "- <cg>**Daily Layout**</c> showcase <cl>Easy Layouts</c> *(Easy to "
      "Insane Difficulty)* for you to grind and play various layouts\n"
      "- <cy>**Weekly Layout**</c> showcase a more <cr>challenging "
      "layouts</c> "
      "to play *(Easy to Hard Demons Difficulty)*\n"
      "- <cp>**Monthly Layout**</c> showcase themed <cp>Layouts</c> that "
      "relates to the current month or showcase well-made/hyped layouts. "
      "Often "
      "usually showcase <cr>Very hard layouts</c> or <cl>Easy Layouts</c> "
      "depending on the monthly layout has to offer.\n",
      "OK")
      ->show();
}

void RLEventLayouts::update(float dt) {
  std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ",
                                            "Next Monthly in "};
  int idx = static_cast<int>(this->m_eventType);
  if (idx < 0 || idx >= 3)
    return;
  auto &sec = m_sections[idx];
  if (sec.secondsLeft > 0) {
    sec.secondsLeft -= dt;
    if (sec.secondsLeft < 0)
      sec.secondsLeft = 0;
  }
  if (sec.timerLabel)
    sec.timerLabel->setString(
        (timerPrefixes[idx] + formatTime(static_cast<long>(sec.secondsLeft)))
            .c_str());

  // check whether the level was stored yet and populate LevelCells when
  // available.
  auto glm = GameLevelManager::sharedState();
  for (int i = 0; i < 3; ++i) {
    auto &psec = m_sections[i];

    if (!psec.pendingKey.empty()) {
      auto stored = glm->getStoredOnlineLevels(psec.pendingKey.c_str());
      if (stored && stored->count() > 0) {
        GJGameLevel *match = nullptr;
        for (unsigned int si = 0; si < stored->count(); ++si) {
          if (auto cand =
                  static_cast<GJGameLevel *>(stored->objectAtIndex(si))) {
            if (cand->m_levelID == psec.pendingLevelId) {
              match = cand;
              break;
            }
          }
        }

        if (!match) {
          Notification::create("Level ID mismatch", NotificationIcon::Warning)
              ->show();
          psec.pendingKey.clear();
          psec.pendingLevelId = -1;
        } else {
          log::debug("RLEventLayouts: Loading level {} into LevelCell",
                     psec.pendingLevelId);
          if (psec.levelCell) {
            psec.levelCell->loadFromLevel(match);
            if (psec.levelCell->m_mainMenu) {
              psec.levelCell->m_mainMenu->setPosition({0, 0});
              auto viewButton =
                  psec.levelCell->m_mainMenu->getChildByID("view-button");
              auto creatorButton =
                  psec.levelCell->m_mainMenu->getChildByID("creator-name");
              creatorButton->setPosition({50, 54});
              creatorButton->setAnchorPoint({0.f, 0.5f});
              viewButton->setPosition(
                  {psec.levelCell->getContentSize().width,
                   psec.levelCell->getContentSize().height / 2.f});
            }
            if (psec.playSpinner)
              psec.playSpinner->setVisible(false);
          }
          psec.pendingKey.clear();
          psec.pendingLevelId = -1;
        }
      }
    }

    if (!psec.pendingPlatKey.empty()) {
      auto stored = glm->getStoredOnlineLevels(psec.pendingPlatKey.c_str());
      if (stored && stored->count() > 0) {
        GJGameLevel *match = nullptr;
        for (unsigned int si = 0; si < stored->count(); ++si) {
          if (auto cand =
                  static_cast<GJGameLevel *>(stored->objectAtIndex(si))) {
            if (cand->m_levelID == psec.pendingPlatLevelId) {
              match = cand;
              break;
            }
          }
        }

        if (!match) {
          Notification::create("Plat level ID mismatch",
                               NotificationIcon::Warning)
              ->show();
          psec.pendingPlatKey.clear();
          psec.pendingPlatLevelId = -1;
        } else {
          log::debug("RLEventLayouts: Loading plat level {} into PlatCell",
                     psec.pendingPlatLevelId);
          if (psec.platLevelCell) {
            psec.platLevelCell->loadFromLevel(match);
            if (psec.platLevelCell->m_mainMenu) {
              psec.platLevelCell->m_mainMenu->setPosition({0, 0});
              auto viewButton =
                  psec.platLevelCell->m_mainMenu->getChildByID("view-button");
              auto creatorButton =
                  psec.platLevelCell->m_mainMenu->getChildByID("creator-name");
              creatorButton->setPosition({50, 54});
              creatorButton->setAnchorPoint({0.f, 0.5f});
              viewButton->setPosition(
                  {psec.platLevelCell->getContentSize().width,
                   psec.platLevelCell->getContentSize().height / 2.f});
            }
            if (psec.platPlaySpinner)
              psec.platPlaySpinner->setVisible(false);
          }
          psec.pendingPlatKey.clear();
          psec.pendingPlatLevelId = -1;
        }
      }
    }
  }
}

static std::string formatTime(long seconds) {
  if (seconds < 0)
    seconds = 0;
  long days = seconds / 86400;
  seconds %= 86400;
  long hours = seconds / 3600;
  seconds %= 3600;
  long minutes = seconds / 60;
  seconds %= 60;
  char buf[64];
  sprintf(buf, "%02ld:%02ld:%02ld:%02ld", days, hours, minutes, seconds);
  return std::string(buf);
}

void RLEventLayouts::onSafeButton(CCObject *sender) {
  int idx = static_cast<int>(this->m_eventType);
  const char *types[] = {"daily", "weekly", "monthly"};
  const char *typeStr = (idx >= 0 && idx < 3) ? types[idx] : "daily";

  RLLevelBrowserLayer::ParamList params;
  params.emplace_back("safe", std::string(typeStr));
  std::string title =
      utils::string::toUpper(std::string(typeStr) + std::string(" Event Safe"));
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::EventSafe, params, title);
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}
