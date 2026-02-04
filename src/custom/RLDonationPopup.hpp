#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLDonationPopup : public geode::Popup {
     public:
      static RLDonationPopup* create();

     private:
      bool init();
      void onClick(CCObject* sender);
      void onGetBadge(CCObject* sender);
};