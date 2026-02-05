#include "RLBadgeRequestPopup.hpp"
#include "RLAchievements.hpp"

using namespace geode::prelude;

RLBadgeRequestPopup* RLBadgeRequestPopup::create() {
      auto ret = new RLBadgeRequestPopup();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      delete ret;
      return nullptr;
}

bool RLBadgeRequestPopup::init() {
      if (!Popup::init(420.f, 280.f, "GJ_square02.png"))
            return false;

      setTitle("Claim Layout Supporter Badge");

      auto cs = m_mainLayer->getContentSize();

      m_discordInput = TextInput::create(370.f, "Discord Username");
      m_discordInput->setCommonFilter(CommonFilter::Any);
      m_discordInput->setPosition({cs.width / 2.f, cs.height - 50.f});
      m_mainLayer->addChild(m_discordInput);

      // info text
      auto infoText = MDTextArea::create(
          "Enter your <co>Discord Username (not display name)</c> that is linked to your <cp>Ko-fi account</c> to receieve a <cp>Layout Supporter Badge</c>.\n\n"
          "Make sure that you got the <cd>Rated Layouts Supporter Membership</c> and have already <cg>linked</c> your <cb>Discord Account</c> through <cp>Ko-fi.</c> beforehand!\n\n"
          "### If you encounter any <cr>issue</c> during this process, please contact <cf>ArcticWoof</c> on <cb>Discord</c>.",
          {cs.width - 40.f, 100.f});
      infoText->setPosition({cs.width / 2.f, cs.height - 120.f});
      infoText->setAnchorPoint({0.5f, 0.5f});
      infoText->setID("rl-badge-request-info");
      m_mainLayer->addChild(infoText);

      // claim button
      auto submitSpr = ButtonSprite::create("Claim", "goldFont.fnt", "GJ_button_01.png");
      auto submitBtn = CCMenuItemSpriteExtra::create(submitSpr, this, menu_selector(RLBadgeRequestPopup::onSubmit));
      submitBtn->setPosition({cs.width / 2.f, 0.f});
      m_buttonMenu->addChild(submitBtn);

      return true;
}

void RLBadgeRequestPopup::onSubmit(CCObject* sender) {
      if (!m_discordInput) return;
      auto upopup = UploadActionPopup::create(nullptr, "Submitting Badge Request...");
      upopup->show();
      auto discord = m_discordInput->getString();
      if (discord.empty()) {
            upopup->showFailMessage("Please enter your Discord username");
            return;
      }

      matjson::Value body = matjson::Value::object();
      body["discordUsername"] = discord;
      body["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
      body["accountId"] = GJAccountManager::get()->m_accountID;

      auto req = web::WebRequest();
      req.bodyJSON(body);
      Ref<RLBadgeRequestPopup> self = this;
      self->m_getSupporterTask.spawn(
          req.post("https://gdrate.arcticwoof.xyz/getSupporter"),
          [self, upopup](web::WebResponse res) {
                if (!self) return;
                if (!res.ok()) {
                      upopup->showFailMessage("Discord Username doesn't exists.");
                      return;
                }

                auto str = res.string().unwrapOrDefault();
                if (!str.empty()) {
                      if (!res.ok()) {
                            upopup->showFailMessage(str);
                            return;
                      }
                      Notification::create(str, NotificationIcon::Success)->show();
                      RLAchievements::onReward("misc_support");
                      self->removeFromParent();
                      return;
                }

                if (!res.ok()) {
                      // show status code
                      upopup->showFailMessage(fmt::format("Failed to submit request (code {})", res.code()));
                      return;
                }

                upopup->showSuccessMessage("Supporter Badge acquired!");
                RLAchievements::onReward("misc_support");
                self->removeFromParent();
          });
}