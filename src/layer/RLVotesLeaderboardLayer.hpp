#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class RLVotesLeaderboardLayer : public CCLayer {
protected:
    cue::ListNode* m_userListNode;
    ScrollLayer* m_scrollLayer;
    LoadingSpinner* m_spinner;

    CCMenuItemSpriteExtra* m_refreshBtn;

    bool init() override;
    void keyBackClicked() override;
    void onAccountClicked(CCObject* sender);
    void fetchLeaderboard(int amount);
    void populateLeaderboard(const std::vector<matjson::Value>& users);
    void onInfoButton(CCObject* sender);
    void onRefreshButton(CCObject* sender);

    geode::async::TaskHolder<geode::utils::web::WebResponse> m_fetchTask;

public:
    static RLVotesLeaderboardLayer* create();
};
