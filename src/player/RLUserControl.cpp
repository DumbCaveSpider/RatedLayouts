#include "RLUserControl.hpp"

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

RLUserControl* RLUserControl::create() {
      auto ret = new RLUserControl();

      if (ret && ret->initAnchored(380.f, 240.f, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      }

      CC_SAFE_DELETE(ret);
      return nullptr;
};

RLUserControl* RLUserControl::create(int accountId) {
      auto ret = new RLUserControl();
      ret->m_targetAccountId = accountId;

      if (ret && ret->initAnchored(380.f, 240.f, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      }

      CC_SAFE_DELETE(ret);
      return nullptr;
};

bool RLUserControl::setup() {
      setTitle("Rated Layouts User Control");

      auto optionsMenu = CCMenu::create();
      optionsMenu->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 - 5});
      optionsMenu->setContentSize({m_mainLayer->getContentSize().width - 40, 170});
      optionsMenu->setLayout(RowLayout::create()
                                 ->setGap(6.f)
                                 ->setGrowCrossAxis(true)
                                 ->setCrossAxisOverflow(false));

      m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
      m_optionsMenu = optionsMenu;

      auto spinner = LoadingSpinner::create(60.f);
      spinner->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f});
      m_mainLayer->addChild(spinner);
      m_spinner = spinner;

      // create action buttons (set and remove) for each option
      auto makeActionButton = [this](const std::string& text, cocos2d::SEL_MenuHandler cb) -> std::pair<ButtonSprite*, CCMenuItemSpriteExtra*> {
            auto spr = ButtonSprite::create(text.c_str(), "goldFont.fnt", "GJ_button_01.png");
            if (spr) spr->updateBGImage("GJ_button_01.png");
            auto item = CCMenuItemSpriteExtra::create(spr, this, cb);
            return {spr, item};
      };

      // Exclude action button
      auto [excludeSpr, excludeBtn] = makeActionButton("Leaderboard Exclude", menu_selector(RLUserControl::onOptionAction));
      excludeBtn->setVisible(false);
      excludeBtn->setEnabled(false);
      optionsMenu->addChild(excludeBtn);
      m_userOptions["exclude"] = {excludeBtn, excludeSpr, false, false};

      // Blacklist action button
      auto [blackSpr, blackBtn] = makeActionButton("Report Blacklist", menu_selector(RLUserControl::onOptionAction));
      blackBtn->setVisible(false);
      blackBtn->setEnabled(false);
      optionsMenu->addChild(blackBtn);
      m_userOptions["blacklisted"] = {blackBtn, blackSpr, false, false};

      m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
      optionsMenu->updateLayout();
      m_mainLayer->addChild(optionsMenu);

      optionsMenu->updateLayout();

      // fetch profile data to determine initial excluded state
      if (m_targetAccountId > 0) {
            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = m_targetAccountId;

            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            m_profileTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");

            Ref<RLUserControl> self = this;
            m_profileTask.listen([self](web::WebResponse* response) {
                  if (!self) return;  // popup destroyed or otherwise invalid

                  if (!response->ok()) {
                        log::warn("Profile fetch returned non-ok status: {}", response->code());
                        return;
                  }

                  auto jsonRes = response->json();
                  if (!jsonRes) {
                        log::warn("Failed to parse JSON response for profile");
                        return;
                  }

                  // might refactor this later
                  auto json = jsonRes.unwrap();
                  bool isExcluded = json["excluded"].asBool().unwrapOrDefault();
                  bool isBlacklisted = json["blacklisted"].asBool().unwrapOrDefault();

                  self->m_isInitializing = true;
                  self->setOptionState("exclude", isExcluded, true);
                  self->setOptionState("blacklisted", isBlacklisted, true);
                  // show and enable buttons now that profile has been loaded
                  for (auto& kv : self->m_userOptions) {
                        auto& opt = kv.second;
                        if (opt.actionButton) {
                              self->m_spinner->setVisible(false);
                              opt.actionButton->setVisible(true);
                              opt.actionButton->setEnabled(true);
                        }
                  }
                  if (self->m_optionsMenu) self->m_optionsMenu->updateLayout();
                  self->m_isInitializing = false;
            });
      }

      return true;
}

void RLUserControl::onOptionAction(CCObject* sender) {
      if (m_isInitializing) return;

      auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!item) return;

      for (auto& kv : m_userOptions) {
            auto& key = kv.first;
            auto& opt = kv.second;
            if (opt.actionButton == item) {
                  // toggle the option (based on persisted state)
                  bool newDesired = !opt.persisted;
                  opt.desired = newDesired;
                  setOptionState(key, newDesired, false);
                  applySingleOption(key, newDesired);
                  break;
            }
      }
}

RLUserControl::OptionState* RLUserControl::getOptionByKey(const std::string& key) {
      auto it = m_userOptions.find(key);
      if (it == m_userOptions.end()) return nullptr;
      return &it->second;
}

void RLUserControl::setOptionState(const std::string& key, bool desired, bool updatePersisted) {
      auto opt = getOptionByKey(key);
      if (!opt) return;
      opt->desired = desired;
      if (updatePersisted) opt->persisted = desired;

      // update action button visuals and label depending on desired state
      if (opt->actionButton && opt->actionSprite) {
            std::string text;
            std::string bg;
            if (key == "exclude") {
                  if (desired) {
                        text = "Remove Leaderboard Exclude";
                        bg = "GJ_button_02.png";
                  } else {
                        text = "Set Leaderboard Exclude";
                        bg = "GJ_button_01.png";
                  }
            } else if (key == "blacklisted") {
                  if (desired) {
                        text = "Remove Report Blacklist";
                        bg = "GJ_button_02.png";
                  } else {
                        text = "Set Report Blacklist";
                        bg = "GJ_button_01.png";
                  }
            }

            // create new sprite and replace normal image so label/background update
            opt->actionSprite = ButtonSprite::create(text.c_str(), "goldFont.fnt", bg.c_str());
            opt->actionButton->setNormalImage(opt->actionSprite);
      }

      if (m_optionsMenu) {
            m_optionsMenu->updateLayout();
      }
}

void RLUserControl::setOptionEnabled(const std::string& key, bool enabled) {
      auto opt = getOptionByKey(key);
      if (!opt) return;
      if (opt->actionButton) opt->actionButton->setEnabled(enabled);
}

void RLUserControl::setAllOptionsEnabled(bool enabled) {
      for (auto& kv : m_userOptions) {
            auto& opt = kv.second;
            if (opt.actionButton) opt.actionButton->setEnabled(enabled);
      }
}

void RLUserControl::applySingleOption(const std::string& key, bool value) {
      auto opt = getOptionByKey(key);
      if (!opt) return;

      auto popup = UploadActionPopup::create(nullptr, fmt::format("Applying {}...", key));
      popup->show();
      Ref<UploadActionPopup> upopup = popup;

      // get token
      auto token = Mod::get()->getSavedValue<std::string>("argon_token");
      if (token.empty()) {
            upopup->showFailMessage("Authentication token not found");
            // revert visual to persisted
            setOptionState(key, opt->persisted, true);
            return;
      }

      // disable this option's button while applying and show center spinner
      setOptionEnabled(key, false);
      if (m_spinner) m_spinner->setVisible(true);

      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] = token;
      jsonBody["targetAccountId"] = m_targetAccountId;
      jsonBody[key] = value;

      log::info("Applying option {}={} for account {}", key, value ? "true" : "false", m_targetAccountId);

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      m_setUserTask = postReq.post("https://gdrate.arcticwoof.xyz/setUser");

      Ref<RLUserControl> self = this;
      m_setUserTask.listen([self, key, value, upopup](web::WebResponse* response) {
            if (!self || !upopup) return;
            // re-enable buttons
            self->setOptionEnabled(key, true);

            if (!response->ok()) {
                  log::warn("setUser returned non-ok status: {}", response->code());
                  upopup->showFailMessage("Failed to update user");
                  // revert visual to persisted
                  auto currentOpt = self->getOptionByKey(key);
                  if (currentOpt) {
                        self->m_isInitializing = true;
                        self->setOptionState(key, currentOpt->persisted, true);
                        self->m_isInitializing = false;
                  }
                  if (self->m_spinner) self->m_spinner->setVisible(false);
                  self->setOptionEnabled(key, true);
                  return;
            }

            auto jsonRes = response->json();
            if (!jsonRes) {
                  log::warn("Failed to parse setUser response");
                  upopup->showFailMessage("Invalid server response");
                  auto currentOpt = self->getOptionByKey(key);
                  if (currentOpt) {
                        self->m_isInitializing = true;
                        self->setOptionState(key, currentOpt->persisted, true);
                        self->m_isInitializing = false;
                  }
                  if (self->m_spinner) self->m_spinner->setVisible(false);
                  self->setOptionEnabled(key, true);
                  return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (success) {
                  auto currentOpt = self->getOptionByKey(key);
                  if (currentOpt) {
                        currentOpt->persisted = value;
                        currentOpt->desired = value;
                        self->m_isInitializing = true;
                        self->setOptionState(key, value, true);
                        self->m_isInitializing = false;
                  }
                  if (self->m_spinner) self->m_spinner->setVisible(false);
                  self->setOptionEnabled(key, true);
                  upopup->showSuccessMessage("User has been updated!");
            } else {
                  upopup->showFailMessage("Failed to update user");
                  auto currentOpt = self->getOptionByKey(key);
                  if (currentOpt) {
                        self->m_isInitializing = true;
                        self->setOptionState(key, currentOpt->persisted, true);
                        self->m_isInitializing = false;
                  }
                  if (self->m_spinner) self->m_spinner->setVisible(false);
                  self->setOptionEnabled(key, true);
            }
      });
}