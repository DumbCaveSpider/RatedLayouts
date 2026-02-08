#include "RLNotificationAlert.hpp"
#include "Geode/utils/general.hpp"
#include "RLNotificationOverlay.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>

using namespace geode::prelude;

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

RLNotificationAlert *RLNotificationAlert::create(
    std::string const &title, std::string const &levelName, int difficulty,
    int featured, int levelId, std::string const &accountName,
    bool isPlatformer, std::string const &eventType,
    std::function<void()> onClose) {
  auto ret = new RLNotificationAlert();
  if (ret && ret->init(title, levelName, difficulty, featured, levelId,
                       accountName, isPlatformer, eventType)) {
    ret->autorelease();
    if (onClose) {
      ret->setOnCloseCallback(onClose);
    }
    return ret;
  }
  delete ret;
  return nullptr;
}

bool RLNotificationAlert::init(std::string const &title,
                               std::string const &levelName, int difficulty,
                               int featured, int levelId,
                               std::string const &accountName,
                               bool isPlatformer,
                               std::string const &eventType) {
  if (!CCLayer::init())
    return false;

  m_levelId = levelId;

  CCSize winSize = CCDirector::sharedDirector()->getWinSize();
  auto alertMenu = CCMenu::create();
  alertMenu->setPosition({0, 0});
  this->addChild(alertMenu);

  // Background
  auto bg = CCScale9Sprite::create("GJ_square05.png");
  bg->setContentSize({250.f, 80.f});
  bg->setID("rl-notification-bg");
  bg->setPosition({5.f, 5.f});
  bg->setAnchorPoint({0, 0});
  bg->setScale(Mod::get()->getSettingValue<float>("notiScale"));

  // Difficulty sprite
  auto diffSprite = GJDifficultySprite::create(getDifficulty(difficulty),
                                               GJDifficultyName::Short);
  if (diffSprite) {
    diffSprite->setPosition({35, bg->getContentSize().height / 2 + 5});
    bg->addChild(diffSprite, 2);
  }

  // Featured coin sprite
  CCSprite *featuredCoin = nullptr;
  switch (featured) {
  case 1:
    featuredCoin =
        CCSprite::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
    break;
  case 2:
    featuredCoin =
        CCSprite::createWithSpriteFrameName("RL_epicFeaturedCoin.png"_spr);
    break;
  case 3:
    featuredCoin =
        CCSprite::createWithSpriteFrameName("RL_legendaryFeaturedCoin.png"_spr);
    break;
  case 4:
    featuredCoin =
        CCSprite::createWithSpriteFrameName("RL_mythicFeaturedCoin.png"_spr);
    break;
  }

  if (featuredCoin) {
    diffSprite->addChild(featuredCoin, -1);
    featuredCoin->setPosition({diffSprite->getContentSize().width / 2,
                               diffSprite->getContentSize().height / 2});
  }

  float yPos = bg->getContentSize().height - 10;

  // Title label
  auto titleLabel = CCLabelBMFont::create(title.c_str(), "goldFont.fnt");
  titleLabel->setPosition({bg->getContentSize().width / 2 + 20, yPos});
  titleLabel->setScale(0.5f);
  titleLabel->limitLabelWidth(200.f, 0.5f, 0.1f);
  bg->addChild(titleLabel);
  yPos -= 25.f;

  // Level name
  auto levelLabel = CCLabelBMFont::create(levelName.c_str(), "bigFont.fnt");
  levelLabel->setPosition({bg->getContentSize().width / 2 + 20, yPos});
  levelLabel->setScale(0.45f);
  levelLabel->limitLabelWidth(200.f, 0.45f, 0.1f);
  bg->addChild(levelLabel);
  yPos -= 18.f;

  // Difficulty and Featured info
  std::string diffStr;
  switch (difficulty) {
  case 10:
    diffStr = "Easy";
    break;
  case 20:
    diffStr = "Normal";
    break;
  case 30:
    diffStr = "Hard";
    break;
  case 40:
    diffStr = "Harder";
    break;
  case 50:
    diffStr = "Insane";
    break;
  case 60:
    diffStr = "Demon";
    break;
  default:
    diffStr = "NA";
    break;
  }

  // Account name
  std::string accountText = fmt::format("By {}", accountName);
  auto accountLabel =
      CCLabelBMFont::create(accountText.c_str(), "goldFont.fnt");
  accountLabel->setPosition({bg->getContentSize().width / 2 + 20, yPos});
  accountLabel->setScale(0.55f);
  accountLabel->limitLabelWidth(200.f, 0.55f, 0.1f);
  bg->addChild(accountLabel);

  // Show difficulty value under the difficulty sprite with icon
  if (diffSprite) {
    const char *iconName =
        isPlatformer ? "RL_planetSmall.png"_spr : "RL_starSmall.png"_spr;
    auto icon = CCSprite::createWithSpriteFrameName(iconName);

    if (icon) {
      icon->setPosition({diffSprite->getContentSize().width / 2 + 8, -8});
      icon->setScale(0.75f);
      diffSprite->addChild(icon, 2);

      auto diffValueLabel =
          CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
      if (diffValueLabel) {
        diffValueLabel->setPosition(
            {icon->getPositionX() - 7, icon->getPositionY()});
        diffValueLabel->setScale(0.4f);
        diffValueLabel->setAnchorPoint({1.0f, 0.5f});
        diffValueLabel->setAlignment(kCCTextAlignmentRight);
        diffSprite->addChild(diffValueLabel, 2);
      }
    }
  }

  auto item = CCMenuItemSpriteExtra::create(
      bg, this, menu_selector(RLNotificationAlert::onNotificationClicked));
  item->setAnchorPoint({0, 0});
  item->setPosition({5, 5});
  item->m_scaleMultiplier = 1.05f;
  alertMenu->addChild(item);

  // Auto-dismiss based on setting value
  int removeSeconds = Mod::get()->getSettingValue<int>("removeSeconds");
  if (removeSeconds <= 0) {
    removeSeconds = 5; // Default to 5 seconds if invalid
  }

  this->runAction(CCSequence::create(
      CCDelayTime::create(static_cast<float>(removeSeconds)),
      CCCallFunc::create(this,
                         callfunc_selector(RLNotificationAlert::closeAlert)),
      nullptr));

  return true;
}

void RLNotificationAlert::closeAlert() {
  if (m_closing) {
    return;
  }
  m_closing = true;
  FMODAudioEngine::sharedEngine()->playEffect("geode.loader/byeNotif00.ogg");

  auto winSize = CCDirector::sharedDirector()->getWinSize();
  float duration = 2.0f;
  auto finalPos = this->getPosition();
  auto targetPos = ccp(-this->getContentWidth(), finalPos.y);
  auto move = CCMoveTo::create(duration, targetPos);
  auto ease = CCEaseOut::create(move, 2.0f);

  this->runAction(CCSequence::create(
      ease,
      CCCallFunc::create(
          this,
          callfunc_selector(RLNotificationAlert::onCloseAnimationComplete)),
      CCRemoveSelf::create(), nullptr));
}

void RLNotificationAlert::onCloseAnimationComplete() {
  if (m_onCloseCallback) {
    m_onCloseCallback();
  }
}

void RLNotificationAlert::setOnCloseCallback(std::function<void()> callback) {
  m_onCloseCallback = callback;
}

void RLNotificationAlert::onNotificationClicked(CCObject *sender) {
  if (m_levelId <= 0) {
    this->closeAlert();
    return;
  }

  this->closeAlert();
  // try to open cached level first
  auto searchObj =
      GJSearchObject::create(SearchType::Search, numToString(m_levelId));
  auto key = std::string(searchObj->getKey());
  auto glm = GameLevelManager::sharedState();
  auto stored = glm->getStoredOnlineLevels(key.c_str());
  if (stored && stored->count() > 0) {
    auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
    if (level && level->m_levelID == m_levelId) {
      auto scene = LevelInfoLayer::scene(level, false);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
      return;
    }
  }

  glm->getOnlineLevels(searchObj);
  m_pendingRetries = 6;
  this->schedule(schedule_selector(RLNotificationAlert::checkPendingLevel),
                 0.5f);
}

void RLNotificationAlert::checkPendingLevel(float dt) {
  if (m_pendingRetries <= 0) {
    this->unschedule(schedule_selector(RLNotificationAlert::checkPendingLevel));
    Notification::create("Level not found", NotificationIcon::Warning)->show();
    return;
  }
  --m_pendingRetries;

  auto searchObj =
      GJSearchObject::create(SearchType::Search, numToString(m_levelId));
  auto key = std::string(searchObj->getKey());
  auto glm = GameLevelManager::sharedState();
  auto stored = glm->getStoredOnlineLevels(key.c_str());
  if (stored && stored->count() > 0) {
    auto level = static_cast<GJGameLevel *>(stored->objectAtIndex(0));
    if (level && level->m_levelID == m_levelId) {
      this->unschedule(
          schedule_selector(RLNotificationAlert::checkPendingLevel));
      auto scene = LevelInfoLayer::scene(level, false);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
      return;
    }
  }
}