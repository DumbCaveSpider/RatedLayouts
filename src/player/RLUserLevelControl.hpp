#include "Geode/cocos/CCDirector.h"
#include <Geode/Geode.hpp>
#include <Geode/binding/LevelManagerDelegate.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class RLUserLevelControl : public geode::Popup, public LevelManagerDelegate {
public:
    static RLUserLevelControl* create(int accountId);

    ~RLUserLevelControl() {
        m_removeLevelTask.cancel();
        m_getCompletionTask.cancel();
    }

private:
    int m_targetId = 0;
    CCLabelBMFont* m_usernameLabel = nullptr;
    async::TaskHolder<web::WebResponse> m_removeLevelTask;

    cue::ListNode* m_listNode = nullptr;
    LoadingSpinner* m_listSpinner = nullptr;
    CCMenuItemSpriteExtra* m_prevPageButton = nullptr;
    CCMenuItemSpriteExtra* m_nextPageButton = nullptr;
    CCMenuItemSpriteExtra* m_togglePlatButton = nullptr;
    CCLabelBMFont* m_emptyLabel = nullptr;
    bool m_filterPlat = false;
    int m_page = 0;
    int m_perPage = 10;
    async::TaskHolder<web::WebResponse> m_getCompletionTask;

    void onRemoveLevel(CCObject* sender);
    void removeLevel(int levelId);
    void onPrevPage(CCObject* sender);
    void onNextPage(CCObject* sender);
    void onPlanetFilter(CCObject* sender);

    void fetchCompletionList(int page);
    void populateCompletionLevels(cocos2d::CCArray* levels);
    void updateCompletionPaging(int resultCount);

    bool init() override;

    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key) override;
    void loadLevelsFailed(char const* key) override;
    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) override;
    void loadLevelsFailed(char const* key, int type) override;
};
