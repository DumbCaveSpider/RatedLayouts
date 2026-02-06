#include "RLModRatePopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// another helper lol set the visual of a CCMenuItemToggler to grayscale or normal
static void setTogglerGrayscale(CCMenuItemToggler* toggler, const char* spriteName, bool /*unused*/) {
      if (!toggler) return;
      // Use toggler state to decide whether the normal image should be grayscale
      bool toggled = toggler->isToggled();
      auto normalSpr = toggled ? CCSprite::createWithSpriteFrameName(spriteName) : CCSpriteGrayscale::create(spriteName);
      auto selectedSpr = CCSprite::createWithSpriteFrameName(spriteName);
      if (auto spriteItem = typeinfo_cast<CCMenuItemSpriteExtra*>(toggler)) {
            if (normalSpr) spriteItem->setNormalImage(normalSpr);
            if (selectedSpr) spriteItem->setSelectedImage(selectedSpr);
      }
}

bool RLModRatePopup::init() {
      if (!Popup::init(380.f, 180.f, "GJ_square02.png")) return false;
      
      m_difficultySprite = nullptr;
      m_isDemonMode = false;
      m_isFeatured = false;
      m_isEpicFeatured = false;
      m_isLegendary = false;
      m_selectedRating = -1;
      m_isRejected = false;
      m_levelId = -1;
      m_accountId = 0;
      m_difficultyInput = nullptr;
      m_featuredValueInput = nullptr;
      m_verifiedToggleItem = nullptr;
      m_legendaryToggleItem = nullptr;

      // get the level ID ya
      if (m_level) {
            m_levelId = m_level->m_levelID;
            m_accountId = m_level->m_accountID;
      }

      // title
      auto titleLabel = CCLabelBMFont::create(m_title.c_str(), "bigFont.fnt");
      titleLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                               m_mainLayer->getContentSize().height - 20});
      m_mainLayer->addChild(titleLabel);

      // normal and demon buttons
      m_normalButtonsContainer = CCMenu::create();
      m_normalButtonsContainer->setPosition({0, 0});
      m_demonButtonsContainer = CCMenu::create();
      m_demonButtonsContainer->setPosition({0, 0});

      if (m_role != PopupRole::Dev) {
            // demon buttons only (ratings 10, 15, 20, 25, 30)
            float startX = 50.f;
            float buttonSpacing = 55.f;
            float firstRowY = 110.f;

            // normal difficulty buttons (1-9)
            for (int i = 1; i <= 9; i++) {
                  auto buttonBg = CCSprite::create("GJ_button_04.png");
                  auto buttonLabel =
                      CCLabelBMFont::create(numToString(i).c_str(), "bigFont.fnt");
                  buttonLabel->setScale(0.75f);
                  buttonLabel->setPosition(buttonBg->getContentSize() / 2);
                  buttonBg->addChild(buttonLabel);
                  buttonBg->setID("button-bg-" + numToString(i));

                  auto ratingButtonItem = CCMenuItemSpriteExtra::create(
                      buttonBg, this, menu_selector(RLModRatePopup::onRatingButton));

                  if (i <= 5) {
                        ratingButtonItem->setPosition(
                            {startX + (i - 1) * buttonSpacing, firstRowY});
                  } else {
                        ratingButtonItem->setPosition(
                            {startX + (i - 6) * buttonSpacing, firstRowY - 55.f});
                  }
                  ratingButtonItem->setTag(i);
                  ratingButtonItem->setID("rating-button-" + numToString(i));
                  m_normalButtonsContainer->addChild(ratingButtonItem);
            }

            // demon difficulty buttons (10, 15, 20, 25, 30)
            std::vector<int> demonRatings = {10, 15, 20, 25, 30};

            for (int idx = 0; idx < demonRatings.size(); idx++) {
                  int rating = demonRatings[idx];
                  auto buttonBg = CCSprite::create("GJ_button_04.png");
                  auto buttonLabel =
                      CCLabelBMFont::create(numToString(rating).c_str(), "bigFont.fnt");
                  buttonLabel->setScale(0.75f);
                  buttonLabel->setPosition(buttonBg->getContentSize() / 2);
                  buttonBg->addChild(buttonLabel);
                  buttonBg->setID("button-bg-" + numToString(rating));

                  auto ratingButtonItem = CCMenuItemSpriteExtra::create(
                      buttonBg, this, menu_selector(RLModRatePopup::onRatingButton));

                  ratingButtonItem->setPosition({startX + idx * buttonSpacing, firstRowY});
                  ratingButtonItem->setTag(rating);
                  ratingButtonItem->setID("rating-button-" + numToString(rating));
                  m_demonButtonsContainer->addChild(ratingButtonItem);
            }

            // add reject
            {
                  auto rejectBg = CCSprite::create("GJ_button_06.png");
                  auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                  rejectLabel->setScale(0.75f);
                  rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                  rejectBg->addChild(rejectLabel);
                  rejectBg->setID("button-bg-reject");

                  auto rejectButton = CCMenuItemSpriteExtra::create(rejectBg, this, menu_selector(RLModRatePopup::onRejectButton));
                  // place to the right of the 9 button
                  rejectButton->setPosition({startX + 4 * buttonSpacing, firstRowY - 55.f});
                  rejectButton->setTag(-2);
                  rejectButton->setID("rating-button-reject");
                  m_normalButtonsContainer->addChild(rejectButton);
            }
      } else {
            // hide both containers for Dev
            m_normalButtonsContainer->setVisible(false);
            m_demonButtonsContainer->setVisible(false);
      }

      m_buttonMenu->addChild(m_normalButtonsContainer);
      m_buttonMenu->addChild(m_demonButtonsContainer);
      m_demonButtonsContainer->setVisible(false);

      // demon toggle
      if (m_role != PopupRole::Dev) {
            auto offDemonSprite =
                CCSpriteGrayscale::createWithSpriteFrameName("GJ_demonIcon_001.png");
            auto onDemonSprite =
                CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
            auto demonToggle =
                CCMenuItemToggler::create(offDemonSprite, onDemonSprite, this,
                                          menu_selector(RLModRatePopup::onToggleDemon));

            if (demonToggle) {
                  demonToggle->setPosition({m_mainLayer->getContentSize().width, 0});
                  demonToggle->setScale(1.2f);
                  m_buttonMenu->addChild(demonToggle);
            }
      }

      // info button
      auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
      infoSpr->setScale(0.7f);
      auto infoButton = CCMenuItemSpriteExtra::create(infoSpr, this,
                                                      menu_selector(RLModRatePopup::onInfoButton));
      if (infoButton) {
            infoButton->setPosition({m_mainLayer->getContentSize().width,
                                     m_mainLayer->getContentSize().height});
            m_buttonMenu->addChild(infoButton);
      }

      // submit button
      int userRole = (m_role == PopupRole::Admin || m_role == PopupRole::Dev) ? 2 : ((m_role == PopupRole::Mod) ? 1 : 0);
      float centerX = m_mainLayer->getContentSize().width / 2;
      auto modActionMenu = CCMenu::create();  // for the bottom menu buttons
      modActionMenu->setPosition({centerX, 0});
      modActionMenu->setID("mod-action-menu");
      modActionMenu->setContentSize({m_mainLayer->getContentSize().width - 40.f, 30.f});
      modActionMenu->setLayout(
          RowLayout::create()
              ->setGap(15.f));

      auto submitButtonSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
      auto submitButtonItem = CCMenuItemSpriteExtra::create(
          submitButtonSpr, this, menu_selector(RLModRatePopup::onSubmitButton));

      modActionMenu->addChild(submitButtonItem);
      m_submitButtonItem = submitButtonItem;

      // unsend button
      auto unsendSpr = ButtonSprite::create("Unsend", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
      auto unsendButtonItem = CCMenuItemSpriteExtra::create(
          unsendSpr, this, menu_selector(RLModRatePopup::onUnsendButton));
      unsendButtonItem->setID("unsend-button");
      modActionMenu->addChild(unsendButtonItem);

      // unrate and suggest buttons (only for admins)
      if (userRole == 2) {
            auto unrateSpr = ButtonSprite::create("Unrate", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
            auto unrateButtonItem = CCMenuItemSpriteExtra::create(
                unrateSpr, this, menu_selector(RLModRatePopup::onUnrateButton));

            unrateButtonItem->setPosition({centerX, 0});
            modActionMenu->addChild(unrateButtonItem);

            // suggest button for admin
            auto suggestSpr = ButtonSprite::create("Suggest", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
            auto suggestButtonItem = CCMenuItemSpriteExtra::create(
                suggestSpr, this, menu_selector(RLModRatePopup::onSuggestButton));
            suggestButtonItem->setID("suggest-button");
            modActionMenu->addChild(suggestButtonItem);
      }

      m_mainLayer->addChild(modActionMenu);
      modActionMenu->updateLayout();

      // featured / epic toggles
      if (m_role != PopupRole::Dev) {
            auto offSprite = CCSpriteGrayscale::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
            auto onSprite = CCSprite::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
            auto toggleFeatured = CCMenuItemToggler::create(
                offSprite, onSprite, this, menu_selector(RLModRatePopup::onToggleFeatured));
            m_featuredToggleItem = toggleFeatured;

            toggleFeatured->setPosition({0, -10});
            m_buttonMenu->addChild(toggleFeatured);

            if (m_role == PopupRole::Admin) {
                  auto offVerifiedSprite = CCSpriteGrayscale::createWithSpriteFrameName("RL_BlueCoinSmall.png"_spr);
                  auto onVerifiedSprite = CCSprite::createWithSpriteFrameName("RL_BlueCoinSmall.png"_spr);
                  auto toggleVerified = CCMenuItemToggler::create(
                      offVerifiedSprite, onVerifiedSprite, this, nullptr);
                  m_verifiedToggleItem = toggleVerified;
                  toggleVerified->setPosition({m_mainLayer->getContentSize().width, m_mainLayer->getContentSize().height - 30});
                  m_buttonMenu->addChild(toggleVerified);
            }

            // legendary toggle
            auto offLegendarySprite = CCSpriteGrayscale::createWithSpriteFrameName("RL_legendaryFeaturedCoin.png"_spr);
            auto onLegendarySprite = CCSprite::createWithSpriteFrameName("RL_legendaryFeaturedCoin.png"_spr);
            auto toggleLegendary = CCMenuItemToggler::create(
                offLegendarySprite, onLegendarySprite, this, menu_selector(RLModRatePopup::onToggleLegendary));
            m_legendaryToggleItem = toggleLegendary;
            toggleLegendary->setPosition({0, 120});
            m_buttonMenu->addChild(toggleLegendary);

            auto offEpicSprite = CCSpriteGrayscale::createWithSpriteFrameName("RL_epicFeaturedCoin.png"_spr);
            auto onEpicSprite = CCSprite::createWithSpriteFrameName("RL_epicFeaturedCoin.png"_spr);
            auto toggleEpicFeatured = CCMenuItemToggler::create(
                offEpicSprite, onEpicSprite, this, menu_selector(RLModRatePopup::onToggleEpicFeatured));
            m_epicFeaturedToggleItem = toggleEpicFeatured;

            toggleEpicFeatured->setPosition({0, 50});
            m_buttonMenu->addChild(toggleEpicFeatured);
      } else {
            // For Dev provide a TextInput to set the featured value directly
            m_featuredValueInput = TextInput::create(100.f, "Featured");
            m_featuredValueInput->setPosition({m_mainLayer->getContentSize().width / 2 + 30.f, 50.f});
            m_featuredValueInput->setID("featured-value-input");
            m_buttonMenu->addChild(m_featuredValueInput);
      }

      // Dev-only toggles
      if (m_role == PopupRole::Dev) {
            auto offSilent = ButtonSprite::create("Silent", 80, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            auto onSilent = ButtonSprite::create("Silent", 80, true, "goldFont.fnt", "GJ_button_02.png", 30.f, 1.f);
            auto silentToggle = CCMenuItemToggler::create(offSilent, onSilent, this, nullptr);
            if (silentToggle) {
                  silentToggle->setPosition({80, 70});
                  silentToggle->setID("silent-toggle");
                  m_buttonMenu->addChild(silentToggle);
                  m_silentToggleItem = silentToggle;
            }

            auto coinVerified = ButtonSprite::create("Verified", 80, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            auto coinVerifiedOn = ButtonSprite::create("Verified", 80, true, "goldFont.fnt", "GJ_button_02.png", 30.f, 1.f);
            auto verifiedToggle = CCMenuItemToggler::create(coinVerified, coinVerifiedOn, this, nullptr);
            if (verifiedToggle) {
                  verifiedToggle->setPosition({m_mainLayer->getContentSize().width / 2, 70});
                  verifiedToggle->setID("verified-toggle");
                  m_buttonMenu->addChild(verifiedToggle);
                  m_verifiedToggleItem = verifiedToggle;
            }

            auto deleteSendSpr = ButtonSprite::create("Delete Sends", 80, true, "goldFont.fnt", "GJ_button_06.png", 30.f, 1.f);
            auto deleteSendBtn = CCMenuItemSpriteExtra::create(deleteSendSpr, this, menu_selector(RLModRatePopup::onDeleteSendsButton));
            if (deleteSendBtn) {
                  deleteSendBtn->setPosition({m_mainLayer->getContentSize().width - 80.f, 70});
                  deleteSendBtn->setID("delete-sends-button");
                  m_buttonMenu->addChild(deleteSendBtn);
            }
      }

      // rating buttons
      if (m_role == PopupRole::Dev) {
            m_difficultyInput = TextInput::create(100.f, "Difficulty");
            m_difficultyInput->setPosition({m_mainLayer->getContentSize().width / 2 - 110.f, 120.f});
            m_difficultyInput->setID("dev-difficulty-input");
            m_mainLayer->addChild(m_difficultyInput);
      }

      // difficulty container on the right side
      m_difficultyContainer = CCNode::create();
      m_difficultyContainer->setPosition(
          {m_mainLayer->getContentSize().width - 50.f, 90.f});
      m_mainLayer->addChild(m_difficultyContainer);

      // For non-Dev users, create the default difficulty sprite
      if (m_role != PopupRole::Dev) {
            m_difficultySprite = GJDifficultySprite::create(0, (GJDifficultyName)-1);
            m_difficultySprite->setPosition({0, 0});
            m_difficultySprite->setScale(1.2f);
            m_difficultyContainer->addChild(m_difficultySprite);
      }

      // Admin-only event buttons: daily / weekly / monthly
      if (userRole == 2) {
            // positions near the right side, stacked vertically
            float eventX = m_mainLayer->getContentSize().width;
            float eventYStart = 120.f;
            float eventSpacing = 38.f;
            std::vector<std::tuple<std::string, std::string, std::string>> events = {// is tuple like two vectors or three vectors
                                                                                     {"event-daily", "daily", "D"},
                                                                                     {"event-weekly", "weekly", "W"},
                                                                                     {"event-monthly", "monthly", "M"}};

            for (size_t i = 0; i < events.size(); ++i) {
                  auto btnSpr = ButtonSprite::create(std::get<2>(events[i]).c_str(), 20, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                  auto btnItem = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(RLModRatePopup::onSetEventButton));
                  btnItem->setPosition({eventX, eventYStart - (float)i * eventSpacing});
                  btnItem->setID(std::get<0>(events[i]));
                  m_buttonMenu->addChild(btnItem);
            }
      }

      // featured score textbox (created conditionally based on role)
      m_featuredScoreInput = TextInput::create(100.f, "Score");
      m_featuredScoreInput->setPosition({300.f, 40.f});
      m_featuredScoreInput->setVisible(false);
      m_featuredScoreInput->setID("featured-score-input");
      m_mainLayer->addChild(m_featuredScoreInput);

      // For Dev, show the featured score next to the difficulty input
      if (m_role == PopupRole::Dev && m_featuredScoreInput && m_difficultyInput) {
            m_featuredScoreInput->setVisible(true);
            m_featuredScoreInput->setPosition({m_mainLayer->getContentSize().width / 2 + 110.f, 120.f});
      }

      // Position featured value input (if present) near the difficulty input for Dev users
      if (m_role == PopupRole::Dev && m_featuredValueInput) {
            m_featuredValueInput->setPosition({m_mainLayer->getContentSize().width / 2, 120.f});
      }

      return true;
}

void RLModRatePopup::onInfoButton(CCObject* sender) {
      // matjson payload

      // disable the button to prevent multiple clicks
      if (auto buttonItem = typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) {
            buttonItem->setEnabled(false);
      }
      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] =
          Mod::get()->getSavedValue<std::string>("argon_token");
      jsonBody["levelId"] = m_levelId;

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);

      Ref<RLModRatePopup> self = this;
      m_getModLevelTask.spawn(
          postReq.post("https://gdrate.arcticwoof.xyz/getModLevel"),
          [self, sender](web::WebResponse response) {
            if (!self) return;
            log::info("Received response from server");

            if (!response.ok()) {
                  log::warn("Server returned non-ok status: {}", response.code());
                  Notification::create("Failed to fetch level info",
                                       NotificationIcon::Error)
                      ->show();
                  return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  Notification::create("Invalid server response", NotificationIcon::Error)
                      ->show();
                  return;
            }

            auto json = jsonRes.unwrap();
            double averageDifficulty =
                json["averageDifficulty"].asDouble().unwrapOrDefault();
            int suggestedTotal = json["suggestedTotal"].asInt().unwrapOrDefault();
            int suggestedFeatured = json["suggestedFeatured"].asInt().unwrapOrDefault();
            int suggestedEpic = json["suggestedEpic"].asInt().unwrapOrDefault();
            int suggestedLegendary = json["suggestedLegendary"].asInt().unwrapOrDefault();
            int featuredScore = json["featuredScore"].asInt().unwrapOrDefault();
            int rejectedTotal = json["rejectedTotal"].asInt().unwrapOrDefault();

            std::string infoText =
                fmt::format(
                    "<cl>Average Difficulty:</c> {:.1f}\n"
                    "<cg>Total Suggested:</c> {}\n"
                    "<co>Total Suggested Featured:</c> {}\n"
                    "<cp>Total Suggested Epic:</c> {}\n"
                    "<cf>Total Suggested Legendary:</c> {}\n"
                    "<cy>Featured Score:</c> {}\n"
                    "<cr>Total Rejected:</c> {}\n",
                    averageDifficulty, suggestedTotal,
                    suggestedFeatured, suggestedEpic, suggestedLegendary, featuredScore, rejectedTotal);

            FLAlertLayer::create("Level Status Info", infoText, "OK")->show();
            // enable the button again
            if (auto buttonItem = typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) {
                  buttonItem->setEnabled(true);
            }
      });
}

void RLModRatePopup::onDeleteSendsButton(CCObject* sender) {
      std::string title = std::string("Delete Sends?");
      geode::createQuickPopup(
          "Delete Sends?",
          "Are you sure you want to <cr>delete sends</c> this layout?\n<cy>This action cannot be undone.</c>",
          "No",
          "Delete",
          [this](auto, bool yes) {
                if (!yes) return;
                auto popup = UploadActionPopup::create(nullptr, "Deleting sends...");
                popup->show();
                log::info("Deleting sends - Level ID: {}", m_levelId);

                // Get argon token
                auto token = Mod::get()->getSavedValue<std::string>("argon_token");
                if (token.empty()) {
                      log::error("Failed to get user token");
                      popup->showFailMessage("Token not found!");
                      return;
                }

                // matjson payload
                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                jsonBody["argonToken"] = token;
                jsonBody["targetLevelId"] = m_levelId;

                log::info("Sending request: {}", jsonBody.dump());

                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                
                Ref<RLModRatePopup> self = this;
                Ref<UploadActionPopup> upopup = popup;
                m_deleteSendsTask.spawn(
                      postReq.post("https://gdrate.arcticwoof.xyz/deleteSends"),
                      [self, upopup](web::WebResponse response) {
                      if (!self || !upopup) return;
                      log::info("Received response from server");

                      if (!response.ok()) {
                            log::warn("Server returned non-ok status: {}", response.code());
                            upopup->showFailMessage("Failed! Try again later.");
                            return;
                      }

                      auto jsonRes = response.json();
                      if (!jsonRes) {
                            log::warn("Failed to parse JSON response");
                            upopup->showFailMessage("Invalid server response.");
                            return;
                      }

                      auto json = jsonRes.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();

                      if (success) {
                            log::info("Delete sends successful!");
                            upopup->showSuccessMessage("Sends deleted!");
                      } else {
                            upopup->showFailMessage("Failed to delete sends.");
                      }
                });
          });
}

void RLModRatePopup::onUnsendButton(CCObject* sender) {
      auto popup = UploadActionPopup::create(nullptr, "Unsending layout...");
      popup->show();
      log::info("Unsending - Level ID: {}", m_levelId);
      // Get argon token
      auto token = Mod::get()->getSavedValue<std::string>("argon_token");
      if (token.empty()) {
            log::error("Failed to get user token");
            popup->showFailMessage("Token not found!");
            return;
      }
      // matjson payload
      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] = token;
      jsonBody["targetLevelId"] = m_levelId;
      log::info("Sending request: {}", jsonBody.dump());
      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      m_unsendTask.spawn(
            postReq.post("https://gdrate.arcticwoof.xyz/setUnsend"),
            [self, upopup](web::WebResponse response) {
            if (!self || !upopup) return;
            log::info("Received response from server");
            if (!response.ok()) {
                  log::warn("Server returned non-ok status: {}", response.code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }
            auto jsonRes = response.json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  upopup->showFailMessage("Invalid server response.");
                  return;
            }
            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (success) {
                  log::info("Unsend successful!");
                  upopup->showSuccessMessage("Layout unsent!");
            } else {
                  upopup->showFailMessage("Failed to unsend layout.");
            }
      });
}

void RLModRatePopup::onSubmitButton(CCObject* sender) {
      auto popup = UploadActionPopup::create(nullptr, "Submitting layout...");
      popup->show();
      log::info("Submitting - Difficulty: {}, Featured: {}, Demon: {}, Rejected: {}",
                m_selectedRating, m_isFeatured ? 1 : 0, m_isDemonMode ? 1 : 0, m_isRejected ? 1 : 0);

      // Get argon token
      auto token = Mod::get()->getSavedValue<std::string>("argon_token");
      if (token.empty()) {
            log::error("Failed to get user token");
            popup->showFailMessage("Token not found!");
            return;
      }

      if (!m_isRejected) {
            if (m_role == PopupRole::Dev) {
                  auto diffStr = (m_difficultyInput) ? m_difficultyInput->getString() : std::string();
                  if (diffStr.empty()) {
                        popup->showFailMessage("Enter a difficulty first!");
                        return;
                  }
            } else {
                  if (m_selectedRating == -1 && m_role != PopupRole::Dev) {
                        popup->showFailMessage("Select a rating first!");
                        return;
                  }
            }
      }

      // matjson payload
      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] = token;
      jsonBody["levelId"] = m_levelId;
      jsonBody["levelOwnerId"] = m_accountId;
      jsonBody["isPlat"] = m_level->isPlatformer();
      int featured = 0;
      if (m_role == PopupRole::Dev && m_featuredValueInput) {
            auto fv = m_featuredValueInput->getString();
            if (!fv.empty()) {
                  featured = numFromString<int>(fv).unwrapOr(0);
            }
      } else {
            if (m_isFeatured) {
                  featured = 1;
            } else if (m_isEpicFeatured) {
                  featured = 2;
            } else if (m_isLegendary) {
                  featured = 3;
            }
      }
      jsonBody["featured"] = featured;

      if (m_isRejected) {
            jsonBody["isRejected"] = true;
      } else {
            if (m_role == PopupRole::Dev && m_difficultyInput) {
                  auto diffStr = m_difficultyInput->getString();
                  int diff = numFromString<int>(diffStr).unwrapOr(0);
                  jsonBody["difficulty"] = diff;
                  // update difficulty sprite preview if possible
                  updateDifficultySprite(diff);
            } else {
                  jsonBody["difficulty"] = m_selectedRating;
            }

            // add featured score if featured, epic, or legendary mode is enabled
            if ((m_isFeatured || m_isEpicFeatured || m_isLegendary) && m_featuredScoreInput) {
                  auto scoreStr = m_featuredScoreInput->getString();
                  if (!scoreStr.empty()) {
                        int score = numFromString<int>(scoreStr).unwrapOr(0);
                        jsonBody["featuredScore"] = score;
                  }
            }

            // send silent=true when enabled
            if (m_role == PopupRole::Dev && m_silentToggleItem && m_silentToggleItem->isToggled()) {
                  jsonBody["silent"] = true;
            }

            // send verified=true when verified toggle is enabled
            if (m_verifiedToggleItem && m_verifiedToggleItem->isToggled()) {
                  jsonBody["verified"] = true;
            } else {
                  jsonBody["verified"] = false;
            }

            if (m_role == PopupRole::Mod) {
                  jsonBody["suggest"] = true;
            }
      }

      log::info("Sending request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      
      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      m_setRateTask.spawn(
            postReq.post("https://gdrate.arcticwoof.xyz/setRate"),
            [self, upopup](web::WebResponse response) {
            if (!self || !upopup) return;
            log::info("Received response from server");

            if (!response.ok()) {
                  log::warn("Server returned non-ok status: {}", response.code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  upopup->showFailMessage("Invalid server response.");
                  return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();

            if (success) {
                  log::info("Rate submission successful!");

                  // delete cached level to force refresh on next view
                  auto cachePath = dirs::getModsSaveDir() / "level_ratings_cache.json";
                  auto existingData = utils::file::readString(utils::string::pathToString(cachePath));
                  if (existingData) {
                        auto parsed = matjson::parse(existingData.unwrap());
                        if (parsed) {
                              auto root = parsed.unwrap();
                              if (root.isObject()) {
                                    std::string key = fmt::format("{}", self->m_levelId);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult =
                                  utils::file::writeString(utils::string::pathToString(cachePath), jsonString);
                              log::debug("Deleted level ID {} from cache after submission",
                                         self->m_levelId);
                        }
                  }

                  upopup->showSuccessMessage("Submitted successfully!");
            } else {
                  log::warn("Rate submission failed: success is false");
                  upopup->showFailMessage("Failed! Try again later.");
            }
      });
}

void RLModRatePopup::onUnrateButton(CCObject* sender) {
      std::string title = std::string("Unrate ") + m_level->m_levelName.c_str() + "?";
      geode::createQuickPopup(
          title.c_str(),
          "Are you sure you want to <cr>unrate</c> this layout?\n<cy>This action will be visible to everyone.</c>",
          "No",
          "Unrate",
          [this](auto, bool yes) {
                if (!yes) return;
                auto popup = UploadActionPopup::create(nullptr, "Unrating layout...");
                popup->show();
                log::info("Unrate button clicked");

                // clear reject state when admin uses unrate
                if (m_isRejected) {
                      m_isRejected = false;
                      auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject");
                      if (rejectBtn) {
                            auto rejectBtnItem = static_cast<CCMenuItemSpriteExtra*>(rejectBtn);
                            auto rejectBg = CCSprite::createWithSpriteFrameName("GJ_button_06.png");
                            auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                            rejectLabel->setScale(0.75f);
                            rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                            rejectBg->addChild(rejectLabel);
                            rejectBg->setID("button-bg-reject");
                            rejectBtnItem->setNormalImage(rejectBg);
                      }
                      // re-enable submit for admin when reject is cleared
                      if (m_role == PopupRole::Admin && m_submitButtonItem) {
                            auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                            m_submitButtonItem->setNormalImage(enabledSpr);
                            m_submitButtonItem->setEnabled(true);
                      }
                }

                // Get argon token
                auto token = Mod::get()->getSavedValue<std::string>("argon_token");
                if (token.empty()) {
                      log::error("Failed to get user token");
                      popup->showFailMessage("Token not found");
                      return;
                }
                // account ID
                auto accountId = GJAccountManager::get()->m_accountID;

                // matjson payload
                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = accountId;
                jsonBody["argonToken"] = token;
                jsonBody["levelId"] = m_levelId;

                if (m_role == PopupRole::Dev && m_silentToggleItem && m_silentToggleItem->isToggled()) {
                      jsonBody["silent"] = true;
                }

                log::info("Sending unrate request: {}", jsonBody.dump());

                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                
                Ref<RLModRatePopup> self = this;
                Ref<UploadActionPopup> upopup = popup;
                m_setUnrateTask.spawn(
                      postReq.post("https://gdrate.arcticwoof.xyz/setUnrate"),
                      [self, upopup](web::WebResponse response) {
                      if (!self || !upopup) return;
                      log::info("Received response from server");

                      if (!response.ok()) {
                            log::warn("Server returned non-ok status: {}", response.code());
                            upopup->showFailMessage("Failed! Try again later.");
                            return;
                      }

                      auto jsonRes = response.json();
                      if (!jsonRes) {
                            log::warn("Failed to parse JSON response");
                            upopup->showFailMessage("Invalid server response.");
                            return;
                      }

                      auto json = jsonRes.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();

                      if (success) {
                            log::info("Unrate submission successful!");

                            // Delete cached level to force refresh on next view
                            auto cachePath = dirs::getModsSaveDir() / "level_ratings_cache.json";
                            auto existingData = utils::file::readString(utils::string::pathToString(cachePath));
                            if (existingData) {
                                  auto parsed = matjson::parse(existingData.unwrap());
                                  if (parsed) {
                                        auto root = parsed.unwrap();
                                        if (root.isObject()) {
                                              std::string key = fmt::format("{}", self->m_levelId);
                                              auto result = root.erase(key);
                                        }
                                        auto jsonString = root.dump();
                                        auto writeResult =
                                            utils::file::writeString(utils::string::pathToString(cachePath), jsonString);
                                        log::debug("Deleted level ID {} from cache after unrate",
                                                   self->m_levelId);
                                  }
                            }

                            upopup->showSuccessMessage("Layout unrated successfully!");
                      } else {
                            log::warn("Unrate submission failed: success is false");
                            upopup->showFailMessage("Failed! Try again later.");
                      }
                });
          });
}

void RLModRatePopup::onRejectButton(CCObject* sender) {
      auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!btn) return;

      // reset previously selected rating's background
      if (m_selectedRating != -1) {
            CCMenu* prevContainer = (m_selectedRating <= 9) ? m_normalButtonsContainer
                                                            : m_demonButtonsContainer;
            auto prevButton = prevContainer->getChildByID(
                "rating-button-" + numToString(m_selectedRating));
            if (prevButton) {
                  auto prevButtonItem = static_cast<CCMenuItemSpriteExtra*>(prevButton);
                  auto prevButtonBg = CCSprite::create("GJ_button_04.png");
                  auto prevButtonLabel = CCLabelBMFont::create(
                      numToString(m_selectedRating).c_str(), "bigFont.fnt");
                  prevButtonLabel->setPosition(prevButtonBg->getContentSize() / 2);
                  prevButtonLabel->setScale(0.75f);
                  prevButtonBg->addChild(prevButtonLabel);
                  prevButtonBg->setID("button-bg-" + numToString(m_selectedRating));
                  prevButtonItem->setNormalImage(prevButtonBg);
            }
            m_selectedRating = -1;
      }

      // set this button to selected style
      auto selectedBg = CCSprite::create("GJ_button_01.png");
      auto selectedLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      selectedLabel->setPosition(selectedBg->getContentSize() / 2);
      selectedLabel->setScale(0.75f);
      selectedBg->addChild(selectedLabel);
      selectedBg->setID("button-bg-reject");
      btn->setNormalImage(selectedBg);

      m_isRejected = true;

      // disable submit for admin when rejected
      if (m_role == PopupRole::Admin && m_submitButtonItem) {
            auto disabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            m_submitButtonItem->setNormalImage(disabledSpr);
            m_submitButtonItem->setEnabled(false);
      }

      // update difficulty UI to neutral (0 => NA)
      updateDifficultySprite(0);
}

void RLModRatePopup::onSuggestButton(CCObject* sender) {
      auto popup = UploadActionPopup::create(nullptr, "Suggesting layout...");
      popup->show();
      log::info("Suggest button clicked");

      // Get argon token
      auto token = Mod::get()->getSavedValue<std::string>("argon_token");
      if (token.empty()) {
            log::error("Failed to get user token");
            popup->showFailMessage("Token not found");
            return;
      }

      if (!m_isRejected && m_selectedRating == -1) {
            popup->showFailMessage("Select a rating first!");
            return;
      }

      // matjson payload
      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] = token;
      jsonBody["levelId"] = m_levelId;
      jsonBody["levelOwnerId"] = m_accountId;
      jsonBody["isPlat"] = (m_level && m_level->isPlatformer());
      jsonBody["suggest"] = true;

      int featured = 0;
      if (m_role == PopupRole::Dev && m_featuredValueInput) {
            auto fv = m_featuredValueInput->getString();
            if (!fv.empty()) {
                  featured = numFromString<int>(fv).unwrapOr(0);
            }
      } else {
            if (m_isFeatured) {
                  featured = 1;
            } else if (m_isEpicFeatured) {
                  featured = 2;
            } else if (m_isLegendary) {
                  featured = 3;
            }
      }
      jsonBody["featured"] = featured;

      // include featuredScore when applicable
      if ((m_isFeatured || m_isEpicFeatured || m_isLegendary) && m_featuredScoreInput) {
            auto scoreStr = m_featuredScoreInput->getString();
            if (!scoreStr.empty()) {
                  int score = numFromString<int>(scoreStr).unwrapOr(0);
                  jsonBody["featuredScore"] = score;
            }
      }

      if (m_isRejected) {
            jsonBody["isRejected"] = true;
      } else {
            if (m_role == PopupRole::Dev && m_difficultyInput) {
                  auto diffStr = m_difficultyInput->getString();
                  int diff = numFromString<int>(diffStr).unwrapOr(0);
                  jsonBody["difficulty"] = diff;
                  updateDifficultySprite(diff);
            } else {
                  jsonBody["difficulty"] = m_selectedRating;
            }

            if (m_role == PopupRole::Dev && m_silentToggleItem && m_silentToggleItem->isToggled()) {
                  jsonBody["silent"] = true;
            }
      }

      log::info("Sending suggest request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);

      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      m_setRateTask.spawn(
            postReq.post("https://gdrate.arcticwoof.xyz/setRate"),
            [self, upopup](web::WebResponse response) {
            if (!self || !upopup) return;
            log::info("Received response from server");

            if (!response.ok()) {
                  log::warn("Server returned non-ok status: {}", response.code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  upopup->showFailMessage("Invalid server response.");
                  return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();

            if (success) {
                  log::info("Suggest submission successful!");

                  // delete cached level to force refresh on next view
                  auto cachePath = dirs::getModsSaveDir() / "level_ratings_cache.json";
                  auto existingData = utils::file::readString(utils::string::pathToString(cachePath));
                  if (existingData) {
                        auto parsed = matjson::parse(existingData.unwrap());
                        if (parsed) {
                              auto root = parsed.unwrap();
                              if (root.isObject()) {
                                    std::string key = fmt::format("{}", self->m_levelId);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult =
                                  utils::file::writeString(utils::string::pathToString(cachePath), jsonString);
                              log::debug("Deleted level ID {} from cache after suggest",
                                         self->m_levelId);
                        }
                  }

                  upopup->showSuccessMessage("Suggested successfully!");
            } else {
                  log::warn("Suggest submission failed: success is false");
                  upopup->showFailMessage("Failed! Try again later.");
            }
      });
}

void RLModRatePopup::onToggleFeatured(CCObject* sender) {
      // compute userRole from popup role
      int userRole = (m_role == PopupRole::Admin) ? 2 : ((m_role == PopupRole::Mod) ? 1 : 0);

      m_isFeatured = !m_isFeatured;

      // ensure rejecting state is cleared when toggling features
      if (m_isRejected) {
            m_isRejected = false;
            auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject");
            if (rejectBtn) {
                  auto rejectBtnItem = static_cast<CCMenuItemSpriteExtra*>(rejectBtn);
                  auto rejectBg = CCSprite::create("GJ_button_06.png");
                  auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                  rejectLabel->setScale(0.75f);
                  rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                  rejectBg->addChild(rejectLabel);
                  rejectBg->setID("button-bg-reject");
                  rejectBtnItem->setNormalImage(rejectBg);
            }
            // re-enable submit for admin when reject is cleared
            if (m_role == PopupRole::Admin && m_submitButtonItem) {
                  auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                  m_submitButtonItem->setNormalImage(enabledSpr);
                  m_submitButtonItem->setEnabled(true);
            }
      }

      // Only modify preview coins for non-Dev users. Dev users should not see/hide preview coins.
      if (m_role != PopupRole::Dev) {
            auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");
            auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
            auto existingLegendaryCoin = m_difficultyContainer->getChildByID("legendary-featured-coin");
            if (existingCoin) {
                  existingCoin->removeFromParent();  // could do setVisible false but whatever
            }

            if (m_isFeatured) {
                  auto featuredCoin = CCSprite::createWithSpriteFrameName("RL_featuredCoin.png"_spr);
                  featuredCoin->setPosition({0, 0});
                  featuredCoin->setScale(1.2f);
                  featuredCoin->setID("featured-coin");
                  m_difficultyContainer->addChild(featuredCoin, -1);
                  // if epic previously set, clear it
                  if (existingEpicCoin) existingEpicCoin->removeFromParent();
                  m_isEpicFeatured = false;
                  if (m_epicFeaturedToggleItem) {
                        m_epicFeaturedToggleItem->toggle(false);
                        setTogglerGrayscale(m_epicFeaturedToggleItem, "RL_epicFeaturedCoin.png"_spr, false);
                  }
                  // if legendary previously set, clear it
                  if (existingLegendaryCoin) existingLegendaryCoin->removeFromParent();
                  m_isLegendary = false;
                  if (m_legendaryToggleItem) {
                        m_legendaryToggleItem->toggle(false);
                        setTogglerGrayscale(m_legendaryToggleItem, "RL_legendaryFeaturedCoin.png"_spr, false);
                  }
                  // score only for admin
                  if (userRole == 2) {
                        m_featuredScoreInput->setVisible(true);
                        // hide reject button while featured score input is shown
                        if (m_normalButtonsContainer) {
                              if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                                    rejectBtn->setVisible(false);
                              }
                        }
                  }

                  // If featured was toggled on and there's no legendary active, re-enable submit
                  if (m_submitButtonItem && m_isFeatured && !m_isLegendary && !m_isRejected) {
                        auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                        m_submitButtonItem->setNormalImage(enabledSpr);
                        m_submitButtonItem->setEnabled(true);
                  }
            } else {
                  m_featuredScoreInput->setVisible(false);
                  // show reject button when featured score input is hidden
                  if (m_normalButtonsContainer) {
                        if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                              rejectBtn->setVisible(true);
                        }
                  }
                  if (m_epicFeaturedToggleItem) {
                        setTogglerGrayscale(m_epicFeaturedToggleItem, "RL_epicFeaturedCoin.png"_spr, false);
                  }
                  if (m_legendaryToggleItem) {
                        setTogglerGrayscale(m_legendaryToggleItem, "RL_legendaryFeaturedCoin.png"_spr, false);
                  }
            }
      }
      if (m_role == PopupRole::Dev) {
            if (m_isFeatured) {
                  m_isEpicFeatured = false;
            }
      }
}

void RLModRatePopup::onToggleDemon(CCObject* sender) {
      m_isDemonMode = !m_isDemonMode;

      m_normalButtonsContainer->setVisible(!m_isDemonMode);
      m_demonButtonsContainer->setVisible(m_isDemonMode);
}

void RLModRatePopup::onToggleLegendary(CCObject* sender) {
      int userRole = (m_role == PopupRole::Admin) ? 2 : ((m_role == PopupRole::Mod) ? 1 : 0);
      m_isLegendary = !m_isLegendary;

      // clear reject if set
      if (m_isRejected) {
            m_isRejected = false;
            auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject");
            if (rejectBtn) {
                  auto rejectBtnItem = static_cast<CCMenuItemSpriteExtra*>(rejectBtn);
                  auto rejectBg = CCSprite::create("GJ_button_06.png");
                  auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                  rejectLabel->setScale(0.75f);
                  rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                  rejectBg->addChild(rejectLabel);
                  rejectBg->setID("button-bg-reject");
                  rejectBtnItem->setNormalImage(rejectBg);
            }
      }

      // Only touch preview coins for non-Dev users
      if (m_role != PopupRole::Dev) {
            auto existingLegendaryCoin = m_difficultyContainer->getChildByID("legendary-featured-coin");
            auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
            auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");

            if (existingLegendaryCoin) {
                  existingLegendaryCoin->removeFromParent();
            }

            if (m_isLegendary) {
                  // clear lower tiers
                  if (existingCoin) existingCoin->removeFromParent();
                  if (existingEpicCoin) existingEpicCoin->removeFromParent();

                  m_isFeatured = false;
                  m_isEpicFeatured = false;

                  if (m_featuredToggleItem) {
                        m_featuredToggleItem->toggle(false);
                        setTogglerGrayscale(m_featuredToggleItem, "RL_featuredCoin.png"_spr, false);
                  }
                  if (m_epicFeaturedToggleItem) {
                        m_epicFeaturedToggleItem->toggle(false);
                        setTogglerGrayscale(m_epicFeaturedToggleItem, "RL_epicFeaturedCoin.png"_spr, false);
                  }

                  auto newLegendary = CCSprite::createWithSpriteFrameName("RL_legendaryFeaturedCoin.png"_spr);
                  newLegendary->setPosition({0, 0});
                  newLegendary->setScale(1.2f);
                  newLegendary->setID("legendary-featured-coin");
                  m_difficultyContainer->addChild(newLegendary, -1);

                  if (userRole == 2) {
                        m_featuredScoreInput->setVisible(true);
                        // hide reject while featured score input is shown
                        if (m_normalButtonsContainer) {
                              if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                                    rejectBtn->setVisible(false);
                              }
                        }
                  }
            } else {
                  m_featuredScoreInput->setVisible(false);
                  // show reject when featured score input is hidden
                  if (m_normalButtonsContainer) {
                        if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                              rejectBtn->setVisible(true);
                        }
                  }
                  if (m_featuredToggleItem) {
                        setTogglerGrayscale(m_featuredToggleItem, "RL_featuredCoin.png"_spr, false);
                  }
                  if (m_epicFeaturedToggleItem) {
                        setTogglerGrayscale(m_epicFeaturedToggleItem, "RL_epicFeaturedCoin.png"_spr, false);
                  }
            }
      } else {
            // Dev users: keep internal state but do not change preview/score UI
            if (m_isLegendary) {
                  m_isFeatured = false;
                  m_isEpicFeatured = false;
            }
      }

      // Disable the submit button when legendary is enabled and is admin
      if (m_submitButtonItem) {
            if (m_isLegendary && m_role == PopupRole::Admin) {
                  auto disabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
                  m_submitButtonItem->setNormalImage(disabledSpr);
                  m_submitButtonItem->setEnabled(false);
            } else {
                  // Re-enable only if another toggle is active
                  if (!m_isRejected && (m_isFeatured || m_isEpicFeatured)) {
                        auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                        m_submitButtonItem->setNormalImage(enabledSpr);
                        m_submitButtonItem->setEnabled(true);
                  }
            }
      }
}

void RLModRatePopup::onToggleEpicFeatured(CCObject* sender) {
      int userRole = (m_role == PopupRole::Admin) ? 2 : ((m_role == PopupRole::Mod) ? 1 : 0);
      m_isEpicFeatured = !m_isEpicFeatured;

      // clear reject if set
      if (m_isRejected) {
            m_isRejected = false;
            auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject");
            if (rejectBtn) {
                  auto rejectBtnItem = static_cast<CCMenuItemSpriteExtra*>(rejectBtn);
                  auto rejectBg = CCSprite::create("GJ_button_06.png");
                  auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                  rejectLabel->setScale(0.75f);
                  rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                  rejectBg->addChild(rejectLabel);
                  rejectBg->setID("button-bg-reject");
                  rejectBtnItem->setNormalImage(rejectBg);
            }
      }

      // Only touch preview coins for non-Dev users
      if (m_role != PopupRole::Dev) {
            auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
            auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");
            auto existingLegendaryCoin = m_difficultyContainer->getChildByID("legendary-featured-coin");

            if (existingEpicCoin) {
                  existingEpicCoin->removeFromParent();
            }

            if (m_isEpicFeatured) {
                  if (existingCoin) existingCoin->removeFromParent();
                  // if legendary previously set, clear it
                  if (existingLegendaryCoin) existingLegendaryCoin->removeFromParent();
                  m_isFeatured = false;
                  m_isLegendary = false;
                  if (m_featuredToggleItem) {
                        m_featuredToggleItem->toggle(false);
                        setTogglerGrayscale(m_featuredToggleItem, "RL_featuredCoin.png"_spr, false);
                  }
                  if (m_legendaryToggleItem) {
                        m_legendaryToggleItem->toggle(false);
                        setTogglerGrayscale(m_legendaryToggleItem, "RL_legendaryFeaturedCoin.png"_spr, false);
                  }
                  auto newEpicCoin = CCSprite::createWithSpriteFrameName("RL_epicFeaturedCoin.png"_spr);
                  newEpicCoin->setPosition({0, 0});
                  newEpicCoin->setScale(1.2f);
                  newEpicCoin->setID("epic-featured-coin");
                  m_difficultyContainer->addChild(newEpicCoin, -1);
                  if (userRole == 2) {
                        m_featuredScoreInput->setVisible(true);
                        // hide reject while featured score input is shown
                        if (m_normalButtonsContainer) {
                              if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                                    rejectBtn->setVisible(false);
                              }
                        }
                  }

                  // If epic was toggled on and there's no legendary active, re-enable submit
                  if (m_submitButtonItem && m_isEpicFeatured && !m_isLegendary && !m_isRejected) {
                        auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                        m_submitButtonItem->setNormalImage(enabledSpr);
                        m_submitButtonItem->setEnabled(true);
                  }
            } else {
                  m_featuredScoreInput->setVisible(false);
                  // show reject when featured score input is hidden
                  if (m_normalButtonsContainer) {
                        if (auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject")) {
                              rejectBtn->setVisible(true);
                        }
                  }
                  if (m_featuredToggleItem) {
                        setTogglerGrayscale(m_featuredToggleItem, "RL_featuredCoin.png"_spr, false);
                  }
                  if (m_legendaryToggleItem) {
                        setTogglerGrayscale(m_legendaryToggleItem, "RL_legendaryFeaturedCoin.png"_spr, false);
                  }
            }
      } else {
            // Dev users: keep internal state but do not change preview/score UI
            if (m_isEpicFeatured) {
                  m_isFeatured = false;
                  m_isLegendary = false;
            }
      }
}

void RLModRatePopup::onRatingButton(CCObject* sender) {
      auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
      int rating = button->getTag();

      // reset the bg of the previously selected button
      if (m_selectedRating != -1) {
            CCMenu* prevContainer = (m_selectedRating <= 9) ? m_normalButtonsContainer
                                                            : m_demonButtonsContainer;
            auto prevButton = prevContainer->getChildByID(
                "rating-button-" + numToString(m_selectedRating));
            if (prevButton) {
                  auto prevButtonItem = static_cast<CCMenuItemSpriteExtra*>(prevButton);
                  auto prevButtonBg = CCSprite::create("GJ_button_04.png");
                  auto prevButtonLabel = CCLabelBMFont::create(
                      numToString(m_selectedRating).c_str(), "bigFont.fnt");
                  prevButtonLabel->setPosition(prevButtonBg->getContentSize() / 2);
                  prevButtonLabel->setScale(0.75f);
                  prevButtonBg->addChild(prevButtonLabel);
                  prevButtonBg->setID("button-bg-" + numToString(m_selectedRating));
                  prevButtonItem->setNormalImage(prevButtonBg);
            }
      }

      auto currentButton = static_cast<CCMenuItemSpriteExtra*>(sender);

      // if previously reject was selected, clear it
      if (m_isRejected) {
            m_isRejected = false;
            auto rejectBtn = m_normalButtonsContainer->getChildByID("rating-button-reject");
            if (rejectBtn) {
                  auto rejectBtnItem = static_cast<CCMenuItemSpriteExtra*>(rejectBtn);
                  auto rejectBg = CCSprite::create("GJ_button_06.png");
                  auto rejectLabel = CCLabelBMFont::create("-", "bigFont.fnt");
                  rejectLabel->setScale(0.75f);
                  rejectLabel->setPosition(rejectBg->getContentSize() / 2);
                  rejectBg->addChild(rejectLabel);
                  rejectBg->setID("button-bg-reject");
                  rejectBtnItem->setNormalImage(rejectBg);
            }
            // re-enable submit for admin when reject is cleared
            if (m_role == PopupRole::Admin && m_submitButtonItem) {
                  auto enabledSpr = ButtonSprite::create("Submit", 80, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
                  m_submitButtonItem->setNormalImage(enabledSpr);
                  m_submitButtonItem->setEnabled(true);
            }
      }

      auto currentButtonBg = CCSprite::create("GJ_button_01.png");
      auto currentButtonLabel =
          CCLabelBMFont::create(numToString(rating).c_str(), "bigFont.fnt");
      currentButtonLabel->setPosition(currentButtonBg->getContentSize() / 2);
      currentButtonLabel->setScale(0.75f);
      currentButtonBg->addChild(currentButtonLabel);
      currentButtonBg->setID("button-bg-" + numToString(rating));
      currentButton->setNormalImage(currentButtonBg);

      m_selectedRating = button->getTag();

      updateDifficultySprite(rating);
}

void RLModRatePopup::updateDifficultySprite(int rating) {
      // Do nothing for Dev users
      if (m_role == PopupRole::Dev) {
            return;
      }

      // Defensive: ensure we have a container to add the difficulty sprite to.
      if (!m_difficultyContainer) {
            if (!m_mainLayer) return;
            m_difficultyContainer = CCNode::create();
            m_difficultyContainer->setPosition({m_mainLayer->getContentSize().width - 50.f, 90.f});
            m_mainLayer->addChild(m_difficultyContainer);
      }

      if (m_difficultySprite) {
            m_difficultySprite->removeFromParent();
      }

      int difficultyLevel;

      switch (rating) {
            case 1:
                  difficultyLevel = -1;
                  break;
            case 2:
                  difficultyLevel = 1;
                  break;
            case 3:
                  difficultyLevel = 2;
                  break;
            case 4:
            case 5:
                  difficultyLevel = 3;
                  break;
            case 6:
            case 7:
                  difficultyLevel = 4;
                  break;
            case 8:
            case 9:
                  difficultyLevel = 5;
                  break;
            case 10:
                  difficultyLevel = 7;
                  break;
            case 15:
                  difficultyLevel = 8;
                  break;
            case 20:
                  difficultyLevel = 6;
                  break;
            case 25:
                  difficultyLevel = 9;
                  break;
            case 30:
                  difficultyLevel = 10;
                  break;
            default:
                  difficultyLevel = 0;
                  break;
      }

      m_difficultySprite =
          GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Short);
      m_difficultySprite->setPosition({0, 0});
      m_difficultySprite->setScale(1.2f);
      m_difficultyContainer->addChild(m_difficultySprite);
}

void RLModRatePopup::onSetEventButton(CCObject* sender) {
      auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!item) return;
      // mapping by ID
      geode::ZStringView id = item->getID();
      log::info("Button ID: {}", id);
      std::string type;
      if (id == "event-daily") {
            type = "daily";
      } else if (id == "event-weekly") {
            type = "weekly";
      } else if (id == "event-monthly") {
            type = "monthly";
      } else {
            log::warn("Unknown event ID: {}", id);
            return;
      }

      // Get argon token
      auto token = Mod::get()->getSavedValue<std::string>("argon_token");
      if (token.empty()) {
            log::error("Failed to get user token");
            Notification::create("Token not found", NotificationIcon::Error)->show();
            return;
      }

      std::string title = std::string("Set ") + type + " layout?";
      std::string content = std::string("Are you sure you want to set this <cg>level</c> as the <co>") + type + " layout?</c>";

      geode::createQuickPopup(
          title.c_str(),
          content.c_str(),
          "No",
          "Yes",
          [this, type, token](auto, bool yes) {
                log::info("Popup callback triggered, yes={}", yes);
                if (!yes) return;
                auto popup = UploadActionPopup::create(nullptr, "Setting event...");
                popup->show();
                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                jsonBody["argonToken"] = token;
                jsonBody["levelId"] = m_levelId;
                jsonBody["type"] = type;
                jsonBody["isPlat"] = (m_level && m_level->isPlatformer());

                log::info("Sending setEvent request: {}", jsonBody.dump());
                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                
                Ref<RLModRatePopup> self = this;
                Ref<UploadActionPopup> upopup = popup;

                self->m_setEventTask.spawn(
                      postReq.post("https://gdrate.arcticwoof.xyz/setEvent"),
                      [self, type, upopup](web::WebResponse response) {
                      if (!self || !upopup) return;
                      log::info("Received setEvent response for type: {}", type);
                      if (!response.ok()) {
                            log::warn("Server returned non-ok status: {}", response.code());
                            upopup->showFailMessage("Failed! Try again later.");
                            return;
                      }
                      auto jsonRes = response.json();
                      if (!jsonRes) {
                            log::warn("Failed to parse setEvent JSON response");
                            upopup->showFailMessage("Invalid server response.");
                            return;
                      }
                      auto json = jsonRes.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();
                      std::string message = json["message"].asString().unwrapOrDefault();
                      if (success || message == "Event set successfully") {
                            upopup->showSuccessMessage(std::string("Event set: ") + type);
                      } else {
                            upopup->showFailMessage("Failed to set event.");
                      }
                });
          });
}

RLModRatePopup* RLModRatePopup::create(RLModRatePopup::PopupRole role, std::string title, GJGameLevel* level) {
      auto ret = new RLModRatePopup();
      ret->m_role = role;
      ret->m_title = title;
      ret->m_level = level;

      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      };

      delete ret;
      return nullptr;
}