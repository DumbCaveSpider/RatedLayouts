#include <Geode/Geode.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

class CacheClearSettingV3 : public SettingV3 {
     public:
      static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
            auto res = std::make_shared<CacheClearSettingV3>();
            auto root = checkJson(json, "CacheClearSettingV3");
            res->init(key, modID, root);
            res->parseNameAndDescription(root);
            res->parseEnableIf(root);
            root.checkUnknownKeys();
            return root.ok(std::static_pointer_cast<SettingV3>(res));
      }

      bool load(matjson::Value const& json) override { return true; }
      bool save(matjson::Value& json) const override { return true; }
      bool isDefaultValue() const override { return true; }
      void reset() override {}

      SettingNodeV3* createNode(float width) override;
};

class CacheClearSettingNodeV3 : public SettingNodeV3 {
     protected:
      ButtonSprite* m_buttonSprite = nullptr;
      CCMenuItemSpriteExtra* m_button = nullptr;

      bool init(std::shared_ptr<CacheClearSettingV3> setting, float width) {
            if (!SettingNodeV3::init(setting, width))
                  return false;

            m_buttonSprite = ButtonSprite::create("Delete Cached Data", "goldFont.fnt", "GJ_button_06.png");
            m_buttonSprite->setScale(.7f);
            m_button = CCMenuItemSpriteExtra::create(m_buttonSprite, this, menu_selector(CacheClearSettingNodeV3::onButton));
            this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Left);
            this->getButtonMenu()->updateLayout();
            m_button->setPositionX(-45.f);

            this->updateState(nullptr);
            return true;
      }

      void updateState(CCNode* invoker) override {
            SettingNodeV3::updateState(invoker);
            auto shouldEnable = this->getSetting()->shouldEnable();
            if (m_button) m_button->setEnabled(shouldEnable);
            if (m_buttonSprite) {
                  m_buttonSprite->setCascadeColorEnabled(true);
                  m_buttonSprite->setCascadeOpacityEnabled(true);
                  m_buttonSprite->setOpacity(shouldEnable ? 255 : 155);
                  m_buttonSprite->setColor(shouldEnable ? ccWHITE : ccGRAY);
            }
      }

      void onButton(CCObject*) {
            geode::createQuickPopup(
                "Delete Cached Data",
                "Are you sure you want to <cr>delete/clear</c> all cached <cl>Rated Layouts</c> level ratings and user roles data?\n<cg>This does not reset/delete your account stats!</c>\n<cy>This action cannot be undone.</c>",
                "No",
                "Delete",
                [this](auto, bool yes) {
                      if (!yes) return;

                      auto saveDir = dirs::getModsSaveDir();
                      auto userCache = saveDir / "user_role_cache.json";
                      auto levelCache = saveDir / "level_ratings_cache.json";

                      bool deletedAny = false;

                      auto userCachePathStr = geode::utils::string::pathToString(userCache);
                      auto levelCachePathStr = geode::utils::string::pathToString(levelCache);

                      if (utils::file::readString(userCachePathStr)) {
                            auto writeRes = utils::file::writeString(userCachePathStr, "{}");
                            if (!writeRes) {
                                  log::warn("Failed to clear user cache file: {}", userCachePathStr);
                            }
                            deletedAny = true;
                      }
                      if (utils::file::readString(levelCachePathStr)) {
                            auto writeRes = utils::file::writeString(levelCachePathStr, "{}");
                            if (!writeRes) {
                                  log::warn("Failed to clear level cache file: {}", levelCachePathStr);
                            }
                            deletedAny = true;
                      }

                      if (deletedAny) {
                            Notification::create("Rated Layouts cache deleted", NotificationIcon::Success)->show();
                      } else {
                            Notification::create("No Rated Layouts cache files found", NotificationIcon::Info)->show();
                      }
                });
      }

      void onCommit() override {}
      void onResetToDefault() override {}

     public:
      static CacheClearSettingNodeV3* create(std::shared_ptr<CacheClearSettingV3> setting, float width) {
            auto ret = new CacheClearSettingNodeV3();
            if (ret->init(setting, width)) {
                  ret->autorelease();
                  return ret;
            }
            delete ret;
            return nullptr;
      }

      bool hasUncommittedChanges() const override { return false; }
      bool hasNonDefaultValue() const override { return false; }

      std::shared_ptr<CacheClearSettingV3> getSetting() const {
            return std::static_pointer_cast<CacheClearSettingV3>(SettingNodeV3::getSetting());
      }
};

SettingNodeV3* CacheClearSettingV3::createNode(float width) {
      return CacheClearSettingNodeV3::create(std::static_pointer_cast<CacheClearSettingV3>(shared_from_this()), width);
}

$execute {
      (void)Mod::get()->registerCustomSettingType("rated-layouts-clear-cache", &CacheClearSettingV3::parse);
}