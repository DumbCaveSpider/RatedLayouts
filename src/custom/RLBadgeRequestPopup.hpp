#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

class RLBadgeRequestPopup : public geode::Popup {
     public:
      static RLBadgeRequestPopup* create();

     private:
      bool init() override;

      void onSubmit(CCObject* sender);

      TextInput* m_discordInput = nullptr;
      async::TaskHolder<web::WebResponse> m_getSupporterTask;
      ~RLBadgeRequestPopup() {
            m_getSupporterTask.cancel();
      }
};