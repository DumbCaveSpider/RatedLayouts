#include <Geode/Geode.hpp>

using namespace geode::prelude;

class $modify(LevelBrowserLayer, LevelBrowserLayer) {
      bool init(GJSearchObject* object) {
            if (!LevelBrowserLayer::init(object)) return false;

            auto list = this->m_list;

            return true;
      }
};