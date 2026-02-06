#pragma once

#include "Geode/ui/Popup.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

class RLSelectSends : public Popup {
public:
  static RLSelectSends *create();

protected:
  bool init() override;
  void onAllSends(CCObject *sender);
  void onThreePlusSends(CCObject *sender);
  void onLegendarySends(CCObject *sender);
};