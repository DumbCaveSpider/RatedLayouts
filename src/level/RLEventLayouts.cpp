#include "RLEventLayouts.hpp"

// global registry of open RLEventLayouts popups
static std::unordered_set<RLEventLayouts*> g_eventLayoutsInstances;
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/Notification.hpp>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

// callbacks for getOnlineLevels responses keyed by search key
static std::unordered_map<std::string, std::vector<std::function<void(cocos2d::CCArray*)>>> g_onlineLevelsCallbacks;

// helper prototypes
static std::string formatTime(long seconds);
static int getDifficulty(int numerator);

RLEventLayouts* RLEventLayouts::create(EventType type) {
      auto ret = new RLEventLayouts();
      ret->m_eventType = type;

      if (ret && ret->initAnchored(420.f, 280.f, "GJ_square01.png")) {
            ret->autorelease();
            return ret;
      }

      delete (ret);
      return nullptr;
};

bool RLEventLayouts::setup() {
      // register instance
      g_eventLayoutsInstances.insert(this);

      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

      auto contentSize = m_mainLayer->getContentSize();

      m_eventMenu = CCLayer::create();
      m_eventMenu->setPosition({contentSize.width / 2, contentSize.height / 2});
      m_eventMenu->setAnchorPoint({0.5f, 0.5f});

      float startY = contentSize.height - 87.f;
      float rowSpacing = 90.f;

      // info button on main layer
      auto infoBtn = CCMenuItemSpriteExtra::create(
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
          this,
          menu_selector(RLEventLayouts::onInfo));
      infoBtn->setPosition({contentSize.width - 25.f, contentSize.height - 25.f});
      m_buttonMenu->addChild(infoBtn);

      // Create a single centered container that represents the chosen EventType
      std::vector<std::string> labels = {"Daily", "Weekly", "Monthly"};
      int idx = static_cast<int>(this->m_eventType);
      if (idx < 0 || idx >= 3) idx = 0;

      // container layer for the chosen event
      auto container = CCLayer::create();
      container->setContentSize({380.f, 116.f});
      container->setAnchorPoint({0.5f, 0.5f});

      const char* bgTex = (idx == 0) ? "GJ_square03.png" : (idx == 1) ? "GJ_square05.png"
                                                                      : "GJ_square04.png";
      auto bgSprite = CCScale9Sprite::create(bgTex);
      if (bgSprite) {
            bgSprite->setContentSize({380.f, 116.f});
            bgSprite->setAnchorPoint({0.5f, 0.5f});
            bgSprite->setPosition({container->getContentSize().width / 2.f, container->getContentSize().height / 2.f});
            container->addChild(bgSprite, -1);
      }

      m_sections[idx].container = container;
      container->setPosition({22, 80});
      m_mainLayer->addChild(container);

      // header label
      auto headerText = labels[idx] + std::string(" Layout");
      auto headerLabel = CCLabelBMFont::create(headerText.c_str(), "goldFont.fnt");
      headerLabel->setAnchorPoint({0.5f, 1.f});
      headerLabel->setScale(1.3f);
      headerLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height - 10.f});
      if (idx == 0)
            headerLabel->setColor({0, 200, 0});
      else if (idx == 1)
            headerLabel->setColor({255, 200, 0});
      else
            headerLabel->setColor({150, 0, 255});
      m_mainLayer->addChild(headerLabel, 3);

      // level name (inside a fixed-width container to cap width at 280.f)
      float nameContainerWidth = 200.f;
      float nameContainerHeight = 20.f;
      auto nameContainer = CCLayer::create();
      nameContainer->setContentSize({nameContainerWidth, nameContainerHeight});
      nameContainer->setPosition({70.f, 65.f});
      nameContainer->setAnchorPoint({0.f, 0.5f});
      container->addChild(nameContainer);
      auto levelNameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      levelNameLabel->setAnchorPoint({0.f, 0.5f});
      levelNameLabel->setPosition({0.f, nameContainerHeight / 2.f});
      levelNameLabel->setScale(0.8f);
      nameContainer->addChild(levelNameLabel);
      m_sections[idx].levelNameLabel = levelNameLabel;

      // rewards label
      auto rewardsLabel = CCLabelBMFont::create("Rewards: -", "bigFont.fnt");
      rewardsLabel->setAnchorPoint({0.5f, 0.5f});
      float rewardsY = m_mainLayer->getContentSize().height - 60.f;
      rewardsLabel->setPosition({m_mainLayer->getContentSize().width / 2.f - 10.f, rewardsY});
      rewardsLabel->setScale(0.65f);
      m_mainLayer->addChild(rewardsLabel, 3);
      m_sections[idx].difficultyValueLabel = rewardsLabel;

      // reward star (to the right of the rewards label)
      auto rewardsStar = CCSprite::create("RL_starMed.png"_spr);
      if (rewardsStar) {
            rewardsStar->setScale(0.9f);
            m_mainLayer->addChild(rewardsStar, 3);
      }
      m_sections[idx].starIcon = rewardsStar;

      // creator menu
      auto creatorMenu = CCMenu::create();
      creatorMenu->setPosition({0, 0});
      creatorMenu->setContentSize(container->getContentSize());
      auto creatorLabel = CCLabelBMFont::create("By", "goldFont.fnt");
      creatorLabel->setAnchorPoint({0.f, 0.5f});
      creatorLabel->setScale(0.8f);

      auto creatorItem = CCMenuItemSpriteExtra::create(creatorLabel, this, menu_selector(RLEventLayouts::onCreatorClicked));
      creatorItem->setTag(0);
      creatorItem->setAnchorPoint({0.f, 0.5f});
      creatorItem->setContentSize({100.f, 12.f});
      creatorItem->setPosition({70.f, 45.f});
      creatorLabel->setPosition({0.f, creatorItem->getContentSize().height / 2.f});
      creatorMenu->addChild(creatorItem);
      container->addChild(creatorMenu, 2);
      m_sections[idx].creatorLabel = creatorLabel;
      m_sections[idx].creatorButton = creatorItem;

      // timer label
      std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
      auto timerLabel = CCLabelBMFont::create((timerPrefixes[idx] + "--:--:--:--").c_str(), "bigFont.fnt");
      timerLabel->setPosition({380.f / 2.f, -6.f});
      timerLabel->setAnchorPoint({.5f, 1.f});
      timerLabel->setScale(0.5f);
      container->addChild(timerLabel);
      m_sections[idx].timerLabel = timerLabel;

      // difficulty sprite
      auto diffSprite = GJDifficultySprite::create(0, GJDifficultyName::Short);
      diffSprite->setPosition({40, 116.f / 2});
      container->addChild(diffSprite);
      m_sections[idx].diff = diffSprite;

      // ensure initial scales and positioning for the rewards label & star
      if (m_sections[idx].difficultyValueLabel) {
            m_sections[idx].difficultyValueLabel->setScale(0.65f);
            m_sections[idx].difficultyValueLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height - 60.f});
      }
      if (m_sections[idx].starIcon && m_sections[idx].difficultyValueLabel) {
            m_sections[idx].starIcon->setScale(0.9f);
            float labelWidth = m_sections[idx].difficultyValueLabel->getContentSize().width * m_sections[idx].difficultyValueLabel->getScaleX();
            float gap = 6.f;
            float centerX = m_mainLayer->getContentSize().width / 2.f;
            float starX = centerX + (labelWidth / 2.f) + gap + (m_sections[idx].starIcon->getContentSize().width * m_sections[idx].starIcon->getScaleX() / 2.f);
            float starY = m_sections[idx].difficultyValueLabel->getPositionY();
            m_sections[idx].starIcon->setPosition({starX, starY});
      }

      // play button
      auto playMenu = CCMenu::create();
      playMenu->setPosition({0, 0});
      auto playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      if (!playSprite) playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      playSprite->setScale(0.65f);
      auto playButton = CCMenuItemSpriteExtra::create(playSprite, this, menu_selector(RLEventLayouts::onPlayEvent));
      playButton->setPosition({330.f, 116.f / 2});
      playButton->setAnchorPoint({0.5f, 0.5f});
      playMenu->addChild(playButton);
      container->addChild(playMenu, 2);
      m_sections[idx].playButton = playButton;

      // safe button
      auto safeMenu = CCMenu::create();
      safeMenu->setPosition({0, 0});
      auto safeSprite = CCSprite::createWithSpriteFrameName("GJ_safeBtn_001.png");
      if (safeSprite) {
            safeSprite->setScale(0.9f);
            auto safeButton = CCMenuItemSpriteExtra::create(safeSprite, this, menu_selector(RLEventLayouts::onSafeButton));
            safeButton->setPosition({25.f, 25.f});
            safeMenu->addChild(safeButton);
            m_mainLayer->addChild(safeMenu, 3);
      }

      this->scheduleUpdate();

      // Fetch event info from server
      {
            Ref<RLEventLayouts> selfRef = this;
            web::WebRequest().get("https://gdrate.arcticwoof.xyz/getEvent").listen([selfRef](web::WebResponse* res) {
                  if (!selfRef) return;  // popup was destroyed
                  if (!res || !res->ok()) {
                        Notification::create("Failed to fetch event info", NotificationIcon::Error)->show();
                        return;
                  }
                  auto jsonResult = res->json();
                  if (!jsonResult) {
                        Notification::create("Invalid event JSON", NotificationIcon::Warning)->show();
                        return;
                  }
                  auto json = jsonResult.unwrap();

                  std::vector<std::string> keys = {"daily", "weekly", "monthly"};
                  int idx = static_cast<int>(selfRef->m_eventType);
                  if (idx < 0 || idx >= 3) return;
                  const auto& key = keys[idx];
                  if (!json.contains(key)) return;
                  auto obj = json[key];
                  auto levelIdValue = obj["levelId"].as<int>();
                  if (!levelIdValue) return;
                  auto levelId = levelIdValue.unwrap();

                  selfRef->m_sections[idx].levelId = levelId;
                  selfRef->m_sections[idx].secondsLeft = obj["secondsLeft"].as<int>().unwrapOrDefault();

                  auto levelName = obj["levelName"].as<std::string>().unwrapOrDefault();
                  auto creator = obj["creator"].as<std::string>().unwrapOrDefault();
                  auto difficulty = obj["difficulty"].as<int>().unwrapOrDefault();
                  auto accountId = obj["accountId"].as<int>().unwrapOrDefault();
                  auto featured = obj["featured"].as<int>().unwrapOrDefault();

                  auto sec = &selfRef->m_sections[idx];
                  if (!sec || !sec->container) return;
                  auto nameLabel = sec->levelNameLabel;
                  auto creatorLabel = sec->creatorLabel;
                  if (nameLabel) {
                        nameLabel->setString(levelName.c_str());
                        const float maxNameWidth = 200.f;
                        const float baseNameScale = 0.8f;
                        nameLabel->setScale(baseNameScale);
                        float labelWidth = nameLabel->getContentSize().width * nameLabel->getScaleX();
                        if (labelWidth > maxNameWidth) {
                              float newScale = maxNameWidth / nameLabel->getContentSize().width;
                              nameLabel->setScale(newScale);
                        }
                  }
                  if (creatorLabel) {
                        creatorLabel->setString(("By " + creator).c_str());
                        creatorLabel->setAnchorPoint({0.f, 0.5f});
                  };

                  if (sec->difficultyValueLabel) {
                        // Remove any old star and create a fresh RL_starMed star to the right of the rewards text
                        sec->difficultyValueLabel->setString(("Rewards: " + numToString(difficulty)).c_str());
                        sec->difficultyValueLabel->setScale(0.65f);
                        float centerX = selfRef->m_mainLayer->getContentSize().width / 2.f;
                        float labelY = selfRef->m_mainLayer->getContentSize().height - 60.f;
                        sec->difficultyValueLabel->setPosition({centerX, labelY});

                        // recreate star to ensure correct sprite (RL_starMed)
                        if (sec->starIcon) {
                              sec->starIcon->removeFromParent();
                              sec->starIcon = nullptr;
                        }
                        auto newStar = CCSprite::create("RL_starMed.png"_spr);
                        if (newStar) {
                              newStar->setScale(0.9f);
                              float labelWidth = sec->difficultyValueLabel->getContentSize().width * sec->difficultyValueLabel->getScaleX();
                              float gap = 6.f;
                              float starX = centerX + (labelWidth / 2.f) + gap + (newStar->getContentSize().width * newStar->getScaleX() / 2.f);
                              newStar->setPosition({starX, labelY});
                              selfRef->m_mainLayer->addChild(newStar, 3);
                              sec->starIcon = newStar;
                        }
                  }
                  if (sec->diff) {
                        sec->diff->updateDifficultyFrame(getDifficulty(difficulty), GJDifficultyName::Long);
                        if (featured == 1 || featured == 2) {
                              sec->featured = featured;
                              const char* coinSprite = (featured == 1) ? "rlfeaturedCoin.png"_spr : "rlepicFeaturedCoin.png"_spr;
                              if (sec->featuredIcon) {
                                    sec->featuredIcon->removeFromParent();
                                    sec->featuredIcon = nullptr;
                              }
                              auto coinIcon = CCSprite::create(coinSprite);
                              if (coinIcon) {
                                    coinIcon->setPosition({sec->diff->getContentSize().width / 2.f, sec->diff->getContentSize().height / 2.f});
                                    coinIcon->setZOrder(-1);
                                    sec->diff->addChild(coinIcon);
                                    sec->featuredIcon = coinIcon;
                              }
                        } else {
                              if (sec->featuredIcon) {
                                    sec->featuredIcon->removeFromParent();
                                    sec->featuredIcon = nullptr;
                                    sec->featured = 0;
                              }
                        }
                  }
                  sec->accountId = accountId;
                  if (sec->creatorButton) {
                        sec->creatorButton->setTag(accountId);
                        sec->creatorButton->setPosition({70.f, 40.f});
                        sec->creatorButton->setContentSize({sec->creatorLabel->getContentSize().width * sec->creatorLabel->getScaleX(), 12.f});
                        sec->creatorLabel->setScale(0.8f);
                  }
                  if (sec->playButton) {
                        sec->playButton->setTag(levelId);
                  }
                  std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
                  if (sec->timerLabel) sec->timerLabel->setString((timerPrefixes[idx] + formatTime((long)sec->secondsLeft)).c_str());
            });
      }

      return true;
}

void RLEventLayouts::onInfo(CCObject* sender) {
      MDPopup::create(
          "Event Layouts",
          "Play <cg>daily</c>, <cy>weekly</c>, and <cp>monthly</c> rated layouts curated by the <cr>Layout Admins.</c>\n\n"
          "Each layout features a <cb>unique selection</c> of levels handpicked for their <co>gameplay and layout design!</c>\n\n"
          "If you want to play <cg>Past Event Layouts</c>, click the <co>Safe</c> button at the bottom-left of the popup to view previously event layouts.\n\n"
          "### <co>Daily Layouts</c> refresh every 24 hours, <cy>Weekly Layouts</c> every 7 days, and <cp>Monthly Layouts</c> every 30 days.\n\n"
          "\r\n\r\n---\r\n\r\n"
          "- <cg>**Daily Layout**</c> showcase <cl>easy layouts</c> *(Easy to Insane Difficulty)* for you to grind and play various layouts\n"
          "- <cy>**Weekly Layout**</c> offer a bit more <cr>challenging layouts</c> *(Easy to Hard Demons Difficulty)*\n"
          "- <cp>**Monthly Layout**</c> shows special events/themed layouts like <cl>Verification Bounties</c> and other special activities.\n",
          "OK")
          ->show();
}

void RLEventLayouts::update(float dt) {
      std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
      int idx = static_cast<int>(this->m_eventType);
      if (idx < 0 || idx >= 3) return;
      auto& sec = m_sections[idx];
      if (sec.secondsLeft <= 0) return;
      sec.secondsLeft -= dt;
      if (sec.secondsLeft < 0) sec.secondsLeft = 0;
      if (sec.timerLabel) sec.timerLabel->setString((timerPrefixes[idx] + formatTime((long)sec.secondsLeft)).c_str());
}

static std::string formatTime(long seconds) {
      if (seconds < 0) seconds = 0;
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

static int getDifficulty(int numerator) {
      int difficultyLevel = 0;
      switch (numerator) {
            case 1:
                  difficultyLevel = -1;
                  break;
            case 2:
                  difficultyLevel = 1;
                  break;
            case 3:
                  difficultyLevel = 2;
                  break;
            case 4:
            case 5:
                  difficultyLevel = 3;
                  break;
            case 6:
            case 7:
                  difficultyLevel = 4;
                  break;
            case 8:
            case 9:
                  difficultyLevel = 5;
                  break;
            case 10:
                  difficultyLevel = 7;
                  break;
            case 15:
                  difficultyLevel = 8;
                  break;
            case 20:
                  difficultyLevel = 6;
                  break;
            case 25:
                  difficultyLevel = 9;
                  break;
            case 30:
                  difficultyLevel = 10;
                  break;
            default:
                  difficultyLevel = 0;
      }
      return difficultyLevel;
}

void RLEventLayouts::onCreatorClicked(CCObject* sender) {
      if (!m_mainLayer) return;
      auto menuItem = static_cast<CCMenuItem*>(sender);
      if (!menuItem) return;
      int accountId = menuItem->getTag();
      if (accountId <= 0) return;
      ProfilePage::create(accountId, false)->show();
}

void RLEventLayouts::onPlayEvent(CCObject* sender) {
      if (!m_mainLayer) return;
      auto menuItem = static_cast<CCMenuItem*>(sender);
      if (!menuItem) return;
      int levelId = menuItem->getTag();
      if (levelId <= 0) return;

      // fetch level metadata and push to LevelInfoLayer scene
      auto searchObj = GJSearchObject::create(SearchType::Search, numToString(levelId));
      auto key = std::string(searchObj->getKey());
      auto glm = GameLevelManager::sharedState();

      // Check if we already have the level stored
      auto stored = glm->getStoredOnlineLevels(key.c_str());
      if (stored && stored->count() > 0) {
            auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
            if (level && level->m_levelID == levelId) {
                  log::debug("RLEventLayouts: Opening LevelInfoLayer for level id {} (from cache)", levelId);
                  auto scene = LevelInfoLayer::scene(level, false);
                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                  CCDirector::sharedDirector()->pushScene(transitionFade);
                  return;
            }
      }

      // If not stored, fetch from server and push when ready
      Ref<RLEventLayouts> selfRef = this;
      g_onlineLevelsCallbacks[key].push_back([selfRef, levelId](cocos2d::CCArray* levels) {
            if (!levels || levels->count() == 0) {
                  Notification::create("Level not found", NotificationIcon::Error)->show();
                  return;
            }
            auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
            if (!level || level->m_levelID != levelId) {
                  Notification::create("Level ID mismatch", NotificationIcon::Warning)->show();
                  return;
            }
            log::debug("RLEventLayouts: Opening LevelInfoLayer for level id {} (from server)", levelId);
            auto scene = LevelInfoLayer::scene(level, false);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
      });

      glm->getOnlineLevels(searchObj);
}

void RLEventLayouts::onSafeButton(CCObject* sender) {
      // safe list depending on event type
      int idx = static_cast<int>(this->m_eventType);
      const char* types[] = {"daily", "weekly", "monthly"};
      const char* typeStr = (idx >= 0 && idx < 3) ? types[idx] : "daily";
      std::string url = std::string("https://gdrate.arcticwoof.xyz/getEvent?safe=") + typeStr;

      Ref<RLEventLayouts> selfRef = this;
      web::WebRequest().get(url).listen([selfRef](web::WebResponse* res) {
            if (!selfRef) return;  // popup destroyed kaboom
            if (!res || !res->ok()) {
                  Notification::create("Failed to fetch safe list", NotificationIcon::Error)->show();
                  return;
            }
            auto jsonRes = res->json();
            if (!jsonRes) {
                  Notification::create("Invalid safe list response", NotificationIcon::Warning)->show();
                  return;
            }
            auto json = jsonRes.unwrap();
            // Expecting an array of level IDs
            std::string levelIDs;
            bool first = true;
            for (auto v : json) {
                  auto idVal = v.as<int>();
                  if (!idVal) continue;
                  if (!first) levelIDs += ",";
                  levelIDs += numToString(idVal.unwrap());
                  first = false;
            }
            if (levelIDs.empty()) {
                  Notification::create("No levels returned", NotificationIcon::Warning)->show();
                  return;
            }
            auto searchObj = GJSearchObject::create(SearchType::Type19, levelIDs);
            auto browserLayer = LevelBrowserLayer::create(searchObj);
            auto scene = CCScene::create();
            scene->addChild(browserLayer);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
      });
}