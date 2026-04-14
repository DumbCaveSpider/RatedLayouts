#include "RLSettingsV3.hpp"
#include "../include/RLNetworkUtils.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include "Geode/ui/Popup.hpp"

using namespace geode::prelude;

namespace rl {

    Result<std::shared_ptr<SettingV3>> RLClearCacheButtonSettingV3::parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json) {
        auto res = std::make_shared<RLClearCacheButtonSettingV3>();
        auto root = checkJson(json, "RLClearCacheButtonSettingV3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);

        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3* RLClearCacheButtonSettingV3::createNode(float width) {
        return RLClearCacheButtonSettingNodeV3::create(
            std::static_pointer_cast<RLClearCacheButtonSettingV3>(shared_from_this()),
            width);
    }

    bool RLClearCacheButtonSettingV3::load(matjson::Value const& json) {
        return true;
    }

    bool RLClearCacheButtonSettingV3::save(matjson::Value& json) const {
        return true;
    }

    bool RLClearCacheButtonSettingV3::isDefaultValue() const {
        return true;
    }

    void RLClearCacheButtonSettingV3::reset() {}

    bool RLClearCacheButtonSettingNodeV3::init(
        std::shared_ptr<RLClearCacheButtonSettingV3> setting,
        float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create(
            "Clear",
            100.f,
            80.f,
            0.5f,
            true,
            "goldFont.fnt",
            "GJ_button_06.png",
            20.f);

        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite,
            this,
            menu_selector(RLClearCacheButtonSettingNodeV3::confirmClear));

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(150);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);
        return true;
    }

    void RLClearCacheButtonSettingNodeV3::updateState(CCNode* invoker) {
        SettingNodeV3::updateState(invoker);
        bool hasCache = rl::requestCacheExists();
        bool shouldEnable = this->getSetting()->shouldEnable() && hasCache;

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

    void RLClearCacheButtonSettingNodeV3::confirmClear(CCObject*) {
        createQuickPopup("Clear Cache",
            "Are you sure you want to <cg>clear the cache</c>?\n"
            "<cy>This will remove cached data for levels and comments.</c>\n"
            "<cr>This action cannot be undone.</c>",
            "No",
            "Yes",
            [this](auto, bool yes) {
                if (!yes)
                    return;
                rl::clearRequestCache();
                this->updateState(nullptr);
                Notification::create(
                    "Cache Cleared",
                    NotificationIcon::Success)
                    ->show();
            });
    }

    void RLClearCacheButtonSettingNodeV3::onCommit() {}
    void RLClearCacheButtonSettingNodeV3::onResetToDefault() {}
    bool RLClearCacheButtonSettingNodeV3::hasUncommittedChanges() const {
        return false;
    }
    bool RLClearCacheButtonSettingNodeV3::hasNonDefaultValue() const {
        return false;
    }

    RLClearCacheButtonSettingNodeV3* RLClearCacheButtonSettingNodeV3::create(
        std::shared_ptr<RLClearCacheButtonSettingV3> setting,
        float width) {
        auto ret = new RLClearCacheButtonSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    std::shared_ptr<RLClearCacheButtonSettingV3>
    RLClearCacheButtonSettingNodeV3::getSetting() const {
        return std::static_pointer_cast<RLClearCacheButtonSettingV3>(SettingNodeV3::getSetting());
    }

    $on_mod(Loaded) {
        (void)Mod::get()->registerCustomSettingType(
            "rl-clear-cache-button",
            &RLClearCacheButtonSettingV3::parse);
    }

}  // namespace rl
