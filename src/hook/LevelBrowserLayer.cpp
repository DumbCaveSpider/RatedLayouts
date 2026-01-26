#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

using namespace geode::prelude;

class $modify(RLLevelBrowserLayer, LevelBrowserLayer) {
      bool init(GJSearchObject* object) {
            if (!LevelBrowserLayer::init(object)) return false;

            auto list = this->m_list;
            log::debug("list: {}", list);

            return true;
      }
};