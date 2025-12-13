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

RLEventLayouts* RLEventLayouts::create() {
      auto ret = new RLEventLayouts();

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

      float startY = contentSize.height - 87.f;
      float rowSpacing = 90.f;

      // info button on main layer
      auto infoBtn = CCMenuItemSpriteExtra::create(
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
          this,
          menu_selector(RLEventLayouts::onInfo));
      infoBtn->setPosition({contentSize.width - 25.f, contentSize.height - 25.f});
      m_buttonMenu->addChild(infoBtn);

      std::vector<std::string> labels = {"Daily", "Weekly", "Monthly"};
      for (int i = 0; i < 3; ++i) {
            // container layer so each event row has its own independent layer
            auto container = CCLayer::create();
            // ensure the container has the expected size
            container->setContentSize({360.f, 64.f});
            container->setAnchorPoint({0, 0.5f});

            // determine correct background for event type
            float cellW = 360.f;
            float cellH = 64.f;
            const char* bgTex = "GJ_square03.png";  // daily default
            if (i == 1) bgTex = "GJ_square05.png";  // weekly
            if (i == 2) bgTex = "GJ_square04.png";  // monthly

            // create a scale9 sprite background
            auto bgSprite = CCScale9Sprite::create(bgTex);
            if (bgSprite) {
                  bgSprite->setContentSize({cellW, cellH});
                  bgSprite->setAnchorPoint({0.f, 0.f});
                  bgSprite->setPosition({0.f, 0.f});
                  container->addChild(bgSprite, -1);
            }
            m_sections[i].container = container;
            float startX = (contentSize.width - cellW) / 2.f;
            container->setPosition({startX, startY - i * rowSpacing});
            m_mainLayer->addChild(container);

            // header label for the row
            auto headerText = labels[i] + std::string(" Layout");
            auto headerLabel = CCLabelBMFont::create(headerText.c_str(), "bigFont.fnt");
            headerLabel->setScale(0.5f);
            headerLabel->setAnchorPoint({0.5f, 0.f});
            headerLabel->setPosition({cellW / 2.f, cellH + 2.f});
            if (i == 0) {
                  headerLabel->setColor({0, 200, 0});  // green
            } else if (i == 1) {
                  headerLabel->setColor({255, 200, 0});  // yellow
            } else {
                  headerLabel->setColor({150, 0, 255});  // purple
            }
            container->addChild(headerLabel, 3);

            // Add a label for level title
            auto levelNameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
            levelNameLabel->setPosition({55.f, 43.f});
            levelNameLabel->setAnchorPoint({0.f, 0.5f});
            levelNameLabel->setScale(0.5f);
            container->addChild(levelNameLabel);
            m_sections[i].levelNameLabel = levelNameLabel;

            // difficulty value label
            auto difficultyValueLabel = CCLabelBMFont::create("-", "bigFont.fnt");
            difficultyValueLabel->setAnchorPoint({0.f, 0.5f});
            difficultyValueLabel->setScale(0.35f);
            difficultyValueLabel->setPosition({levelNameLabel->getPositionX() + levelNameLabel->getContentSize().width * levelNameLabel->getScaleX() + 12.f, 43.f});
            container->addChild(difficultyValueLabel);
            m_sections[i].difficultyValueLabel = difficultyValueLabel;

            // star icon (to the right of difficulty value)
            auto starIcon = CCSprite::create("rlStarIcon.png"_spr);
            if (starIcon) {
                  starIcon->setAnchorPoint({0.f, 0.5f});
                  starIcon->setScale(0.8f);
                  starIcon->setPosition({difficultyValueLabel->getPositionX() + difficultyValueLabel->getContentSize().width * difficultyValueLabel->getScaleX() + 2.f, difficultyValueLabel->getPositionY()});
                  container->addChild(starIcon);
            }
            m_sections[i].starIcon = starIcon;

            // create a menu for this creatorItem and add it to the container
            auto creatorMenu = CCMenu::create();
            creatorMenu->setPosition({0, 0});
            creatorMenu->setContentSize(container->getContentSize());
            // creator label
            auto creatorLabel = CCLabelBMFont::create("By", "goldFont.fnt");
            creatorLabel->setAnchorPoint({0.5f, 0.5f});
            creatorLabel->setScale(0.6f);

            auto creatorItem = CCMenuItemSpriteExtra::create(creatorLabel, this, menu_selector(RLEventLayouts::onCreatorClicked));
            creatorItem->setTag(0);
            creatorItem->setAnchorPoint({0.f, 0.5f});
            creatorItem->setContentSize({100.f, 12.f});  // maybe have to preset this? cuz of the thingy bug wha
            creatorItem->setPosition({55.f, 22.f});
            creatorLabel->setPosition({0.f, creatorItem->getContentSize().height / 2.f});
            creatorLabel->setAnchorPoint({0.f, 0.5f});

            creatorMenu->addChild(creatorItem);
            container->addChild(creatorMenu, 2);
            m_sections[i].creatorLabel = creatorLabel;
            m_sections[i].creatorButton = creatorItem;

            // timer label on right side
            std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
            auto timerLabel = CCLabelBMFont::create((timerPrefixes[i] + "--:--:--:--").c_str(), "bigFont.fnt");
            timerLabel->setPosition({cellW - 5.f, 10.f});
            timerLabel->setAnchorPoint({1.f, 0.5f});
            timerLabel->setScale(0.25f);
            container->addChild(timerLabel);
            m_sections[i].timerLabel = timerLabel;

            // difficulty sprite
            auto diffSprite = GJDifficultySprite::create(0, GJDifficultyName::Short);
            diffSprite->setPosition({30, cellH / 2});
            diffSprite->setScale(0.8f);
            container->addChild(diffSprite);
            m_sections[i].diff = diffSprite;

            // play button on the right side
            auto playMenu = CCMenu::create();
            playMenu->setPosition({0, 0});
            auto playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
            if (!playSprite) playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
            playSprite->setScale(0.5f);
            auto playButton = CCMenuItemSpriteExtra::create(playSprite, this, menu_selector(RLEventLayouts::onPlayEvent));
            playButton->setPosition({cellW - 32.f, cellH / 2 + 2.5f});
            playButton->setAnchorPoint({0.5f, 0.5f});
            playMenu->addChild(playButton);
            container->addChild(playMenu, 2);
            // add spinner for play button (default visible)
            auto spinner = LoadingSpinner::create(32.f);
            spinner->setPosition({cellW - 32.f, cellH / 2 + 2.5f});
            spinner->setVisible(true);
            playButton->setVisible(false);
            playMenu->addChild(spinner);
            m_sections[i].spinner = spinner;
            m_sections[i].playButton = playButton;
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
                  for (int idx = 0; idx < 3; ++idx) {
                        const auto& key = keys[idx];
                        if (!json.contains(key)) continue;
                        auto obj = json[key];
                        auto levelIdValue = obj["levelId"].as<int>();
                        if (!levelIdValue) continue;
                        auto levelId = levelIdValue.unwrap();

                        if (idx < 0 || idx >= 3) continue;  // safety bounds check

                        selfRef->m_sections[idx].levelId = levelId;
                        selfRef->m_sections[idx].secondsLeft = obj["secondsLeft"].as<int>().unwrapOrDefault();

                        auto levelName = obj["levelName"].as<std::string>().unwrapOrDefault();
                        auto creator = obj["creator"].as<std::string>().unwrapOrDefault();
                        auto difficulty = obj["difficulty"].as<int>().unwrapOrDefault();
                        auto accountId = obj["accountId"].as<int>().unwrapOrDefault();
                        auto featured = obj["featured"].as<int>().unwrapOrDefault();

                        // update UI
                        auto sec = &selfRef->m_sections[idx];
                        if (!sec || !sec->container) continue;
                        auto nameLabel = sec->levelNameLabel;
                        auto creatorLabel = sec->creatorLabel;
                        if (nameLabel) nameLabel->setString(levelName.c_str());
                        if (creatorLabel) creatorLabel->setString(("By " + creator).c_str());

                        // dynamically position difficulty value and star based on scaled widths
                        if (nameLabel && sec->difficultyValueLabel) {
                              float nameRightX = nameLabel->getPositionX() + nameLabel->getContentSize().width * nameLabel->getScaleX();
                              float diffX = nameRightX + 12.f;
                              sec->difficultyValueLabel->setString(numToString(difficulty).c_str());
                              sec->difficultyValueLabel->setPosition({diffX, nameLabel->getPositionY()});

                              // compute width of difficulty label after setting text
                              float diffWidth = sec->difficultyValueLabel->getContentSize().width * sec->difficultyValueLabel->getScaleX();

                              // position star icon to the right
                              if (sec->starIcon) {
                                    sec->starIcon->setPosition({diffX + diffWidth + 3.f, nameLabel->getPositionY()});
                              }
                        }
                        if (sec->diff) {
                              sec->diff->updateDifficultyFrame(getDifficulty(difficulty), GJDifficultyName::Long);

                              // featured/epic coin (place on diff sprite)
                              if (featured == 1 || featured == 2) {
                                    sec->featured = featured;
                                    const char* coinSprite = (featured == 1) ? "rlfeaturedCoin.png"_spr : "rlepicFeaturedCoin.png"_spr;

                                    // remove old featured icon if exists
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
                              sec->creatorButton->setPosition({55.f, 22.f});
                              sec->creatorButton->setContentSize({creatorLabel->getContentSize().width * creatorLabel->getScaleX(), 12.f});
                        }
                        // set level id on play button, update UI based on download state
                        if (sec->playButton) {
                              sec->playButton->setTag(levelId);
                              auto glm = GameLevelManager::sharedState();
                              if (glm->hasDownloadedLevel(levelId)) {
                                    sec->playButton->setEnabled(true);
                                    sec->playButton->setVisible(true);
                                    if (sec->spinner) {
                                          sec->spinner->setVisible(false);
                                          if (sec->playButton) sec->playButton->setVisible(true);
                                    }
                                    // clear background download flag
                                    selfRef->m_backgroundDownloads.erase(levelId);
                              } else {
                                    // if a background download is pending, show spinner and disable play
                                    if (selfRef->m_backgroundDownloads.find(levelId) != selfRef->m_backgroundDownloads.end()) {
                                          sec->playButton->setEnabled(false);
                                          sec->playButton->setVisible(false);
                                          if (sec->spinner) {
                                                sec->spinner->setVisible(true);
                                                if (sec->playButton) sec->playButton->setVisible(false);
                                          }
                                    } else {
                                          // otherwise, show play button enabled by default
                                          sec->playButton->setEnabled(true);
                                          sec->playButton->setVisible(true);
                                          if (sec->spinner) {
                                                sec->spinner->setVisible(false);
                                                if (sec->playButton) sec->playButton->setVisible(true);
                                          }
                                    }
                              }
                        }
                        std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
                        if (sec->timerLabel) sec->timerLabel->setString((timerPrefixes[idx] + formatTime((long)sec->secondsLeft)).c_str());
                  }
                  // prefetch the three event levels (type19 search with comma-separated ids)
                  {
                        std::string prefetchIds;
                        bool firstPrefetch = true;
                        for (int i = 0; i < 3; ++i) {
                              auto id = selfRef->m_sections[i].levelId;
                              if (id <= 0) continue;
                              if (!firstPrefetch) prefetchIds += ",";
                              prefetchIds += numToString(id);
                              firstPrefetch = false;
                        }
                        if (!prefetchIds.empty()) {
                              auto searchObj = GJSearchObject::create(SearchType::Type19, prefetchIds);
                              auto key = std::string(searchObj->getKey());
                              auto glm = GameLevelManager::sharedState();
                              // register a callback to download levels when metadata arrives
                              g_onlineLevelsCallbacks[key].push_back([selfRef, glm](cocos2d::CCArray* levels) {
                                    if (!levels || levels->count() == 0) return;
                                    for (int i = 0; i < levels->count(); ++i) {
                                          auto lvl = static_cast<GJGameLevel*>(levels->objectAtIndex(i));
                                          if (!lvl) continue;
                                          int id = lvl->m_levelID;
                                          if (id <= 0) continue;
                                          // if not downloaded, initiate a download
                                          if (!glm->hasDownloadedLevel(id)) {
                                                selfRef->m_backgroundDownloads.insert(id);
                                                // start background download and set spinner visible on UI if present
                                                log::info("RLEventLayouts: Starting background download for level id {}", id);
                                                glm->downloadLevel(id, false);
                                                for (int j = 0; j < 3; ++j) {
                                                      if (selfRef->m_sections[j].levelId == id && selfRef->m_sections[j].spinner) {
                                                            selfRef->m_sections[j].spinner->setVisible(true);
                                                            if (selfRef->m_sections[j].playButton) selfRef->m_sections[j].playButton->setVisible(false);
                                                            break;
                                                      }
                                                }
                                          }
                                    }
                              });
                              GameLevelManager::sharedState()->getOnlineLevels(searchObj);
                        }
                  }
            });
      }

      return true;
}

void RLEventLayouts::onInfo(CCObject* sender) {
      MDPopup::create(
          "Event Layouts",
          "Play <cg>daily</c>, <cy>weekly</c>, and <cp>monthly</c> rated layouts curated by the <cr>Layout Admins.</c>\n\n"
          "Each layout features a <cb>unique selection</c> of levels handpicked for their <co>design and quality!</c>\n\n"
          "### <co>Daily Layouts</c> refresh every 24 hours, <cy>Weekly Layouts</c> every 7 days, and <cp>Monthly Layouts</c> every 30 days.\n\n"
          "\r\n\r\n---\r\n\r\n"
          "- <cg>**Daily Layouts**</c> showcase easy layouts *(mostly levels from 2-9 stars)* for you to grind and play various layouts\n\n"
          "- <cy>**Weekly Layouts**</c> offer a bit more <cr>challenge</c> *(Easy to Hard Demons)*\n\n"
          "- <cp>**Monthly Layouts**</c> hold special events like <cl>Verification Bounties</c> and other special activities.\n\n",
          "OK")
          ->show();
}

RLEventLayouts::~RLEventLayouts() {
      g_eventLayoutsInstances.erase(this);
}

void RLEventLayouts::update(float dt) {
      std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
      for (int i = 0; i < 3; ++i) {
            auto& sec = m_sections[i];
            if (sec.secondsLeft <= 0) continue;
            sec.secondsLeft -= dt;
            if (sec.secondsLeft < 0) sec.secondsLeft = 0;
            if (sec.timerLabel) sec.timerLabel->setString((timerPrefixes[i] + formatTime((long)sec.secondsLeft)).c_str());
      }

      // Check background downloads and clear completed ones
      if (!m_backgroundDownloads.empty()) {
            std::vector<int> bgCompleted;
            auto glm = GameLevelManager::sharedState();
            for (auto id : m_backgroundDownloads) {
                  if (glm->hasDownloadedLevel(id)) {
                        bgCompleted.push_back(id);
                  }
            }
            for (auto id : bgCompleted) {
                  m_backgroundDownloads.erase(id);
            }
      }
}

void RLEventLayouts::onDownloadCompleted(int id) {
      m_backgroundDownloads.erase(id);
      restoreUIForLevel(id);
}

void RLEventLayouts::onDownloadFailed(int id) {
      m_backgroundDownloads.erase(id);
      restoreUIForLevel(id);
}

void RLEventLayouts::restoreUIForLevel(int id) {
      for (int i = 0; i < 3; ++i) {
            if (m_sections[i].levelId == id) {
                  if (m_sections[i].spinner) m_sections[i].spinner->setVisible(false);
                  if (m_sections[i].playButton) {
                        m_sections[i].playButton->setVisible(true);
                        m_sections[i].playButton->setEnabled(true);
                  }
                  break;
            }
      }
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

// Hook LevelBrowserLayer::loadLevelsFinished to dispatch callbacks when online levels load
class $modify(RLLevelBrowserLayer, LevelBrowserLayer) {
      void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) {
            LevelBrowserLayer::loadLevelsFinished(levels, key, type);
            if (!key) return;
            std::string k = key;
            auto it = g_onlineLevelsCallbacks.find(k);
            if (it == g_onlineLevelsCallbacks.end()) return;
            for (auto& cb : it->second) {
                  cb(levels);
            }
            g_onlineLevelsCallbacks.erase(it);
      }
};

// Helper to parse a level ID from a tag string (extract first numeric substring)
static int parseLevelIdFromTag(const std::string& tag) {
      std::string digits;
      for (char c : tag) {
            if (std::isdigit(static_cast<unsigned char>(c)))
                  digits.push_back(c);
            else if (!digits.empty())
                  break;
      }
      if (digits.empty()) return -1;
      return atoi(digits.c_str());
}

class $modify(RLGameLevelManager, GameLevelManager) {
      void processOnDownloadLevelCompleted(gd::string response, gd::string tag, bool update) {
            GameLevelManager::processOnDownloadLevelCompleted(response, tag, update);
            std::string res = response;
            std::string t = tag;
            int id = parseLevelIdFromTag(t);
            if (id <= 0) return;
            if (res == "-1") {
                  log::error("Download failed for level id {} tag {}", id, t);
                  for (auto popup : g_eventLayoutsInstances) {
                        if (!popup) continue;
                        popup->onDownloadFailed(id);
                  }
            } else {
                  log::info("Download completed for level id {}: {}", id, res);
                  for (auto popup : g_eventLayoutsInstances) {
                        if (!popup) continue;
                        popup->onDownloadCompleted(id);
                  }
            }
      }
};