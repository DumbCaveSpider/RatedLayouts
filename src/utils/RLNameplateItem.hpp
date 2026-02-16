#pragma once

#include <Geode/Geode.hpp>
#include <map>
#include <string>

using namespace geode::prelude;

namespace ratedlayouts {

struct RLNameplateInfo {
  int index = 0;
  int value = 0;     // cost in rubies
  int creatorId = 0; // account id
  std::string creatorUsername;
};

class RLNameplateItem {
public:
  static CCMenuItemSpriteExtra *create(int index, int value, int creatorId,
                                       const std::string &creatorUsername,
                                       CCObject *target,
                                       SEL_MenuHandler selector);

  static bool getInfo(int index, RLNameplateInfo &out);

  // ownership persistence
  static bool isOwned(int index);
  static bool markOwned(int index); // add and persist
  static std::vector<int> getOwnedItems();

private:
  static std::map<int, RLNameplateInfo> s_items;
};

} // namespace ratedlayouts
