#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>
#include <unordered_map>

using namespace geode::prelude;

class RLUserControl : public geode::Popup {
     public:
      static RLUserControl* create(int accountId);
      ~RLUserControl() {
            m_profileTask.cancel();
            m_setUserTask.cancel();
            m_deleteUserTask.cancel();
            m_promoteUserTask.cancel();
      }

     private:
      int m_targetAccountId = 0;
      RowLayout* m_optionsLayout = nullptr;
      CCMenu* m_optionsMenu = nullptr;

      struct OptionState {
            CCMenuItemSpriteExtra* actionButton = nullptr;
            ButtonSprite* actionSprite = nullptr;
            bool persisted = false;
            bool desired = false;
      };

      std::unordered_map<std::string, OptionState> m_userOptions;
      CCMenuItemSpriteExtra* m_optionsButton = nullptr;
      CCMenuItemSpriteExtra* m_wipeButton = nullptr;
      bool m_isInitializing = false;
      LoadingSpinner* m_spinner = nullptr;
      async::TaskHolder<web::WebResponse> m_profileTask;
      async::TaskHolder<web::WebResponse> m_setUserTask;
      async::TaskHolder<web::WebResponse> m_deleteUserTask;
      async::TaskHolder<web::WebResponse> m_promoteUserTask;
      // promotion buttons
      CCMenuItemSpriteExtra* m_promoteLeaderboardModButton = nullptr;
      CCMenuItemSpriteExtra* m_promoteClassicModButton = nullptr;
      CCMenuItemSpriteExtra* m_promoteClassicAdminButton = nullptr;
      CCMenuItemSpriteExtra* m_promotePlatModButton = nullptr;
      CCMenuItemSpriteExtra* m_promotePlatAdminButton = nullptr;
      CCMenuItemSpriteExtra* m_demoteButton = nullptr;
      // tracked state of the target account's roles (for UI updates)
      bool m_targetIsLeaderboardMod = false;
      bool m_targetIsClassicMod = false;
      bool m_targetIsClassicAdmin = false;
      bool m_targetIsPlatMod = false;
      bool m_targetIsPlatAdmin = false;
      void onOptionAction(CCObject* sender);
      void onWipeAction(CCObject* sender);
      void onPromoteAction(CCObject* sender);
      void applySingleOption(const std::string& key, bool value);

      OptionState* getOptionByKey(const std::string& key);
      void setOptionState(const std::string& key, bool desired, bool updatePersisted = false);
      void setOptionEnabled(const std::string& key, bool enabled);
      void setAllOptionsEnabled(bool enabled);

      bool init() override;
};
