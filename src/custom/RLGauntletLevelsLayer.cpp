#include "RLGauntletLevelsLayer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cue/RepeatingBackground.hpp>
#include <vector>

using namespace geode::prelude;

RLGauntletLevelsLayer *
RLGauntletLevelsLayer::create(matjson::Value const &gauntletData) {
  auto ret = new RLGauntletLevelsLayer();
  if (ret && ret->init(gauntletData)) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

bool RLGauntletLevelsLayer::init(matjson::Value const &gauntletData) {
  if (!CCLayer::init())
    return false;
  auto winSize = CCDirector::sharedDirector()->getWinSize();

  auto bg = cue::RepeatingBackground::create("game_bg_01_001.png", 1.f,
                                             cue::RepeatMode::X);
  int bgR = gauntletData["r"].asInt().unwrapOr(50);
  int bgG = gauntletData["g"].asInt().unwrapOr(50);
  int bgB = gauntletData["b"].asInt().unwrapOr(50);
  bg->setColor({static_cast<GLubyte>(bgR), static_cast<GLubyte>(bgG),
                static_cast<GLubyte>(bgB)});
  bg->setSpeed(0.0f);
  this->addChild(bg, -1);
  m_bgSprite = static_cast<CCSprite *>(bg);
  if (m_bgSprite) {
    m_bgSprite->setAnchorPoint({0.5f, 0.5f});
    m_bgOriginPos = ccp(winSize.width / 2.0f, winSize.height / 2.0f);
    m_bgSprite->setPosition(m_bgOriginPos);
  }
  m_gauntletName = gauntletData["name"].asString().unwrapOr("Unknown");
  m_gauntletId = gauntletData["id"].asInt().unwrapOr(0);
  m_gauntletDescription = gauntletData["description"].asString().unwrapOr(
      "No description available.");

  auto titleLabel =
      CCLabelBMFont::create(m_gauntletName.c_str(), "goldFont.fnt");
  titleLabel->setPosition({winSize.width / 2, winSize.height - 20});
  // shadow
  auto titleLabelShadow =
      CCLabelBMFont::create(m_gauntletName.c_str(), "goldFont.fnt");
  titleLabelShadow->setPosition(
      {titleLabel->getPositionX() + 2, titleLabel->getPositionY() - 2});
  titleLabelShadow->setColor({0, 0, 0});
  titleLabelShadow->setOpacity(60);
  this->addChild(titleLabelShadow, 9);
  this->addChild(titleLabel, 10);

  auto menu = CCMenu::create();
  menu->setPosition({0, 0});

  addBackButton(this, BackButtonStyle::Green);

  this->setKeypadEnabled(true);

  m_loadingCircle = LoadingSpinner::create(100.f);
  m_loadingCircle->setPosition(winSize / 2);
  this->addChild(m_loadingCircle);

  auto dragText = CCLabelBMFont::create("Drag to move around", "chatFont.fnt");
  dragText->setPosition(winSize.width / 2, 5);
  dragText->setAnchorPoint({0.5f, 0.f});
  dragText->setScale(0.675f);
  this->addChild(dragText, 10);

  // info
  auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoButtonSpr->setScale(0.7f);
  CCMenuItemSpriteExtra *infoButton = CCMenuItemSpriteExtra::create(
      infoButtonSpr, this,
      menu_selector(RLGauntletLevelsLayer::onGauntletInfo));
  infoButton->setPosition({winSize.width - 25, winSize.height - 25});
  menu->addChild(infoButton);

  // Fetch level details
  fetchLevelDetails(m_gauntletId);

  return true;
}

void RLGauntletLevelsLayer::fetchLevelDetails(int gauntletId) {
  web::WebRequest request;
  matjson::Value postData;
  postData["id"] = gauntletId;

  request.bodyJSON(postData);
  Ref<RLGauntletLevelsLayer> self = this;
  m_getLevelsTask.spawn(
      request.post("https://gdrate.arcticwoof.xyz/getLevelsGauntlets"),
      [self](web::WebResponse const &response) {
        if (!self)
          return;
        if (response.ok()) {
          auto jsonRes = response.json();
          if (jsonRes.isOk()) {
            self->onLevelDetailsFetched(jsonRes.unwrap());
          } else {
            log::error("Failed to parse level details JSON: {}",
                       jsonRes.unwrapErr());
            Notification::create("Failed to parse level data",
                                 NotificationIcon::Error)
                ->show();
            if (self->m_loadingCircle) {
              self->m_loadingCircle->setVisible(false);
            }
          }
        } else {
          log::error("Failed to fetch level details: {}",
                     response.string().unwrapOr("Unknown error"));
          Notification::create("Failed to fetch level details",
                               NotificationIcon::Error)
              ->show();
          if (self->m_loadingCircle) {
            self->m_loadingCircle->setVisible(false);
          }
        }
      });
}

void RLGauntletLevelsLayer::onLevelDetailsFetched(matjson::Value const &json) {
  if (m_loadingCircle) {
    m_loadingCircle->setVisible(false);
  }

  createLevelButtons(json, m_gauntletId);
  this->refreshCompletionCache();
}

void RLGauntletLevelsLayer::createLevelButtons(matjson::Value const &levelsData,
                                               int gauntletId) {
  auto winSize = CCDirector::sharedDirector()->getWinSize();

  m_levelsMenu = CCMenu::create();
  m_levelsMenu->setContentSize({winSize.width + 100, 800});
  m_levelsMenu->setID("rl-levels-menu");
  float menuStartX = (winSize.width - m_levelsMenu->getContentSize().width) / 2;
  const float menuStartY = 0.0f;
  m_levelsMenu->setPosition(ccp(menuStartX, menuStartY));
  this->addChild(m_levelsMenu);

  m_menuOriginPos = ccp(menuStartX, menuStartY);
  this->updateBackgroundParallax(m_menuOriginPos);

  m_occupiedRects.clear();
  m_pendingButtons.clear();

  if (!levelsData.isArray()) {
    log::error("Expected levels data to be an array");
    return;
  }

  auto levelsArray = levelsData.asArray().unwrap();

  const float buttonWidth = 120.0f;
  const float spacingX = 100.0f;

  std::string levelIDsCSV;
  bool firstId = true;
  m_levelsSearchIds.clear();
  for (auto &value : levelsArray) {
    if (!value.isObject())
      continue;
    auto idOpt = value["levelid"].asInt();
    if (!idOpt)
      continue;
    int lid = idOpt.unwrap();
    m_levelsSearchIds.push_back(lid);
    if (!firstId)
      levelIDsCSV += ",";
    levelIDsCSV += numToString(lid);
    firstId = false;
  }

  m_levelsSearchKey.clear();
  if (!levelIDsCSV.empty()) {
    auto glm = GameLevelManager::sharedState();
    if (glm) {
      auto multiSearch =
          GJSearchObject::create(SearchType::Type19, levelIDsCSV.c_str());
      m_levelsSearchKey = std::string(multiSearch->getKey());
      glm->getOnlineLevels(multiSearch);
      this->runAction(CCSequence::create(
          CCDelayTime::create(0.15f),
          CCCallFunc::create(
              this,
              callfunc_selector(RLGauntletLevelsLayer::refreshCompletionCache)),
          nullptr));
    }
  }

  size_t validCount = 0;
  for (auto &v : levelsArray)
    if (v.isObject())
      ++validCount;
  if (validCount == 0)
    return;

  float totalWidth = validCount * buttonWidth + (validCount - 1) * spacingX;

  for (size_t i = 0; i < levelsArray.size(); i++) {
    auto level = levelsArray[i];

    if (!level.isObject())
      continue;

    int levelId = level["levelid"].asInt().unwrapOr(0);
    std::string levelName = level["levelname"].asString().unwrapOr("Unknown");
    int difficulty = level["difficulty"].asInt().unwrapOr(0);

    std::string gauntletName =
        fmt::format("RL_gauntlet-{}.png"_spr, gauntletId);
    auto gauntletSprite =
        CCSprite::createWithSpriteFrameName(gauntletName.c_str());
    auto gauntletSpriteShadow =
        CCSprite::createWithSpriteFrameName(gauntletName.c_str());
    gauntletSpriteShadow->setScaleY(1.2f);
    gauntletSpriteShadow->setColor({0, 0, 0});
    gauntletSpriteShadow->setOpacity(50);
    gauntletSpriteShadow->setAnchorPoint({0.f, .15f});
    gauntletSprite->addChild(gauntletSpriteShadow, -1);

    // subtle floating animation
    {
      const float floatDistance = 8.0f; // pixels
      const float floatDuration = 1.8f; // seconds
      auto moveUp = CCMoveBy::create(floatDuration, ccp(0, floatDistance));
      auto moveDown = CCMoveBy::create(floatDuration, ccp(0, -floatDistance));
      auto easeUp = CCEaseSineInOut::create(moveUp);
      auto easeDown = CCEaseSineInOut::create(moveDown);
      auto seq = CCSequence::create(easeUp, easeDown, nullptr);
      gauntletSprite->runAction(CCRepeatForever::create(seq));
    }

    auto nameLabel = CCLabelBMFont::create(levelName.c_str(), "bigFont.fnt");
    nameLabel->setAlignment(kCCTextAlignmentCenter);
    nameLabel->setAnchorPoint({0.5f, 1.0f});
    nameLabel->setScale(0.5f);
    // position above the sprite top so labels aren't clipped at random
    // positions
    nameLabel->setPosition({gauntletSprite->getContentSize().width / 2,
                            gauntletSprite->getContentSize().height + 18});
    nameLabel->limitLabelWidth(100.0f, 0.35f, 0.1f);
    gauntletSprite->addChild(nameLabel);

    auto difficultyLabel =
        CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
    difficultyLabel->setAnchorPoint({1.0f, 0.5f});
    difficultyLabel->setScale(0.625f);
    difficultyLabel->setPosition(
        {gauntletSprite->getContentSize().width / 2, -10});
    difficultyLabel->setID(fmt::format("rl-gauntlet-diff-{}", levelId).c_str());
    gauntletSprite->addChild(difficultyLabel);

    auto starSpr = CCSprite::createWithSpriteFrameName("RL_starBig.png"_spr);
    starSpr->setAnchorPoint({0.0f, 0.5f});
    starSpr->setScale(0.5f);
    starSpr->setPosition({difficultyLabel->getPositionX(), -10});
    gauntletSprite->addChild(starSpr);

    bool isCompleted = false;
    if (levelId > 0 && GameStatsManager::sharedState()) {
      auto glm = GameLevelManager::sharedState();
      if (glm && !m_levelsSearchKey.empty()) {
        auto storedAll = glm->getStoredOnlineLevels(m_levelsSearchKey.c_str());
        if (storedAll && storedAll->count() > 0) {
          for (unsigned int si = 0; si < storedAll->count(); ++si) {
            auto g = static_cast<GJGameLevel *>(storedAll->objectAtIndex(si)); // press "g"
            if (g && static_cast<int>(g->m_levelID) == levelId) {
              isCompleted =
                  GameStatsManager::sharedState()->hasCompletedLevel(g);
              break;
            }
          }
        }
      }
    }

    if (isCompleted) {
      // avoid duplicate icon
      auto existing = gauntletSprite->getChildByID(
          fmt::format("rl-gauntlet-complete-{}", levelId).c_str());
      if (!existing) {
        auto completeIcon =
            CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        if (completeIcon) {
          completeIcon->setPosition(
              {gauntletSprite->getContentSize().width / 2,
               gauntletSprite->getContentSize().height / 2});
          completeIcon->setID(
              fmt::format("rl-gauntlet-complete-{}", levelId).c_str());
          gauntletSprite->addChild(completeIcon, 20);
        }
      }
      difficultyLabel->setColor({0, 150, 255});
    }

    auto button = CCMenuItemSpriteExtra::create(
        gauntletSprite, this,
        menu_selector(RLGauntletLevelsLayer::onGauntletClick));
    // tag with the level id for easy lookup in the click handler
    button->setTag(levelId);

    float spriteW = gauntletSprite->getContentSize().width;
    float spriteH = gauntletSprite->getContentSize().height;

    // store along with level id to allow ordering
    int id = levelId;
    m_pendingButtons.push_back({button, spriteW, spriteH, id});
  }

  const float gap = 100.0f;
  if (!m_pendingButtons.empty()) {
    float y = m_padding + m_pendingButtons[0].h / 2.0f;
    for (size_t i = 0; i < m_pendingButtons.size(); ++i) {
      auto &pb = m_pendingButtons[i];
      if (i == 0) {
        // first stays at base y
      } else {
        float prevH = m_pendingButtons[i - 1].h;
        y += prevH / 2.0f + gap + pb.h / 2.0f;
      }
      // randomize X within +/-50px from center, clamp inside menu with padding
      float centerX = m_levelsMenu->getContentSize().width / 2.0f;
      float randOffset = (CCRANDOM_0_1() * 200.0f) - 100.0f; // -100..+100
      float x = centerX + randOffset;
      float minX = pb.w / 2.0f + m_padding;
      float maxX =
          m_levelsMenu->getContentSize().width - pb.w / 2.0f - m_padding;
      x = std::max(minX, std::min(maxX, x));

      pb.button->setPosition(ccp(x, y));
      // record occupied rect to prevent future overlap if needed
      CCRect placedRect = {x - pb.w / 2.0f, y - pb.h / 2.0f, pb.w, pb.h};
      m_occupiedRects.push_back(placedRect);
      // store center for path drawing
      m_buttonCenters.push_back(ccp(x, y));
      m_levelsMenu->addChild(pb.button);
    }

    float lastH = m_pendingButtons.back().h;
    float requiredHeight = y + lastH / 2.0f + m_padding + m_topPadding;
    if (requiredHeight > m_levelsMenu->getContentSize().height) {
      float oldH = m_levelsMenu->getContentSize().height;
      m_levelsMenu->setContentSize(
          CCSizeMake(m_levelsMenu->getContentSize().width, requiredHeight));
      const float newMenuStartY = 0.0f;
      m_levelsMenu->setPositionY(newMenuStartY);
      float menuStartX = (CCDirector::sharedDirector()->getWinSize().width -
                          m_levelsMenu->getContentSize().width) /
                         2.0f;
      m_levelsMenu->setPositionX(menuStartX);

      m_menuOriginPos = ccp(menuStartX, newMenuStartY);
    }

    // draw dot path between consecutive centers (y spacing consistent, x
    // randomized)
    if (m_buttonCenters.size() >= 2) {
      for (size_t i = 0; i + 1 < m_buttonCenters.size(); ++i) {
        auto a = m_buttonCenters[i];
        auto b = m_buttonCenters[i + 1];
        if (b.y <= a.y)
          continue; // ensure increasing Y

        // compute number of dots to place and make spacing consistent
        float dy = b.y - a.y;
        int count = 8;
        float actualSpacing = dy / (count + 1); // even spacing between a and b
        for (int k = 1; k <= count; ++k) {
          float yPos = a.y + k * actualSpacing;
          float t = (yPos - a.y) / dy;
          float baseX = a.x + (b.x - a.x) * t;

          // compute a smooth lateral offset using a sine curve for a coherent
          // path
          float phase = (static_cast<float>(k) / (count + 1)) *
                        static_cast<float>(M_PI); // 0..PI
          float dir = (b.x >= a.x) ? 1.0f : -1.0f;
          float smoothAmp =
              std::min(30.0f, std::fabs(b.x - a.x) * 0.15f + 8.0f);
          float offsetX = std::sin(phase) * smoothAmp * dir;
          float dotXBase = baseX + offsetX;

          // get dot frame size without creating a temporary sprite
          auto dotFrame =
              CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                  m_dotSprite.c_str());
          float dotW = 8.0f;
          float dotH = 8.0f;
          if (dotFrame) {
            dotW = dotFrame->getRect().size.width;
            dotH = dotFrame->getRect().size.height;
          }

          // try placements starting from dotXBase, nudging left/right if
          // overlapping
          bool placed = false;
          const int maxNudgeAttempts = 6;
          const float nudgeStep = 6.0f;
          for (int n = 0; n <= maxNudgeAttempts && !placed; ++n) {
            float nudge = 0.0f;
            if (n > 0) {
              int mult = (n + 1) / 2;
              float sign = (n % 2 == 1) ? 1.0f : -1.0f;
              nudge = sign * mult * nudgeStep; // +6, -6, +12, -12 ...
            }
            float dotX = dotXBase + nudge;
            float minX = m_padding + dotW / 2.0f + 2.0f;
            float maxX = m_levelsMenu->getContentSize().width - m_padding -
                         dotW / 2.0f - 2.0f;
            dotX = std::max(minX, std::min(maxX, dotX));

            CCRect dotRect = {dotX - dotW / 2.0f, yPos - dotH / 2.0f, dotW,
                              dotH};
            bool overlap = false;
            for (auto &orct : m_occupiedRects) {
              if (dotRect.intersectsRect(orct)) {
                overlap = true;
                break;
              }
            }

            if (!overlap) {
              auto dot =
                  CCSprite::createWithSpriteFrameName(m_dotSprite.c_str());
              if (dot) {
                dot->setPosition(ccp(dotX, yPos));
                m_levelsMenu->addChild(dot, -1);
              }
              // reserve dot rect to prevent dots overlapping sprites or each
              // other
              m_occupiedRects.push_back(dotRect);
              placed = true;
            }
          }
        }
      }
    }

    m_pendingButtons.clear();
    this->refreshCompletionCache();
  }
}

void RLGauntletLevelsLayer::onEnter() {
  CCLayer::onEnter();
  this->setTouchEnabled(true);
  this->scheduleUpdate();
  this->refreshCompletionCache();
}

void RLGauntletLevelsLayer::onGauntletClick(CCObject *sender) {
  auto menuItem = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (!menuItem)
    return;
  int levelId = menuItem->getTag();
  if (levelId <= 0)
    return;

  auto searchObj =
      GJSearchObject::create(SearchType::Search, numToString(levelId));
  auto key = std::string(searchObj->getKey());
  auto glm = GameLevelManager::sharedState();

  // check stored cache first
  auto stored = glm->getStoredOnlineLevels(key.c_str());
  if (stored && stored->count() > 0) {
    auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
    if (level && level->m_levelID == levelId) {
      auto scene = LevelInfoLayer::scene(level, false);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
      return;
    }
  }

  // prepare pending state and show spinner on the clicked button
  if (m_pendingSpinner) {
    m_pendingSpinner->removeFromParent();
    m_pendingSpinner = nullptr;
  }
  m_pendingKey = key;
  m_pendingLevelId = levelId;
  m_pendingTimeout = 10.0; // seconds

  auto spinner = LoadingSpinner::create(36.f);
  if (spinner) {
    spinner->setPosition(menuItem->getPosition());
    spinner->setVisible(true);
    m_levelsMenu->addChild(spinner);
    m_pendingSpinner = spinner;
  }
  menuItem->setEnabled(false);

  glm->getOnlineLevels(searchObj);
}

void RLGauntletLevelsLayer::onGauntletInfo(CCObject *sender) {
  MDPopup::create(m_gauntletName, m_gauntletDescription, "OK")->show();
}

void RLGauntletLevelsLayer::ccTouchesBegan(CCSet *touches, CCEvent *event) {
  if (!m_levelsMenu)
    return;

  // start dragging if inside menu
  CCTouch *touch = static_cast<CCTouch *>(touches->anyObject());
  auto touchLoc = touch->getLocation();
  auto local = m_levelsMenu->convertToNodeSpace(touchLoc);
  auto size = m_levelsMenu->getContentSize();
  if (local.x >= 0 && local.x <= size.width && local.y >= 0 &&
      local.y <= size.height) {
    m_dragging = true;
    m_touchStart = touchLoc;
    m_menuStartPos = m_levelsMenu->getPosition();
    m_lastTouchPos = touchLoc;
    m_lastTouchTime = std::chrono::steady_clock::now();
    m_velocity = ccp(0, 0);
    m_flinging = false;
  }
}

void RLGauntletLevelsLayer::ccTouchesMoved(CCSet *touches, CCEvent *event) {
  if (!m_levelsMenu)
    return;

  // single touch dragging
  if (m_dragging) {
    CCTouch *touch = static_cast<CCTouch *>(touches->anyObject());
    auto touchLoc = touch->getLocation();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> dt = now - m_lastTouchTime;
    float secs = dt.count();
    if (secs <= 0.0f)
      secs = 1e-6f;
    CCPoint delta = ccpSub(touchLoc, m_touchStart);
    CCPoint newPos = ccpAdd(m_menuStartPos, delta);

    // compute velocity based on last move
    m_velocity = ccp((touchLoc.x - m_lastTouchPos.x) / secs,
                     (touchLoc.y - m_lastTouchPos.y) / secs);
    m_lastTouchPos = touchLoc;
    m_lastTouchTime = now;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto content = m_levelsMenu->getContentSize();

    float minX = std::min(0.0f, winSize.width - content.width);
    float maxX = std::max(0.0f, winSize.width - content.width);
    float minY = std::min(0.0f, winSize.height - content.height);
    float maxY = std::max(0.0f, winSize.height - content.height);

    newPos.x = std::max(minX, std::min(maxX, newPos.x));
    newPos.y = std::max(minY, std::min(maxY, newPos.y));
    m_levelsMenu->setPosition(newPos);
    this->updateBackgroundParallax(newPos);
  }
}

void RLGauntletLevelsLayer::ccTouchesEnded(CCSet *touches, CCEvent *event) {

  if (m_dragging) {
    m_dragging = false;
    // if velocity large enough, start fling
    float speed = std::hypotf(m_velocity.x, m_velocity.y);
    if (speed > 300.0f) {
      m_flinging = true;
    } else {
      m_velocity = ccp(0, 0);
      m_flinging = false;
    }
  }
}

void RLGauntletLevelsLayer::update(float dt) {
  // handle pending level fetches first
  if (!m_pendingKey.empty()) {
    auto glm = GameLevelManager::sharedState();
    auto stored = glm->getStoredOnlineLevels(m_pendingKey.c_str());
    if (stored && stored->count() > 0) {
      auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
      if (level && level->m_levelID == m_pendingLevelId) {
        // open level info
        auto scene = LevelInfoLayer::scene(level, false);
        auto transitionFade = CCTransitionFade::create(0.5f, scene);
        CCDirector::sharedDirector()->pushScene(transitionFade);
        // cleanup pending state
        if (m_pendingSpinner) {
          m_pendingSpinner->removeFromParent();
          m_pendingSpinner = nullptr;
        }
        // restore visibility of clicked
        if (m_levelsMenu) {
          auto children = m_levelsMenu->getChildren();
          for (unsigned int i = 0; i < children->count(); ++i) {
            auto child = static_cast<CCNode *>(children->objectAtIndex(i));
            auto btn = typeinfo_cast<CCMenuItemSpriteExtra *>(child);
            if (btn && btn->getTag() == m_pendingLevelId) {
              btn->setEnabled(true);
              break;
            }
          }
        }
        m_pendingKey.clear();
        m_pendingLevelId = -1;
        m_pendingTimeout = 0.0;
        return;
      }
    }

    m_pendingTimeout -= dt;
    if (m_pendingTimeout <= 0.0) {
      if (m_pendingSpinner) {
        m_pendingSpinner->removeFromParent();
        m_pendingSpinner = nullptr;
      }
      // restore clicked button
      if (m_levelsMenu) {
        auto children = m_levelsMenu->getChildren();
        for (unsigned int i = 0; i < children->count(); ++i) {
          auto child = static_cast<CCNode *>(children->objectAtIndex(i));
          auto btn = typeinfo_cast<CCMenuItemSpriteExtra *>(child);
          if (btn && btn->getTag() == m_pendingLevelId) {
            btn->setEnabled(true);
            break;
          }
        }
      }
      Notification::create("Level not found", NotificationIcon::Warning)
          ->show();
      m_pendingKey.clear();
      m_pendingLevelId = -1;
      m_pendingTimeout = 0.0;
    }
  }

  //  fling/inertia update
  if (!m_flinging || !m_levelsMenu)
    return;

  float vlen = std::hypotf(m_velocity.x, m_velocity.y);
  if (vlen <= 1.0f) {
    m_velocity = ccp(0, 0);
    m_flinging = false;
    return;
  }

  float decel = m_deceleration * dt;
  float newLen = std::max(0.0f, vlen - decel);
  if (newLen == 0.0f) {
    m_velocity = ccp(0, 0);
    m_flinging = false;
    return;
  }

  // scale velocity vector to new length
  m_velocity = ccpMult(m_velocity, newLen / vlen);

  // move menu
  CCPoint delta = ccpMult(m_velocity, dt);
  CCPoint newPos = ccpAdd(m_levelsMenu->getPosition(), delta);

  auto winSize = CCDirector::sharedDirector()->getWinSize();
  auto content = m_levelsMenu->getContentSize();

  float minX = std::min(0.0f, winSize.width - content.width);
  float maxX = std::max(0.0f, winSize.width - content.width);
  float minY = std::min(0.0f, winSize.height - content.height);
  float maxY = std::max(0.0f, winSize.height - content.height);

  // stop the corresponding velocity component
  if (newPos.x < minX) {
    newPos.x = minX;
    m_velocity.x = 0;
  }
  if (newPos.x > maxX) {
    newPos.x = maxX;
    m_velocity.x = 0;
  }
  if (newPos.y < minY) {
    newPos.y = minY;
    m_velocity.y = 0;
  }
  if (newPos.y > maxY) {
    newPos.y = maxY;
    m_velocity.y = 0;
  }

  m_levelsMenu->setPosition(newPos);
  this->updateBackgroundParallax(newPos);

  // stop if velocities are nearly zero
  if (std::hypotf(m_velocity.x, m_velocity.y) < 5.0f) {
    m_velocity = ccp(0, 0);
    m_flinging = false;
  }
}

void RLGauntletLevelsLayer::updateBackgroundParallax(CCPoint const &menuPos) {
  if (!m_bgSprite)
    return;
  CCPoint menuDelta = ccpSub(menuPos, m_menuOriginPos);
  CCPoint bgOffset = ccpMult(menuDelta, m_bgParallax);
  CCPoint target = ccpAdd(m_bgOriginPos, bgOffset);
  m_bgSprite->setPosition(target);
  if (m_bgSprite2) {
    m_bgSprite2->setPosition(ccpAdd(m_bgOriginPos, ccpMult(bgOffset, 1.0f)));
  }
}

void RLGauntletLevelsLayer::refreshCompletionCache() {
  if (m_levelsSearchKey.empty() || m_levelsSearchIds.empty())
    return;
  auto glm = GameLevelManager::sharedState();
  if (!glm)
    return;

  auto storedAll = glm->getStoredOnlineLevels(m_levelsSearchKey.c_str());
  if (!storedAll || storedAll->count() == 0)
    return; // nothing cached yet

  for (int levelId : m_levelsSearchIds) {
    bool isCompleted = false;
    GJGameLevel *found = nullptr;
    for (unsigned int i = 0; i < storedAll->count(); ++i) {
      auto g = static_cast<GJGameLevel *>(storedAll->objectAtIndex(i));
      if (g && static_cast<int>(g->m_levelID) == levelId) {
        found = g;
        break;
      }
    }
    if (found && GameStatsManager::sharedState()) {
      isCompleted = GameStatsManager::sharedState()->hasCompletedLevel(found);
    }

    // find UI button for this level and update visuals
    if (m_levelsMenu) {
      auto children = m_levelsMenu->getChildren();
      for (unsigned int ci = 0; ci < children->count(); ++ci) {
        auto child = static_cast<CCNode *>(children->objectAtIndex(ci));
        auto btn = typeinfo_cast<CCMenuItemSpriteExtra *>(child);
        if (!btn)
          continue;
        if (btn->getTag() != levelId)
          continue;

        auto normal = btn->getNormalImage();
        auto spr = static_cast<CCSprite *>(normal);
        if (!spr)
          break;

        auto diffId = fmt::format("rl-gauntlet-diff-{}", levelId);
        auto diffNode = spr->getChildByID(diffId.c_str());
        auto diffLabel = typeinfo_cast<CCLabelBMFont *>(diffNode);

        auto completeId = fmt::format("rl-gauntlet-complete-{}", levelId);
        auto existingIcon = spr->getChildByID(completeId.c_str());

        if (isCompleted) {
          if (!existingIcon) {
            auto completeIcon =
                CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
            if (completeIcon) {
              completeIcon->setPosition({spr->getContentSize().width / 2,
                                         spr->getContentSize().height / 2});
              completeIcon->setID(completeId.c_str());
              spr->addChild(completeIcon, 20);
            }
          }
          if (diffLabel)
            diffLabel->setColor({0, 150, 255});
        } else {
          // not completed: remove any stale icon and reset color
          if (existingIcon)
            existingIcon->removeFromParent();
          if (diffLabel)
            diffLabel->setColor({255, 255, 255});
        }
        break; // found button for this level
      }
    }
  }
}

void RLGauntletLevelsLayer::keyBackClicked() {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}