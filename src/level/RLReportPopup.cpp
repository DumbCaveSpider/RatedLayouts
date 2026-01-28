#include "RLReportPopup.hpp"
#include "../custom/RLAchievements.hpp"

RLReportPopup* RLReportPopup::create(int levelId) {
      RLReportPopup* popup = new RLReportPopup();
      // @geode-ignore(unknown-resource)
      if (popup && popup->initAnchored(440.f, 280.f, "geode.loader/GE_square01.png")) {
            popup->m_levelId = levelId;
            popup->autorelease();
            return popup;
      }
      delete popup;
      return nullptr;
}

bool RLReportPopup::setup() {
      setTitle("Rated Layouts Report Level");
      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);

      // report
      auto reportButtonSpr = ButtonSprite::create(
          "Report Level",
          "goldFont.fnt",
          "GJ_button_06.png");

      auto reportButton = CCMenuItemSpriteExtra::create(
          reportButtonSpr,
          this,
          menu_selector(RLReportPopup::onSubmit));

      m_buttonMenu->addChild(reportButton);
      reportButton->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, 25.f});

      // info button
      auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(RLReportPopup::onInfo));
      infoBtn->setPosition({m_mainLayer->getScaledContentSize().width - 25, m_mainLayer->getScaledContentSize().height - 25});
      m_buttonMenu->addChild(infoBtn);

      auto toggleMenu = CCMenu::create();
      auto toggleLayout = RowLayout::create();
      toggleLayout->setGap(8.f)->setGrowCrossAxis(true)->setCrossAxisOverflow(false)->setAxisAlignment(AxisAlignment::Center);
      toggleMenu->setLayout(toggleLayout);
      toggleMenu->setPosition({0, 0});

      // helper to create a toggler with two visual states
      auto makeToggle = [this](const std::string& text) -> CCMenuItemToggler* {
            auto offSpr = ButtonSprite::create(text.c_str(), "goldFont.fnt", "GJ_button_01.png");
            auto onSpr = ButtonSprite::create(text.c_str(), "goldFont.fnt", "GJ_button_02.png");
            auto togg = CCMenuItemToggler::create(offSpr, onSpr, this, nullptr);
            return togg;
      };

      m_plagiarismToggle = makeToggle("Plagiarism");
      m_secretWayToggle = makeToggle("Secret Way");
      m_lowEffortToggle = makeToggle("Low Effort");
      m_unverifiedToggle = makeToggle("Unverified");
      m_nsfwContentToggle = makeToggle("NSFW Content");
      m_misrateToggle = makeToggle("Misrate");
      m_decoratedToggle = makeToggle("Decorated");
      m_otherToggle = makeToggle("Other");

      toggleMenu->addChild(m_plagiarismToggle);
      toggleMenu->addChild(m_secretWayToggle);
      toggleMenu->addChild(m_lowEffortToggle);
      toggleMenu->addChild(m_unverifiedToggle);
      toggleMenu->addChild(m_nsfwContentToggle);
      toggleMenu->addChild(m_misrateToggle);
      toggleMenu->addChild(m_decoratedToggle);
      toggleMenu->addChild(m_otherToggle);

      // layout/position the toggle menu
      float menuY = m_mainLayer->getContentSize().height / 2.f + 30.f;
      toggleMenu->setContentSize({m_mainLayer->getScaledContentSize().width - 40.f, 150.f});
      toggleMenu->setAnchorPoint({0.5f, 0.5f});
      toggleMenu->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, menuY});
      m_mainLayer->addChild(toggleMenu);
      toggleMenu->updateLayout();
      m_toggleMenu = toggleMenu;

      // reasons
      m_reasonInput = TextInput::create(m_mainLayer->getScaledContentSize().width - 80.f, "Reasons/Additional Information", "chatFont.fnt");
      m_reasonInput->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, 70.f});
      m_reasonInput->setCommonFilter(CommonFilter::Any);
      m_reasonInput->setID("report-reason-input");
      m_mainLayer->addChild(m_reasonInput);

      return true;
}

void RLReportPopup::onSubmit(CCObject* sender) {
      // Require at least one toggle selected
      bool anyToggleSelected = false;
      if ((m_plagiarismToggle && m_plagiarismToggle->isToggled()) ||
          (m_secretWayToggle && m_secretWayToggle->isToggled()) ||
          (m_lowEffortToggle && m_lowEffortToggle->isToggled()) ||
          (m_unverifiedToggle && m_unverifiedToggle->isToggled()) ||
          (m_nsfwContentToggle && m_nsfwContentToggle->isToggled()) ||
          (m_misrateToggle && m_misrateToggle->isToggled()) ||
          (m_decoratedToggle && m_decoratedToggle->isToggled()) ||
          (m_otherToggle && m_otherToggle->isToggled())) {
            anyToggleSelected = true;
      }

      if (!anyToggleSelected) {
            auto uploadPopup = UploadActionPopup::create(nullptr, "Sending Report...");
            uploadPopup->show();
            uploadPopup->showFailMessage("Select at least one report reason");
            return;
      }

      createQuickPopup(
          "Report Level",
          "Are you sure you want to report this level?\n<cy>Layout moderators/admins will review your report and take appropriate action.</c>\n<cr>False reports will lead to your account being blacklisted from reporting layouts.</c>",
          "Cancel", "Report",
          [this](auto, bool yes) {
                if (!yes) return;
                auto uploadPopup = UploadActionPopup::create(nullptr, "Sending Report...");
                uploadPopup->show();
                int accountId = GJAccountManager::get()->m_accountID;
                auto argonToken = Mod::get()->getSavedValue<std::string>("argon_token");
                if (argonToken.empty()) {
                      uploadPopup->showFailMessage("Argon Auth is missing/invalid");
                      return;
                }

                bool anyReason = false;
                matjson::Value body = matjson::Value::object();
                body["accountId"] = accountId;
                body["argonToken"] = argonToken;
                body["levelId"] = m_levelId;

                if (m_plagiarismToggle && m_plagiarismToggle->isToggled()) {
                      body["plagiarism"] = true;
                      anyReason = true;
                } else {
                      body["plagiarism"] = false;
                }
                if (m_secretWayToggle && m_secretWayToggle->isToggled()) {
                      body["secretWay"] = true;
                      anyReason = true;
                } else {
                      body["secretWay"] = false;
                }
                if (m_lowEffortToggle && m_lowEffortToggle->isToggled()) {
                      body["lowEffort"] = true;
                      anyReason = true;
                } else {
                      body["lowEffort"] = false;
                }
                if (m_unverifiedToggle && m_unverifiedToggle->isToggled()) {
                      body["unverified"] = true;
                      anyReason = true;
                } else {
                      body["unverified"] = false;
                }
                if (m_nsfwContentToggle && m_nsfwContentToggle->isToggled()) {
                      body["nsfwContent"] = true;
                      anyReason = true;
                } else {
                      body["nsfwContent"] = false;
                }
                if (m_misrateToggle && m_misrateToggle->isToggled()) {
                      body["misrate"] = true;
                      anyReason = true;
                } else {
                      body["misrate"] = false;
                }

                if (m_decoratedToggle && m_decoratedToggle->isToggled()) {
                      body["decorated"] = true;
                      anyReason = true;
                } else {
                      body["decorated"] = false;
                }
                if (m_otherToggle && m_otherToggle->isToggled()) {
                      body["other"] = true;
                      anyReason = true;
                } else {
                      body["other"] = false;
                }

                if (m_reasonInput) {
                      auto s = m_reasonInput->getString();
                      if (!s.empty()) {
                            body["reason"] = s;
                            anyReason = true;
                      }
                }

                if (!anyReason) {
                      uploadPopup->showFailMessage("No report reason provided");
                      return;
                }

                auto req = web::WebRequest();
                req.bodyJSON(body);
                Ref<RLReportPopup> self = this;
                self->m_reportTask = req.post("https://gdrate.arcticwoof.xyz/setReport");
                self->m_reportTask.listen([self, uploadPopup](web::WebResponse* res) {
                      if (!self) return;
                      if (!res || !res->ok()) {
                            uploadPopup->showFailMessage("Failed to submit report");
                            return;
                      }
                      auto j = res->json();
                      if (!j) {
                            uploadPopup->showFailMessage("Invalid server response");
                            return;
                      }
                      auto json = j.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();
                      if (success) {
                            uploadPopup->showSuccessMessage("Report submitted!");
                            RLAchievements::onReward("misc_report"); // first time report
                            self->removeFromParent();
                            // disable inputs
                            if (self->m_reasonInput) {
                                  self->m_reasonInput->setString("");
                                  self->m_reasonInput->setEnabled(false);
                            }
                            if (self->m_plagiarismToggle) self->m_plagiarismToggle->setEnabled(false);
                            if (self->m_secretWayToggle) self->m_secretWayToggle->setEnabled(false);
                            if (self->m_lowEffortToggle) self->m_lowEffortToggle->setEnabled(false);
                            if (self->m_unverifiedToggle) self->m_unverifiedToggle->setEnabled(false);
                            if (self->m_nsfwContentToggle) self->m_nsfwContentToggle->setEnabled(false);
                            if (self->m_misrateToggle) self->m_misrateToggle->setEnabled(false);
                            if (self->m_decoratedToggle) self->m_decoratedToggle->setEnabled(false);
                            if (self->m_otherToggle) self->m_otherToggle->setEnabled(false);
                      } else {
                            uploadPopup->showFailMessage("Failed to submit report");
                      }
                });
          });
}

void RLReportPopup::onInfo(CCObject* sender) {
      MDPopup::create(
          "Reporting Rated Layouts",
          "Use the toggles to select the reason(s) for <cr>reporting</c> this Rated Layout. You can select <cg>multiple reasons if applicable</c>.\n\n"
          "To provide additional context, you can provide details in the <cy>'Reasons'</c> input field.\n\n"
          "Once you submit the report, <cl>Layout Moderators</c> or <cr>Layout Admins</c> will review it and take <cg>appropriate action</c> based on our Rated Layouts guidelines.\n\n"
          "List of report reasons:\n"
          "- <cy>**Plagiarism:**</c> The layout is either stolen or directly copied from a decorated or officially rated level *(e.g., removing the decorations from an existing level and claiming it as your own)*.\n"
          "- <cg>**Secret Way:**</c> The layout contains secret ways or 'swag routes' that allow players to beat the level without actually playing through the intended path.\n"
          "- <co>**Low Effort:**</c> The layout shows minimal effort or a lack of creativity regarding gameplay.\n"
          "- <cl>**Unverified:**</c> The layout has not been beaten legitimately or has not been properly verified.\n"
          "- <cr>**NSFW Content:**</c> The layout contains NSFW *(explicit/suggestive)*, sensitive *(political/religious/controversial)*, or harassing imagery and text.\n"
          "- <cp>**Misrate:**</c> The level was rated incorrectly, such as having the wrong difficulty or rating category assigned.\n"
          "- <cf>**Decorated:**</c> The layout has been heavily or partially decorated to the point where it is no longer a 'layout' and could potentially be officially rated in-game.\n"
          "- <ca>**Other:**</c> The report reason does not fit into any of the above categories and requires additional context.",
          "OK")
          ->show();
}