#include "RLAnnouncementPopup.hpp"

using namespace geode::prelude;

RLAnnouncementPopup* RLAnnouncementPopup::create() {
      auto popup = new RLAnnouncementPopup();
      if (popup && popup->init()) {
            popup->autorelease();
            return popup;
      }
      delete popup;
      return nullptr;
}

bool RLAnnouncementPopup::init() {
      if (!Popup::init(400.f, 225.f))
            return false;

      auto imageSpr = LazySprite::create({m_mainLayer->getScaledContentSize()}, true);
      imageSpr->loadFromUrl("https://gdrate.arcticwoof.xyz/cdn/gauntletBanner.png", CCImage::kFmtPng, true);
      imageSpr->setAutoResize(true);

      auto sStencil = CCScale9Sprite::create("GJ_square06.png");
      if (sStencil) {
            sStencil->setAnchorPoint({0.f, 0.f});
            sStencil->setPosition({0.f, 0.f});
            sStencil->setContentSize({m_mainLayer->getScaledContentSize()});
      }

      auto clip = CCClippingNode::create(sStencil);
      clip->setPosition({0.f, 0.f});
      clip->setContentSize({m_mainLayer->getScaledContentSize()});
      clip->setAlphaThreshold(0.01f);

      // position image centered inside clip
      imageSpr->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height / 2.f});
      clip->addChild(imageSpr);

      // button
      auto buttonSpr = ButtonSprite::create("Learn More", "goldFont.fnt", "GJ_button_01.png");
      auto buttonItem = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(RLAnnouncementPopup::onClick));
      buttonItem->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, 0});
      m_buttonMenu->addChild(buttonItem);

      m_mainLayer->addChild(clip, -1);
      return true;
}

void RLAnnouncementPopup::onClick(CCObject* sender) {
      Notification::create("Opening a new link to the browser", NotificationIcon::Info)->show();
      utils::web::openLinkInBrowser("https://gdrate.arcticwoof.xyz/getRedirectURL");
}
