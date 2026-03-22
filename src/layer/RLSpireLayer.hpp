#include <Geode/Geode.hpp>
#include "Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h"
#include "Geode/cocos/menu_nodes/CCMenuItem.h"

using namespace geode::prelude;

class RLSpireLayer : public CCLayer {
protected:
    bool init() override;
    void keyBackClicked() override;
    void onEnter() override;

public:
    static RLSpireLayer* create();

private:
    void onSpireClick(CCObject* sender);
    void onSpireEnterComplete();

    int m_indexDia = 0;
    bool m_readyReset = false;
    CCSprite* m_spireSpr = nullptr;
    CCMenuItemSpriteExtra* m_enterBtn = nullptr;
    CCLayerColor* m_blackout = nullptr;
    CCPoint m_spireOriginalPos = {0, 0};
    float m_spireOriginalScale = 1.0f;
};
