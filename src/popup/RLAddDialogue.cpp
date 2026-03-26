#include "RLAddDialogue.hpp"
#include <Geode/binding/FLAlertLayer.hpp>
#include "../include/RLNetworkUtils.hpp"
#include "../include/RLConstants.hpp"

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
    if (!Popup::init(400.f, 120.f))
        return false;
    setTitle("Add Custom Dialogue");

    m_dialogueInput = TextInput::create(360.f, "Dialogue...", "chatFont.fnt");
    m_dialogueInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChild(m_dialogueInput);
    m_dialogueInput->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f + 10.f});

    // submit button
    auto submitSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png");
    auto submitBtn = CCMenuItemSpriteExtra::create(submitSpr, this, menu_selector(RLAddDialogue::onSubmit));

    submitBtn->setPosition({m_mainLayer->getContentSize().width / 2.f + 60.f, 25.f});
    m_buttonMenu->addChild(submitBtn);

    // preview button
    auto previewSpr = ButtonSprite::create("Preview", "goldFont.fnt", "GJ_button_05.png");
    auto previewBtn = CCMenuItemSpriteExtra::create(previewSpr, this, menu_selector(RLAddDialogue::onPreview));
    previewBtn->setPosition({m_mainLayer->getContentSize().width / 2.f - 60.f, 25.f});
    m_buttonMenu->addChild(previewBtn);

    // info button
    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.7f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(RLAddDialogue::onInfo));
    infoBtn->setPosition({m_mainLayer->getContentSize().width, m_mainLayer->getContentSize().height});
    m_buttonMenu->addChild(infoBtn);

    return true;
}

void RLAddDialogue::onInfo(CCObject* sender) {
    const std::string content =
        "1. Dialogue <cr>cannot</c> be empty and must not exceed 500 characters.\n"
        "2. Dialogue must be <cg>appropriate</c>. Submitting an <cr>inappropriate dialogue</c> may result in a <cr>ban</c> from using this feature.\n"
        "3. Do <cr>not</c> bypass any of these rules.\n"
        "### <cy>By submitting a custom dialogue, you agree to follow these rules.\n"
        "### <cl>ArcticWoof has the rights to remove any dialogue that is inappropriate without any notice.</c>\n"
        "### <cc>All dialogues submitted are logged and visible by Layout Admins/Mods for reviewing purposes.</c>";
    MDPopup::create(
        "Custom Dialogue Rules",
        content.c_str(),
        "OK")
        ->show();
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
    if (dialogueText.length() > 500) {
        upopup->showFailMessage("Dialogue cannot exceed 500 characters!");
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
        req.post(std::string(rl::BASE_API_URL) + "/setDialogue"),
        [self, upopup](web::WebResponse res) {
            if (!self) return;
            if (!res.ok()) {
                upopup->showFailMessage(rl::getResponseFailMessage(res, "Failed to submit dialogue!"));
                return;
            }
            upopup->showSuccessMessage("Dialogue submitted successfully!");
            self->m_dialogueInput->setString("");
        });
}
