#include "Geode/cocos/cocoa/CCObject.h"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCCounterLabel.hpp>

using namespace geode::prelude;

class RLShopLayer : public CCLayer {
protected:
  bool init() override;
  void keyBackClicked() override;

public:
  static RLShopLayer *create();

private:
  void onLayoutCreator(CCObject *sender);
  void onResetRubies(CCObject *sender);
  CCCounterLabel *m_rubyLabel;
};