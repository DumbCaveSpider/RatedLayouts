#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include <unordered_map>

using namespace geode::prelude;

class RLUserControl : public geode::Popup<> {
     public:
      static RLUserControl* create();
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
      utils::web::WebTask m_profileTask;
      utils::web::WebTask m_setUserTask;
      utils::web::WebTask m_deleteUserTask;
      utils::web::WebTask m_promoteUserTask;
      CCMenuItemSpriteExtra* m_promoteModButton = nullptr;
      CCMenuItemSpriteExtra* m_promoteAdminButton = nullptr;
      CCMenuItemSpriteExtra* m_demoteButton = nullptr;
      void onOptionAction(CCObject* sender);
      void onWipeAction(CCObject* sender);
      void onPromoteAction(CCObject* sender);
      void applySingleOption(const std::string& key, bool value);

      OptionState* getOptionByKey(const std::string& key);
      void setOptionState(const std::string& key, bool desired, bool updatePersisted = false);
      void setOptionEnabled(const std::string& key, bool enabled);
      void setAllOptionsEnabled(bool enabled);

      bool setup() override;
};
