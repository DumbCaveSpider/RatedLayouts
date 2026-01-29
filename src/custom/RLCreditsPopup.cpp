#include "RLCreditsPopup.hpp"
#include "../custom/RLAchievements.hpp"

#include <Geode/modify/ProfilePage.hpp>

using namespace geode::prelude;

RLCreditsPopup* RLCreditsPopup::create() {
      auto ret = new RLCreditsPopup();

      if (ret && ret->initAnchored(380.f, 250.f, "GJ_square01.png")) {
            ret->autorelease();
            return ret;
      }

      delete ret;
      return nullptr;
};

bool RLCreditsPopup::setup() {
      setTitle("Rated Layouts Credits");

      auto scrollLayer = ScrollLayer::create({340.f, 195.f});
      scrollLayer->setPosition({20.f, 23.f});
      m_mainLayer->addChild(scrollLayer);
      addListBorders(m_mainLayer, {m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 - 5.f}, {340.f, 195.f});
      m_scrollLayer = scrollLayer;

      // info button
      auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
      infoSpr->setScale(0.75f);
      auto infoBtn = CCMenuItemSpriteExtra::create(
          infoSpr,
          this,
          menu_selector(RLCreditsPopup::onInfo));
      infoBtn->setPosition({m_mainLayer->getContentSize().width, m_mainLayer->getContentSize().height - 3});
      m_buttonMenu->addChild(infoBtn);

      // create spinner
      auto contentLayer = m_scrollLayer->m_contentLayer;
      if (contentLayer) {
            auto layout = ColumnLayout::create();
            contentLayer->setLayout(layout);
            layout->setGap(0.f);
            layout->setAutoGrowAxis(0.f);
            layout->setAxisReverse(true);

            auto spinner = LoadingSpinner::create(48.f);
            spinner->setPosition(contentLayer->getContentSize() / 2);
            contentLayer->addChild(spinner);
            m_spinner = spinner;
      }

      // Fetch mod players
      Ref<RLCreditsPopup> self = this;
      web::WebRequest()
          .get("https://gdrate.arcticwoof.xyz/getCredits")
          .listen([self](web::WebResponse* response) {
                if (!self) return;
                if (!response || !response->ok()) {
                      log::warn("getCredits returned non-ok status: {}", response ? response->code() : -1);
                      if (self->m_spinner) {
                            self->m_spinner->removeFromParent();
                            self->m_spinner = nullptr;
                      }
                      Notification::create("Failed to fetch mod players", NotificationIcon::Error)->show();
                      return;
                }

                auto jsonRes = response->json();
                if (!jsonRes) {
                      log::warn("Failed to parse mod players JSON");
                      if (self->m_spinner) {
                            self->m_spinner->removeFromParent();
                            self->m_spinner = nullptr;
                      }
                      Notification::create("Invalid server response", NotificationIcon::Error)->show();
                      return;
                }

                auto json = jsonRes.unwrap();
                bool success = json["success"].asBool().unwrapOrDefault();
                if (!success) {
                      log::warn("Server returned success=false for getMod");
                      if (self->m_spinner) {
                            self->m_spinner->removeFromParent();
                            self->m_spinner = nullptr;
                      }
                      return;
                }
                
                RLAchievements::onReward("misc_credits");

                // populate players
                auto content = self->m_scrollLayer->m_contentLayer;
                if (!content) return;
                if (self->m_spinner) {
                      self->m_spinner->removeFromParent();
                      self->m_spinner = nullptr;
                }
                content->removeAllChildrenWithCleanup(true);

                auto addHeader = [&](std::string_view text) {  // header yesz
                      auto tableCell = TableViewCell::create();
                      tableCell->setContentSize({340.f, 30.f});
                      auto label = CCLabelBMFont::create(std::string{text}.c_str(), "bigFont.fnt");
                      label->setScale(0.6f);
                      label->setAnchorPoint({0.5f, 0.5f});
                      // center the label directly
                      label->setPosition({340.f / 2.f, 15.f});

                      tableCell->addChild(label);

                      // divider lines for header
                      const float contentW = tableCell->getContentSize().width;
                      const float contentH = tableCell->getContentSize().height;
                      const float dividerH = 1.f;  // thin line
                      const float halfDivider = dividerH / 2.f;
                      const float topY = contentH - halfDivider;
                      const float bottomY = halfDivider;

                      if (content->getChildren()->count() > 0) {
                            auto headerTopDivider = CCSprite::create();
                            headerTopDivider->setTextureRect(CCRectMake(0, 0, contentW, dividerH));
                            headerTopDivider->setPosition({contentW / 2.f, topY});
                            headerTopDivider->setColor({0, 0, 0});
                            headerTopDivider->setOpacity(80);
                            tableCell->addChild(headerTopDivider, 2);
                      }

                      auto headerDivider = CCSprite::create();
                      headerDivider->setTextureRect(CCRectMake(0, 0, contentW, dividerH));
                      headerDivider->setPosition({contentW / 2.f, bottomY});
                      headerDivider->setColor({0, 0, 0});
                      headerDivider->setOpacity(80);
                      tableCell->addChild(headerDivider, 2);
                      content->addChild(tableCell);
                };

                auto addPlayer = [&](const matjson::Value& userVal, bool isAdmin) {
                      if (!userVal.isObject()) return;

                      int accountId = userVal["accountId"].asInt().unwrapOrDefault();
                      std::string username = userVal["username"].asString().unwrapOrDefault();
                      int iconId = userVal["iconid"].asInt().unwrapOrDefault();
                      int color1 = userVal["color1"].asInt().unwrapOrDefault();
                      int color2 = userVal["color2"].asInt().unwrapOrDefault();
                      int color3 = userVal["color3"].asInt().unwrapOrDefault();

                      auto cell = TableViewCell::create();
                      cell->setContentSize({340.f, 50.f});

                      // color bg ass
                      auto bgSprite = CCSprite::create();
                      bgSprite->setTextureRect(CCRectMake(0, 0, 340.f, 50.f));
                      bgSprite->setPosition({170.f, 25.f});
                      if (isAdmin) {
                            bgSprite->setColor({161, 88, 44});
                      } else {
                            bgSprite->setColor({194, 114, 62});
                      }
                      cell->addChild(bgSprite);

                      auto gm = GameManager::sharedState();
                      auto player = SimplePlayer::create(iconId);
                      player->updatePlayerFrame(iconId, IconType::Cube);
                      player->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
                      if (color3 != 0) player->setGlowOutline(gm->colorForIdx(color3));
                      player->setPosition({40.f, 25.f});
                      cell->addChild(player);

                      auto nameLabel = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
                      nameLabel->setAnchorPoint({0.f, 0.5f});
                      nameLabel->setScale(0.8f);
                      nameLabel->setPosition({80.f, 25.f});

                      auto menu = CCMenu::create();
                      menu->setPosition({0, 0});
                      auto accountButton = CCMenuItemSpriteExtra::create(nameLabel, self, menu_selector(RLCreditsPopup::onAccountClicked));
                      accountButton->setTag(accountId);
                      accountButton->setPosition({70.f, 25.f});
                      accountButton->setAnchorPoint({0.f, 0.5f});
                      menu->addChild(accountButton);
                      cell->addChild(menu);

                      content->addChild(cell);
                };

                if (json.contains("admins") && json["admins"].isArray()) {
                      addHeader("Layout Admins");
                      auto admins = json["admins"].asArray().unwrap();
                      for (auto& val : admins) addPlayer(val, true);
                }

                if (json.contains("moderators") && json["moderators"].isArray()) {
                      addHeader("Layout Moderators");
                      auto mods = json["moderators"].asArray().unwrap();
                      for (auto& val : mods) addPlayer(val, false);
                }

                if (json.contains("supporters") && json["supporters"].isArray()) {
                      addHeader("Layout Supporters");
                      auto sup = json["supporters"].asArray().unwrap();
                      for (auto& val : sup) {
                            addPlayer(val, false);
                      }
                }

                content->updateLayout();
                if (self->m_scrollLayer) self->m_scrollLayer->scrollToTop();
          });

      return true;
}

void RLCreditsPopup::onAccountClicked(CCObject* sender) {
      auto button = static_cast<CCMenuItem*>(sender);
      int accountId = button->getTag();
      ProfilePage::create(accountId, false)->show();
}

void RLCreditsPopup::onInfo(CCObject* sender) {
      MDPopup::create(
          "How to become a Layout Moderator",
          "Firstly, you need to be active on the <cl>Rated Layouts Discord server</c> and actively show interest in helping with the mod.\n\n"
          "<cl>Rated Layouts</c> don't promote any <cg>Layout Moderators</c> outside the Discord Server due to various reason and to keep things in track.\n\n"
          "### <cr>Begging for Layout Moderator will less likely for ArcticWoof to promote you!</c>\n\n"
          "All decision on who becomes a <cg>Layout Admin/Moderator</c> is up to <cf>ArcticWoof</c>.\n\n"
          "There will be an <cy>application</c> that will be posted on the <cl>Discord server</c> for more information!\n\n"
          "If you do show that you have interest in helping with the mod, you may be contacted by <cf>ArcticWoof</c>."
          "\r\n\r\n---\r\n\r\n"
          "We currenly looking for those who has good knowledge in <cb>fun Gameplay and Layout Designs</c> to become <cg>Layout Moderators/Admins</c> or if other moderators/admins has shown interest in you, you may get the chance to get promoted!\n\n",
          "OK")
          ->show();
}