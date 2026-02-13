#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLEventLayouts : public geode::Popup {
public:
  enum class EventType {
    Daily = 0,
    Weekly = 1,
    Monthly = 2,
  };

  static RLEventLayouts *create(EventType type);

private:
  bool init() override;
  void update(float dt) override;
  void onInfo(CCObject *sender);
  void onSafeButton(CCObject *sender);

  struct EventSection {
    CCLayer *container = nullptr;
    CCLayer *platContainer = nullptr;
    LevelCell *levelCell = nullptr;
    LevelCell *platLevelCell = nullptr;
    CCLabelBMFont *timerLabel = nullptr;

    int accountId = -1;
    int levelId = -1;
    int platLevelId = -1;
    int featured = 0;
    time_t createdAt = 0;
    double secondsLeft = 0.0;
    double platSecondsLeft = 0.0;
    std::string pendingKey;
    int pendingLevelId = -1;
    std::string pendingPlatKey;
    int pendingPlatLevelId = -1;
    LoadingSpinner *pendingSpinner = nullptr;
    LoadingSpinner *playSpinner = nullptr;
    LoadingSpinner *platPlaySpinner = nullptr;
    double pendingTimeout = 0.0; // seconds
    double pendingRetry = 0.0;   // seconds until next retry
  };

  EventSection m_sections[3];
  EventType m_eventType = EventType::Daily;
  CCLayer *m_eventMenu = nullptr;
  bool m_setupFinished = false;
  async::TaskHolder<web::WebResponse> m_eventTask;
  async::TaskHolder<web::WebResponse> m_safeListTask;
  ~RLEventLayouts() {
    m_eventTask.cancel();
    m_safeListTask.cancel();
  }
};