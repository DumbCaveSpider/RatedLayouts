#include "RLUserLevelControl.hpp"
#include "../include/RLNetworkUtils.hpp"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include <Geode/Geode.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

RLUserLevelControl* RLUserLevelControl::create(int accountId) {
    auto ret = new RLUserLevelControl();
    ret->m_targetId = accountId;

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLUserLevelControl::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square04.png"))
        return false;
    setTitle("Rated Layouts User Level Mod Panel");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);
    m_noElasticity = true;

    // show target user's name in title if possible
    std::string username =
        fmt::format("Target: {}",
            GameLevelManager::sharedState()->tryGetUsername(m_targetId));
    if (!username.empty()) {
        m_usernameLabel = CCLabelBMFont::create(username.c_str(), "bigFont.fnt");
        m_usernameLabel->setPosition(m_title->getPositionX(),
            m_title->getPositionY() - 20);
        m_usernameLabel->limitLabelWidth(m_mainLayer->getContentWidth(), .5f, 0.3f);
        m_usernameLabel->setString(username.c_str());
        m_mainLayer->addChild(m_usernameLabel);
    }

    auto cs = m_mainLayer->getContentSize();

    // numeric input for target level id
    m_levelInput = TextInput::create(220.f, "Level ID to Uncomplete");
    m_levelInput->setCommonFilter(CommonFilter::Int);
    m_levelInput->setPosition({cs.width / 2.f, cs.height / 2.f});
    m_mainLayer->addChild(m_levelInput);

    // removal button
    auto removeSpr =
        ButtonSprite::create("Remove", "goldFont.fnt", "GJ_button_06.png");
    m_removeButton = CCMenuItemSpriteExtra::create(
        removeSpr, this, menu_selector(RLUserLevelControl::onRemoveLevel));
    m_removeButton->setPosition({cs.width / 2.f, 30.f});
    m_buttonMenu->addChild(m_removeButton);

    return true;
}

void RLUserLevelControl::onRemoveLevel(CCObject* sender) {
    if (!m_levelInput)
        return;

    std::string text = m_levelInput->getString();
    auto upopup =
        UploadActionPopup::create(nullptr, "Removing level completion...");
    upopup->show();
    if (text.empty()) {
        upopup->showFailMessage("Please enter a level ID");
        return;
    }
    int levelId = atoi(text.c_str());

    m_removeButton->setEnabled(false);

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        upopup->showFailMessage("Authentication token not found");
        m_removeButton->setEnabled(true);
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = token;
    body["targetAccountId"] = m_targetId;
    body["targetLevelId"] = levelId;

    auto req = web::WebRequest();
    req.bodyJSON(body);
    Ref<RLUserLevelControl> self = this;
    self->m_removeLevelTask.spawn(
        req.post("https://gdrate.arcticwoof.xyz/deleteCompletionLevel"),
        [self, upopup](web::WebResponse res) {
            if (!self || !upopup)
                return;

            self->m_removeButton->setEnabled(true);

            if (!res.ok()) {
                upopup->showFailMessage(
                    rl::getResponseFailMessage(res, "Failed to remove level"));
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                upopup->showFailMessage("Invalid server response");
                return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (success) {
                upopup->showSuccessMessage("Level removed!");
                self->m_levelInput->setString("");
            } else {
                upopup->showFailMessage("Failed to remove level");
            }
        });
}
