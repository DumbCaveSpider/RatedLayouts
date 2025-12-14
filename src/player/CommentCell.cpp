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
      return p;
}

static void cacheUserProfile(int accountId, int role, int stars) {
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
      root[fmt::format("{}", accountId)] = obj;

      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(geode::utils::string::pathToString(cachePath), jsonString);
      if (writeResult) {
            log::debug("Cached user role {} for account ID: {}", role, accountId);
      }
}

class $modify(RLCommentCell, CommentCell) {
      struct Fields {
            int role = 0;
            int stars = 0;
      };

      void loadFromComment(GJComment* comment) {
            CommentCell::loadFromComment(comment);

            if (!comment) {
                  log::warn("Comment is null");
                  return;
            }

            // check cache first
            auto cachedProfile = getCachedUserProfile(comment->m_accountID);
            if (cachedProfile) {
                  m_fields->role = cachedProfile->role;
                  m_fields->stars = cachedProfile->stars;
                  log::debug("Loaded cached role {} and stars {} for user {}", m_fields->role, m_fields->stars, comment->m_accountID);
                  loadBadgeForComment(comment->m_accountID);
                  applyStarGlow(comment->m_accountID, m_fields->stars);
                  return;
            }

            // disable comment fetching if enabled
            if (!Mod::get()->getSettingValue<bool>("compatibilityMode")) {
                  fetchUserRole(comment->m_accountID);
            }
      }

      void applyCommentTextColor(int accountId) {
            if (m_fields->role == 0) {
                  return;
            }

            if (!m_mainLayer) {
                  log::warn("main layer is null, cannot apply color");
                  return;
            }

            ccColor3B color;
            if (accountId == 7689052) {
                  color = ccc3(150, 255, 255);  // ArcticWoof
            } else if (m_fields->role == 1) {
                  color = ccc3(0, 150, 255);  // mod comment color
            } else if (m_fields->role == 2) {
                  color = ccc3(253, 106, 106);  // admin comment color
            }

            log::debug("Applying comment text color for role: {} in {} mode", m_fields->role, m_compactMode ? "compact" : "non-compact");

            // apply the color to the comment text label
            if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont*>(
                    m_mainLayer->getChildByIDRecursive("comment-text-label"))) {  // compact mode (easy face mode)
                  log::debug("Found comment-text-label, applying color");
                  commentTextLabel->setColor(color);
            } else if (auto textArea = m_mainLayer->getChildByIDRecursive("comment-text-area")) {  // non-compact mode is ass (extreme demon face)
                  log::debug("TextArea found, searching for MultilineBitmapFont child");
                  auto children = textArea->getChildren();
                  if (children && children->count() > 0) {
                        auto child = static_cast<CCNode*>(children->objectAtIndex(0));
                        if (auto multilineFont = typeinfo_cast<MultilineBitmapFont*>(child)) {
                              log::debug("Found MultilineBitmapFont, applying color to nested CCLabelBMFont children");
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
            auto getTask = web::WebRequest()
                               .param("accountId", accountId)
                               .get("https://gdrate.arcticwoof.xyz/commentProfile");

            Ref<RLCommentCell> cellRef = this;  // commentcell ref

            getTask.listen([cellRef, accountId](web::WebResponse* response) {
                  log::debug("Received role response from server for comment");

                  // did this so it doesnt crash if the cell is deleted before
                  // response yea took me a while
                  if (!cellRef) {
                        log::warn("CommentCell has been destroyed, skipping role update");
                        return;
                  }

                  if (!response->ok()) {
                        log::warn("Server returned non-ok status: {}", response->code());
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
                  cellRef->m_fields->role = role;
                  cellRef->m_fields->stars = stars;

                  cacheUserProfile(accountId, role, stars);

                  log::debug("User comment role: {}", role);

                  cellRef->loadBadgeForComment(accountId);
                  cellRef->applyStarGlow(accountId, stars);
            });
      }

      void loadBadgeForComment(int accountId) {
            auto userNameMenu = typeinfo_cast<CCMenu*>(
                m_mainLayer->getChildByIDRecursive("username-menu"));
            if (!userNameMenu) {
                  log::warn("username-menu not found in comment cell");
                  return;
            }
            if (accountId == 7689052) {  // ArcticWoof
                  auto ownerBadgeSprite = CCSprite::create("rlBadgeOwner.png"_spr);
                  ownerBadgeSprite->setScale(0.7f);
                  auto ownerBadgeButton = CCMenuItemSpriteExtra::create(
                      ownerBadgeSprite, this, menu_selector(RLCommentCell::onOwnerBadge));
                  ownerBadgeButton->setID("rl-comment-owner-badge");
                  userNameMenu->addChild(ownerBadgeButton);
            } else if (m_fields->role == 1) {
                  auto modBadgeSprite = CCSprite::create("rlBadgeMod.png"_spr);
                  modBadgeSprite->setScale(0.7f);
                  auto modBadgeButton = CCMenuItemSpriteExtra::create(
                      modBadgeSprite, this, menu_selector(RLCommentCell::onModBadge));

                  modBadgeButton->setID("rl-comment-mod-badge");
                  userNameMenu->addChild(modBadgeButton);
            } else if (m_fields->role == 2) {
                  auto adminBadgeSprite = CCSprite::create("rlBadgeAdmin.png"_spr);
                  adminBadgeSprite->setScale(0.7f);
                  auto adminBadgeButton = CCMenuItemSpriteExtra::create(
                      adminBadgeSprite, this, menu_selector(RLCommentCell::onAdminBadge));
                  adminBadgeButton->setID("rl-comment-admin-badge");
                  userNameMenu->addChild(adminBadgeButton);
            }
            userNameMenu->updateLayout();
            applyCommentTextColor(accountId);
      }

      void onModBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Moderator",
                "This user can <cj>suggest layout levels</c> for <cl>Rated "
                "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to <co>moderate the leaderboards</c>.",
                "OK")
                ->show();
      }

      void onAdminBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Layout Administrator",
                "This user can <cj>rate layout levels</c> for <cl>Rated "
                "Layouts</c>. They can change the <cy>featured ranking on the "
                "featured layout levels.</c>",
                "OK")
                ->show();
      }
      void onOwnerBadge(CCObject* sender) {
            FLAlertLayer::create(
                "Rated Layout Creator",
                "<cf>ArcticWoof</c> is the <ca>Creator and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
                "OK")
                ->show();
      }

      void applyStarGlow(int accountId, int stars) {
            if (stars <= 0) return;
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
                  glow->setOpacity(150);
                  glow->setRotation(90);
                  glow->setScaleX(0.725f);
                  glow->setScaleY(5.f);
            } else {
                  glow->setID(glowId.c_str());
                  glow->setAnchorPoint({0.26f, 0.5f});
                  glow->setPosition({80, 4});
                  glow->setColor({135, 180, 255});
                  glow->setOpacity(150);
                  glow->setRotation(90);
                  glow->setScaleX(1.6f);
                  glow->setScaleY(7.f);
            }
            m_mainLayer->addChild(glow, -2);
      }
};