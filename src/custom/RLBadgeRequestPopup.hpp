#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLBadgeRequestPopup : public geode::Popup<> {
     public:
      static RLBadgeRequestPopup* create();

     private:
      bool setup() override;

      void onSubmit(CCObject* sender);

      TextInput* m_discordInput = nullptr;
      utils::web::WebTask m_getSupporterTask;
      ~RLBadgeRequestPopup() {
            m_getSupporterTask.cancel();
      }
};