#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

namespace rl {

    class RLClearCacheButtonSettingV3 : public SettingV3 {
    public:
        static Result<std::shared_ptr<SettingV3>> parse(
            std::string const& key,
            std::string const& modID,
            matjson::Value const& json);

        SettingNodeV3* createNode(float width) override;

        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        bool isDefaultValue() const override;
        void reset() override;
    };

    class RLClearCacheButtonSettingNodeV3 : public SettingNodeV3 {
    protected:
        ButtonSprite* m_buttonSprite = nullptr;
        CCMenuItemSpriteExtra* m_button = nullptr;

        bool init(std::shared_ptr<RLClearCacheButtonSettingV3> setting, float width);
        void updateState(CCNode* invoker) override;
        void confirmClear(CCObject* sender);
        void onCommit() override;
        void onResetToDefault() override;
        bool hasUncommittedChanges() const override;
        bool hasNonDefaultValue() const override;

    public:
        static RLClearCacheButtonSettingNodeV3* create(
            std::shared_ptr<RLClearCacheButtonSettingV3> setting,
            float width);

        std::shared_ptr<RLClearCacheButtonSettingV3> getSetting() const;
    };

}  // namespace rl
