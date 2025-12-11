#include <Geode/Geode.hpp>
#include <Geode/modify/CommentCell.hpp>

using namespace geode::prelude;

// helper functions for caching user roles cuz caching is good.
static std::string getUserRoleCachePath() {
      auto saveDir = dirs::getModsSaveDir();
      return geode::utils::string::pathToString(saveDir / "user_role_cache.json");
}

static std::optional<int> getCachedUserRole(int accountId) {
      auto cachePath = getUserRoleCachePath();
      auto data = utils::file::readString(cachePath);
      if (!data) return std::nullopt;

      auto json = matjson::parse(data.unwrap());
      if (!json) return std::nullopt;

      auto root = json.unwrap();
      if (root.isObject() && root.contains(fmt::format("{}", accountId))) {
            return root[fmt::format("{}", accountId)].asInt().unwrapOrDefault();
      }
      return std::nullopt;
}

static void cacheUserRole(int accountId, int role) {
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

      root[fmt::format("{}", accountId)] = role;

      auto jsonString = root.dump();
      auto writeResult = utils::file::writeString(geode::utils::string::pathToString(cachePath), jsonString);
      if (writeResult) {
            log::debug("Cached user role {} for account ID: {}", role, accountId);
      }
}

class $modify(RLCommentCell, CommentCell) {
      struct Fields {
            int role = 0;
      };

      void loadFromComment(GJComment* comment) {
            CommentCell::loadFromComment(comment);

            if (!comment) {
                  log::warn("Comment is null");
                  return;
            }

            // check cache first
            auto cachedRole = getCachedUserRole(comment->m_accountID);
            if (cachedRole) {
                  m_fields->role = cachedRole.value();
                  log::debug("Loaded cached role {} for user {}", m_fields->role, comment->m_accountID);
                  loadBadgeForComment(comment->m_accountID);
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

            log::debug("Applying comment text color for role: {}", m_fields->role);

            if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont*>(
                    m_mainLayer->getChildByID("comment-text-label"))) {
                  log::debug("Found comment-text-label, applying color");
                  commentTextLabel->setColor(color);
            }
      }

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
                  cellRef->m_fields->role = role;

                  cacheUserRole(accountId, role);

                  log::debug("User comment role: {}", role);

                  cellRef->loadBadgeForComment(accountId);
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
};