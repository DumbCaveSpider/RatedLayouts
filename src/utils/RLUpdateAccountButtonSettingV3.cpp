#include "RLUpdateAccountButtonSettingV3.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"
#include <Geode/binding/UploadActionPopup.hpp>

using namespace geode::prelude;

namespace rl {

    Result<std::shared_ptr<SettingV3>> RLUpdateAccountButtonSettingV3::parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json) {
        auto res = std::make_shared<RLUpdateAccountButtonSettingV3>();
        auto root = checkJson(json, "RLUpdateAccountButtonSettingV3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);

        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3* RLUpdateAccountButtonSettingV3::createNode(float width) {
        return RLUpdateAccountButtonSettingNodeV3::create(
            std::static_pointer_cast<RLUpdateAccountButtonSettingV3>(shared_from_this()),
            width);
    }

    bool RLUpdateAccountButtonSettingV3::load(matjson::Value const& json) {
        return true;
    }

    bool RLUpdateAccountButtonSettingV3::save(matjson::Value& json) const {
        return true;
    }

    bool RLUpdateAccountButtonSettingV3::isDefaultValue() const {
        return true;
    }

    void RLUpdateAccountButtonSettingV3::reset() {}

    bool RLUpdateAccountButtonSettingNodeV3::init(
        std::shared_ptr<RLUpdateAccountButtonSettingV3> setting,
        float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create(
            "Update",
            120.f,
            80.f,
            0.5f,
            true,
            "goldFont.fnt",
            "GJ_button_01.png",
            20.f);

        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite,
            this,
            menu_selector(RLUpdateAccountButtonSettingNodeV3::onUpdateAccount));

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(180);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);
        return true;
    }

    void RLUpdateAccountButtonSettingNodeV3::updateState(CCNode* invoker) {
        SettingNodeV3::updateState(invoker);
        bool hasAccount = GJAccountManager::get()->m_accountID != 0;
        bool hasToken = !Mod::get()->getSavedValue<std::string>("argon_token").empty();
        bool shouldEnable = this->getSetting()->shouldEnable() && hasAccount && hasToken;

        if (m_button) {
            m_button->setEnabled(shouldEnable);
        }

        if (m_buttonSprite) {
            m_buttonSprite->setCascadeColorEnabled(true);
            m_buttonSprite->setCascadeOpacityEnabled(true);
            if (shouldEnable) {
                m_buttonSprite->setColor(ccWHITE);
            } else {
                m_buttonSprite->setColor(ccGRAY);
            }
        }
    }

    void RLUpdateAccountButtonSettingNodeV3::onUpdateAccount(CCObject*) {
        if (GJAccountManager::get()->m_accountID == 0) {
            Notification::create("No account logged in", NotificationIcon::Warning)
                ->show();
            return;
        }

        auto token = Mod::get()->getSavedValue<std::string>("argon_token");
        if (token.empty()) {
            Notification::create("Argon token missing", NotificationIcon::Warning)
                ->show();
            return;
        }

        auto popup = UploadActionPopup::create(nullptr, "Updating Account...");
        popup->show();

        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
        jsonBody["argonToken"] = token;

        auto req = web::WebRequest();
        req.bodyJSON(jsonBody);

        Ref<RLUpdateAccountButtonSettingNodeV3> self = this;
        Ref<UploadActionPopup> popupRef = popup;
        async::spawn(req.post(std::string(rl::BASE_API_URL) + "/resetAccountInfo"),
            [self, popupRef](web::WebResponse res) {
                if (!self || !popupRef)
                    return;
                if (!res.ok()) {
                    std::string err = rl::getResponseFailMessage(res, "Failed to update account.");
                    popupRef->showFailMessage(err);
                    return;
                }
                auto jsonRes = res.json();
                if (!jsonRes) {
                    std::string err = rl::getResponseFailMessage(res, "Failed to update account.");
                    popupRef->showFailMessage(err);
                    return;
                }
                auto json = jsonRes.unwrap();
                bool success = json["success"].asBool().unwrapOr(false);
                if (success) {
                    popupRef->showSuccessMessage("Account updated successfully.");
                } else {
                    std::string message = rl::getResponseFailMessage(res, "Failed to update account.");
                    popupRef->showFailMessage(message);
                }
            });
    }

    void RLUpdateAccountButtonSettingNodeV3::onCommit() {}
    void RLUpdateAccountButtonSettingNodeV3::onResetToDefault() {}
    bool RLUpdateAccountButtonSettingNodeV3::hasUncommittedChanges() const {
        return false;
    }
    bool RLUpdateAccountButtonSettingNodeV3::hasNonDefaultValue() const {
        return false;
    }

    RLUpdateAccountButtonSettingNodeV3* RLUpdateAccountButtonSettingNodeV3::create(
        std::shared_ptr<RLUpdateAccountButtonSettingV3> setting,
        float width) {
        auto ret = new RLUpdateAccountButtonSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    std::shared_ptr<RLUpdateAccountButtonSettingV3>
    RLUpdateAccountButtonSettingNodeV3::getSetting() const {
        return std::static_pointer_cast<RLUpdateAccountButtonSettingV3>(SettingNodeV3::getSetting());
    }

    $on_mod(Loaded) {
        (void)Mod::get()->registerCustomSettingType(
            "rl-update-account-button",
            &RLUpdateAccountButtonSettingV3::parse);
    }

}  // namespace rl
