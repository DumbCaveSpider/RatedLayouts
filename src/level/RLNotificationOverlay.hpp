#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLNotificationOverlay : public CCLayer {
public:
  static RLNotificationOverlay *create();
  ~RLNotificationOverlay();

  void pushAlert(CCNode* alert);
  void showNextAlert();

protected:
  bool init() override;
  void callRateNotification(float dt);

private:
  CCArray* m_alertQueue = nullptr;
  bool m_isShowingAlert = false;
};