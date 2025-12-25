#include "RLAnnoucementPopup.hpp"

using namespace geode::prelude;

RLAnnoucementPopup* RLAnnoucementPopup::create() {
      auto popup = new RLAnnoucementPopup();
      if (popup && popup->initAnchored(400.f, 225.f, "GJ_square07.png")) {
            popup->autorelease();
            return popup;
      }
      CC_SAFE_DELETE(popup);
      return nullptr;
}

bool RLAnnoucementPopup::setup() {
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
      auto buttonSpr = ButtonSprite::create("Learn More", "bigFont.fnt", "GJ_button_01.png");
      auto buttonItem = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(RLAnnoucementPopup::onClick));
      buttonItem->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, 0});
      m_buttonMenu->addChild(buttonItem);

      m_mainLayer->addChild(clip, -1);
      return true;
}

void RLAnnoucementPopup::onClick(CCObject* sender) {
      utils::web::openLinkInBrowser("https://www.youtube.com/post/Ugkxrmmsw1brLhqV-1uXd_lxtb_thT87_qQy");
}
