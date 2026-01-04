#include <Geode/Geode.hpp>
#include <chrono>
#include <vector>

using namespace geode::prelude;

class RLGauntletLevelsLayer : public CCLayer {
     public:
      static RLGauntletLevelsLayer* create(matjson::Value const& gauntletData);

     protected:
      bool init(matjson::Value const& gauntletData);
      void onBackButton(CCObject* sender);
      void keyBackClicked() override;
      void fetchLevelDetails(int gauntletId);
      void onLevelDetailsFetched(matjson::Value const& json);
      void createLevelButtons(matjson::Value const& levelsData, int gauntletId);
      void onGauntletClick(CCObject* sender);

      void onEnter() override;
      void registerWithTouchDispatcher() override;
      void ccTouchesBegan(CCSet* touches, CCEvent* event) override;
      void ccTouchesMoved(CCSet* touches, CCEvent* event) override;
      void ccTouchesEnded(CCSet* touches, CCEvent* event) override;

      void update(float dt) override;
      void updateBackgroundParallax(CCPoint const& menuPos);

     private:
      std::string m_gauntletName;
      CCMenu* m_levelsMenu = nullptr;
      LoadingSpinner* m_loadingCircle = nullptr;
      CCLabelBMFont* m_dragText = nullptr;
      int m_gauntletId = 0;

      // Drag / fling state
      bool m_dragging = false;
      CCPoint m_touchStart = {0, 0};
      CCPoint m_menuStartPos = {0, 0};

      // For velocity/inertia
      CCPoint m_lastTouchPos = {0, 0};
      std::chrono::steady_clock::time_point m_lastTouchTime;
      CCPoint m_velocity = {0, 0};
      bool m_flinging = false;
      float m_deceleration = 1800.0f;

      bool m_multiTouch = false;
      float m_startPinchDist = 0.0f;
      float m_startScale = 1.0f;
      float m_minScale = 0.6f;
      float m_maxScale = 2.0f;

      float m_padding = 30.0f;
      std::vector<CCRect> m_occupiedRects;

      // Top extra padding for menu
      float m_topPadding = 50.0f;
      float m_dotSpacing = 24.0f;
      std::string m_dotSprite = "uiDot_001.png";

      // Background parallax
      CCSprite* m_bgSprite = nullptr;
      CCSprite* m_bgSprite2 = nullptr;
      CCPoint m_bgOriginPos = ccp(0, 0);
      CCPoint m_menuOriginPos = ccp(0, 0);
      float m_bgParallax = 0.25f;

      struct PendingButton {
            CCMenuItemSpriteExtra* button;
            float w;
            float h;
            int id;
      };
      std::vector<PendingButton> m_pendingButtons;

      // pending level fetch state when clicking a button
      std::string m_pendingKey;
      int m_pendingLevelId = -1;
      LoadingSpinner* m_pendingSpinner = nullptr;
      double m_pendingTimeout = 0.0; // seconds

      // Center points of placed buttons
      std::vector<CCPoint> m_buttonCenters;
};
