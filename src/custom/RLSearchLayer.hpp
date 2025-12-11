#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// forward declaration for GJDifficultySprite from the engine
class GJDifficultySprite;

class RLSearchLayer : public CCLayer {
     protected:
      bool init();
      void onBackButton(CCObject* sender);
      void keyBackClicked() override;
      void onSearchButton(CCObject* sender);
      void onClearButton(CCObject* sender);

     public:
      static RLSearchLayer* create();

      // background/ground tile moving helpers
      CCNode* m_bgContainer = nullptr;
      std::vector<CCSprite*> m_bgTiles;
      CCNode* m_groundContainer = nullptr;
      std::vector<CCSprite*> m_groundTiles;
      float m_bgSpeed = 40.f;       // pixels per second
      float m_groundSpeed = 150.f;  // pixels per second
      void update(float dt) override;

      // search input menu and text input
      CCMenu* m_searchInputMenu = nullptr;
      TextInput* m_searchInput = nullptr;
      RowLayout* m_optionsLayout = nullptr;
      CCMenuItemSpriteExtra* m_featuredItem = nullptr;
      CCMenuItemSpriteExtra* m_awardedItem = nullptr;

      // difficulty filter buttons
      CCMenu* m_difficultyFilterMenu = nullptr;
      std::vector<GJDifficultySprite*> m_difficultySprites;
      std::vector<CCMenuItemSpriteExtra*> m_difficultyMenuItems;
      std::vector<bool> m_difficultySelected;            // selection flags for each difficulty group (groups like 4/5)
      std::vector<std::vector<int>> m_difficultyGroups;  // array of groups (each group is a list of ratings represented by one sprite)
      // demon difficulties menu and items
      CCMenu* m_demonFilterMenu = nullptr;
      std::vector<GJDifficultySprite*> m_demonSprites;
      std::vector<CCMenuItemSpriteExtra*> m_demonMenuItems;
      std::vector<bool> m_demonSelected;
      bool m_demonModeActive = false;
      bool m_featuredActive = false;
      bool m_awardedActive = false;
      void onAwardedToggle(CCObject* sender);
      void onFeaturedToggle(CCObject* sender);
      void onDemonToggle(CCObject* sender);
      void onDemonDifficultyButton(CCObject* sender);
      void onDifficultyButton(CCObject* sender);
};