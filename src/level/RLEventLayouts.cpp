#include "RLEventLayouts.hpp"

#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/ui/Notification.hpp>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

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
      setTitle("Event Layouts");
      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

      auto contentSize = m_mainLayer->getContentSize();

      m_eventMenu = CCLayer::create();
      m_eventMenu->setPosition({contentSize.width / 2, contentSize.height / 2});

      float startY = contentSize.height - 100.f;
      float rowSpacing = 80.f;

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
            if (!playSprite) playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");\
            playSprite->setScale(0.5f);
            auto playButton = CCMenuItemSpriteExtra::create(playSprite, this, menu_selector(RLEventLayouts::onPlayEvent));
            playButton->setPosition({cellW - 32.f, cellH / 2 + 1.f});
            playButton->setAnchorPoint({0.5f, 0.5f});
            playMenu->addChild(playButton);
            container->addChild(playMenu, 2);
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
                              sec->difficultyValueLabel->setString(std::to_string(difficulty).c_str());
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
                        // set level id on play button so we can download when clicked
                        if (sec->playButton) {
                              sec->playButton->setTag(levelId);
                        }
                        std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
                        if (sec->timerLabel) sec->timerLabel->setString((timerPrefixes[idx] + formatTime((long)sec->secondsLeft)).c_str());
                  }
            });
      }

      return true;
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

}