#pragma once

#include "RLShopLayer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <string>

using namespace geode::prelude;

class RLShopLayer;

class RLBuyItemPopup : public Popup {
protected:
  bool init() override;

  CCMenuItemSpriteExtra *m_buyBtn = nullptr;
  CCMenuItemSpriteExtra *m_applyBtn = nullptr;
  CCMenuItemSpriteExtra *m_cancelBtn = nullptr;
  TextInput *m_quantityInput = nullptr;
  CCLabelBMFont *m_statusLabel = nullptr;
  int m_itemId = 0;

  // metadata for the item being bought
  int m_creatorId = 0;
  std::string m_creatorUsername;
  int m_value = 0; // cost in rubies

  RLShopLayer *m_owner = nullptr;

  void onBuy(CCObject *sender);
  void onCancel(CCObject *sender);
  void onApply(CCObject *sender);
  void onProfile(CCObject *sender);

public:
  static RLBuyItemPopup *create(int itemId, int creatorId,
                                const std::string &creatorUsername, int value,
                                RLShopLayer *owner = nullptr);
  static RLBuyItemPopup *create(int itemId) {
    return create(itemId, 0, std::string(), 0, nullptr);
  }
};