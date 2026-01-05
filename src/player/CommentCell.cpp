#include <Geode/Geode.hpp>
#include <Geode/modify/CommentCell.hpp>

using namespace geode::prelude;

// helper functions for caching user roles cuz caching is good.
static std::string getUserRoleCachePath() {
      auto saveDir = dirs::getModsSaveDir();
      return geode::utils::string::pathToString(saveDir / "user_role_cache.json");
}

struct CachedUserProfile {
      int role = 0;
      int stars = 0;
      int planets = 0;
      bool supporter = false;
};

static std::optional<CachedUserProfile> getCachedUserProfile(int accountId) {
      auto cachePath = getUserRoleCachePath();
      auto data = utils::file::readString(cachePath);
      if (!data) return std::nullopt;

      auto json = matjson::parse(data.unwrap());
      if (!json) return std::nullopt;

      auto root = json.unwrap();
      if (!root.isObject() || !root.contains(fmt::format("{}", accountId))) return std::nullopt;
      auto entry = root[fmt::format("{}", accountId)];
      if (!entry.isObject()) return std::nullopt;  // require cached entry to be an object
      CachedUserProfile p;
      p.role = entry["role"].asInt().unwrapOrDefault();
      p.stars = entry["stars"].asInt().unwrapOrDefault();
      p.planets = entry["planets"].asInt().unwrapOrDefault();
      p.supporter = entry["isSupporter"].asBool().unwrapOrDefault();
      // missing oh no
      if (p.role == 0 && p.stars == 0 && p.planets == 0 && !p.supporter) return std::nullopt;
      return p;
}

static void removeCachedUserProfile(int accountId) {
      auto cachePath = getUserRoleCachePath();
      auto existingData = utils::file::readString(cachePath);
      if (!existingData) return;

      auto parsed = matjson::parse(existingData.unwrap());
      if (!parsed) return;
      auto root = parsed.unwrap();
      if (!root.isObject()) return;
      auto key = fmt::format("{}", accountId);
      if (!root.contains(key)) return;
      root.erase(key);
      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(geode::utils::string::pathToString(cachePath), jsonString);
      if (writeResult) {
            log::debug("Removed cached user profile for account ID: {}", accountId);
      }
}

static void cacheUserProfile(int accountId, int role, int stars, int planets, bool isSupporter) {
      if (role == 0 && stars == 0 && planets == 0 && !isSupporter) {
            removeCachedUserProfile(accountId);
            return;
      }

      auto saveDir = dirs::getModsSaveDir();
      auto createDirResult = utils::file::createDirectory(saveDir);
      if (!createDirResult) {
            log::warn("Failed to create save directory for user role cache");
            return;
      }

      auto cachePath = getUserRoleCachePath();

      matjson::Value root = matjson::Value::object();
      auto existingData = utils::file::readString(cachePath);
      if (existingData) {
            auto parsed = matjson::parse(existingData.unwrap());
            if (parsed) root = parsed.unwrap();
      }

      matjson::Value obj = matjson::Value::object();
      obj["role"] = role;
      obj["stars"] = stars;
      obj["planets"] = planets;
      obj["isSupporter"] = isSupporter;
      root[fmt::format("{}", accountId)] = obj;

      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(geode::utils::string::pathToString(cachePath), jsonString);
      if (writeResult) {
            log::debug("Cached user profile for account ID: {} (role={} stars={} planets={} supporter={})", accountId, role, stars, planets, isSupporter ? 1 : 0);
      }
}

class $modify(RLCommentCell, CommentCell) {
      struct Fields {
            int role = 0;
            int stars = 0;
            int planets = 0;
            bool supporter = false;
      };

      void loadFromComment(GJComment* comment) {
            CommentCell::loadFromComment(comment);

            if (!comment) {
                  log::warn("Comment is null");
                  return;
            }

            // check cache first and populate immediately
            auto cachedProfile = getCachedUserProfile(comment->m_accountID);
            if (cachedProfile) {
                  m_fields->role = cachedProfile->role;
                  m_fields->stars = cachedProfile->stars;
                  m_fields->planets = cachedProfile->planets;
                  m_fields->supporter = cachedProfile->supporter;
                  log::debug("Loaded cached role {} stars {} planets {} for user {}", m_fields->role, m_fields->stars, m_fields->planets, comment->m_accountID);
                  loadBadgeForComment(comment->m_accountID);
                  applyCommentTextColor(comment->m_accountID);
                  applyStarGlow(comment->m_accountID, m_fields->stars, m_fields->planets);

                  // If compatibility mode is disabled, always attempt to refresh the cache from the server
                  if (!Mod::get()->getSettingValue<bool>("compatibilityMode")) {
                        log::debug("compatibilityMode disabled — fetching fresh profile for {}", comment->m_accountID);
                        fetchUserRole(comment->m_accountID);
                  }

                  return;
            }

            // If not cached, fetch profile data unless compatibility mode is enabled
            if (!Mod::get()->getSettingValue<bool>("compatibilityMode")) {
                  fetchUserRole(comment->m_accountID);
            }
      }

      void applyCommentTextColor(int accountId) {
            if (m_fields->role == 0 && !m_fields->supporter) {
                  return;
            }

            if (!m_mainLayer) {
                  log::warn("main layer is null, cannot apply color");
                  return;
            }

            ccColor3B color;
            if (m_fields->supporter) {
                  color = {255, 187, 255};  // supporter color
            } else if (accountId == 7689052) {
                  color = {150, 255, 255};  // ArcticWoof
            } else if (m_fields->role == 1) {
                  color = {156, 187, 255};  // mod comment color
            } else if (m_fields->role == 2) {
                  color = {255, 187, 187};  // admin comment color
            }

            log::debug("Applying comment text color for role: {} in {} mode", m_fields->role, m_compactMode ? "compact" : "non-compact");

            // check for prevter.comment_emojis custom text area first (comment reloaded mod)
            if (auto emojiTextArea = m_mainLayer->getChildByIDRecursive("prevter.comment_emojis/comment-text-area")) {
                  log::debug("using comment emoji text area, applying color");
                  if (auto label = typeinfo_cast<CCLabelBMFont*>(emojiTextArea)) {
                        label->setColor(color);
                  }  // cant bothered adding colors support for non-compact mode for the emojis mod thingy
            }
            // apply the color to the comment text label
            else if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont*>(
                         m_mainLayer->getChildByIDRecursive("comment-text-label"))) {  // compact mode (easy face mode)
                  log::debug("Found comment-text-label, applying color");
                  commentTextLabel->setColor(color);
            } else if (auto textArea = m_mainLayer->getChildByIDRecursive("comment-text-area")) {  // non-compact mode is ass (extreme demon face)
                  log::debug("TextArea found, searching for MultilineBitmapFont child");
                  auto children = textArea->getChildren();
                  if (children && children->count() > 0) {
                        auto child = static_cast<CCNode*>(children->objectAtIndex(0));
                        if (auto multilineFont = typeinfo_cast<MultilineBitmapFont*>(child)) {
                              log::debug("Found MultilineBitmapFont, applying color");
                              auto multilineChildren = multilineFont->getChildren();
                              if (multilineChildren) {
                                    for (std::size_t i = 0; i < multilineChildren->count(); ++i) {
                                          auto labelChild = static_cast<CCNode*>(multilineChildren->objectAtIndex(i));
                                          if (auto label = typeinfo_cast<CCLabelBMFont*>(labelChild)) {
                                                label->setColor(color);
                                          }
                                    }
                              }
                        } else if (auto label = typeinfo_cast<CCLabelBMFont*>(child)) {
                              log::debug("Found CCLabelBMFont child, applying color");
                              label->setColor(color);
                        }
                  }
            }
      };

      void fetchUserRole(int accountId) {
            log::debug("Fetching role for comment user ID: {}", accountId);

            // Use POST with argon token (required) and accountId in JSON body
            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                  log::warn("Argon token missing, aborting role fetch for {}", accountId);
                  return;
            }

            matjson::Value body = matjson::Value::object();
            body["accountId"] = accountId;
            body["argonToken"] = token;

            auto postTask = web::WebRequest()
                                .bodyJSON(body)
                                .post("https://gdrate.arcticwoof.xyz/profile");

            Ref<RLCommentCell> cellRef = this;  // commentcell ref

            postTask.listen([cellRef, accountId](web::WebResponse* response) {
                  log::debug("Received role response from server for comment");

                  // did this so it doesnt crash if the cell is deleted before
                  // response yea took me a while
                  if (!cellRef) {
                        log::warn("CommentCell has been destroyed, skipping role update");
                        return;
                  }

                  if (!response->ok()) {
                        log::warn("Server returned non-ok status: {}", response->code());
                        // If server says the profile doesn't exist (404), remove cached entry
                        if (response->code() == 404) {
                              log::debug("Profile not found on server, removing cached entry for {}", accountId);
                              removeCachedUserProfile(accountId);
                              if (!cellRef) return;
                              cellRef->m_fields->role = 0;
                              cellRef->m_fields->stars = 0;

                              // remove any role badges if present (very unlikely scenario lol)
                              if (cellRef->m_mainLayer) {
                                    if (auto userNameMenu = typeinfo_cast<CCMenu*>(cellRef->m_mainLayer->getChildByIDRecursive("username-menu"))) {
                                          if (auto owner = userNameMenu->getChildByID("rl-comment-owner-badge")) owner->removeFromParent();
                                          if (auto mod = userNameMenu->getChildByID("rl-comment-mod-badge")) mod->removeFromParent();
                                          if (auto admin = userNameMenu->getChildByID("rl-comment-admin-badge")) admin->removeFromParent();
                                          userNameMenu->updateLayout();
                                    }
                                    // remove any glow
                                    auto glowId = fmt::format("rl-comment-glow-{}", accountId);
                                    if (auto glow = cellRef->m_mainLayer->getChildByIDRecursive(glowId)) glow->removeFromParent();
                              }
                        }
                        return;
                  }

                  auto jsonRes = response->json();
                  if (!jsonRes) {
                        log::warn("Failed to parse JSON response");
                        return;
                  }

                  auto json = jsonRes.unwrap();
                  int role = json["role"].asInt().unwrapOrDefault();
                  int stars = json["stars"].asInt().unwrapOrDefault();
                  int planets = json["planets"].asInt().unwrapOrDefault();

                  // If user has no role and no stars and no planets, delete from cache and clear UI
                  if (role == 0 && stars == 0 && planets == 0) {
                        log::debug("User {} has no role/stars/planets - removing from cache", accountId);
                        removeCachedUserProfile(accountId);
                        if (!cellRef) return;
                        cellRef->m_fields->role = 0;
                        cellRef->m_fields->stars = 0;
                        // remove any role badges if present
                        if (cellRef->m_mainLayer) {
                              if (auto userNameMenu = typeinfo_cast<CCMenu*>(cellRef->m_mainLayer->getChildByIDRecursive("username-menu"))) {
                                    if (auto owner = userNameMenu->getChildByID("rl-comment-owner-badge")) owner->removeFromParent();
                                    if (auto mod = userNameMenu->getChildByID("rl-comment-mod-badge")) mod->removeFromParent();
                                    if (auto admin = userNameMenu->getChildByID("rl-comment-admin-badge")) admin->removeFromParent();
                                    userNameMenu->updateLayout();
                              }
                              // remove any glow
                              auto glowId = fmt::format("rl-comment-glow-{}", accountId);
                              if (auto glow = cellRef->m_mainLayer->getChildByIDRecursive(glowId)) glow->removeFromParent();
                        }
                        return;
                  }

                  cellRef->m_fields->role = role;
                  cellRef->m_fields->stars = stars;
                  cellRef->m_fields->planets = planets;
                  cellRef->m_fields->supporter = json["isSupporter"].asBool().unwrapOrDefault();

                  cacheUserProfile(accountId, role, stars, planets, cellRef->m_fields->supporter);

                  log::debug("User comment role: {} supporter={} stars={} planets={}", role, cellRef->m_fields->supporter, stars, planets);

                  cellRef->loadBadgeForComment(accountId);
                  cellRef->applyCommentTextColor(accountId);
                  cellRef->applyStarGlow(accountId, stars, planets);
            });
      }

      void loadBadgeForComment(int accountId) {
            auto userNameMenu = typeinfo_cast<CCMenu*>(
                m_mainLayer->getChildByIDRecursive("username-menu"));
            if (!userNameMenu) {
                  log::warn("username-menu not found in comment cell");
                  return;
            }
            // Avoid creating duplicate badges if one already exists
            if (accountId == 7689052) {  // ArcticWoof
                  if (!userNameMenu->getChildByID("rl-comment-owner-badge")) {
                        auto ownerBadgeSprite = CCSprite::create("RL_badgeOwner.png"_spr);
                        ownerBadgeSprite->setScale(0.7f);
                        auto ownerBadgeButton = CCMenuItemSpriteExtra::create(
                            ownerBadgeSprite, this, menu_selector(RLCommentCell::onOwnerBadge));
                        ownerBadgeButton->setID("rl-comment-owner-badge");
                        userNameMenu->addChild(ownerBadgeButton);
                  }
            } else if (m_fields->role == 1) {
                  if (!userNameMenu->getChildByID("rl-comment-mod-badge")) {
                        auto modBadgeSprite = CCSprite::create("RL_badgeMod01.png"_spr);
                        modBadgeSprite->setScale(0.7f);
                        auto modBadgeButton = CCMenuItemSpriteExtra::create(
                            modBadgeSprite, this, menu_selector(RLCommentCell::onModBadge));

                        modBadgeButton->setID("rl-comment-mod-badge");
                        userNameMenu->addChild(modBadgeButton);
                  }
            } else if (m_fields->role == 2) {
                  if (!userNameMenu->getChildByID("rl-comment-admin-badge")) {
                        auto adminBadgeSprite = CCSprite::create("RL_badgeAdmin01.png"_spr);
                        adminBadgeSprite->setScale(0.7f);
                        auto adminBadgeButton = CCMenuItemSpriteExtra::create(
                            adminBadgeSprite, this, menu_selector(RLCommentCell::onAdminBadge));
                        adminBadgeButton->setID("rl-comment-admin-badge");
                        userNameMenu->addChild(adminBadgeButton);
                  }
            }

            // supporter badge
            if (m_fields->supporter) {
                  if (!userNameMenu->getChildByID("rl-comment-supporter-badge")) {
                        auto supporterSprite = CCSprite::create("RL_badgeSupporter.png"_spr);
                        supporterSprite->setScale(0.7f);
                        auto supporterButton = CCMenuItemSpriteExtra::create(supporterSprite, this, menu_selector(RLCommentCell::onSupporterBadge));
                        supporterButton->setID("rl-comment-supporter-badge");
                        userNameMenu->addChild(supporterButton);
                  }
            }

            userNameMenu->updateLayout();
            applyCommentTextColor(accountId);
      }

      void onModBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Moderator",
                "This user can <cj>suggest layout levels</c> for <cl>Rated "
                "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to <co>moderate the leaderboard</c>.",
                "OK")
                ->show();
      }

      void onAdminBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Administrator",
                "This user can <cj>rate layout levels</c> for <cl>Rated "
                "Layouts</c>. They have the same power as <cg>Moderators</c> but including the ability to change the <cy>featured ranking on the "
                "featured layout levels</c> and <cg>set event layouts</c>.",
                "OK")
                ->show();
      }
      void onOwnerBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Rated Layouts Creator",
                "<cf>ArcticWoof</c> is the <ca>Creator and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
                "OK")
                ->show();
      }
      void onSupporterBadge(CCObject* sender) {
            geode::createQuickPopup(
                "Layout Supporter",
                "This user is a <cp>Layout Supporter</c>! They have supported the development of <cl>Rated Layouts</c> through membership donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via <cp>Ko-Fi</c>",
                "OK",
                "Ko-Fi", [this](auto, bool yes) {
                      if (!yes) return;
                      utils::web::openLinkInBrowser("https://ko-fi.com/arcticwoof");
                });
      }
      void applyStarGlow(int accountId, int stars, int planets) {
            if (stars <= 0 && planets <= 0) return;
            if (!m_mainLayer) return;
            auto glowId = fmt::format("rl-comment-glow-{}", accountId);
            // don't create duplicate glow
            if (m_mainLayer->getChildByIDRecursive(glowId)) return;
            auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
            if (!glow) return;
            if (m_accountComment) return;  // no glow for account comments
            if (Mod::get()->getSettingValue<bool>("disableCommentGlow")) return;
            if (m_compactMode) {
                  glow->setID(glowId.c_str());
                  glow->setAnchorPoint({0.195f, 0.5f});
                  glow->setPosition({100, 10});
                  glow->setColor({135, 180, 255});
                  glow->setOpacity(100);
                  glow->setRotation(90);
                  glow->setScaleX(0.725f);
                  glow->setScaleY(5.f);
            } else {
                  glow->setID(glowId.c_str());
                  glow->setAnchorPoint({0.26f, 0.5f});
                  glow->setPosition({80, 4});
                  glow->setColor({135, 180, 255});
                  glow->setOpacity(100);
                  glow->setRotation(90);
                  glow->setScaleX(1.6f);
                  glow->setScaleY(7.f);
            }
            m_mainLayer->addChild(glow, -2);
      }
};