#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

class RLCreatorLayer : public CCLayer {
     protected:
      bool init() override;
      void onBackButton(CCObject* sender);
      void keyBackClicked() override;

      void onLeaderboard(CCObject* sender);
      void onFeaturedLayouts(CCObject* sender);
      void onNewRated(CCObject* sender);
      void onSentLayouts(CCObject* sender);
      void onInfoButton(CCObject* sender);
      void onDailyLayouts(CCObject* sender);
      void onWeeklyLayouts(CCObject* sender);
      void onMonthlyLayouts(CCObject* sender);
      void onUnknownButton(CCObject* sender);
      void onLayoutGauntlets(CCObject* sender);
      void onAnnoucementButton(CCObject* sender);
      void onSearchLayouts(CCObject* sender);
      void onAchievementsButton(CCObject* sender);
      void onCreditsButton(CCObject* sender);
      void onDemonListButton(CCObject* sender);
      void onShopButton(CCObject* sender);
      void onSettingsButton(CCObject* sender);
      void onDiscordButton(CCObject* sender);
      void onBrowserButton(CCObject* sender);
      void onSecretDialogueButton(CCObject* sender);
      void onSupporterButton(CCObject* sender);

      MenuGameLayer* m_menuLayer = nullptr;
      std::vector<CCSprite*> m_bgDecorations;
      float m_decoGridX = 30.f;
      float m_decoGridY = 30.f;
      int m_decoCols = 0;
      int m_decoRows = 0;
      CCNode* m_floorContainer = nullptr;
      std::vector<CCSprite*> m_floorTiles;
      float m_bgSpeed = 40.f;     // pixels per second
      float m_groundSpeed = 150.f; // pixels per second

      // news / announcement UI
      CCMenuItemSpriteExtra* m_newsIconBtn = nullptr;
      CCSprite* m_newsBadge = nullptr;
      void update(float dt) override;

      // labels for mod info
      CCLabelBMFont* m_modStatusLabel = nullptr;
      CCLabelBMFont* m_modVersionLabel = nullptr;

      geode::async::TaskHolder<geode::utils::web::WebResponse> m_announcementTask;
      geode::async::TaskHolder<geode::utils::web::WebResponse> m_dialogueTask;

     public:
      void onEnter() override;
      static RLCreatorLayer* create();
};
