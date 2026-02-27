#include "Geode/cocos/CCDirector.h"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLUserLevelControl : public geode::Popup {
public:
  static RLUserLevelControl *create(int accountId);

  ~RLUserLevelControl() { m_removeLevelTask.cancel(); }

private:
  int m_targetId = 0;
  CCLabelBMFont *m_usernameLabel = nullptr;

  TextInput *m_levelInput = nullptr;
  CCMenuItemSpriteExtra *m_removeButton = nullptr;
  async::TaskHolder<web::WebResponse> m_removeLevelTask;

  void onRemoveLevel(CCObject *sender);

protected:
  bool init() override;
};