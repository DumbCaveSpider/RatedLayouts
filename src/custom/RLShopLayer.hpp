#pragma once

#include "Geode/cocos/cocoa/CCObject.h"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCCounterLabel.hpp>
#include <vector>

using namespace geode::prelude;

class RLShopLayer : public CCLayer {
protected:
  bool init() override;
  void keyBackClicked() override;

public:
  static RLShopLayer *create();
  void updateShopPage();
  void refreshRubyLabel();

private:
  struct ShopItem {
    int idx;
    int price;
    int creatorId;
    std::string creatorUsername;
  };

  void onLayoutCreator(CCObject *sender);
  void onResetRubies(CCObject *sender);
  void onUnequipNameplate(CCObject *sender);
  void onBuyItem(CCObject *sender);

  // pagination
  void onPrevPage(CCObject *sender);
  void onNextPage(CCObject *sender);

  // UI state
  CCCounterLabel *m_rubyLabel;
  CCMenu *m_shopRow1 = nullptr;
  CCMenu *m_shopRow2 = nullptr;
  CCMenuItemSpriteExtra *m_prevPageBtn = nullptr;
  CCMenuItemSpriteExtra *m_nextPageBtn = nullptr;
  CCLabelBMFont *m_pageLabel = nullptr;

  // data
  std::vector<ShopItem> m_shopItems;
  int m_shopPage = 0; // zero-based
};