#include "RLNotificationOverlay.hpp"
#include "RLNotificationAlert.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

RLNotificationOverlay *RLNotificationOverlay::create() {
  auto ret = new RLNotificationOverlay();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

RLNotificationOverlay::~RLNotificationOverlay() {
  if (m_alertQueue) {
    m_alertQueue->release();
    m_alertQueue = nullptr;
  }
}

void RLNotificationOverlay::pushAlert(CCNode *alert) {
  if (!alert || !m_alertQueue)
    return;

  m_alertQueue->addObject(alert);

  if (!m_isShowingAlert) {
    showNextAlert();
  }
}

void RLNotificationOverlay::showNextAlert() {
  if (!m_alertQueue || m_alertQueue->count() == 0) {
    m_isShowingAlert = false;
    return;
  }

  m_isShowingAlert = true;
  FMODAudioEngine::sharedEngine()->playEffect("geode.loader/newNotif00.ogg");

  auto alert = static_cast<CCNode *>(m_alertQueue->objectAtIndex(0));
  m_alertQueue->removeObjectAtIndex(0);

  this->addChild(alert);

  auto winSize = CCDirector::sharedDirector()->getWinSize();
  auto finalPos = alert->getPosition();
  alert->setPosition(ccp(-winSize.width, finalPos.y));
  alert->runAction(CCSequence::create(
      CCEaseIn::create(CCMoveTo::create(0.30f, finalPos), 2.0f), nullptr));

  if (auto notifAlert = typeinfo_cast<RLNotificationAlert *>(alert)) {
    notifAlert->setOnCloseCallback([this]() { this->showNextAlert(); });
  }
}

bool RLNotificationOverlay::init() {
  if (!CCLayer::init())
    return false;

  CCSize winSize = CCDirector::sharedDirector()->getWinSize();

  m_alertQueue = CCArray::create();
  m_alertQueue->retain();
  m_isShowingAlert = false;

  int latestLevelId =
      Mod::get()->getSettingValue<int>("latestNotifiedRateLevelId");
  int latestEventId =
      Mod::get()->getSettingValue<int>("latestNotifiedRateEventId");
  float pollingInterval = Mod::get()->getSettingValue<float>("pollingInterval");

  if (pollingInterval <= 0) {
    return true;
  }

  this->schedule(schedule_selector(RLNotificationOverlay::callRateNotification),
                 static_cast<float>(pollingInterval));

  return true;
}

void RLNotificationOverlay::callRateNotification(float dt) {
  if (Mod::get()->getSettingValue<bool>("disableRateNotifications")) {
    log::info("notifications are disabled");
    return;
  }

  auto req = web::WebRequest();

  log::debug("Polling for new rate notifications...");
  async::spawn(
      req.get("https://gdrate.arcticwoof.xyz/getNewRate"),
      [this](web::WebResponse response) {
        if (!response.ok()) {
          log::warn("Rate notification fetch failed: {}", response.code());
          return;
        }

        auto jsonRes = response.json();
        if (!jsonRes) {
          log::warn("Failed to parse JSON response for rate notification");
          return;
        }

        auto json = jsonRes.unwrap();

        // Helper struct and parser for nested objects
        struct RateInfo {
          bool present = false;
          int levelId = 0;
          std::string levelName;
          int difficulty = 0;
          int featured = 0;
          int featuredScore = 0;
          int accountId = 0;
          std::string accountName;
          bool isSuggested = false;
          bool coinVerified = false;
          bool isPlatformer = false;
          std::string eventType;
        };

        auto parseObj = [](const matjson::Value &obj) {
          RateInfo r;
          if (!obj.isObject())
            return r;
          r.present = true;
          r.levelId = obj["levelId"].asInt().unwrapOr(0);
          r.levelName = obj["levelName"].asString().unwrapOr(std::string(""));
          r.difficulty = obj["difficulty"].asInt().unwrapOr(0);
          r.featured = obj["featured"].asInt().unwrapOr(0);
          r.featuredScore = obj["featuredScore"].asInt().unwrapOr(0);
          r.accountId = obj["accountId"].asInt().unwrapOr(0);
          r.accountName =
              obj["accountName"].asString().unwrapOr(std::string(""));
          r.isSuggested = obj["isSuggested"].asBool().unwrapOr(false);
          r.coinVerified = obj["coinVerified"].asBool().unwrapOr(false);
          r.isPlatformer = obj["isPlat"].asBool().unwrapOr(false);
          r.eventType = obj["eventType"].asString().unwrapOr(std::string(""));
          return r;
        };

        RateInfo newRate = parseObj(json["newRate"]);
        RateInfo newEvent = parseObj(json["newEvent"]);

        // store new rate levelid and event id to avoid duplicate notifications
        if (!newRate.present && !newEvent.present) {
          log::debug("No new rate or event notifications");
          return;
        }
        int currentLatestLevelId =
            Mod::get()->getSavedValue<int>("latestNotifiedRateLevelId");
        int currentLatestEventId =
            Mod::get()->getSavedValue<int>("latestNotifiedRateEventId");
        if (newRate.present) {
          if (newRate.levelId != currentLatestLevelId) {
            Loader::get()->queueInMainThread([this, newRate]() {
              auto alert = RLNotificationAlert::create(
                  "New Rated Layout", newRate.levelName, newRate.difficulty,
                  newRate.featured, newRate.levelId, newRate.accountName,
                  newRate.isPlatformer, "rate");
              if (alert) {
                this->pushAlert(alert);
                Mod::get()->setSavedValue<int>("latestNotifiedRateLevelId",
                                               newRate.levelId);
              }
            });
          } else {
            log::info("No new rate notifications");
          }
        }
        if (newEvent.present) {
          if (newEvent.levelId != currentLatestEventId) {
            Loader::get()->queueInMainThread([this, newEvent]() {
              auto alert = RLNotificationAlert::create(
                  fmt::format("New {} Layout", newEvent.eventType),
                  newEvent.levelName, newEvent.difficulty, newEvent.featured,
                  newEvent.levelId, newEvent.accountName, newEvent.isPlatformer,
                  newEvent.eventType);
              if (alert) {
                this->pushAlert(alert);
                Mod::get()->setSavedValue<int>("latestNotifiedRateEventId",
                                               newEvent.levelId);
              }
            });
          } else {
            log::info("No new event notifications");
          }
        }
      });
}
