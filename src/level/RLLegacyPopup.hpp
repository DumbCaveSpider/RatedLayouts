#pragma once

#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include <Geode/Geode.hpp>
#include <Geode/ui/MDTextArea.hpp>

using namespace geode::prelude;

class RLLegacyPopup : public geode::Popup {
public:
    static RLLegacyPopup* create(GJGameLevel* level);

private:
    bool init() override;
    void updateFromJson(const matjson::Value &json);
    void onInfoButton(CCObject* sender);
    void onDeleteLegacy(CCObject* sender);

    GJGameLevel* m_level = nullptr;

    CCLabelBMFont* m_levelLabel = nullptr;
    GJDifficultySprite* m_diffSprite = nullptr;
    MDTextArea* m_infoText = nullptr;
    CCSprite* m_featuredRing = nullptr;
    CCSprite* m_rewardIcon = nullptr;
    CCLabelBMFont* m_rewardLabel = nullptr;
    CCMenuItemSpriteExtra* m_infoButton = nullptr;

    async::TaskHolder<web::WebResponse> m_fetchTask;
    async::TaskHolder<web::WebResponse> m_deleteTask;
    ~RLLegacyPopup() {
        m_fetchTask.cancel();
        m_deleteTask.cancel();
    }
};
