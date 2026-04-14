#include <Geode/Geode.hpp>
#include <cue/RepeatingBackground.hpp>
#include "Geode/cocos/menu_nodes/CCMenu.h"

using namespace geode::prelude;

class RLSpireSelectLevelLayer : public CCLayer {
private:
    bool init() override;
    void keyBackClicked() override;

    void fetchSpireLevels();
    void applySpireLevelsJson(matjson::Value const& json);
    void createSpireDoors();
    void refreshDoorStates();
    void updateNavArrowState();
    void onInfoClick(CCObject* sender);
    void onSpireDoorClick(CCObject* sender);
    void onNavArrowUpClick(CCObject* sender);
    void onNavArrowDownClick(CCObject* sender);
    bool isLevelCompleted(int levelId);
    void rewardRoomTransition();
    void onEnter() override;
    void update(float dt) override;

    cue::RepeatingBackground* m_bg = nullptr;
    CCMenu* m_levelsMenu = nullptr;
    CCMenu* m_infoMenu = nullptr;
    CCMenuItemSpriteExtra* m_navArrowUp = nullptr;
    CCMenuItemSpriteExtra* m_navArrowDown = nullptr;
    CCLabelBMFont* m_roomLabel = nullptr;
    std::vector<int> m_levelIds;
    std::vector<int> m_levelDifficulty;
    std::vector<bool> m_levelCompleted;

    int m_spireRoomIndex = 1;
    bool m_allCompleted = false;

    LoadingSpinner* m_loadingSpinner = nullptr;
    std::string m_pendingKey;
    int m_pendingLevelId = -1;
    double m_pendingTimeout = 0.0;
    async::TaskHolder<web::WebResponse> m_fetchTask;
    CCLayerColor* m_transitionLayer = nullptr;
    bool m_isRoomTransitionActive = false;
    bool m_didAdvanceRoom = false;

public:
    static RLSpireSelectLevelLayer* create();

    static bool isSpireLevelCompleted(int levelId);
    static void setSpireLevelCompleted(int levelId);

    void showRoomTransition();
    void onRoomTransitionFetch();
    void completeRoomTransition();
    void onRoomTransitionComplete();
};
