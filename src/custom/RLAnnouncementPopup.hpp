#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLAnnouncementPopup : public geode::Popup<> {
     public:
      static RLAnnouncementPopup* create();

     private:
      bool setup() override;
      void onClick(CCObject* sender);
};