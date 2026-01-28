#include "RLAchievementsPopup.hpp"

#include <Geode/binding/GJCommentListLayer.hpp>

using namespace geode::prelude;

RLAchievementsPopup* RLAchievementsPopup::create() {
      auto ret = new RLAchievementsPopup();

      if (ret && ret->initAnchored(380.f, 260.f, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      }

      delete ret;
      return nullptr;
}

static RLAchievements::Collectable tabIndexToType(int idx) {
      switch (idx) {
            case 1:
                  return RLAchievements::Collectable::Sparks;
            case 2:
                  return RLAchievements::Collectable::Planets;
            case 3:
                  return RLAchievements::Collectable::Coins;
            case 4:
                  return RLAchievements::Collectable::Points;
            case 5:
                  return RLAchievements::Collectable::Misc;
            default:
                  return RLAchievements::Collectable::Sparks;  // unused for All
      }
}

void RLAchievementsPopup::onTab(CCObject* sender) {
      auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!item) return;
      int idx = item->getTag();
      m_selectedTab = idx;

      if (m_tabMenu) {
            auto children = m_tabMenu->getChildren();
            for (unsigned i = 0; i < children->count(); ++i) {
                  auto node = static_cast<CCMenuItemSpriteExtra*>(children->objectAtIndex(i));
                  if (!node) continue;

                  bool active = (node->getTag() == m_selectedTab);
            }
      }

      populate(m_selectedTab);
}

void RLAchievementsPopup::populate(int tabIndex) {
      if (!m_scrollLayer) return;
      auto content = m_scrollLayer->m_contentLayer;
      if (!content) return;

      content->removeAllChildrenWithCleanup(true);

      auto all = RLAchievements::getAll();
      for (auto const& ach : all) {
            if (tabIndex != 0) {
                  auto type = tabIndexToType(tabIndex);
                  if (ach.type != type) continue;
            }
            bool unlocked = RLAchievements::isAchieved(ach.id);
            auto cell = RLAchievementCell(ach, unlocked);
            if (cell) content->addChild(cell);
      }

      content->updateLayout();
      if (m_scrollLayer) m_scrollLayer->scrollToTop();
}

bool RLAchievementsPopup::setup() {
      m_tabMenu = CCMenu::create();
      m_tabMenu->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 30.f});
      m_tabMenu->setContentSize({m_mainLayer->getContentSize().width - 10, 50});
      m_tabMenu->setLayout(RowLayout::create()
                               ->setGap(5.f)
                               ->setGrowCrossAxis(true)
                               ->setCrossAxisOverflow(false));
      m_mainLayer->addChild(m_tabMenu, 1);

      for (unsigned i = 0; i < m_tabNames.size(); ++i) {
            auto name = m_tabNames[i];
            auto spr = ButtonSprite::create(name.c_str(), "goldFont.fnt", "GJ_button_01.png");
            auto item = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RLAchievementsPopup::onTab));
            item->setTag(static_cast<int>(i));
            m_tabMenu->addChild(item);
      }

      m_tabMenu->updateLayout();

      if (m_tabMenu && m_tabMenu->getChildren()->count() > 0) {
            auto initial = static_cast<CCMenuItemSpriteExtra*>(m_tabMenu->getChildren()->objectAtIndex(static_cast<unsigned>(m_selectedTab)));
            if (initial) onTab(initial);
      }

      auto commentLayer = GJCommentListLayer::create(nullptr, "Rated Layouts Achievements", {191, 114, 62, 255}, 340.f, 185.f, true);
      commentLayer->setPosition({20.f, 15.f});
      m_mainLayer->addChild(commentLayer);
      m_commentList = commentLayer;

      m_scrollLayer = ScrollLayer::create({340.f, 185.f});
      m_scrollLayer->setPosition({0, 0});
      commentLayer->addChild(m_scrollLayer);
      if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
            auto contentLayer = m_scrollLayer->m_contentLayer;
            auto layout = ColumnLayout::create();
            layout->setGap(0.f);
            layout->setAutoGrowAxis(185.f);
            layout->setAxisReverse(true);
            contentLayer->setLayout(layout);
      }

      populate(0);  // All

      return true;
}
