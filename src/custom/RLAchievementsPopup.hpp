#pragma once

#include <Geode/Geode.hpp>
#include "RLAchievementCell.hpp"
#include "RLAchievements.hpp"

using namespace geode::prelude;

class RLAchievementsPopup : public geode::Popup<> {
public:
    static RLAchievementsPopup* create();

private:
    bool setup() override;

    void populate(int tabIndex);
    void onTab(CCObject* sender);
    void onInfo(CCObject* sender);

    GJCommentListLayer* m_commentList = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    CCMenu* m_tabMenu = nullptr;
    CCLabelBMFont* m_statusLabel = nullptr;
    int m_selectedTab = 0;
    std::vector<std::string> m_tabNames = {"All", "Sparks", "Planets", "Coins", "Blueprints", "Votes", "Misc"};
};
