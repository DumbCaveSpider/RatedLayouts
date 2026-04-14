#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class RLLeaderboardLayer : public CCLayer {
protected:
    cue::ListNode* m_userListNode = nullptr;
    ScrollLayer* m_scrollLayer;
    LoadingSpinner* m_spinner;
    TabButton* m_starsTab;
    TabButton* m_planetsTab;
    TabButton* m_creatorTab;
    TabButton* m_coinsTab;
    TabButton* m_votesTab;
    CCMenuItemSpriteExtra* m_creatorTypeToggleBtn = nullptr;
    bool m_creatorType6 = false;
    CCMenuItemSpriteExtra* m_refreshBtn;

    bool init() override;
    void keyBackClicked() override;
    void onLeaderboardTypeButton(CCObject* sender);
    void onCreatorTypeToggle(CCObject* sender);
    void onAccountClicked(CCObject* sender);
    void fetchLeaderboard(int type, int amount);
    void populateLeaderboard(const std::vector<matjson::Value>& users);
    void onInfoButton(CCObject* sender);
    void onRefreshButton(CCObject* sender);

    geode::async::TaskHolder<geode::utils::web::WebResponse> m_fetchTask;

public:
    static RLLeaderboardLayer* create();
};
