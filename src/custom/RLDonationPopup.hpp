#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLDonationPopup : public geode::Popup<> {
     public:
      static RLDonationPopup* create();

     private:
      bool setup() override;
      void onClick(CCObject* sender);
};