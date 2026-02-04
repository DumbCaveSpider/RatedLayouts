#include "RLCommunityVotePopup.hpp"
#include "../custom/RLAchievements.hpp"

using namespace geode::prelude;
#include <algorithm>

RLCommunityVotePopup* RLCommunityVotePopup::create() {
      return RLCommunityVotePopup::create(0);
}

RLCommunityVotePopup* RLCommunityVotePopup::create(int levelId) {
      auto popup = new RLCommunityVotePopup();
      popup->m_levelId = levelId;
      if (popup && popup->init()) {
            popup->autorelease();
            return popup;
      }
      delete popup;
      return nullptr;
}

void RLCommunityVotePopup::onSubmit(CCObject*) {
      // prepare payload (plays payload by dex)
      createQuickPopup(
          "Submit Vote",
          "Are you sure you want to submit your vote?\n<cy>You won't be able to change it later.</c>",
          "Cancel", "Submit",
          [this](auto, bool yes) {
                if (yes) {
                      int accountId = GJAccountManager::get()->m_accountID;
                      auto argonToken = Mod::get()->getSavedValue<std::string>("argon_token");
                      if (argonToken.empty()) {
                            Notification::create("Auth required to submit vote", NotificationIcon::Error)->show();
                            return;
                      }
                      int gameplayVote = 0;
                      int originalityVote = 0;
                      int difficultyVote = 0;

                      bool includeGameplay = false;
                      bool includeOriginality = false;
                      bool includeDifficulty = false;

                      if (m_gameplayInput) {
                            auto s = m_gameplayInput->getString();
                            if (!s.empty()) {
                                  gameplayVote = numFromString<int>(s).unwrapOr(0);
                                  gameplayVote = std::clamp(gameplayVote, 1, 30);
                                  includeGameplay = true;
                            }
                      }
                      if (m_originalityInput) {
                            auto s = m_originalityInput->getString();
                            if (!s.empty()) {
                                  originalityVote = numFromString<int>(s).unwrapOr(0);
                                  originalityVote = std::clamp(originalityVote, 1, 10);
                                  includeOriginality = true;
                            }
                      }
                      if (m_difficultyInput) {
                            auto string = m_difficultyInput->getString();
                            if (!string.empty()) {
                                  difficultyVote = numFromString<int>(string).unwrapOr(0);
                                  difficultyVote = std::clamp(difficultyVote, 1, 10);
                                  includeDifficulty = true;
                            }
                      }

                      if (!includeGameplay && !includeOriginality && !includeDifficulty) {
                            Notification::create("No votes provided", NotificationIcon::Warning)->show();
                            return;
                      }

                      matjson::Value body = matjson::Value::object();
                      body["accountId"] = accountId;
                      body["argonToken"] = argonToken;
                      body["levelId"] = m_levelId;
                      if (includeGameplay) body["gameplayScore"] = gameplayVote;
                      if (includeOriginality) body["originalityScore"] = originalityVote;
                      if (includeDifficulty) body["difficultyScore"] = difficultyVote;

                      auto req = web::WebRequest();
                      req.bodyJSON(body);
                      Ref<RLCommunityVotePopup> self = this;
                      self->m_submitVoteTask.spawn(
                          req.post("https://gdrate.arcticwoof.xyz/setSuggestScore"),
                          [self, gameplayVote, originalityVote, difficultyVote](web::WebResponse res) {
                                if (!self) return;
                                if (!res.ok()) {
                                      Notification::create("Failed to submit vote", NotificationIcon::Error)->show();
                                      return;
                                }
                                auto j = res.json();
                                if (!j) {
                                      Notification::create("Invalid submit response", NotificationIcon::Warning)->show();
                                      return;
                                }
                                auto json = j.unwrap();
                                bool success = json["success"].asBool().unwrapOrDefault();
                                if (success) {
                                      if (!self->m_mainLayer || !self->getParent()) {
                                            Notification::create("Vote submitted!", NotificationIcon::Success)->show();
                                            // increment community vote achievements progress and notify achievements system
                                            int oldVotes = Mod::get()->getSavedValue<int>("community_votes");
                                            int votes = oldVotes + 1;
                                            Mod::get()->setSavedValue<int>("community_votes", votes);
                                            log::info("Community votes total submitted: {}", votes);
                                            RLAchievements::onUpdated(RLAchievements::Collectable::Votes, oldVotes, votes);
                                            return;
                                      }

                                      Notification::create("Vote submitted!", NotificationIcon::Success)->show();
                                      // increment community vote achievements progress and notify achievements system
                                      int oldVotes = Mod::get()->getSavedValue<int>("community_votes");
                                      int votes = oldVotes + 1;
                                      Mod::get()->setSavedValue<int>("community_votes", votes);
                                      log::info("Community votes total submitted: {}", votes);
                                      RLAchievements::onUpdated(RLAchievements::Collectable::Votes, oldVotes, votes);

                                      // Optimistically mark inputs as VOTED and disable
                                      if (self->m_gameplayInput && gameplayVote > 0) {
                                            self->m_gameplayInput->setString("VOTED");
                                            self->m_gameplayInput->setEnabled(false);
                                      }
                                      if (self->m_originalityInput && originalityVote > 0) {
                                            self->m_originalityInput->setString("VOTED");
                                            self->m_originalityInput->setEnabled(false);
                                      }
                                      if (self->m_difficultyInput && difficultyVote > 0) {
                                            self->m_difficultyInput->setString("VOTED");
                                            self->m_difficultyInput->setEnabled(false);
                                      }

                                      // Refresh UI by re-fetching data
                                      self->refreshFromServer();
                                } else {
                                      Notification::create("Vote submission failed", NotificationIcon::Error)->show();
                                }
                          });
                }
          });
}

bool RLCommunityVotePopup::init() {
      if (!Popup::init(420.f, 240.f))
            return false;
      setTitle("Rated Layouts Community Vote");
      addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);
      // Use taller vertical rows so label, score and input stack vertically
      float rowW = 100.f;
      float rowH = 120.f;
      float rowSpacing = 8.f;

      // originality row (vertical layout: label top, score under label, input under score)
      auto originalityRow = CCLayer::create();
      originalityRow->setContentSize({rowW, rowH});
      originalityRow->setPosition({10.f, 55.f});
      m_mainLayer->addChild(originalityRow);

      auto originalityLabel = CCLabelBMFont::create("Originality", "chatFont.fnt");
      originalityLabel->setPosition({rowW / 2.f, rowH - 10.f});
      originalityRow->addChild(originalityLabel);

      m_originalityScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      m_originalityScoreLabel->setAnchorPoint({0.5f, 0.5f});
      m_originalityScoreLabel->setPosition({rowW / 2.f, rowH - 40.f});
      m_originalityScoreLabel->setScale(0.7f);
      originalityRow->addChild(m_originalityScoreLabel);

      // numeric text input below score
      m_originalityInput = TextInput::create(80.f, "1-10");
      m_originalityInput->setPosition({rowW / 2.f, rowH - 80.f});
      m_originalityInput->setID("originality-vote-input");
      m_originalityInput->setCommonFilter(CommonFilter::Int);
      originalityRow->addChild(m_originalityInput);

      m_originalityVote = numFromString<int>(m_originalityInput->getString()).unwrapOr(0);

      // difficulty row (vertical layout)
      auto difficultyRow = CCLayer::create();
      difficultyRow->setContentSize({rowW, rowH});
      difficultyRow->setPosition({originalityRow->getPositionX() + 100.f, originalityRow->getPositionY()});
      m_mainLayer->addChild(difficultyRow);

      auto diffLabel = CCLabelBMFont::create("Difficulty", "chatFont.fnt");
      diffLabel->setPosition({rowW / 2.f, rowH - 10.f});
      difficultyRow->addChild(diffLabel);

      m_difficultyScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      m_difficultyScoreLabel->setAnchorPoint({0.5f, 0.5f});
      m_difficultyScoreLabel->setPosition({rowW / 2.f, rowH - 40.f});
      m_difficultyScoreLabel->setScale(0.7f);
      difficultyRow->addChild(m_difficultyScoreLabel);

      // numeric input for difficulty vote
      m_difficultyInput = TextInput::create(80.f, "1-30");
      m_difficultyInput->setPosition({rowW / 2.f, rowH - 80.f});
      m_difficultyInput->setID("difficulty-vote-input");
      m_difficultyInput->setCommonFilter(CommonFilter::Int);
      difficultyRow->addChild(m_difficultyInput);

      m_difficultyVote = numFromString<int>(m_difficultyInput->getString()).unwrapOr(0);

      // gameplay row (vertical layout)
      auto gameplayRow = CCLayer::create();
      gameplayRow->setContentSize({rowW, rowH});
      gameplayRow->setPosition({difficultyRow->getPositionX() + 100.f, originalityRow->getPositionY()});
      m_mainLayer->addChild(gameplayRow);

      auto gpLabel = CCLabelBMFont::create("Gameplay", "chatFont.fnt");
      gpLabel->setPosition({rowW / 2.f, rowH - 10.f});
      gameplayRow->addChild(gpLabel);

      m_gameplayScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      m_gameplayScoreLabel->setAnchorPoint({0.5f, 0.5f});
      m_gameplayScoreLabel->setPosition({rowW / 2.f, rowH - 40.f});
      m_gameplayScoreLabel->setScale(0.7f);
      gameplayRow->addChild(m_gameplayScoreLabel);

      // numeric input for gameplay vote
      m_gameplayInput = TextInput::create(80.f, "1-10");
      m_gameplayInput->setPosition({rowW / 2.f, rowH - 80.f});
      m_gameplayInput->setID("gameplay-vote-input");
      m_gameplayInput->setCommonFilter(CommonFilter::Int);
      gameplayRow->addChild(m_gameplayInput);

      m_gameplayVote = numFromString<int>(m_gameplayInput->getString()).unwrapOr(0);

      // moderators' average difficulty (vertical layout)
      auto modRow = CCLayer::create();
      modRow->setContentSize({rowW, rowH});
      modRow->setPosition({gameplayRow->getPositionX() + 100.f, originalityRow->getPositionY()});
      m_mainLayer->addChild(modRow);

      auto modLabel = CCLabelBMFont::create("Layout Mods'\nAverage Difficulty", "chatFont.fnt");
      modLabel->setAlignment(kCCTextAlignmentCenter);
      modLabel->setPosition({rowW / 2.f, rowH - 5.f});
      modLabel->setScale(0.8f);
      modRow->addChild(modLabel);

      m_modDifficultyLabel = CCLabelBMFont::create("-", "bigFont.fnt");
      m_modDifficultyLabel->setPosition({rowW / 2.f, rowH - 40.f});
      m_modDifficultyLabel->setScale(0.7f);
      modRow->addChild(m_modDifficultyLabel);

      // submit button
      auto submitSpr = ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png");
      auto submitBtn = CCMenuItemSpriteExtra::create(submitSpr, this, menu_selector(RLCommunityVotePopup::onSubmit));
      submitBtn->setPosition({m_mainLayer->getContentSize().width / 2.f, 25.f});
      m_submitBtn = submitBtn;
      m_buttonMenu->addChild(submitBtn);

      // total votes label
      m_totalVotesLabel = CCLabelBMFont::create("Total votes: -", "goldFont.fnt");
      m_totalVotesLabel->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height - 40.f});
      m_totalVotesLabel->setScale(0.7f);
      m_totalVotesLabel->setAlignment(kCCTextAlignmentCenter);
      m_mainLayer->addChild(m_totalVotesLabel, 3);

      // info button
      auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
      infoSpr->setScale(0.7f);
      auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(RLCommunityVotePopup::onInfo));
      infoBtn->setPosition({25.f, 25.f});
      m_buttonMenu->addChild(infoBtn, 3);

      // single toggle for moderators to show/hide all scores at once
      int userRole2 = Mod::get()->getSavedValue<int>("role");
      if (userRole2 == 1 || userRole2 == 2) {
            auto allSpr = CCSprite::createWithSpriteFrameName("hideBtn_001.png");
            allSpr->setOpacity(120);
            m_toggleAllBtn = CCMenuItemSpriteExtra::create(allSpr, this, menu_selector(RLCommunityVotePopup::onToggleAll));
            m_toggleAllBtn->setPosition({m_mainLayer->getScaledContentSize().width - 30.f, 25.f});
            m_buttonMenu->addChild(m_toggleAllBtn, 3);
      }

      // fetch current scores from server
      if (m_levelId > 0) {
            refreshFromServer();
      }

      return true;
}

void RLCommunityVotePopup::refreshFromServer() {
      if (!m_forceShowScores) {
            if (m_originalityScoreLabel) m_originalityScoreLabel->setVisible(false);
            if (m_difficultyScoreLabel) m_difficultyScoreLabel->setVisible(false);
            if (m_gameplayScoreLabel) m_gameplayScoreLabel->setVisible(false);
      } else {
            if (m_originalityScoreLabel) m_originalityScoreLabel->setVisible(true);
            if (m_difficultyScoreLabel) m_difficultyScoreLabel->setVisible(true);
            if (m_gameplayScoreLabel) m_gameplayScoreLabel->setVisible(true);
      }

      Ref<RLCommunityVotePopup> self = this;

      // Then query whether this account has already voted on this level and
      // show/hide score labels accordingly
      matjson::Value voteBody = matjson::Value::object();
      voteBody["accountId"] = GJAccountManager::get()->m_accountID;
      voteBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
      voteBody["levelId"] = m_levelId;

      web::WebRequest voteReq;
      voteReq.bodyJSON(voteBody);
      self->m_getVoteTask.spawn(
          voteReq.post("https://gdrate.arcticwoof.xyz/getVote"),
          [self](web::WebResponse vres) {
                if (!self) return;
                if (!vres.ok()) {
                      return;
                }
                auto vj = vres.json();
                if (!vj) return;
                auto vjson = vj.unwrap();

            if (!self->m_mainLayer || !self->getParent()) {
                  return;
            }

            bool hasGameplay = vjson["hasGameplayVoted"].asBool().unwrapOrDefault();
            bool hasOriginality = vjson["hasOriginalityVoted"].asBool().unwrapOrDefault();
            bool hasDifficulty = vjson["hasDifficultyVoted"].asBool().unwrapOrDefault();
            int totalVotes = vjson["totalVotes"].asInt().unwrapOrDefault();
            if (self->m_totalVotesLabel) {
                  self->m_totalVotesLabel->setString(fmt::format("Total votes: {}", totalVotes).c_str());
            }

            double originalityScore = -1.0;
            if (vjson.contains("originalityScore")) {
                  auto osRes = vjson["originalityScore"].asDouble();
                  if (osRes) originalityScore = osRes.unwrap();
            }
            double difficultyScore = -1.0;
            if (vjson.contains("difficultyScore")) {
                  auto dsRes = vjson["difficultyScore"].asDouble();
                  if (dsRes) difficultyScore = dsRes.unwrap();
            }
            double gameplayScore = -1.0;
            if (vjson.contains("gameplayScore")) {
                  auto gpRes = vjson["gameplayScore"].asDouble();
                  if (gpRes) gameplayScore = gpRes.unwrap();
            }

            if (self->m_originalityScoreLabel) {
                  if (originalityScore >= 0.0) {
                        self->m_originalityScoreLabel->setString(fmt::format("{:.2f}", originalityScore).c_str());
                  }
            }
            if (self->m_difficultyScoreLabel) {
                  if (difficultyScore >= 0.0) {
                        self->m_difficultyScoreLabel->setString(fmt::format("{:.2f}", difficultyScore).c_str());
                  }
            }
            if (self->m_gameplayScoreLabel) {
                  if (gameplayScore >= 0.0) {
                        self->m_gameplayScoreLabel->setString(fmt::format("{:.2f}", gameplayScore).c_str());
                  }
            }

            // moderator difficulty (read-only) - prefer averageDifficulty from getVote
            double avg = -1.0;
            if (vjson.contains("averageDifficulty")) {
                  auto avgRes = vjson["averageDifficulty"].asDouble();
                  if (avgRes) avg = avgRes.unwrap();
            }

            if (self->m_modDifficultyLabel) {
                  if (avg >= 0.0) {
                        if (std::floor(avg) == avg) {
                              self->m_modDifficultyLabel->setString(numToString(static_cast<int>(avg)).c_str());
                        } else {
                              self->m_modDifficultyLabel->setString(fmt::format("{:.1f}", avg).c_str());
                        }
                  } else {
                        self->m_modDifficultyLabel->setString("-");
                  }
            }

            // If moderators forced show, keep labels visible regardless of 'hasX' vote state
            if (self->m_forceShowScores) {
                  if (self->m_gameplayInput) {
                        // do not overwrite moderator's ability to see scores
                        // but keep VOTED status if applicable
                        if (hasGameplay) {
                              self->m_gameplayInput->setString("VOTED");
                              self->m_gameplayInput->setEnabled(false);
                        }
                  }
                  if (self->m_originalityInput) {
                        if (hasOriginality) {
                              self->m_originalityInput->setString("VOTED");
                              self->m_originalityInput->setEnabled(false);
                        }
                  }
                  if (self->m_difficultyInput) {
                        if (hasDifficulty) {
                              self->m_difficultyInput->setString("VOTED");
                              self->m_difficultyInput->setEnabled(false);
                        }
                  }

                  if (self->m_gameplayScoreLabel) self->m_gameplayScoreLabel->setVisible(true);
                  if (self->m_originalityScoreLabel) self->m_originalityScoreLabel->setVisible(true);
                  if (self->m_difficultyScoreLabel) self->m_difficultyScoreLabel->setVisible(true);
            } else {
                  if (hasGameplay) {
                        if (self->m_gameplayInput) {
                              self->m_gameplayInput->setString("VOTED");
                              self->m_gameplayInput->setEnabled(false);
                        }
                        if (self->m_gameplayScoreLabel) self->m_gameplayScoreLabel->setVisible(true);
                  } else {
                        if (self->m_gameplayScoreLabel) self->m_gameplayScoreLabel->setVisible(false);
                  }

                  if (hasOriginality) {
                        if (self->m_originalityInput) {
                              self->m_originalityInput->setString("VOTED");
                              self->m_originalityInput->setEnabled(false);
                        }
                        if (self->m_originalityScoreLabel) self->m_originalityScoreLabel->setVisible(true);
                  } else {
                        if (self->m_originalityScoreLabel) self->m_originalityScoreLabel->setVisible(false);
                  }

                  if (hasDifficulty) {
                        if (self->m_difficultyInput) {
                              self->m_difficultyInput->setString("VOTED");
                              self->m_difficultyInput->setEnabled(false);
                        }
                        if (self->m_difficultyScoreLabel) self->m_difficultyScoreLabel->setVisible(true);
                  } else {
                        if (self->m_difficultyScoreLabel) self->m_difficultyScoreLabel->setVisible(false);
                  }
            }

            // Ensure toggle buttons reflect current visibility state for moderators
            int userRole = Mod::get()->getSavedValue<int>("role");
            if (userRole == 1 || userRole == 2) {
                  if (self->m_toggleAllBtn) self->m_toggleAllBtn->setVisible(true);
            } else {
                  if (self->m_toggleAllBtn) self->m_toggleAllBtn->setVisible(false);
            }

            // If all three categories have already been voted on, disable submit
            if (hasGameplay && hasOriginality && hasDifficulty) {
                  if (self->m_submitBtn) {
                        self->m_submitBtn->setEnabled(false);
                        self->m_submitBtn->setNormalImage(ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_04.png"));
                  }
            } else {
                  if (self->m_submitBtn) {
                        self->m_submitBtn->setEnabled(true);
                        self->m_submitBtn->setNormalImage(ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png"));
                  }
            }

            // When moderators toggle, their actions should not be blocked by 'already voted' state
            if (self->m_toggleAllBtn) self->m_toggleAllBtn->setEnabled(true);
      });
}

void RLCommunityVotePopup::onInfo(CCObject*) {
      MDPopup::create(
          "Community Voting Info",
          "You can vote on <cl>suggested/rated layouts</c> based on three categories:\n\n"
          "<co>Originality (1 to 10)</c>: How original and distinct the layout is.\n\n"
          "<cr>Difficulty (1 to 30)</c>: The difficulty the level is according to your experience.\n\n"
          "<cg>Gameplay (1 to 10)</c>: How fun and enjoyable the layout is overall.\n\n"
          "### Please rate suggested/rated layouts <cg>honestly and fairly</c> based on your experience playing them as <cr>Layout Admins</c> uses this votes to help accurately rate layouts!",
          "OK")
          ->show();
}

void RLCommunityVotePopup::onToggleAll(CCObject* sender) {
      // toggle visibility for all three score labels
      if (!m_originalityScoreLabel || !m_difficultyScoreLabel || !m_gameplayScoreLabel) return;

      bool anyVisible = m_originalityScoreLabel->isVisible() || m_difficultyScoreLabel->isVisible() || m_gameplayScoreLabel->isVisible();
      bool newVis = !anyVisible;
      m_forceShowScores = newVis;

      m_originalityScoreLabel->setVisible(newVis);
      m_difficultyScoreLabel->setVisible(newVis);
      m_gameplayScoreLabel->setVisible(newVis);

      // update toggle visual state if possible
      if (m_toggleAllBtn) {
            auto normal = m_toggleAllBtn->getNormalImage();
            auto spr = static_cast<CCSprite*>(normal);
            if (spr) spr->setOpacity(newVis ? 255 : 120);
      }

      if (newVis) {
            // refresh scores when showing
            refreshFromServer();
      }
}
