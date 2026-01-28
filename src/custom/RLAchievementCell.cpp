#include "RLAchievementCell.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

TableViewCell* RLAchievementCell(RLAchievements::Achievement const& ach, bool unlocked) {
      auto cellBase = TableViewCell::create();
      auto cell = static_cast<TableViewCell*>(cellBase);
      if (!cell) return nullptr;
      cell->setContentSize({340.f, 64.f});

      // left icon
      cocos2d::CCSprite* icon = nullptr;
      if (!ach.sprite.empty()) {
            icon = CCSprite::createWithSpriteFrameName(ach.sprite.c_str());
            if (!icon) icon = CCSprite::create(ach.sprite.c_str());
      }
      if (icon) {
            icon->setPosition({30.f, 32.f});
            if (ach.type == RLAchievements::Collectable::Sparks || ach.type == RLAchievements::Collectable::Planets || ach.type == RLAchievements::Collectable::Coins) {
                  icon->setScale(.6f);
            } else {
                  icon->setScale(1.f);
            }
            cell->addChild(icon);
      }

      // title
      auto title = CCLabelBMFont::create(ach.name.c_str(), "goldFont.fnt");
      title->setScale(0.7f);
      title->limitLabelWidth(380.f, 0.6f, 0.45f);
      title->setAnchorPoint({0.f, 1.f});
      title->setPosition({60.f, 50.f});
      cell->addChild(title);

      // description
      auto desc = CCLabelBMFont::create(ach.desc.c_str(), "chatFont.fnt");
      desc->setScale(0.7f);
      desc->setAnchorPoint({0.f, 1.f});
      desc->setPosition({60.f, 25.f});
      cell->addChild(desc);

      // status icon (check or lock)
      cocos2d::CCSprite* status = CCSprite::createWithSpriteFrameName(unlocked ? "GJ_completesIcon_001.png" : "GJ_lock_001.png");
      if (!status) status = CCSprite::create(unlocked ? "GJ_completesIcon_001.png" : "GJ_lock_001.png");
      if (status) {
            status->setPosition({300.f, 32.f});
            status->setScale(.8f);
            cell->addChild(status);
      } else {
            auto statLabel = CCLabelBMFont::create(unlocked ? "Unlocked" : "Locked", "bigFont.fnt");
            statLabel->setScale(0.45f);
            statLabel->setPosition({310.f, 32.f});
            cell->addChild(statLabel);
      }

      return cell;
}
