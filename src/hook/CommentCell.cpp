#include <Geode/Geode.hpp>
#include <Geode/modify/CommentCell.hpp>

using namespace geode::prelude;

class $modify(RLCommentCell, CommentCell) {
  struct Fields {
    int stars = 0;
    int planets = 0;
    bool supporter = false;
    bool booster = false;
    int nameplate = 0;

    bool isClassicMod = false;
    bool isClassicAdmin = false;
    bool isLeaderboardMod = false;
    bool isPlatMod = false;
    bool isPlatAdmin = false;

    async::TaskHolder<web::WebResponse> m_fetchTask;
    ~Fields() { m_fetchTask.cancel(); }
  };

  void loadFromComment(GJComment *comment) {
    CommentCell::loadFromComment(comment);

    if (!comment) {
      return;
    }

    if (m_accountComment) {
      return;
    }

    fetchUserRole(comment->m_accountID);
  }

  void applyCommentTextColor(int accountId) {
    // nothing to do if user has no special state
    if (!m_fields->supporter && !m_fields->isClassicMod &&
        !m_fields->isClassicAdmin && !m_fields->isLeaderboardMod &&
        !m_fields->isPlatMod && !m_fields->isPlatAdmin && !m_fields->booster) {
      return;
    }

    if (!m_mainLayer) {
      log::warn("main layer is null, cannot apply color");
      return;
    }

    ccColor3B color = {255, 255, 255}; // default white
    // choose highest-priority role for color
    if (accountId == 7689052) {
      color = {150, 255, 255}; // ArcticWoof cyan
    } else if (m_fields->isClassicAdmin) {
      color = {255, 120, 120}; // bright red
    } else if (m_fields->isPlatAdmin) {
      color = {255, 235, 161}; // bright orange
    } else if (m_fields->isLeaderboardMod) {
      color = {183, 255, 183}; // bright green
    } else if (m_fields->isClassicMod) {
      color = {120, 200, 255}; // bright blue
    } else if (m_fields->isPlatMod) {
      color = {234, 255, 143}; // bright cyan
    } else if (m_fields->supporter) {
      color = {255, 200, 255}; // bright pink
    } else if (m_fields->booster) {
      color = {200, 200, 255}; // light purple
    }

    log::debug("Applying comment text color for role: {} in {} mode",
               m_fields->isClassicAdmin ? "admin"
               : m_fields->isClassicMod ? "mod"
               : m_fields->supporter    ? "supporter"
               : m_fields->booster      ? "booster"
                                        : "normal",
               m_compactMode ? "compact" : "non-compact");

    // check for prevter.comment_emojis custom text area first (comment reloaded
    // mod)
    if (auto emojiTextArea = m_mainLayer->getChildByIDRecursive(
            "prevter.comment_emojis/comment-text-area")) {
      log::debug("using comment emoji text area, applying color");
      if (auto label = typeinfo_cast<CCLabelBMFont *>(emojiTextArea)) {
        label->setColor(color);
      } // cant bothered adding colors support for non-compact mode for the
        // emojis mod thingy
    }
    // apply the color to the comment text label
    else if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont *>(
                 m_mainLayer->getChildByIDRecursive(
                     "comment-text-label"))) { // compact mode (easy face mode)
      log::debug("Found comment-text-label, applying color");
      commentTextLabel->setColor(color);
    } else if (auto textArea = m_mainLayer->getChildByIDRecursive(
                   "comment-text-area")) { // non-compact mode is ass (extreme
                                           // demon face)
      log::debug("TextArea found, searching for MultilineBitmapFont child");
      auto children = textArea->getChildren();
      if (children && children->count() > 0) {
        auto child = static_cast<CCNode *>(children->objectAtIndex(0));
        if (auto multilineFont = typeinfo_cast<MultilineBitmapFont *>(child)) {
          log::debug("Found MultilineBitmapFont, applying color");
          auto multilineChildren = multilineFont->getChildren();
          if (multilineChildren) {
            for (std::size_t i = 0; i < multilineChildren->count(); ++i) {
              auto labelChild =
                  static_cast<CCNode *>(multilineChildren->objectAtIndex(i));
              if (auto label = typeinfo_cast<CCLabelBMFont *>(labelChild)) {
                label->setColor(color);
              }
            }
          }
        } else if (auto label = typeinfo_cast<CCLabelBMFont *>(child)) {
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

    auto postTask = web::WebRequest().bodyJSON(body).post(
        "https://gdrate.arcticwoof.xyz/profile");

    Ref<RLCommentCell> cellRef = this; // commentcell ref

    m_fields->m_fetchTask.spawn(std::move(postTask), [cellRef, accountId](
                                                         web::WebResponse
                                                             response) {
      log::debug("Received role response from server for comment");

      // did this so it doesnt crash if the cell is deleted before
      // response yea took me a while
      if (!cellRef) {
        log::warn("CommentCell has been destroyed, skipping role update");
        return;
      }

      if (!response.ok()) {
        log::warn("Server returned non-ok status: {}", response.code());
        if (response.code() == 404) {
          log::debug("Profile not found on server for {}", accountId);
          if (!cellRef)
            return;

          cellRef->m_fields->stars = 0;
          cellRef->m_fields->isClassicMod = false;
          cellRef->m_fields->isClassicAdmin = false;
          cellRef->m_fields->isLeaderboardMod = false;
          cellRef->m_fields->isPlatMod = false;
          cellRef->m_fields->isPlatAdmin = false;

          // remove any role badges if present (very unlikely scenario lol)
          if (cellRef->m_mainLayer) {
            if (auto userNameMenu = static_cast<CCMenu *>(
                    cellRef->m_mainLayer->getChildByIDRecursive(
                        "username-menu"))) {
              if (auto owner =
                      userNameMenu->getChildByID("rl-comment-owner-badge"))
                owner->removeFromParent();
              if (auto badge = userNameMenu->getChildByID(
                      "rl-comment-classic-admin-badge"))
                badge->removeFromParent();
              if (auto badge = userNameMenu->getChildByID(
                      "rl-comment-classic-mod-badge"))
                badge->removeFromParent();
              if (auto badge =
                      userNameMenu->getChildByID("rl-comment-lb-mod-badge"))
                badge->removeFromParent();
              if (auto badge =
                      userNameMenu->getChildByID("rl-comment-plat-admin-badge"))
                badge->removeFromParent();
              if (auto badge =
                      userNameMenu->getChildByID("rl-comment-plat-mod-badge"))
                badge->removeFromParent();
              if (auto admin =
                      userNameMenu->getChildByID("rl-comment-admin-badge"))
                admin->removeFromParent();
              userNameMenu->updateLayout();
            }
            // remove any glow
            auto glowId = fmt::format("rl-comment-glow-{}", accountId);
            if (auto glow = cellRef->m_mainLayer->getChildByIDRecursive(glowId))
              glow->removeFromParent();
          }
        }
        return;
      }

      auto jsonRes = response.json();
      if (!jsonRes) {
        log::warn("Failed to parse JSON response");
        return;
      }

      auto json = jsonRes.unwrap();
      int stars = json["stars"].asInt().unwrapOrDefault();
      int planets = json["planets"].asInt().unwrapOrDefault();
      bool isSupporter = json["isSupporter"].asBool().unwrapOrDefault();
      bool isBooster = json["isBooster"].asBool().unwrapOrDefault();
      int nameplate = json["nameplate"].asInt().unwrapOrDefault();

      // new role flags returned from server
      bool isClassicMod = json["isClassicMod"].asBool().unwrapOrDefault();
      bool isClassicAdmin = json["isClassicAdmin"].asBool().unwrapOrDefault();
      bool isLeaderboardMod =
          json["isLeaderboardMod"].asBool().unwrapOrDefault();
      bool isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
      bool isPlatAdmin = json["isPlatAdmin"].asBool().unwrapOrDefault();

      if (stars == 0 && planets == 0 && !isClassicMod && !isClassicAdmin &&
          !isLeaderboardMod && !isPlatMod && !isPlatAdmin) {
        log::debug("User {} has no role/stars/planets", accountId);
        if (!cellRef)
          return;
        cellRef->m_fields->stars = 0;
        cellRef->m_fields->isClassicMod = false;
        cellRef->m_fields->isClassicAdmin = false;
        cellRef->m_fields->isLeaderboardMod = false;
        cellRef->m_fields->isPlatMod = false;
        cellRef->m_fields->isPlatAdmin = false;
        // remove any role badges and glow only if UI exists
        if (cellRef->m_mainLayer) {
          if (auto userNameMenu = typeinfo_cast<CCMenu *>(
                  cellRef->m_mainLayer->getChildByIDRecursive(
                      "username-menu"))) {
            if (auto owner =
                    userNameMenu->getChildByID("rl-comment-owner-badge"))
              owner->removeFromParent();
            if (auto badge = userNameMenu->getChildByID(
                    "rl-comment-classic-admin-badge"))
              badge->removeFromParent();
            if (auto badge =
                    userNameMenu->getChildByID("rl-comment-classic-mod-badge"))
              badge->removeFromParent();
            if (auto badge =
                    userNameMenu->getChildByID("rl-comment-lb-mod-badge"))
              badge->removeFromParent();
            if (auto badge =
                    userNameMenu->getChildByID("rl-comment-plat-admin-badge"))
              badge->removeFromParent();
            if (auto badge =
                    userNameMenu->getChildByID("rl-comment-plat-mod-badge"))
              badge->removeFromParent();
            if (auto mod = userNameMenu->getChildByID("rl-comment-mod-badge"))
              mod->removeFromParent();
            if (auto admin =
                    userNameMenu->getChildByID("rl-comment-admin-badge"))
              admin->removeFromParent();
            userNameMenu->updateLayout();
          }
          // remove any glow
          auto glowId = fmt::format("rl-comment-glow-{}", accountId);
          if (auto glow = cellRef->m_mainLayer->getChildByIDRecursive(glowId))
            glow->removeFromParent();
        }
        return;
      }

      // nameplate thing
      if (cellRef->m_backgroundLayer && nameplate != 0 &&
          !Mod::get()->getSettingValue<bool>("disableNameplateInComment")) {
        auto nameplateSpr = CCSprite::createWithSpriteFrameName(
            fmt::format("nameplate_{}.png"_spr, nameplate).c_str());
        if (cellRef->m_compactMode) {
          nameplateSpr->setPosition(
              {cellRef->m_backgroundLayer->getContentSize().width / 2,
               cellRef->m_backgroundLayer->getContentSize().height / 2});
          nameplateSpr->setScale(0.925f);
          cellRef->m_backgroundLayer->setOpacity(150);
          cellRef->m_backgroundLayer->addChild(nameplateSpr, -1);
        } else {
          nameplateSpr->setScale(2.f);
          nameplateSpr->setPosition(
              {-20, cellRef->m_backgroundLayer->getContentSize().height / 2});
          cellRef->m_backgroundLayer->setOpacity(150);
          cellRef->m_backgroundLayer->addChild(nameplateSpr, -1);
        }
      }

      cellRef->m_fields->stars = stars;
      cellRef->m_fields->planets = planets;
      cellRef->m_fields->supporter = isSupporter;
      cellRef->m_fields->booster = isBooster;
      cellRef->m_fields->isClassicMod = isClassicMod;
      cellRef->m_fields->isClassicAdmin = isClassicAdmin;
      cellRef->m_fields->isLeaderboardMod = isLeaderboardMod;
      cellRef->m_fields->isPlatMod = isPlatMod;
      cellRef->m_fields->isPlatAdmin = isPlatAdmin;

      log::debug("User comment supporter={}, booster={}, classicMod={}, "
                 "classicAdmin={}, leaderboardMod={}, platMod={}, platAdmin={}",
                 cellRef->m_fields->supporter, cellRef->m_fields->booster,
                 cellRef->m_fields->isClassicMod,
                 cellRef->m_fields->isClassicAdmin,
                 cellRef->m_fields->isLeaderboardMod,
                 cellRef->m_fields->isPlatMod, cellRef->m_fields->isPlatAdmin);

      cellRef->loadBadgeForComment(accountId);
      cellRef->applyCommentTextColor(accountId);
      cellRef->applyStarGlow(accountId, stars, planets);
    });
    // Only update UI if it still exists
    if (cellRef->m_mainLayer) {
      cellRef->loadBadgeForComment(accountId);
      cellRef->applyCommentTextColor(accountId);
      cellRef->applyStarGlow(accountId, cellRef->m_fields->stars,
                             cellRef->m_fields->planets);
    }
  }

  void loadBadgeForComment(int accountId) {
    if (!m_mainLayer) {
      log::warn("main layer is null, cannot load badge for comment");
      return;
    }
    auto userNameMenu = typeinfo_cast<CCMenu *>(
        m_mainLayer->getChildByIDRecursive("username-menu"));
    if (!userNameMenu) {
      log::warn("username-menu not found in comment cell");
      return;
    }
    auto addBadgeItem = [&](CCSprite* sprite, int tag, const char* id) {
      if (!sprite) return;
      if (userNameMenu->getChildByID(id)) {
        return; // already added
      }
      sprite->setScale(0.7f);
      auto btn = CCMenuItemSpriteExtra::create(sprite, this,
          menu_selector(RLCommentCell::onBadgeClicked));
      btn->setTag(tag);
      btn->setID(id);
      userNameMenu->addChild(btn);
    };

    if (m_fields->isClassicAdmin) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgeAdmin01.png"_spr),
          5, "rl-comment-classic-admin-badge:2");
    }
    if (m_fields->isClassicMod) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr),
          6, "rl-comment-classic-mod-badge:3");
    }
    if (m_fields->isLeaderboardMod) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgelbMod01.png"_spr),
          9, "rl-comment-lb-mod-badge:3");
    }
    if (m_fields->isPlatAdmin) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgePlatAdmin01.png"_spr),
          7, "rl-comment-plat-admin-badge:2");
    }
    if (m_fields->isPlatMod) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgePlatMod01.png"_spr),
          8, "rl-comment-plat-mod-badge:3");
    }

    // supporter badge
    if (m_fields->supporter) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgeSupporter.png"_spr),
          3, "rl-comment-supporter-badge:4");
    }

    // booster badge
    if (m_fields->booster) {
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgeBooster.png"_spr),
          4, "rl-comment-booster-badge:4");
    }

    if (accountId == 7689052) { // ArcticWoof
      addBadgeItem(
          CCSprite::createWithSpriteFrameName("RL_badgeOwner.png"_spr),
          10, "rl-comment-owner-badge:1");
    }
    userNameMenu->updateLayout();
    applyCommentTextColor(accountId);
  }

  // show explanatory alert when a badge in a comment is tapped
  void onBadgeClicked(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra *>(sender);
    if (!btn)
      return;
    int tag = btn->getTag();
    switch (tag) {
    case 3: // Supporters
      FLAlertLayer::create(
          "Layout Supporter",
          "<cp>Layout Supporter</c> are those who have supported development "
          "of "
          "<cl>Rated Layouts</c> through <cp>Ko-fi</c> membership donation.",
          "OK")
          ->show();
      break;
    case 4: // Boosters
      FLAlertLayer::create(
          "Layout Booster",
          "<ca>Layout Booster</c> are those who boosted the <cl>Rated "
          "Layouts Discord server</c>, they also have the same "
          "benefits as <cp>Layout Supporter</c>.",
          "OK")
          ->show();
      break;
    case 5: // Classic Admins
      FLAlertLayer::create("Classic Layout Admin",
                           "<cr>Classic Layout Admin</c> has the ability to "
                           "rate, suggest levels "
                           "and <cg>manage Featured Layouts</c> for "
                           "<cc>classic levels</c> of <cl>Rated Layouts</c>.",
                           "OK")
          ->show();
      break;
    case 6: // Classic Mods
      FLAlertLayer::create(
          "Classic Layout Mod",
          "<cb>Classic Layout Moderator</c> can suggest levels "
          "for classic layouts to <cr>Classic Layout Admins</c>.",
          "OK")
          ->show();
      break;
    case 7: // Plat Admins
      FLAlertLayer::create(
          "Platformer Layout Admin",
          "<cr>Platformer Layout Admin</c> has the abilities to rate, suggest "
          "levels and <cg>manage Featured Layouts</c> for "
          "<cc>platformer levels</c> of <cl>Rated Layouts</c>.",
          "OK")
          ->show();
      break;
    case 8: // Plat Mods
      FLAlertLayer::create(
          "Platformer Layout Mod",
          "<cb>Platformer Layout Mod</c> can suggest levels for "
          "<cc>platformer layouts</c> to <cr>Platformer Layout Admins</c>.",
          "OK")
          ->show();
      break;
    case 9: // Leaderboard Mods
      FLAlertLayer::create(
          "LB Layout Mod",
          "<cb>Leaderboard Layout Mod</c> is responsible for <co>managing and "
          "moderating the leaderboard</c> section of <cl>Rated Layouts</c>.",
          "OK")
          ->show();
      break;
    case 10: // Owner
      FLAlertLayer::create(
          "Rated Layouts Owner",
          "<cf>ArcticWoof</c> is the creator and owner of "
          "<cl>Rated Layouts</c>. He is the main developer and maintainer of "
          "this <cp>Geode Mod</c> and has the ability to <cg>promote "
          "users</c>.",
          "OK")
          ->show();
      break;
    default:
      break;
    }
  }

  void applyStarGlow(int accountId, int stars, int planets) {
    if (stars <= 0 && planets <= 0)
      return;
    if (!m_mainLayer)
      return;
    auto glowId = fmt::format("rl-comment-glow-{}", accountId);
    // don't create duplicate glow
    if (m_mainLayer->getChildByIDRecursive(glowId))
      return;
    auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
    if (!glow)
      return;
    if (m_accountComment)
      return; // no glow for account comments
    if (Mod::get()->getSettingValue<bool>("disableCommentGlow"))
      return;
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