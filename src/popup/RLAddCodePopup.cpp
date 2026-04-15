#include "RLAddCodePopup.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"
#include <fmt/format.h>
#include <Geode/binding/UploadActionPopup.hpp>
#include "Geode/utils/general.hpp"

using namespace geode::prelude;

RLAddCodePopup* RLAddCodePopup::create(const std::string& code, const std::string& reward, long long id, std::function<void()> onSuccess) {
    auto popup = new RLAddCodePopup();
    if (popup && popup->init(code, reward, id, onSuccess)) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool RLAddCodePopup::init(const std::string& code, const std::string& reward, long long id, std::function<void()> onSuccess) {
    m_id = id;
    m_onSuccess = onSuccess;
    if (!Popup::init(300.f, 200.f, "GJ_square05.png"))
        return false;

    setTitle("Add Oracle Rubies Code");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    if (m_id >= 0) {
        std::string labelText = fmt::format("Editing Code #{}", m_id);
        m_title->setString("Edit Oracle Rubies Code");
        m_editingLabel = CCLabelBMFont::create(labelText.c_str(), "chatFont.fnt");
        if (m_editingLabel) {
            m_editingLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, 160.f});
            m_editingLabel->setScale(0.7f);
            m_mainLayer->addChild(m_editingLabel);
        }
    }

    m_codeInput = TextInput::create(240.f, "Code", "bigFont.fnt");
    m_codeInput->setString(code.empty() ? "" : code);
    m_codeInput->setCommonFilter(CommonFilter::Alphanumeric);
    m_codeInput->setPosition({m_mainLayer->getContentSize().width / 2.f, 130.f});
    m_mainLayer->addChild(m_codeInput);

    m_rewardInput = TextInput::create(240.f, "Reward", "bigFont.fnt");
    m_rewardInput->setString(reward.empty() ? "" : reward);
    m_rewardInput->setCommonFilter(CommonFilter::Int);
    m_rewardInput->setPosition({m_mainLayer->getContentSize().width / 2.f, 80.f});
    m_mainLayer->addChild(m_rewardInput);

    auto addSpr = ButtonSprite::create("Add", "goldFont.fnt", "GJ_button_01.png");
    auto addBtn = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(RLAddCodePopup::onAdd));
    addBtn->setPosition({m_mainLayer->getContentSize().width / 2.f, 30.f});
    m_buttonMenu->addChild(addBtn);

    return true;
}

void RLAddCodePopup::onAdd(CCObject* sender) {
    if (!m_codeInput || !m_rewardInput)
        return;

    std::string codeText = m_codeInput->getString();
    std::string rewardText = m_rewardInput->getString();

    std::transform(codeText.begin(), codeText.end(), codeText.begin(), ::tolower);

    auto upopup = UploadActionPopup::create(nullptr, m_id >= 0 ? "Updating code..." : "Adding code...");
    upopup->show();

    if (codeText.empty()) {
        upopup->showFailMessage("Code cannot be empty");
        return;
    }
    if (rewardText.empty()) {
        upopup->showFailMessage("Reward cannot be empty");
        return;
    }

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        upopup->showFailMessage("Argon token missing");
        return;
    }

    auto accountId = GJAccountManager::get()->m_accountID;
    if (accountId != rl::DEV_ACCOUNT_ID || accountId != rl::NOVA_ACCOUNT_ID) {
        upopup->showFailMessage("Not Authorized");
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountId;
    body["argonToken"] = token;
    body["code"] = codeText;
    body["reward"] = numFromString<int>(rewardText).unwrapOr(1);

    std::string endpoint;
    if (m_id >= 0) {
        endpoint = std::string(rl::BASE_API_URL) + "/setRubiesCode";
        body["id"] = m_id;
    } else {
        endpoint = std::string(rl::BASE_API_URL) + "/addRubiesCode";
    }

    Ref<RLAddCodePopup> self = this;
    m_submitTask.spawn(
        web::WebRequest().bodyJSON(body).post(endpoint),
        [self, upopup](web::WebResponse response) {
            if (!self || !upopup)
                return;
            if (!response.ok()) {
                upopup->showFailMessage(rl::getResponseFailMessage(response, "Failed to save code"));
                return;
            }
            auto jsonRes = response.json();
            if (!jsonRes) {
                upopup->showFailMessage("Invalid server response");
                return;
            }
            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (!success) {
                auto msg = json["message"].asString().unwrapOrDefault();
                if (msg.empty())
                    msg = "Failed to save code";
                upopup->showFailMessage(msg);
                return;
            }
            upopup->showSuccessMessage("Code saved");
            if (self && self->m_onSuccess) {
                self->m_onSuccess();
            }
            self->removeFromParentAndCleanup(true);
        });
}
