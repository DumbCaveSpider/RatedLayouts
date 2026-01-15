#include "RLModRatePopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// another helper lol set the visual of a CCMenuItemToggler to grayscale or normal
static void setTogglerGrayscale(CCMenuItemToggler* toggler, const char* spriteName, bool /*unused*/) {
      if (!toggler) return;
      // Use toggler state to decide whether the normal image should be grayscale
      bool toggled = toggler->isToggled();
      auto normalSpr = toggled ? CCSprite::create(spriteName) : CCSpriteGrayscale::create(spriteName);
      auto selectedSpr = CCSprite::create(spriteName);
      if (auto spriteItem = typeinfo_cast<CCMenuItemSpriteExtra*>(toggler)) {
            if (normalSpr) spriteItem->setNormalImage(normalSpr);
            if (selectedSpr) spriteItem->setSelectedImage(selectedSpr);
      }
}

bool RLModRatePopup::setup(std::string title, GJGameLevel* level) {
      m_title = title;
      m_level = level;
      m_difficultySprite = nullptr;
      m_isDemonMode = false;
      m_isFeatured = false;
      m_isEpicFeatured = false;
      m_selectedRating = -1;
      m_isRejected = false;
      m_levelId = -1;
      m_accountId = 0;

      // get the level ID ya
      if (level) {
            m_levelId = level->m_levelID;
            m_accountId = level->m_accountID;
      }

      // title
      auto titleLabel = CCLabelBMFont::create(m_title.c_str(), "bigFont.fnt");
      titleLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                               m_mainLayer->getContentSize().height - 20});
      m_mainLayer->addChild(titleLabel);

      // menu time
      auto menuButtons = CCMenu::create();
      menuButtons->setPosition({0, 0});

      // normal and demon buttons
      m_normalButtonsContainer = CCMenu::create();
      m_normalButtonsContainer->setPosition({0, 0});
      m_demonButtonsContainer = CCMenu::create();
      m_demonButtonsContainer->setPosition({0, 0});

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

      menuButtons->addChild(m_normalButtonsContainer);
      menuButtons->addChild(m_demonButtonsContainer);
      m_demonButtonsContainer->setVisible(false);

      // demon toggle
      auto offDemonSprite =
          CCSpriteGrayscale::createWithSpriteFrameName("GJ_demonIcon_001.png");
      auto onDemonSprite =
          CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
      auto demonToggle =
          CCMenuItemToggler::create(offDemonSprite, onDemonSprite, this,
                                    menu_selector(RLModRatePopup::onToggleDemon));

      demonToggle->setPosition({m_mainLayer->getContentSize().width, 0});
      demonToggle->setScale(1.2f);
      menuButtons->addChild(demonToggle);

      // info button
      auto infoButton = CCMenuItemSpriteExtra::create(
          CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this,
          menu_selector(RLModRatePopup::onInfoButton));
      infoButton->setPosition({m_mainLayer->getContentSize().width,
                               m_mainLayer->getContentSize().height});
      menuButtons->addChild(infoButton);

      // submit button
      int userRole = (m_role == PopupRole::Admin) ? 2 : ((m_role == PopupRole::Mod) ? 1 : 0);
      float centerX = m_mainLayer->getContentSize().width / 2;

      auto submitButtonSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png", .8f);
      auto submitButtonItem = CCMenuItemSpriteExtra::create(
          submitButtonSpr, this, menu_selector(RLModRatePopup::onSubmitButton));

      if (userRole == 2) {
            // when admin, arrange Submit / Unrate / Suggest evenly around center
            float spacing = 105.f;
            submitButtonItem->setPosition({centerX - spacing, 0});
      } else {
            // non-admins only have Submit centered
            submitButtonItem->setPosition({centerX, 0});
      }
      menuButtons->addChild(submitButtonItem);
      m_submitButtonItem = submitButtonItem;

      // unrate and suggest buttons (only for admins)
      if (userRole == 2) {
            float spacing = 105.f;

            auto unreateSpr = ButtonSprite::create("Unrate", .8f);
            auto unrateButtonItem = CCMenuItemSpriteExtra::create(
                unreateSpr, this, menu_selector(RLModRatePopup::onUnrateButton));

            unrateButtonItem->setPosition({centerX, 0});
            menuButtons->addChild(unrateButtonItem);

            // suggest button for admin
            auto suggestSpr = ButtonSprite::create("Suggest", .8f);
            auto suggestButtonItem = CCMenuItemSpriteExtra::create(
                suggestSpr, this, menu_selector(RLModRatePopup::onSuggestButton));
            suggestButtonItem->setPosition({centerX + spacing, 0});
            suggestButtonItem->setID("suggest-button");
            menuButtons->addChild(suggestButtonItem);
      }

      // toggle between featured or stars only
      auto offSprite = CCSpriteGrayscale::create("RL_featuredCoin.png"_spr);
      auto onSprite = CCSprite::create("RL_featuredCoin.png"_spr);
      auto toggleFeatured = CCMenuItemToggler::create(
          offSprite, onSprite, this, menu_selector(RLModRatePopup::onToggleFeatured));
      m_featuredToggleItem = toggleFeatured;

      toggleFeatured->setPosition({0, -10});
      menuButtons->addChild(toggleFeatured);

      m_mainLayer->addChild(menuButtons);

      if (userRole >= 1) {
            auto offEpicSprite = CCSpriteGrayscale::create("RL_epicFeaturedCoin.png"_spr);
            auto onEpicSprite = CCSprite::create("RL_epicFeaturedCoin.png"_spr);
            auto toggleEpicFeatured = CCMenuItemToggler::create(
                offEpicSprite, onEpicSprite, this, menu_selector(RLModRatePopup::onToggleEpicFeatured));
            m_epicFeaturedToggleItem = toggleEpicFeatured;
            toggleEpicFeatured->setPosition({0, 50});
            menuButtons->addChild(toggleEpicFeatured);
      }

      // difficulty sprite on the right side (NA face by default)
      m_difficultyContainer = CCNode::create();
      m_difficultyContainer->setPosition(
          {m_mainLayer->getContentSize().width - 50.f, 90.f});
      m_difficultySprite = GJDifficultySprite::create(0, (GJDifficultyName)-1);
      m_difficultySprite->setPosition({0, 0});
      m_difficultySprite->setScale(1.2f);
      m_difficultyContainer->addChild(m_difficultySprite);
      m_mainLayer->addChild(m_difficultyContainer);

      // Admin-only event buttons: daily / weekly / monthly
      if (userRole == 2) {
            auto eventMenu = CCMenu::create();
            eventMenu->setPosition({0, 0});

            // positions near the right side, stacked vertically
            float eventX = m_mainLayer->getContentSize().width;
            float eventYStart = 120.f;
            float eventSpacing = 38.f;
            std::vector<std::tuple<std::string, std::string, std::string>> events = {// is tuple like two vectors or three vectors
                                                                                     {"event-daily", "daily", "D"},
                                                                                     {"event-weekly", "weekly", "W"},
                                                                                     {"event-monthly", "monthly", "M"}};

            for (size_t i = 0; i < events.size(); ++i) {
                  auto btnSpr = ButtonSprite::create(std::get<2>(events[i]).c_str(), 1.f);
                  auto btnItem = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(RLModRatePopup::onSetEventButton));
                  btnItem->setPosition({eventX, eventYStart - (float)i * eventSpacing});
                  btnItem->setID(std::get<0>(events[i]));
                  eventMenu->addChild(btnItem);
            }

            m_mainLayer->addChild(eventMenu);
      }

      // featured score textbox (created conditionally based on role)
      m_featuredScoreInput = TextInput::create(100.f, "Score");
      m_featuredScoreInput->setPosition({300.f, 40.f});
      m_featuredScoreInput->setVisible(false);
      m_featuredScoreInput->setID("featured-score-input");
      m_mainLayer->addChild(m_featuredScoreInput);

      return true;
}

void RLModRatePopup::onInfoButton(CCObject* sender) {
      // matjson payload

      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] =
          Mod::get()->getSavedValue<std::string>("argon_token");
      jsonBody["levelId"] = m_levelId;

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      m_getModLevelTask = postReq.post("https://gdrate.arcticwoof.xyz/getModLevel");

      Ref<RLModRatePopup> self = this;
      m_getModLevelTask.listen([self](web::WebResponse* response) {
            if (!self) return;
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  Notification::create("Failed to fetch level info",
                                       NotificationIcon::Error)
                      ->show();
                  return;
            }

            auto jsonRes = response->json();
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
            int featuredScore = json["featuredScore"].asInt().unwrapOrDefault();
            int rejectedTotal = json["rejectedTotal"].asInt().unwrapOrDefault();

            std::string infoText =
                fmt::format(
                    "<cl>Average Difficulty:</c> {:.1f}\n"
                    "<cg>Total Suggested:</c> {}\n"
                    "<co>Total Suggested Featured:</c> {}\n"
                    "<cp>Total Suggested Epic:</c> {}\n"
                    "<cy>Featured Score:</c> {}\n"
                    "<cr>Total Rejected:</c> {}\n",
                    averageDifficulty, suggestedTotal,
                    suggestedFeatured, suggestedEpic, featuredScore, rejectedTotal);

            FLAlertLayer::create("Level Status Info", infoText, "OK")->show();
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
      jsonBody["isPlat"] = m_level->isPlatformer();
      int featured = 0;
      if (m_isFeatured) {
            featured = 1;
      } else if (m_isEpicFeatured) {
            featured = 2;
      }
      jsonBody["featured"] = featured;

      if (m_isRejected) {
            jsonBody["isRejected"] = true;
      } else {
            jsonBody["difficulty"] = m_selectedRating;

            // add featured score if featured or epic featured mode is enabled
            if ((m_isFeatured || m_isEpicFeatured) && m_featuredScoreInput) {
                  auto scoreStr = m_featuredScoreInput->getString();
                  if (!scoreStr.empty()) {
                        int score = numFromString<int>(scoreStr).unwrapOr(0);
                        jsonBody["featuredScore"] = score;
                  }
            }
      }

      log::info("Sending request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      auto postTask = m_setRateTask = postReq.post("https://gdrate.arcticwoof.xyz/setRate");

      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      postTask.listen([self, upopup](web::WebResponse* response) {
            if (!self || !upopup) return;
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response->json();
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
      auto popup = UploadActionPopup::create(nullptr, "Unrating layout...");
      popup->show();
      log::info("Unrate button clicked");

      // clear reject state when admin uses unrate
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
                  auto enabledSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png", .8f);
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

      log::info("Sending unrate request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      m_setUnrateTask = postReq.post("https://gdrate.arcticwoof.xyz/setUnrate");

      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      m_setUnrateTask.listen([self, upopup](web::WebResponse* response) {
            if (!self || !upopup) return;
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response->json();
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
            auto disabledSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_04.png", .8f);
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
      if (m_isFeatured) {
            featured = 1;
      } else if (m_isEpicFeatured) {
            featured = 2;
      }
      jsonBody["featured"] = featured;

      // include featuredScore when applicable
      if ((m_isFeatured || m_isEpicFeatured) && m_featuredScoreInput) {
            auto scoreStr = m_featuredScoreInput->getString();
            if (!scoreStr.empty()) {
                  int score = numFromString<int>(scoreStr).unwrapOr(0);
                  jsonBody["featuredScore"] = score;
            }
      }

      if (m_isRejected) {
            jsonBody["isRejected"] = true;
      } else {
            jsonBody["difficulty"] = m_selectedRating;
      }

      log::info("Sending suggest request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      m_setRateTask = postReq.post("https://gdrate.arcticwoof.xyz/setRate");

      Ref<RLModRatePopup> self = this;
      Ref<UploadActionPopup> upopup = popup;
      m_setRateTask.listen([self, upopup](web::WebResponse* response) {
            if (!self || !upopup) return;
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  upopup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response->json();
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
                  auto enabledSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png", .8f);
                  m_submitButtonItem->setNormalImage(enabledSpr);
                  m_submitButtonItem->setEnabled(true);
            }
      }

      auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");
      auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
      if (existingCoin) {
            existingCoin->removeFromParent();  // could do setVisible false but whatever
      }

      if (m_isFeatured) {
            auto featuredCoin = CCSprite::create("RL_featuredCoin.png"_spr);
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
      }
}

void RLModRatePopup::onToggleDemon(CCObject* sender) {
      m_isDemonMode = !m_isDemonMode;

      m_normalButtonsContainer->setVisible(!m_isDemonMode);
      m_demonButtonsContainer->setVisible(m_isDemonMode);
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

      auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
      auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");

      if (existingEpicCoin) {
            existingEpicCoin->removeFromParent();
      }

      if (m_isEpicFeatured) {
            if (existingCoin) existingCoin->removeFromParent();
            m_isFeatured = false;
            if (m_featuredToggleItem) {
                  m_featuredToggleItem->toggle(false);
                  setTogglerGrayscale(m_featuredToggleItem, "RL_featuredCoin.png"_spr, false);
            }
            auto newEpicCoin = CCSprite::create("RL_epicFeaturedCoin.png"_spr);
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
                  auto enabledSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png", .8f);
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
      std::string id = item->getID();
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

      std::string title = "Set " + type + " layout?";
      std::string content = "Are you sure you want to set this <cg>level</c> as the <co>" + type + " layout?</c>";

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
                Ref<RLModRatePopup> popupRef = this;
                auto postTask = popupRef->m_setEventTask = postReq.post("https://gdrate.arcticwoof.xyz/setEvent");

                Ref<RLModRatePopup> self = this;
                Ref<UploadActionPopup> upopup = popup;
                postTask.listen([self, type, upopup](web::WebResponse* response) {
                      if (!self || !upopup) return;
                      log::info("Received setEvent response for type: {}", type);
                      if (!response->ok()) {
                            log::warn("Server returned non-ok status: {}", response->code());
                            upopup->showFailMessage("Failed! Try again later.");
                            return;
                      }
                      auto jsonRes = response->json();
                      if (!jsonRes) {
                            log::warn("Failed to parse setEvent JSON response");
                            upopup->showFailMessage("Invalid server response.");
                            return;
                      }
                      auto json = jsonRes.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();
                      std::string message = json["message"].asString().unwrapOrDefault();
                      if (success || message == "Event set successfully") {
                            upopup->showSuccessMessage("Event set: " + type);
                      } else {
                            upopup->showFailMessage("Failed to set event.");
                      }
                });
          });
}

RLModRatePopup* RLModRatePopup::create(RLModRatePopup::PopupRole role, std::string title, GJGameLevel* level) {
      auto ret = new RLModRatePopup();
      ret->m_role = role;

      if (ret && ret->initAnchored(380.f, 180.f, title, level, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      };

      CC_SAFE_DELETE(ret);
      return nullptr;
};