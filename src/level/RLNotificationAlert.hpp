#include <Geode/Geode.hpp>
#include <functional>

using namespace geode::prelude;

class RLNotificationAlert : public CCLayer {
public:
  static RLNotificationAlert *create(std::string const &title,
                                     std::string const &levelName,
                                     int difficulty, int featured, int levelId,
                                     std::string const &accountName,
                                     bool isPlatformer = false,
                                     std::string const &eventType = "",
                                     std::function<void()> onClose = nullptr);

  void closeAlert();
  void setOnCloseCallback(std::function<void()> callback);
  void onNotificationClicked(CCObject* sender);
  void checkPendingLevel(float dt);

protected:
  bool init(std::string const &title, std::string const &levelName,
            int difficulty, int featured, int levelId, std::string const &accountName,
            bool isPlatformer, std::string const &eventType);

private:
  std::function<void()> m_onCloseCallback;
  bool m_closing = false;
  int m_levelId = 0;
  int m_pendingRetries = 0;
  void onCloseAnimationComplete();
};