#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GameLevelManager.hpp>

using namespace geode::prelude;

class RLLevelBrowserLayer : public CCLayer, public LevelManagerDelegate, public SetIDPopupDelegate {
     public:
      enum class Mode {
            Featured = 1,
            Sent = 2,
            AdminSent = 3,
            Search = 4,
            Account = 5,
            EventSafe = 6,
            LegendarySends = 7
      };

      using ParamList = std::vector<std::pair<std::string, std::string>>;

      static RLLevelBrowserLayer* create(Mode mode, ParamList const& params = ParamList(), std::string const& title = "Rated Layouts");
      bool init(GJSearchObject* object);

      void loadLevelsFinished(CCArray* levels, char const* key, int p2) override;
      void loadLevelsFailed(char const* key, int p1) override;

      void refreshLevels(bool force);
      void startLoading();
      void stopLoading();

      virtual void onEnter() override;
      virtual void onExit() override;
      virtual void update(float dt) override;

      void performSearchQuery(ParamList const& params);

     protected:
      GJSearchObject* m_searchObject;
      std::string m_title;
      int m_totalLevels{0};

      GJListLayer* m_listLayer;
      ScrollLayer* m_scrollLayer;
      bool m_loading = false;
      bool m_needsLayout = false;

      CCLabelBMFont* m_levelsLabel;
      LoadingSpinner* m_circle;
      CCMenuItemSpriteExtra* m_prevButton;
      CCMenuItemSpriteExtra* m_nextButton;
      CCMenuItemSpriteExtra* m_refreshBtn;   

      std::unordered_map<long long, GJGameLevel*> m_levelCache;

      // compact mode toggle
      bool m_compactMode = false;
      CCMenuItemSpriteExtra* m_compactToggleBtn = nullptr;
      CCLabelBMFont* m_compactToggleLabel = nullptr;

      int m_page = 0;
      int m_totalPages = 1;

      Mode m_mode = Mode::Featured;
      ParamList m_modeParams;
      async::TaskHolder<web::WebResponse> m_searchTask;
      ~RLLevelBrowserLayer() { 
            m_searchTask.cancel();
            auto glm = GameLevelManager::get();
            if (glm && glm->m_levelManagerDelegate == this) {
                  glm->m_levelManagerDelegate = nullptr;
            }
      }

      // UI: tabs and search input
      TabButton* m_featuredTab;
      TabButton* m_sentTab;
      TabButton* m_searchTab;

      CCMenu* m_searchInputMenu;
      geode::TextInput* m_searchInput;
      CCMenuItemSpriteExtra* m_searchButton;
      CCMenuItemSpriteExtra* m_clearButton;

      CCNode* m_bgContainer = nullptr;
      CCNode* m_groundContainer = nullptr;
      std::vector<CCSprite*> m_bgTiles;
      std::vector<CCSprite*> m_groundTiles;
      float m_bgSpeed = 40.f;
      float m_groundSpeed = 150.f;

      // page picker UI
      CCMenuItemSpriteExtra* m_pageButton = nullptr;
      CCLabelBMFont* m_pageButtonLabel = nullptr;

      // helpers
      void populateFromArray(CCArray* levels);
      void fetchLevelsForType(int type);
      void fetchAccountLevels(int accountId);
      void updatePageButton();

      // UI handlers
      void onBackButton(CCObject* sender);
      void keyBackClicked() override;
      void onPrevPage(CCObject* sender);
      void onNextPage(CCObject* sender);
      void onRefresh(CCObject* sender);
      void onModeButton(CCObject* sender);
      void onSearchButton(CCObject* sender);
      void onClearButton(CCObject* sender);
      void onPageButton(CCObject* sender);
      void onInfoButton(CCObject* sender);
      void onCompactToggle(CCObject* sender);

      // SetIDPopup delegate
      void setIDPopupClosed(SetIDPopup* popup, int value) override;
};