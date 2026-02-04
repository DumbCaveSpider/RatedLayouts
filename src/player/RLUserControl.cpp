#include "RLUserControl.hpp"

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

const int DEV_ACCOUNT_ID = 7689052;

RLUserControl* RLUserControl::create() {
      auto ret = new RLUserControl();

      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }

      delete ret;
      return nullptr;
};

RLUserControl* RLUserControl::create(int accountId) {
      auto ret = new RLUserControl();
      ret->m_targetAccountId = accountId;

      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }

      delete ret;
      return nullptr;
};

bool RLUserControl::init() {
      if (!Popup::init(380.f, 240.f))
            return false;
      setTitle("Rated Layouts User Mod Panel");
      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

      std::string username = GameLevelManager::get()->tryGetUsername(m_targetAccountId);
      if (username.empty()) {
            username = "Unknown User";
      }
      // username label
      auto usernameLabel = CCLabelBMFont::create(("Target: " + username).c_str(), "bigFont.fnt", m_mainLayer->getContentSize().width - 40, kCCTextAlignmentCenter);
      usernameLabel->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 40});
      usernameLabel->setScale(0.7f);
      m_mainLayer->addChild(usernameLabel);

      auto optionsMenu = CCMenu::create();
      optionsMenu->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 - 15});
      optionsMenu->setContentSize({m_mainLayer->getContentSize().width - 40, 170});
      optionsMenu->setLayout(RowLayout::create()
                                 ->setGap(6.f)
                                 ->setGrowCrossAxis(true)
                                 ->setCrossAxisOverflow(false));

      m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
      m_optionsMenu = optionsMenu;

      auto spinner = LoadingSpinner::create(60.f);
      spinner->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f});
      m_mainLayer->addChild(spinner, 5);
      m_spinner = spinner;

      // create action buttons (set and remove) for each option
      auto makeActionButton = [this](const std::string& text, cocos2d::SEL_MenuHandler cb) -> std::pair<ButtonSprite*, CCMenuItemSpriteExtra*> {
            auto spr = ButtonSprite::create(text.c_str(), 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
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

      // Wipe User Data button
      auto [wipeSpr, wipeBtn] = makeActionButton("Wipe User Data", menu_selector(RLUserControl::onWipeAction));
      wipeBtn->setVisible(false);
      wipeBtn->setEnabled(false);
      optionsMenu->addChild(wipeBtn);
      m_wipeButton = wipeBtn;
      if (GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID) {
            wipeBtn->setVisible(true);
            wipeBtn->setEnabled(true);
      }

      // Promote/Demote buttons (dev-only)
      auto [promModSpr, promModBtn] = makeActionButton("Promote to Mod", menu_selector(RLUserControl::onPromoteAction));
      promModBtn->setVisible(false);
      promModBtn->setEnabled(false);
      optionsMenu->addChild(promModBtn);
      m_promoteModButton = promModBtn;

      auto [promAdminSpr, promAdminBtn] = makeActionButton("Promote to Admin", menu_selector(RLUserControl::onPromoteAction));
      promAdminBtn->setVisible(false);
      promAdminBtn->setEnabled(false);
      optionsMenu->addChild(promAdminBtn);
      m_promoteAdminButton = promAdminBtn;

      auto [demoteSpr, demoteBtn] = makeActionButton("Demote", menu_selector(RLUserControl::onPromoteAction));
      demoteBtn->setVisible(false);
      demoteBtn->setEnabled(false);
      optionsMenu->addChild(demoteBtn);
      m_demoteButton = demoteBtn;

      // If there's no profile to load (no target account), show dev buttons immediately
      if (m_targetAccountId <= 0 && GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID) {
            promModBtn->setVisible(true);
            promModBtn->setEnabled(true);
            promAdminBtn->setVisible(true);
            promAdminBtn->setEnabled(true);
            demoteBtn->setVisible(true);
            demoteBtn->setEnabled(true);
            wipeBtn->setVisible(true);
            wipeBtn->setEnabled(false);
      }

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
            Ref<RLUserControl> self = this;
            m_profileTask.spawn(
                postReq.post("https://gdrate.arcticwoof.xyz/profile"),
                [self](web::WebResponse response) {
                  if (!self) return;  // popup destroyed or otherwise invalid

                  // default values if any part of the fetch fails
                  bool isExcluded = false;
                  bool isBlacklisted = false;
                  int fetchedRole = 0;

                  if (!response.ok()) {
                        log::warn("Profile fetch returned non-ok status: {}", response.code());
                  } else {
                        auto jsonRes = response.json();
                        if (!jsonRes) {
                              log::warn("Failed to parse JSON response for profile");
                        } else {
                              auto json = jsonRes.unwrap();
                              isExcluded = json["excluded"].asBool().unwrapOrDefault();
                              isBlacklisted = json["blacklisted"].asBool().unwrapOrDefault();
                              fetchedRole = json["role"].asInt().unwrapOrDefault();
                        }
                  }

                  // Apply option states
                  self->m_isInitializing = true;
                  self->setOptionState("exclude", isExcluded, true);
                  self->setOptionState("blacklisted", isBlacklisted, true);

                  // show and enable option buttons now that profile loading has finished (successfully or not)
                  for (auto& kv : self->m_userOptions) {
                        auto& opt = kv.second;
                        if (opt.actionButton) {
                              if (self->m_spinner) self->m_spinner->setVisible(false);
                              opt.actionButton->setVisible(true);
                              opt.actionButton->setEnabled(true);
                        }
                  }

                  // Update wipe button appearance and enabled state based on exclude state
                  auto excludeOpt = self->getOptionByKey("exclude");
                  if (self->m_wipeButton) {
                        if (!excludeOpt || !excludeOpt->persisted) {
                              self->m_wipeButton->setNormalImage(
                                  ButtonSprite::create("Wipe User Data", 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                              self->m_wipeButton->setEnabled(false);
                        } else {
                              // restore normal active appearance
                              self->m_wipeButton->setNormalImage(
                                  ButtonSprite::create("Wipe User Data", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_wipeButton->setEnabled(true);
                        }
                  }

                  // show or remove dev-only buttons depending on current account
                  if (GJAccountManager::get()->m_accountID != DEV_ACCOUNT_ID) {
                        if (self->m_wipeButton) {
                              self->m_wipeButton->removeFromParentAndCleanup(true);
                              self->m_wipeButton = nullptr;
                        }
                        if (self->m_promoteModButton) {
                              self->m_promoteModButton->removeFromParentAndCleanup(true);
                              self->m_promoteModButton = nullptr;
                        }
                        if (self->m_promoteAdminButton) {
                              self->m_promoteAdminButton->removeFromParentAndCleanup(true);
                              self->m_promoteAdminButton = nullptr;
                        }
                        if (self->m_demoteButton) {
                              self->m_demoteButton->removeFromParentAndCleanup(true);
                              self->m_demoteButton = nullptr;
                        }
                  } else {
                        // developer: ensure buttons are visible and enabled now that loading finished
                        if (self->m_wipeButton) {
                              self->m_wipeButton->setVisible(true);
                              self->m_wipeButton->setEnabled(excludeOpt && excludeOpt->persisted);
                        }
                        if (self->m_promoteModButton) {
                              self->m_promoteModButton->setVisible(true);
                        }
                        if (self->m_promoteAdminButton) {
                              self->m_promoteAdminButton->setVisible(true);
                        }
                        if (self->m_demoteButton) {
                              self->m_demoteButton->setVisible(true);
                        }
                                          // update promote/demote button visibility based on fetched role
                  if (fetchedRole == 0) {
                        // normal user
                        if (self->m_promoteModButton) {
                              self->m_promoteModButton->setNormalImage(
                                  ButtonSprite::create("Promote to Mod", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_promoteModButton->setEnabled(true);
                        }
                        if (self->m_promoteAdminButton) {
                              self->m_promoteAdminButton->setNormalImage(
                                  ButtonSprite::create("Promote to Admin", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_promoteAdminButton->setEnabled(true);
                        }
                        if (self->m_demoteButton) {
                              self->m_demoteButton->setNormalImage(
                                  ButtonSprite::create("Demote", 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                              self->m_demoteButton->setEnabled(false);
                        }
                  } else if (fetchedRole == 1) {
                        // moderator
                        if (self->m_promoteModButton) {
                              self->m_promoteModButton->setNormalImage(
                                  ButtonSprite::create("Promote to Mod", 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                              self->m_promoteModButton->setEnabled(false);
                        }
                        if (self->m_promoteAdminButton) {
                              self->m_promoteAdminButton->setNormalImage(
                                  ButtonSprite::create("Promote to Admin", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_promoteAdminButton->setEnabled(true);
                        }
                        if (self->m_demoteButton) {
                              self->m_demoteButton->setNormalImage(
                                  ButtonSprite::create("Demote", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_demoteButton->setEnabled(true);
                        }
                  } else if (fetchedRole == 2) {
                        // admin
                        if (self->m_promoteModButton) {
                              self->m_promoteModButton->setNormalImage(
                                  ButtonSprite::create("Promote to Mod", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_promoteModButton->setEnabled(true);
                        }
                        if (self->m_promoteAdminButton) {
                              self->m_promoteAdminButton->setNormalImage(
                                  ButtonSprite::create("Promote to Admin", 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                              self->m_promoteAdminButton->setEnabled(false);
                        }
                        if (self->m_demoteButton) {
                              self->m_demoteButton->setNormalImage(
                                  ButtonSprite::create("Demote", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                              self->m_demoteButton->setEnabled(true);
                        }
                  }
                  }

                  

                  if (self->m_optionsMenu) self->m_optionsMenu->updateLayout();
                  self->m_isInitializing = false;
            });
      }

      return true;
}
void RLUserControl::onWipeAction(CCObject* sender) {
      if (m_isInitializing) return;
      if (!m_wipeButton) return;

      auto excludeOpt = getOptionByKey("exclude");
      if (!excludeOpt || !excludeOpt->persisted) {
            FLAlertLayer::create(
                "Cannot Wipe User",
                "You can only wipe users who are <cr>excluded from the leaderboard</c>.",
                "OK")
                ->show();
            return;
      }

      // Confirm wipe
      std::string title = fmt::format("Wipe user {}?", m_targetAccountId);
      std::string body = fmt::format("Are you sure you want to <cr>wipe</c> the data for user <cg>{}</c>?\n<cy>This action is irreversible.</c>", m_targetAccountId);
      geode::createQuickPopup(
          title.c_str(),
          body.c_str(),
          "No",
          "Wipe",
          [this](auto, bool yes) {
                if (!yes) return;

                auto popup = UploadActionPopup::create(nullptr, "Wiping user data...");
                popup->show();
                Ref<UploadActionPopup> upopup = popup;

                // Get token
                auto token = Mod::get()->getSavedValue<std::string>("argon_token");
                if (token.empty()) {
                      upopup->showFailMessage("Authentication token not found");
                      return;
                }

                // disable UI and show spinner
                this->setAllOptionsEnabled(false);
                if (this->m_wipeButton) this->m_wipeButton->setEnabled(false);
                if (this->m_spinner) this->m_spinner->setVisible(true);

                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                jsonBody["argonToken"] = token;
                jsonBody["targetAccountId"] = this->m_targetAccountId;

                log::info("Sending deleteUser request: {}", jsonBody.dump());

                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                Ref<RLUserControl> self = this;
                self->m_deleteUserTask.spawn(
                    postReq.post("https://gdrate.arcticwoof.xyz/deleteUser"),
                    [self, upopup](web::WebResponse response) {
                          if (!self || !upopup) return;

                          // re-enable UI
                          self->setAllOptionsEnabled(true);
                          if (self->m_wipeButton) self->m_wipeButton->setEnabled(true);

                          if (!response.ok()) {
                                log::warn("deleteUser returned non-ok status: {}", response.code());
                                upopup->showFailMessage("Failed to delete user");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                return;
                          }

                          auto jsonRes = response.json();
                          if (!jsonRes) {
                                log::warn("Failed to parse deleteUser response");
                                upopup->showFailMessage("Invalid server response");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                return;
                          }

                          auto json = jsonRes.unwrap();
                          bool success = json["success"].asBool().unwrapOrDefault();
                          if (success) {
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                upopup->showSuccessMessage("User data wiped!");
                          } else {
                                upopup->showFailMessage("Failed to delete user");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                          }
                    });
          });
}

void RLUserControl::onPromoteAction(CCObject* sender) {
      if (m_isInitializing) return;

      if (GJAccountManager::get()->m_accountID != DEV_ACCOUNT_ID) {
            FLAlertLayer::create(
                "Access Denied",
                "You do not have permission to perform this action.",
                "OK")
                ->show();
            return;
      }

      int role = -1;  // 1 == Mod, 2 == Admin, 0 == Demote
      std::string actionText = "";
      std::string confirmText = "";
      std::string titleText = "";

      if (sender == m_promoteModButton) {
            role = 1;
            actionText = "Promoting to Mod...";
            confirmText = "promote this user to Mod";
            titleText = "Promote User?";
      } else if (sender == m_promoteAdminButton) {
            role = 2;
            actionText = "Promoting to Admin...";
            confirmText = "promote this user to Admin";
            titleText = "Promote User?";
      } else if (sender == m_demoteButton) {
            role = 0;
            actionText = "Demoting user...";
            confirmText = "demote this user";
            titleText = "Demote User?";
      } else {
            return;  // unknown sender
      }

      createQuickPopup(
          titleText.c_str(),
          ("Are you sure you want to <cg>" + confirmText + "</c>?").c_str(),
          "Cancel",
          "Confirm",
          [this, sender, role, actionText](auto, bool yes) {
                if (!yes) return;

                auto popup = UploadActionPopup::create(nullptr, actionText.c_str());
                popup->show();
                Ref<UploadActionPopup> upopup = popup;

                // Get token
                auto token = Mod::get()->getSavedValue<std::string>("argon_token");
                if (token.empty()) {
                      upopup->showFailMessage("Authentication token not found");
                      return;
                }

                // disable UI and show spinner
                this->setAllOptionsEnabled(false);
                if (this->m_promoteModButton) this->m_promoteModButton->setEnabled(false);
                if (this->m_promoteAdminButton) this->m_promoteAdminButton->setEnabled(false);
                if (this->m_demoteButton) this->m_demoteButton->setEnabled(false);
                if (this->m_wipeButton) this->m_wipeButton->setEnabled(false);
                if (this->m_spinner) this->m_spinner->setVisible(true);

                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                jsonBody["argonToken"] = token;
                jsonBody["targetAccountId"] = this->m_targetAccountId;
                jsonBody["role"] = role;

                log::info("Sending promoteUser request: {}", jsonBody.dump());

                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                Ref<RLUserControl> self = this;
                self->m_promoteUserTask.spawn(
                    postReq.post("https://gdrate.arcticwoof.xyz/promoteUser"),
                    [self, upopup, role](web::WebResponse response) {
                          if (!self || !upopup) return;

                          // re-enable UI
                          self->setAllOptionsEnabled(true);
                          if (self->m_promoteModButton) self->m_promoteModButton->setEnabled(true);
                          if (self->m_promoteAdminButton) self->m_promoteAdminButton->setEnabled(true);
                          if (self->m_demoteButton) self->m_demoteButton->setEnabled(true);
                          if (self->m_wipeButton) self->m_wipeButton->setEnabled(true);

                          if (!response.ok()) {
                                log::warn("promoteUser returned non-ok status: {}", response.code());
                                upopup->showFailMessage("Failed to update user role");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                return;
                          }

                          auto jsonRes = response.json();
                          if (!jsonRes) {
                                log::warn("Failed to parse promoteUser response");
                                upopup->showFailMessage("Invalid server response");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                return;
                          }

                          auto json = jsonRes.unwrap();
                          bool success = json["success"].asBool().unwrapOrDefault();
                          if (success) {
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                                if (role == 1)
                                      upopup->showSuccessMessage("User promoted to Mod!");
                                else if (role == 2)
                                      upopup->showSuccessMessage("User promoted to Admin!");
                                else
                                      upopup->showSuccessMessage("User demoted!");
                          } else {
                                upopup->showFailMessage("Failed to update user role");
                                if (self->m_spinner) self->m_spinner->setVisible(false);
                          }
                    });
          });
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
            opt->actionSprite = ButtonSprite::create(text.c_str(), 200, true, "goldFont.fnt", bg.c_str(), 30.f, 1.f);
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
      Ref<RLUserControl> self = this;
      m_setUserTask.spawn(
          postReq.post("https://gdrate.arcticwoof.xyz/setUser"),
          [self, key, value, upopup](web::WebResponse response) {
                if (!self || !upopup) return;
                // re-enable buttons
                self->setOptionEnabled(key, true);

                if (!response.ok()) {
                      log::warn("setUser returned non-ok status: {}", response.code());
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

                auto jsonRes = response.json();
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