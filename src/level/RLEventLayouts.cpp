#include "RLEventLayouts.hpp"

#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include "../custom/RLLevelBrowserLayer.hpp"
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
          infoSpr,
          this,
          menu_selector(RLEventLayouts::onInfo));
      infoBtn->setPosition({contentSize.width - 25.f, contentSize.height - 25.f});
      m_buttonMenu->addChild(infoBtn);

      // Create a single centered container that represents the chosen EventType
      std::vector<std::string> labels = {"Daily", "Weekly", "Monthly"};
      int idx = static_cast<int>(this->m_eventType);
      if (idx < 0 || idx >= 3) idx = 0;

      // container layer for the chosen event (main: classic layouts)
      auto container = CCLayer::create();
      // event container size set to 380x84
      container->setContentSize({380.f, 84.f});
      container->setAnchorPoint({0.5f, 0.5f});

      const char* bgTex = (idx == 0) ? "GJ_square03.png" : (idx == 1) ? "GJ_square05.png"
                                                                      : "GJ_square04.png";
      auto bgSprite = CCScale9Sprite::create(bgTex);
      if (bgSprite) {
            bgSprite->setContentSize({380.f, 84.f});
            bgSprite->setAnchorPoint({0.5f, 0.5f});
            bgSprite->setPosition({container->getContentSize().width / 2.f, container->getContentSize().height / 2.f});
            container->addChild(bgSprite, -1);
      }

      m_sections[idx].container = container;
      const float containerTopY = 145.f;
      container->setPosition({22, containerTopY});
      m_mainLayer->addChild(container);

      // also create platformer (planet) container directly underneath the main container
      auto platContainer = CCLayer::create();
      platContainer->setContentSize({380.f, 84.f});
      platContainer->setAnchorPoint({0.5f, 0.5f});
      auto platBg = CCScale9Sprite::create(bgTex);
      if (platBg) {
            platBg->setContentSize({380.f, 84.f});
            platBg->setAnchorPoint({0.5f, 0.5f});
            platBg->setPosition({platContainer->getContentSize().width / 2.f, platContainer->getContentSize().height / 2.f});
            platContainer->addChild(platBg, -1);
      }
      m_sections[idx].platContainer = platContainer;
      // space of 6px between containers
      float platY = containerTopY - (container->getContentSize().height / 2.f + platContainer->getContentSize().height / 2.f + 6.f);
      platContainer->setPosition({22, platY});
      m_mainLayer->addChild(platContainer);

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
      const float nameContainerWidth = 200.f;
      const float nameContainerHeight = 18.f;
      auto nameContainer = CCLayer::create();
      nameContainer->setContentSize({nameContainerWidth, nameContainerHeight});
      const float nameY = container->getContentSize().height * 0.6f;
      nameContainer->setPosition({70.f, nameY});
      nameContainer->setAnchorPoint({0.f, 0.5f});
      container->addChild(nameContainer);
      auto levelNameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      levelNameLabel->setAnchorPoint({0.f, 0.5f});
      levelNameLabel->setPosition({0.f, nameContainerHeight / 2.f});
      levelNameLabel->setScale(0.8f);
      nameContainer->addChild(levelNameLabel);
      m_sections[idx].levelNameLabel = levelNameLabel;

      // platformer name container
      auto platNameContainer = CCLayer::create();
      platNameContainer->setContentSize({nameContainerWidth, nameContainerHeight});
      float platNameY = platContainer->getContentSize().height * 0.6f;
      platNameContainer->setPosition({70.f, platNameY});
      platNameContainer->setAnchorPoint({0.f, 0.5f});
      platContainer->addChild(platNameContainer);
      auto platLevelNameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      platLevelNameLabel->setAnchorPoint({0.f, 0.5f});
      platLevelNameLabel->setPosition({0.f, nameContainerHeight / 2.f});
      platLevelNameLabel->setScale(0.8f);
      platNameContainer->addChild(platLevelNameLabel);
      m_sections[idx].platLevelNameLabel = platLevelNameLabel;

      // rewards label (classic)
      auto rewardsLabel = CCLabelBMFont::create("Rewards: -", "bigFont.fnt");
      rewardsLabel->setAnchorPoint({0.5f, 0.5f});
      rewardsLabel->setScale(0.4f);
      float initialCenterX = container->getContentSize().width / 2.f;
      float initialLabelY = 12.f;
      rewardsLabel->setPosition({initialCenterX, initialLabelY});
      container->addChild(rewardsLabel, 2);
      m_sections[idx].difficultyValueLabel = rewardsLabel;

      // rewards label (platformer)
      auto platRewardsLabel = CCLabelBMFont::create("Rewards: -", "bigFont.fnt");
      platRewardsLabel->setAnchorPoint({0.5f, 0.5f});
      platRewardsLabel->setScale(0.4f);
      float initialCenterXp = platContainer->getContentSize().width / 2.f;
      float initialLabelYp = 12.f;
      platRewardsLabel->setPosition({initialCenterXp, initialLabelYp});
      platContainer->addChild(platRewardsLabel, 2);
      m_sections[idx].platDifficultyValueLabel = platRewardsLabel;

      // reward star (classic)
      auto rewardsStar = CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
      if (rewardsStar) {
            rewardsStar->setScale(0.6f);
            // position to the right of centered label
            float labelWidth = rewardsLabel->getContentSize().width * rewardsLabel->getScaleX();
            float gap = 6.f;
            float starX = initialCenterX + (labelWidth / 2.f) + gap + (rewardsStar->getContentSize().width * rewardsStar->getScaleX() / 2.f);
            rewardsStar->setPosition({starX, initialLabelY});
            container->addChild(rewardsStar, 2);
      }
      m_sections[idx].starIcon = rewardsStar;

      // platform reward icon (planet)
      auto platStar = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
      if (platStar) {
            platStar->setScale(0.6f);
            float labelWidthP = platRewardsLabel->getContentSize().width * platRewardsLabel->getScaleX();
            float gapP = 6.f;
            float starXp = initialCenterXp + (labelWidthP / 2.f) + gapP + (platStar->getContentSize().width * platStar->getScaleX() / 2.f);
            platStar->setPosition({starXp, initialLabelYp});
            platContainer->addChild(platStar, 2);
      }
      m_sections[idx].platStarIcon = platStar;

      // creator menu (classic)
      auto creatorMenu = CCMenu::create();
      creatorMenu->setPosition({0, 0});
      creatorMenu->setContentSize(container->getContentSize());
      auto creatorLabel = CCLabelBMFont::create("By", "goldFont.fnt");
      creatorLabel->setAnchorPoint({0.f, 0.5f});
      creatorLabel->setScale(0.55f);

      auto creatorItem = CCMenuItemSpriteExtra::create(creatorLabel, this, menu_selector(RLEventLayouts::onCreatorClicked));
      creatorItem->setTag(0);
      creatorItem->setAnchorPoint({0.f, 0.5f});
      creatorItem->setContentSize({100.f, 12.f});
      // position creator for classic container
      creatorItem->setPosition({70.f, 35.f});
      creatorLabel->setPosition({0.f, creatorItem->getContentSize().height / 2.f});
      creatorLabel->setAnchorPoint({0.f, 0.5f});
      creatorMenu->addChild(creatorItem);
      container->addChild(creatorMenu, 2);
      m_sections[idx].creatorLabel = creatorLabel;
      m_sections[idx].creatorButton = creatorItem;

      // creator menu (platformer)
      auto platCreatorMenu = CCMenu::create();
      platCreatorMenu->setPosition({0, 0});
      platCreatorMenu->setContentSize(platContainer->getContentSize());
      auto platCreatorLabel = CCLabelBMFont::create("By", "goldFont.fnt");
      platCreatorLabel->setAnchorPoint({0.f, 0.5f});
      platCreatorLabel->setScale(0.55f);
      auto platCreatorItem = CCMenuItemSpriteExtra::create(platCreatorLabel, this, menu_selector(RLEventLayouts::onCreatorClicked));
      platCreatorItem->setTag(0);
      platCreatorItem->setAnchorPoint({0.f, 0.5f});
      platCreatorItem->setContentSize({100.f, 12.f});
      // position creator for platformer container
      platCreatorItem->setPosition({70.f, 35.f});
      platCreatorLabel->setPosition({0.f, platCreatorItem->getContentSize().height / 2.f});
      platCreatorLabel->setAnchorPoint({0.f, 0.5f});
      platCreatorMenu->addChild(platCreatorItem);
      platContainer->addChild(platCreatorMenu, 2);
      m_sections[idx].platCreatorLabel = platCreatorLabel;
      m_sections[idx].platCreatorButton = platCreatorItem;

      // timer label
      std::vector<std::string> timerPrefixes = {"Next Daily in ", "Next Weekly in ", "Next Monthly in "};
      auto timerLabel = CCLabelBMFont::create((timerPrefixes[idx] + "--:--:--:--").c_str(), "bigFont.fnt");
      timerLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, 15.f});
      timerLabel->setScale(0.4f);
      m_mainLayer->addChild(timerLabel);
      m_sections[idx].timerLabel = timerLabel;

      // difficulty sprite (classic)
      auto diffSprite = GJDifficultySprite::create(0, GJDifficultyName::Short);
      diffSprite->setPosition({40, container->getContentSize().height / 2});
      container->addChild(diffSprite);
      m_sections[idx].diff = diffSprite;

      // difficulty sprite (platformer)
      auto platDiffSprite = GJDifficultySprite::create(0, GJDifficultyName::Short);
      platDiffSprite->setPosition({40, platContainer->getContentSize().height / 2});
      platContainer->addChild(platDiffSprite);
      m_sections[idx].platDiff = platDiffSprite;

      // play button (classic)
      auto playMenu = CCMenu::create();
      playMenu->setPosition({0, 0});
      auto playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      if (!playSprite) playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      playSprite->setScale(0.65f);
      auto playButton = CCMenuItemSpriteExtra::create(playSprite, this, menu_selector(RLEventLayouts::onPlayEvent));
      playButton->setPosition({330.f, container->getContentSize().height / 2});
      playButton->setAnchorPoint({0.5f, 0.5f});
      playMenu->addChild(playButton);
      auto playSpinner = LoadingSpinner::create(40.f);
      if (playSpinner) {
            playSpinner->setPosition(playButton->getPosition());
            playSpinner->setVisible(true);
            playMenu->addChild(playSpinner);
            m_sections[idx].playSpinner = playSpinner;
      }
      playButton->setVisible(false);
      container->addChild(playMenu, 2);
      m_sections[idx].playButton = playButton;

      // play button (platformer)
      auto platPlayMenu = CCMenu::create();
      platPlayMenu->setPosition({0, 0});
      auto platPlaySprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      if (!platPlaySprite) platPlaySprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
      platPlaySprite->setScale(0.65f);
      auto platPlayButton = CCMenuItemSpriteExtra::create(platPlaySprite, this, menu_selector(RLEventLayouts::onPlayEvent));
      platPlayButton->setPosition({330.f, platContainer->getContentSize().height / 2});
      platPlayButton->setAnchorPoint({0.5f, 0.5f});
      platPlayMenu->addChild(platPlayButton);
      auto platPlaySpinner = LoadingSpinner::create(40.f);
      if (platPlaySpinner) {
            platPlaySpinner->setPosition(platPlayButton->getPosition());
            platPlaySpinner->setVisible(true);
            platPlayMenu->addChild(platPlaySpinner);
            m_sections[idx].platPlaySpinner = platPlaySpinner;
      }
      platPlayButton->setVisible(false);
      platContainer->addChild(platPlayMenu, 2);
      m_sections[idx].platPlayButton = platPlayButton;

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
            Ref<RLEventLayouts> self = this;
            self->m_eventTask = web::WebRequest().get("https://gdrate.arcticwoof.xyz/getEvent");
            self->m_eventTask.listen([self](web::WebResponse* res) {
                  if (!self) return;
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
                  int idx = static_cast<int>(self->m_eventType);
                  if (idx < 0 || idx >= 3) return;
                  const auto& key = keys[idx];
                  auto sec = &self->m_sections[idx];
                  // if the key is missing or contains no levelId, keep showing the loading spinner and hide play buttons
                  if (!json.contains(key)) {
                        if (sec->playSpinner) sec->playSpinner->setVisible(true);
                        if (sec->playButton) sec->playButton->setVisible(false);
                        if (sec->platPlaySpinner) sec->platPlaySpinner->setVisible(true);
                        if (sec->platPlayButton) sec->platPlayButton->setVisible(false);
                        return;
                  }
                  auto obj = json[key];
                  int levelId = -1;
                  auto levelIdValue = obj["levelId"].as<int>();
                  if (levelIdValue) {
                        levelId = levelIdValue.unwrap();
                        if (sec->playSpinner) sec->playSpinner->setVisible(false);
                        if (sec->playButton) {
                              sec->playButton->setVisible(true);
                              sec->playButton->setEnabled(true);
                        }

                        self->m_sections[idx].levelId = levelId;
                        self->m_sections[idx].secondsLeft = obj["secondsLeft"].as<int>().unwrapOrDefault();
                  } else {
                        if (sec->playSpinner) sec->playSpinner->setVisible(true);
                        if (sec->playButton) sec->playButton->setVisible(false);
                  }

                  auto levelName = obj["levelName"].as<std::string>().unwrapOrDefault();
                  auto creator = obj["creator"].as<std::string>().unwrapOrDefault();
                  auto difficulty = obj["difficulty"].as<int>().unwrapOrDefault();
                  auto accountId = obj["accountId"].as<int>().unwrapOrDefault();
                  auto featured = obj["featured"].as<int>().unwrapOrDefault();

                  if (!sec || !sec->container) return;
                  auto nameLabel = sec->levelNameLabel;
                  auto creatorLabel = sec->creatorLabel;
                  if (nameLabel) {
                        nameLabel->setString(levelName.c_str());
                        const float maxNameWidth = 200.f;
                        const float baseNameScale = 0.8f;
                        nameLabel->setScale(baseNameScale);
                        const float labelWidth = nameLabel->getContentSize().width * nameLabel->getScaleX();
                        if (labelWidth > maxNameWidth) {
                              const float newScale = maxNameWidth / nameLabel->getContentSize().width;
                              nameLabel->setScale(newScale);
                        }
                  }
                  if (creatorLabel) {
                        creatorLabel->setString(("By " + creator).c_str());
                        creatorLabel->setAnchorPoint({0.f, 0.5f});
                  };

                  if (sec->difficultyValueLabel && sec->container) {
                        sec->difficultyValueLabel->setString(("Rewards: " + numToString(difficulty)).c_str());
                        const float centerX = sec->container->getContentSize().width / 2.f;
                        const float labelY = 12.f;
                        sec->difficultyValueLabel->setPosition({centerX, labelY});

                        // recreate star to ensure correct sprite and positioning
                        if (sec->starIcon) {
                              sec->starIcon->removeFromParent();
                              sec->starIcon = nullptr;
                        }
                        auto newStar = CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
                        if (newStar) {
                              newStar->setScale(0.6f);
                              float labelWidth = sec->difficultyValueLabel->getContentSize().width * sec->difficultyValueLabel->getScaleX();
                              float gap = 2.f;
                              float starX = centerX + (labelWidth / 2.f) + gap + (newStar->getContentSize().width * newStar->getScaleX() / 2.f);
                              newStar->setPosition({starX, labelY});
                              sec->container->addChild(newStar, 2);
                              sec->starIcon = newStar;
                        }
                  }

                  // update classic difficulty sprite and featured coin
                  if (sec->diff) {
                        sec->diff->updateDifficultyFrame(getDifficulty(difficulty), GJDifficultyName::Short);
                        if (featured >= 1 && featured <= 4) {
                              sec->featured = featured;
                              const char* coinSprite = (featured == 1) ? "RL_featuredCoin.png"_spr
                                                                    : (featured == 2) ? "RL_epicFeaturedCoin.png"_spr
                                                                    : (featured == 3) ? "RL_legendaryFeaturedCoin.png"_spr
                                                                                      : "RL_mythicFeaturedCoin.png"_spr;
                              if (sec->featuredIcon) {
                                    sec->featuredIcon->removeFromParent();
                                    sec->featuredIcon = nullptr;
                              }
                              auto coinIcon = CCSprite::createWithSpriteFrameName(coinSprite);
                              if (coinIcon) {
                                    coinIcon->setPosition({sec->diff->getContentSize().width / 2.f, sec->diff->getContentSize().height / 2.f});
                                    coinIcon->setZOrder(-1);
                                    sec->diff->addChild(coinIcon);
                                    sec->featuredIcon = coinIcon;

                                    // add particle for epic/legendary/mythic rings
                                    if (featured == 2 || featured == 3 || featured == 4) {
                                          const std::string& pString = (featured == 2) ? epicPString : (featured == 3) ? legendaryPString : mythicPString;
                                          if (!pString.empty()) {
                                                if (auto existingP = coinIcon->getChildByID("rating-particles")) {
                                                      existingP->removeFromParent();
                                                }
                                                ParticleStruct pStruct;
                                                GameToolbox::particleStringToStruct(pString, pStruct);
                                                CCParticleSystemQuad* particle = GameToolbox::particleFromStruct(pStruct, nullptr, false);
                                                if (particle) {
                                                      coinIcon->addChild(particle, 1);
                                                      particle->resetSystem();
                                                      particle->setPosition(coinIcon->getContentSize() / 2.f);
                                                      particle->setID("rating-particles"_spr);
                                                      particle->update(0.15f);
                                                      particle->update(0.15f);
                                                }
                                          }
                                    }
                              }
                        } else {
                              if (sec->featuredIcon) {
                                    sec->featuredIcon->removeFromParent();
                                    sec->featuredIcon = nullptr;
                                    sec->featured = 0;
                              }
                        }
                  }

                  // platformer support: check for dailyPlat/weeklyPlat/monthlyPlat variant
                  std::string platKey = key + "Plat";
                  if (json.contains(platKey)) {
                        auto pobj = json[platKey];
                        auto platLevelVal = pobj["levelId"].as<int>();
                        if (platLevelVal) {
                              int platLevelId = platLevelVal.unwrap();
                              sec->platLevelId = platLevelId;
                              sec->platSecondsLeft = pobj["secondsLeft"].as<int>().unwrapOrDefault();

                              // we have a platformer level: hide spinner and show play button
                              if (sec->platPlaySpinner) sec->platPlaySpinner->setVisible(false);
                              if (sec->platPlayButton) {
                                    sec->platPlayButton->setVisible(true);
                                    sec->platPlayButton->setEnabled(true);
                                    sec->platPlayButton->setTag(platLevelId);
                              }

                              auto platLevelName = pobj["levelName"].as<std::string>().unwrapOrDefault();
                              auto platCreator = pobj["creator"].as<std::string>().unwrapOrDefault();
                              auto platDifficulty = pobj["difficulty"].as<int>().unwrapOrDefault();
                              auto platAccountId = pobj["accountId"].as<int>().unwrapOrDefault();
                              auto platFeatured = pobj["featured"].as<int>().unwrapOrDefault();

                              // update name label
                              if (sec->platLevelNameLabel) {
                                    sec->platLevelNameLabel->setString(platLevelName.c_str());
                                    const float maxNameWidth = 200.f;
                                    const float baseNameScale = 0.8f;
                                    sec->platLevelNameLabel->setScale(baseNameScale);
                                    float labelWidth = sec->platLevelNameLabel->getContentSize().width * sec->platLevelNameLabel->getScaleX();
                                    if (labelWidth > maxNameWidth) {
                                          float newScale = maxNameWidth / sec->platLevelNameLabel->getContentSize().width;
                                          sec->platLevelNameLabel->setScale(newScale);
                                    }
                              }

                              // update creator
                              if (sec->platCreatorLabel) {
                                    sec->platCreatorLabel->setString(("By " + platCreator).c_str());
                                    sec->platCreatorLabel->setAnchorPoint({0.f, 0.5f});
                              }

                              // update rewards / planet icon
                              if (sec->platDifficultyValueLabel && sec->platContainer) {
                                    sec->platDifficultyValueLabel->setString(("Rewards: " + numToString(platDifficulty)).c_str());
                                    const float centerXp = sec->platContainer->getContentSize().width / 2.f;
                                    const float labelYp = 12.f;
                                    sec->platDifficultyValueLabel->setPosition({centerXp, labelYp});

                                    // recreate planet icon to ensure correct sprite and positioning
                                    if (sec->platStarIcon) {
                                          sec->platStarIcon->removeFromParent();
                                          sec->platStarIcon = nullptr;
                                    }
                                    auto newPlat = CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
                                    if (newPlat) {
                                          newPlat->setScale(0.6f);
                                          const float labelWidthP = sec->platDifficultyValueLabel->getContentSize().width * sec->platDifficultyValueLabel->getScaleX();
                                          const float gapP = 2.f;
                                          const float starXp = centerXp + (labelWidthP / 2.f) + gapP + (newPlat->getContentSize().width * newPlat->getScaleX() / 2.f);
                                          newPlat->setPosition({starXp, labelYp});
                                          sec->platContainer->addChild(newPlat, 2);
                                          sec->platStarIcon = newPlat;
                                    }
                              }

                              // update difficulty and featured for platformer
                              if (sec->platDiff) {
                                    sec->platDiff->updateDifficultyFrame(getDifficulty(platDifficulty), GJDifficultyName::Short);
                                    if (platFeatured == 1 || platFeatured == 2) {
                                          sec->platFeatured = platFeatured;
                                          const char* coinSprite = (platFeatured == 1) ? "RL_featuredCoin.png"_spr : "RL_epicFeaturedCoin.png"_spr;
                                          if (sec->platFeaturedIcon) {
                                                sec->platFeaturedIcon->removeFromParent();
                                                sec->platFeaturedIcon = nullptr;
                                          }
                                          auto coinIconP = CCSprite::createWithSpriteFrameName(coinSprite);
                                          if (coinIconP) {
                                                coinIconP->setPosition({sec->platDiff->getContentSize().width / 2.f, sec->platDiff->getContentSize().height / 2.f});
                                                coinIconP->setZOrder(-1);
                                                sec->platDiff->addChild(coinIconP);
                                                sec->platFeaturedIcon = coinIconP;

                                                // platformer particle
                                                if (platFeatured == 2 || platFeatured == 3 || platFeatured == 4) {
                                                      const std::string& pString = (platFeatured == 2) ? epicPString : (platFeatured == 3) ? legendaryPString : mythicPString;
                                                      if (!pString.empty()) {
                                                            if (auto existingP = coinIconP->getChildByID("rating-particles")) {
                                                                  existingP->removeFromParent();
                                                            }
                                                            ParticleStruct pStruct;
                                                            GameToolbox::particleStringToStruct(pString, pStruct);
                                                            CCParticleSystemQuad* particle = GameToolbox::particleFromStruct(pStruct, nullptr, false);
                                                            if (particle) {
                                                                  coinIconP->addChild(particle, 1);
                                                                  particle->resetSystem();
                                                                  particle->setPosition(coinIconP->getContentSize() / 2.f);
                                                                  particle->setID("rating-particles"_spr);
                                                                  particle->update(0.15f);
                                                                  particle->update(0.15f);
                                                            }
                                                      }
                                                }
                                          }
                                    } else {
                                          if (sec->platFeaturedIcon) {
                                                sec->platFeaturedIcon->removeFromParent();
                                                sec->platFeaturedIcon = nullptr;
                                                sec->platFeatured = 0;
                                          }
                                    }
                              }

                              sec->platAccountId = platAccountId;
                              if (sec->platCreatorButton) {
                                    sec->platCreatorButton->setTag(platAccountId);
                                    sec->platCreatorButton->setPosition({70.f, 35.f});
                                    sec->platCreatorButton->setContentSize({sec->platCreatorLabel->getContentSize().width * sec->platCreatorLabel->getScaleX(), 12.f});
                              }
                        } else {
                              // no platformer level yet
                              if (sec->platPlaySpinner) sec->platPlaySpinner->setVisible(true);
                              if (sec->platPlayButton) sec->platPlayButton->setVisible(false);
                        }
                  }

                  sec->accountId = accountId;
                  if (sec->creatorButton) {
                        sec->creatorButton->setTag(accountId);
                        sec->creatorButton->setContentSize({sec->creatorLabel->getContentSize().width * sec->creatorLabel->getScaleX(), 12.f});
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
          "*<cy>All event layouts are set manually so sometimes layouts might be there for long than expected</c>*\n\n"
          "\r\n\r\n---\r\n\r\n"
          "- <cg>**Daily Layout**</c> showcase <cl>Easy Layouts</c> *(Easy to Insane Difficulty)* for you to grind and play various layouts\n"
          "- <cy>**Weekly Layout**</c> showcase a more <cr>challenging layouts</c> to play *(Easy to Hard Demons Difficulty)*\n"
          "- <cp>**Monthly Layout**</c> showcase themed <cp>Layouts</c> that relates to the current month or showcase well-made/hyped layouts. Often usually showcase <cr>Very hard layouts</c> or <cl>Easy Layouts</c> depending on the monthly layout has to offer.\n",
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

      // check whether the level was stored yet and open LevelInfo when available.
      auto glm = GameLevelManager::sharedState();
      for (int i = 0; i < 3; ++i) {
            auto& psec = m_sections[i];
            if (psec.pendingKey.empty()) continue;

            // check stored levels
            auto stored = glm->getStoredOnlineLevels(psec.pendingKey.c_str());
            if (stored && stored->count() > 0) {
                  auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
                  if (!level || level->m_levelID != psec.pendingLevelId) {
                        Notification::create("Level ID mismatch", NotificationIcon::Warning)->show();
                  } else {
                        log::debug("RLEventLayouts: Opening LevelInfoLayer for level id {} (from stored cache)", psec.pendingLevelId);
                        auto scene = LevelInfoLayer::scene(level, false);
                        auto transitionFade = CCTransitionFade::create(0.5f, scene);
                        CCDirector::sharedDirector()->pushScene(transitionFade);
                  }

                  // hide any initialized spinners and restore play buttons
                  if (psec.playSpinner) psec.playSpinner->setVisible(false);
                  if (psec.platPlaySpinner) psec.platPlaySpinner->setVisible(false);
                  if (psec.playButton) {
                        psec.playButton->setEnabled(true);
                        psec.playButton->setVisible(true);
                  }
                  if (psec.platPlayButton) {
                        psec.platPlayButton->setEnabled(true);
                        psec.platPlayButton->setVisible(true);
                  }
                  psec.pendingKey.clear();
                  psec.pendingLevelId = -1;
                  psec.pendingTimeout = 0.0;
            } else {
                  psec.pendingTimeout -= dt;
                  if (psec.pendingTimeout <= 0.0) {
                        if (psec.playSpinner) psec.playSpinner->setVisible(false);
                        if (psec.platPlaySpinner) psec.platPlaySpinner->setVisible(false);
                        if (psec.playButton) {
                              psec.playButton->setEnabled(true);
                              psec.playButton->setVisible(true);
                        }
                        if (psec.platPlayButton) {
                              psec.platPlayButton->setEnabled(true);
                              psec.platPlayButton->setVisible(true);
                        }
                        Notification::create("Level not found", NotificationIcon::Warning)->show();
                        psec.pendingKey.clear();
                        psec.pendingLevelId = -1;
                        psec.pendingTimeout = 0.0;
                  }
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
      if (!m_mainLayer) {
            log::warn("mainLayer doesn't exist");
            return;
      }
      auto menuItem = static_cast<CCMenuItem*>(sender);
      if (!menuItem) {
            log::warn("menuItem doesn't exist");
            return;
      }
      int levelId = menuItem->getTag();
      if (levelId <= 0) {
            log::warn("levelid doesn't exist");
            return;
      }

      // fetch level metadata and push to LevelInfoLayer scene
      auto searchObj = GJSearchObject::create(SearchType::Search, numToString(levelId));
      auto key = std::string(searchObj->getKey());
      auto glm = GameLevelManager::sharedState();

      // Check if we already have the level stored
      auto stored = glm->getStoredOnlineLevels(key.c_str());
      if (stored && stored->count() > 0) {
            log::debug("stored online level count: {}", stored->count());
            auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
            if (level && level->m_levelID == levelId) {
                  log::debug("RLEventLayouts: Opening LevelInfoLayer for level id {} (from cache)", levelId);
                  auto scene = LevelInfoLayer::scene(level, false);
                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                  CCDirector::sharedDirector()->pushScene(transitionFade);
                  return;
            }
      }

      int secIdx = -1;
      for (int i = 0; i < 3; ++i) {
            if (m_sections[i].playButton == menuItem || m_sections[i].platPlayButton == menuItem) {
                  secIdx = i;
                  break;
            }
      }
      if (secIdx < 0) secIdx = 0;
      auto& sec = m_sections[secIdx];

      // clear any previous pending spinner for this section
      if (sec.pendingSpinner) {
            sec.pendingSpinner->removeFromParent();
            sec.pendingSpinner = nullptr;
      }

      sec.pendingKey = key;
      sec.pendingLevelId = levelId;
      sec.pendingTimeout = 10.0;  // 10s timeout

      // show the initialized spinner and hide the clicked menu item
      if (menuItem == sec.playButton) {
            if (sec.playSpinner) sec.playSpinner->setVisible(true);
            menuItem->setVisible(false);
            menuItem->setEnabled(false);
      } else if (menuItem == sec.platPlayButton) {
            if (sec.platPlaySpinner) sec.platPlaySpinner->setVisible(true);
            menuItem->setVisible(false);
            menuItem->setEnabled(false);
      }

      glm->getOnlineLevels(searchObj);
}

void RLEventLayouts::onSafeButton(CCObject* sender) {
      int idx = static_cast<int>(this->m_eventType);
      const char* types[] = {"daily", "weekly", "monthly"};
      const char* typeStr = (idx >= 0 && idx < 3) ? types[idx] : "daily";

      RLLevelBrowserLayer::ParamList params;
      params.emplace_back("safe", std::string(typeStr));
      std::string title = std::string(typeStr) + std::string(" Event Safe");
      auto browserLayer = RLLevelBrowserLayer::create(RLLevelBrowserLayer::Mode::EventSafe, params, title);
      auto scene = CCScene::create();
      scene->addChild(browserLayer);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}
