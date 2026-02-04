#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLCreditsPopup : public geode::Popup {
     public:
      static RLCreditsPopup* create();

     private:
      bool init();
      void onAccountClicked(CCObject* sender);
      void onInfo(CCObject* sender);
      ScrollLayer* m_scrollLayer = nullptr;
      LoadingSpinner* m_spinner = nullptr;
};