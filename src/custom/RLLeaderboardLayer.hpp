#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLLeaderboardLayer : public CCLayer {
     protected:
      GJListLayer* m_listLayer;
      ScrollLayer* m_scrollLayer;
      LoadingSpinner* m_spinner;
      TabButton* m_starsTab;
      TabButton* m_planetsTab;
      TabButton* m_creatorTab;
      TabButton* m_coinsTab;

      bool init();
      void onBackButton(CCObject* sender);
      void onLeaderboardTypeButton(CCObject* sender);
      void onAccountClicked(CCObject* sender);
      void fetchLeaderboard(int type, int amount);
      void populateLeaderboard(const std::vector<matjson::Value>& users);
      void onInfoButton(CCObject* sender);
      void keyBackClicked() override;

      CCNode* m_bgContainer = nullptr;
      std::vector<CCSprite*> m_bgTiles;
      CCNode* m_groundContainer = nullptr;
      std::vector<CCSprite*> m_groundTiles;
      CCNode* m_floorContainer = nullptr;
      std::vector<CCSprite*> m_floorTiles;
      float m_bgSpeed = 40.f;       // pixels per second
      float m_groundSpeed = 150.f;  // pixels per second
      void update(float dt) override;

     public:
      static RLLeaderboardLayer* create();
};
