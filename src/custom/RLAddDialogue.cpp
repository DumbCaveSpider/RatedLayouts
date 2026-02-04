#include "RLAddDialogue.hpp"

using namespace geode::prelude;

RLAddDialogue* RLAddDialogue::create() {
      auto popup = new RLAddDialogue();
      if (popup && popup->init()) {
            popup->autorelease();
            return popup;
      }
      delete popup;
      return nullptr;
}

bool RLAddDialogue::init() {
      if (!Popup::init(400.f, 90.f))
            return false;
      setTitle("Add Custom Dialogue");

      m_dialogueInput = TextInput::create(360.f, "Dialogue...", "chatFont.fnt");
      m_dialogueInput->setCommonFilter(CommonFilter::Any);
      m_mainLayer->addChild(m_dialogueInput);
      m_dialogueInput->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f - 5.f});

      // submit button
      auto submitSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png");
      auto submitBtn = CCMenuItemSpriteExtra::create(submitSpr, this, menu_selector(RLAddDialogue::onSubmit));

      submitBtn->setPosition({m_mainLayer->getContentSize().width / 2.f + 60.f, 0.f});
      m_buttonMenu->addChild(submitBtn);

      // preview button
      auto previewSpr = ButtonSprite::create("Preview", "goldFont.fnt", "GJ_button_01.png");
      auto previewBtn = CCMenuItemSpriteExtra::create(previewSpr, this, menu_selector(RLAddDialogue::onPreview));
      previewBtn->setPosition({m_mainLayer->getContentSize().width / 2.f - 60.f, 0.f});
      m_buttonMenu->addChild(previewBtn);
      return true;
}

void RLAddDialogue::onPreview(CCObject* sender) {
      if (!m_dialogueInput) return;
      std::string dialogueText = m_dialogueInput->getString();
      if (dialogueText.empty()) {
            Notification::create("Dialogue cannot be empty!", NotificationIcon::Error)->show();
            return;
      }
      DialogObject* dialogObj = DialogObject::create(
          "Layout Creator",
          dialogueText.c_str(),
          28,
          1.f,
          false,
          ccWHITE);
      if (dialogObj) {
            auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
            dialog->addToMainScene();
            dialog->animateInRandomSide();
      }
}

void RLAddDialogue::onSubmit(CCObject* sender) {
      if (!m_dialogueInput) return;
      auto upopup = UploadActionPopup::create(nullptr, "Submitting Dialogue...");
      upopup->show();
      std::string dialogueText = m_dialogueInput->getString();
      if (dialogueText.empty()) {
            upopup->showFailMessage("Dialogue cannot be empty!");
            return;
      }
      // send to server
      matjson::Value body = matjson::Value::object();
      body["body"] = dialogueText;
      body["accountId"] = GJAccountManager::get()->m_accountID;
      body["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");

      auto req = web::WebRequest();
      req.bodyJSON(body);
      Ref<RLAddDialogue> self = this;
      self->m_setDialogueTask.spawn(
          req.post("https://gdrate.arcticwoof.xyz/setDialogue"),
          [self, upopup](web::WebResponse res) {
                if (!self) return;
                if (!res.ok()) {
                      upopup->showFailMessage("Failed to submit dialogue!");
                      return;
                }
                upopup->showSuccessMessage("Dialogue submitted successfully!");
          });
}