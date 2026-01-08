#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class RLModRatePopup : public geode::Popup<std::string, GJGameLevel*> {
     public:
      enum class PopupRole {
            Mod,
            Admin,
      };

      static RLModRatePopup* create(PopupRole role, std::string title = "Rate Layout",
                                    GJGameLevel* level = nullptr);
      ~RLModRatePopup() {
            m_getModLevelTask.cancel();
            m_setRateTask.cancel();
            m_setUnrateTask.cancel();
            m_setEventTask.cancel();
      }

     private:
      PopupRole m_role = PopupRole::Mod;
      std::string m_title;
      GJGameLevel* m_level;
      GJDifficultySprite* m_difficultySprite;
      bool m_isDemonMode;
      bool m_isFeatured;
      bool m_isEpicFeatured;
      CCMenuItemToggler* m_featuredToggleItem;
      CCMenuItemToggler* m_epicFeaturedToggleItem;
      CCMenu* m_normalButtonsContainer;
      CCMenu* m_demonButtonsContainer;
      CCNode* m_difficultyContainer;
      geode::TextInput* m_featuredScoreInput;
      int m_selectedRating;
      CCMenuItemSpriteExtra* m_submitButtonItem;
      bool m_isRejected;
      int m_levelId;
      int m_accountId;
      utils::web::WebTask m_getModLevelTask;
      utils::web::WebTask m_setRateTask;
      utils::web::WebTask m_setUnrateTask;
      utils::web::WebTask m_setEventTask;
      bool setup(std::string title, GJGameLevel* level) override;
      void onSubmitButton(CCObject* sender);
      void onUnrateButton(CCObject* sender);
      void onSuggestButton(CCObject* sender);
      void onRejectButton(CCObject* sender);
      void onToggleFeatured(CCObject* sender);
      void onToggleDemon(CCObject* sender);
      void onRatingButton(CCObject* sender);
      void onInfoButton(CCObject* sender);
      void onSetEventButton(CCObject* sender);
      void onToggleEpicFeatured(CCObject* sender);
      void updateDifficultySprite(int rating);
};
