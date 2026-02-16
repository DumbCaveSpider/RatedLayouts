#include "RLNameplateItem.hpp"
#include <fmt/core.h>

using namespace geode::prelude;

namespace ratedlayouts {

std::map<int, RLNameplateInfo> RLNameplateItem::s_items;

CCMenuItemSpriteExtra *
RLNameplateItem::create(int index, int value, int creatorId,
                        const std::string &creatorUsername, CCObject *target,
                        SEL_MenuHandler selector) {
  auto iconName = fmt::format("icon_{}.png"_spr, index);

  auto iconSpr = CCSprite::createWithSpriteFrameName(iconName.c_str());
  if (!iconSpr)
    iconSpr = CCSprite::createWithSpriteFrameName("RL_bob.png"_spr);

  auto texSize = iconSpr->getTextureRect().size;
  float width = std::max(texSize.width, 64.f);
  float height = texSize.height + 26.f; // room for price label

  // container node used as the menu item's normal image
  auto container = CCSprite::create();
  container->setContentSize({width, height});

  iconSpr->setPosition({width / 2.f, height - texSize.height / 2.f - 5.f});
  container->addChild(iconSpr);

  // price label underneath (show "Owned" when purchased)
  bool owned = isOwned(index);
  auto priceLabel = CCLabelBMFont::create(
      owned ? "Owned" : fmt::format("{}", value).c_str(), "bigFont.fnt");
  priceLabel->setScale(0.4f);
  priceLabel->setPosition({width / 2.f - 6.f, 13.f});
  if (owned) {
    priceLabel->setColor({120, 255, 140});
    priceLabel->setPosition({width / 2.f, 13.f});
  }
  container->addChild(priceLabel);

  // small ruby icon to the right of the price
  if (!owned) {
    auto rubyIcon = CCSprite::createWithSpriteFrameName("RL_rubySmall.png"_spr);
    if (rubyIcon) {
      rubyIcon->setScale(0.8f);
      float priceHalfW =
          priceLabel->getContentSize().width * priceLabel->getScale() * 0.5f;
      rubyIcon->setPosition({priceLabel->getPositionX() + priceHalfW + 8.f,
                             priceLabel->getPositionY() - 1.f});
      container->addChild(rubyIcon);
    }
  }

  auto item = CCMenuItemSpriteExtra::create(container, target, selector);
  item->setContentSize(container->getContentSize());
  item->setTag(index);

  // register metadata for lookup
  s_items[index] = RLNameplateInfo{index, value, creatorId, creatorUsername};
  return item;
}

bool RLNameplateItem::getInfo(int index, RLNameplateInfo &out) {
  auto it = s_items.find(index);
  if (it == s_items.end())
    return false;
  out = it->second;
  return true;
}

static std::filesystem::path ownedPath() {
  return dirs::getModsSaveDir() / Mod::get()->getID() / "owned_items.json";
}

std::vector<int> RLNameplateItem::getOwnedItems() {
  std::vector<int> out;
  auto path = ownedPath();
  auto existing = utils::file::readString(utils::string::pathToString(path));
  if (!existing)
    return out;
  auto parsed = matjson::parse(existing.unwrap());
  if (!parsed)
    return out;
  auto root = parsed.unwrap();
  if (root.isArray()) {
    auto arr = root.asArray().unwrap();
    for (auto &v : arr) {
      int n = v.asInt().unwrapOr(-1);
      if (n >= 0)
        out.push_back(n);
    }
  }
  return out;
}

bool RLNameplateItem::isOwned(int index) {
  auto owned = getOwnedItems();
  for (auto i : owned)
    if (i == index)
      return true;
  return false;
}

bool RLNameplateItem::markOwned(int index) {
  auto path = ownedPath();
  auto owned = getOwnedItems();
  for (auto i : owned)
    if (i == index)
      return true; // already owned
  owned.push_back(index);
  // write simple JSON array
  std::string out = "[";
  for (size_t i = 0; i < owned.size(); ++i) {
    if (i)
      out += ",";
    out += fmt::format("{}", owned[i]);
  }
  out += "]";
  auto writeRes =
      utils::file::writeString(utils::string::pathToString(path), out);
  return static_cast<bool>(writeRes);
}

} // namespace ratedlayouts
