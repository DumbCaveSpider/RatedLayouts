#pragma once

#include "Geode/cocos/cocoa/CCObject.h"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCCounterLabel.hpp>
#include <vector>
#include <cue/DropdownNode.hpp>

using namespace geode::prelude;

class RLShopLayer : public CCLayer {
protected:
    bool init() override;
    void keyBackClicked() override;
    void onExitTransitionDidStart() override;
    void onEnterTransitionDidFinish() override;

public:
    static RLShopLayer* create();
    void updateShopPage();
    void refreshRubyLabel();

private:
    struct ShopItem {
        int idx;
        int price;
        int creatorId;
        std::string creatorUsername;
        std::string iconUrl;
    };
    void onResetRubies();
    void onUnequipNameplate();
    void onTestNameplate();
    void onForm();

    void onShopkeeper(CCObject* sender);
    void onBuyItem(CCObject* sender);
    void onRedeemLayer(CCObject* sender);

    void initDropdownMenu();
    void performDropdownAction();

    // pagination
    void onPrevPage(CCObject* sender);
    void onNextPage(CCObject* sender);

    // UI state
    CCCounterLabel* m_rubyLabel;
    CCMenu* m_shopRow1 = nullptr;
    CCMenu* m_shopRow2 = nullptr;
    CCMenuItemSpriteExtra* m_prevPageBtn = nullptr;
    CCMenuItemSpriteExtra* m_nextPageBtn = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    cue::DropdownNode* m_dropdownMenu = nullptr;
    int m_pendingDropdownAction = 0;

    // data
    std::vector<ShopItem> m_shopItems;
    int m_shopPage = 0;  // zero-based
    int m_totalPages = 1;

    // banner from server now
    void loadShopPage(int page);
};
