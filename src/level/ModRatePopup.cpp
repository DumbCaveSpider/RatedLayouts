#include "ModRatePopup.hpp"

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

bool ModRatePopup::setup(std::string title, GJGameLevel* level) {
      m_title = title;
      m_level = level;
      m_difficultySprite = nullptr;
      m_isDemonMode = false;
      m_isFeatured = false;
      m_isEpicFeatured = false;
      m_selectedRating = -1;
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
                buttonBg, this, menu_selector(ModRatePopup::onRatingButton));

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
                buttonBg, this, menu_selector(ModRatePopup::onRatingButton));

            ratingButtonItem->setPosition({startX + idx * buttonSpacing, firstRowY});
            ratingButtonItem->setTag(rating);
            ratingButtonItem->setID("rating-button-" + numToString(rating));
            m_demonButtonsContainer->addChild(ratingButtonItem);
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
                                    menu_selector(ModRatePopup::onToggleDemon));

      demonToggle->setPosition({m_mainLayer->getContentSize().width, 0});
      demonToggle->setScale(1.2f);
      menuButtons->addChild(demonToggle);

      // submit button
      int userRole = Mod::get()->getSavedValue<int>("role", 0);
      float submitButtonX = (userRole == 2)
                                ? m_mainLayer->getContentSize().width / 2 - 65
                                : m_mainLayer->getContentSize().width / 2;

      auto submitButtonSpr = ButtonSprite::create("Submit", 1.f);
      auto submitButtonItem = CCMenuItemSpriteExtra::create(
          submitButtonSpr, this, menu_selector(ModRatePopup::onSubmitButton));

      submitButtonItem->setPosition({submitButtonX, 0});
      menuButtons->addChild(submitButtonItem);

      // unrate button (only for admins)
      if (userRole == 2) {
            auto unreateSpr = ButtonSprite::create("Unrate", 1.f);
            auto unrateButtonItem = CCMenuItemSpriteExtra::create(
                unreateSpr, this, menu_selector(ModRatePopup::onUnrateButton));

            unrateButtonItem->setPosition(
                {m_mainLayer->getContentSize().width / 2 + 65, 0});
            menuButtons->addChild(unrateButtonItem);

            // info button for admin
            auto infoButton = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this,
                menu_selector(ModRatePopup::onInfoButton));
            infoButton->setPosition({m_mainLayer->getContentSize().width,
                                     m_mainLayer->getContentSize().height});
            menuButtons->addChild(infoButton);
      }

      // toggle between featured or stars only
      auto offSprite = CCSpriteGrayscale::create("rlfeaturedCoin.png"_spr);
      auto onSprite = CCSprite::create("rlfeaturedCoin.png"_spr);
      auto toggleFeatured = CCMenuItemToggler::create(
          offSprite, onSprite, this, menu_selector(ModRatePopup::onToggleFeatured));
      m_featuredToggleItem = toggleFeatured;

      toggleFeatured->setPosition({0, -10});
      menuButtons->addChild(toggleFeatured);

      m_mainLayer->addChild(menuButtons);

      if (userRole >= 1) {
            auto offEpicSprite = CCSpriteGrayscale::create("rlepicFeaturedCoin.png"_spr);
            auto onEpicSprite = CCSprite::create("rlepicFeaturedCoin.png"_spr);
            auto toggleEpicFeatured = CCMenuItemToggler::create(
                offEpicSprite, onEpicSprite, this, menu_selector(ModRatePopup::onToggleEpicFeatured));
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
                  auto btnItem = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(ModRatePopup::onSetEventButton));
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

void ModRatePopup::onInfoButton(CCObject* sender) {
      // matjson payload

      matjson::Value jsonBody = matjson::Value::object();
      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
      jsonBody["argonToken"] =
          Mod::get()->getSavedValue<std::string>("argon_token");
      jsonBody["levelId"] = m_levelId;

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/level");

      postTask.listen([this](web::WebResponse* response) {
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

            std::string infoText =
                fmt::format(
                    "Average Difficulty: {:.1f}\n"
                    "Total Suggested: {}\n"
                    "Total Suggested Featured: {}\n"
                    "Total Suggested Epic: {}\n"
                    "Featured Score: {}\n",
                    averageDifficulty, suggestedTotal,
                    suggestedFeatured, suggestedEpic, featuredScore);

            FLAlertLayer::create("Level Status Info", infoText, "OK")->show();
      });
}

void ModRatePopup::onSubmitButton(CCObject* sender) {
      auto popup = UploadActionPopup::create(nullptr, "Submitting layout...");
      popup->show();
      log::info("Submitting - Difficulty: {}, Featured: {}, Demon: {}",
                m_selectedRating, m_isFeatured ? 1 : 0, m_isDemonMode ? 1 : 0);

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
      jsonBody["levelId"] = m_levelId;
      jsonBody["levelOwnerId"] = m_accountId;
      jsonBody["difficulty"] = m_selectedRating;
      jsonBody["isPlat"] = m_level->isPlatformer();
      int featured = 0;
      if (m_isFeatured) {
            featured = 1;
      } else if (m_isEpicFeatured) {
            featured = 2;
      }
      jsonBody["featured"] = featured;

      // add featured score if featured or epic featured mode is enabled
      if ((m_isFeatured || m_isEpicFeatured) && m_featuredScoreInput) {
            auto scoreStr = m_featuredScoreInput->getString();
            if (!scoreStr.empty()) {
                  int score = numFromString<int>(scoreStr).unwrapOr(0);
                  jsonBody["featuredScore"] = score;
            }
      }

      log::info("Sending request: {}", jsonBody.dump());

      auto postReq = web::WebRequest();
      postReq.bodyJSON(jsonBody);
      auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/rate");

      postTask.listen([this, popup](web::WebResponse* response) {
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  popup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response->json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  popup->showFailMessage("Invalid server response.");
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
                                    std::string key = fmt::format("{}", this->m_levelId);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult =
                                  utils::file::writeString(utils::string::pathToString(cachePath), jsonString);
                              log::debug("Deleted level ID {} from cache after submission",
                                         this->m_levelId);
                        }
                  }

                  popup->showSuccessMessage("Submitted successfully!");
            } else {
                  log::warn("Rate submission failed: success is false");
                  popup->showFailMessage("Failed! Try again later.");
            }
      });
}

void ModRatePopup::onUnrateButton(CCObject* sender) {
      auto popup = UploadActionPopup::create(nullptr, "Unrating layout...");
      popup->show();
      log::info("Unrate button clicked");

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
      auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/unrate");

      postTask.listen([this, popup](web::WebResponse* response) {
            log::info("Received response from server");

            if (!response->ok()) {
                  log::warn("Server returned non-ok status: {}", response->code());
                  popup->showFailMessage("Failed! Try again later.");
                  return;
            }

            auto jsonRes = response->json();
            if (!jsonRes) {
                  log::warn("Failed to parse JSON response");
                  popup->showFailMessage("Invalid server response.");
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
                                    std::string key = fmt::format("{}", this->m_levelId);
                                    auto result = root.erase(key);
                              }
                              auto jsonString = root.dump();
                              auto writeResult =
                                  utils::file::writeString(utils::string::pathToString(cachePath), jsonString);
                              log::debug("Deleted level ID {} from cache after unrate",
                                         this->m_levelId);
                        }
                  }

                  popup->showSuccessMessage("Layout unrated successfully!");
            } else {
                  log::warn("Unrate submission failed: success is false");
                  popup->showFailMessage("Failed! Try again later.");
            }
      });
}

void ModRatePopup::onToggleFeatured(CCObject* sender) {
      // Check if user has admin role
      int userRole = Mod::get()->getSavedValue<int>("role", 0);

      m_isFeatured = !m_isFeatured;

      auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");
      auto existingEpicCoin = m_difficultyContainer->getChildByID("epic-featured-coin");
      if (existingCoin) {
            existingCoin->removeFromParent();  // could do setVisible false but whatever
      }

      if (m_isFeatured) {
            auto featuredCoin = CCSprite::create("rlfeaturedCoin.png"_spr);
            featuredCoin->setPosition({0, 0});
            featuredCoin->setScale(1.2f);
            featuredCoin->setID("featured-coin");
            m_difficultyContainer->addChild(featuredCoin, -1);
            // if epic previously set, clear it
            if (existingEpicCoin) existingEpicCoin->removeFromParent();
            m_isEpicFeatured = false;
            if (m_epicFeaturedToggleItem) {
                  m_epicFeaturedToggleItem->toggle(false);
                  setTogglerGrayscale(m_epicFeaturedToggleItem, "rlepicFeaturedCoin.png"_spr, false);
            }
            // score only for admin
            if (userRole == 2) {
                  m_featuredScoreInput->setVisible(true);
            }
      } else {
            m_featuredScoreInput->setVisible(false);
            if (m_epicFeaturedToggleItem) {
                  setTogglerGrayscale(m_epicFeaturedToggleItem, "rlepicFeaturedCoin.png"_spr, false);
            }
      }
}

void ModRatePopup::onToggleDemon(CCObject* sender) {
      m_isDemonMode = !m_isDemonMode;

      m_normalButtonsContainer->setVisible(!m_isDemonMode);
      m_demonButtonsContainer->setVisible(m_isDemonMode);
}

void ModRatePopup::onToggleEpicFeatured(CCObject* sender) {
      int userRole = Mod::get()->getSavedValue<int>("role", 0);
      m_isEpicFeatured = !m_isEpicFeatured;

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
                  setTogglerGrayscale(m_featuredToggleItem, "rlfeaturedCoin.png"_spr, false);
            }
            auto newEpicCoin = CCSprite::create("rlepicFeaturedCoin.png"_spr);
            newEpicCoin->setPosition({0, 0});
            newEpicCoin->setScale(1.2f);
            newEpicCoin->setID("epic-featured-coin");
            m_difficultyContainer->addChild(newEpicCoin, -1);
            if (userRole == 2) {
                  m_featuredScoreInput->setVisible(true);
            }
      } else {
            m_featuredScoreInput->setVisible(false);
            if (m_featuredToggleItem) {
                  setTogglerGrayscale(m_featuredToggleItem, "rlfeaturedCoin.png"_spr, false);
            }
      }
}

void ModRatePopup::onRatingButton(CCObject* sender) {
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

void ModRatePopup::updateDifficultySprite(int rating) {
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

void ModRatePopup::onSetEventButton(CCObject* sender) {
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
                auto popup = UploadActionPopup::create(nullptr, "Setting event...");
                popup->show();
                log::info("Popup callback triggered, yes={}", yes);
                if (!yes) return;
                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                jsonBody["argonToken"] = token;
                jsonBody["levelId"] = m_levelId;
                jsonBody["type"] = type;
                jsonBody["isPlat"] = (m_level && m_level->isPlatformer());

                log::info("Sending setEvent request: {}", jsonBody.dump());
                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);
                auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/setEvent");

                postTask.listen([this, type, popup](web::WebResponse* response) {
                      log::info("Received setEvent response for type: {}", type);
                      if (!response->ok()) {
                            log::warn("Server returned non-ok status: {}", response->code());
                            popup->showFailMessage("Failed! Try again later.");
                            return;
                      }
                      auto jsonRes = response->json();
                      if (!jsonRes) {
                            log::warn("Failed to parse setEvent JSON response");
                            popup->showFailMessage("Invalid server response.");
                            return;
                      }
                      auto json = jsonRes.unwrap();
                      bool success = json["success"].asBool().unwrapOrDefault();
                      std::string message = json["message"].asString().unwrapOrDefault();
                      if (success || message == "Event set successfully") {
                            popup->showSuccessMessage("Event set: " + type);
                      } else {
                            popup->showFailMessage("Failed to set event.");
                      }
                });
          });
}

ModRatePopup* ModRatePopup::create(std::string title, GJGameLevel* level) {
      auto ret = new ModRatePopup();

      if (ret && ret->initAnchored(380.f, 180.f, title, level, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      };

      CC_SAFE_DELETE(ret);
      return nullptr;
};