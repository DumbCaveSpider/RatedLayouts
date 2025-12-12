#include "RLEventLayouts.hpp"

using namespace geode::prelude;

RLEventLayouts* RLEventLayouts::create() {
      auto ret = new RLEventLayouts();

      if (ret && ret->initAnchored(420.f, 280.f, "GJ_square01.png")) {
            ret->autorelease();
            return ret;
      }

      delete (ret);
      return nullptr;
};

bool RLEventLayouts::setup() {
      setTitle("Event Layouts");
      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

      return true;
}