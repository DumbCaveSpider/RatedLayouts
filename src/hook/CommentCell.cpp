#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/loader/Loader.hpp"
#include "Geode/loader/Mod.hpp"
#include "Geode/ui/NineSlice.hpp"
#include "ccTypes.h"
#include <Geode/Geode.hpp>
#include <Geode/modify/CommentCell.hpp>

#include "../include/RLConstants.hpp"

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
        bool isLeaderboardAdmin = false;
        bool isPlatMod = false;
        bool isPlatAdmin = false;

        async::TaskHolder<web::WebResponse> m_fetchTask;
        ~Fields() { m_fetchTask.cancel(); }
    };

    static void onModify(auto& self) {
        if (!self.setHookPriorityAfterPost("CommentCell::fetchUserRole", "prevter.comment_emojis")) {
            log::warn("Failed to set hook priority from prevter.comment_emojis.");
        }
    }

    void loadFromComment(GJComment* comment) {
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

        ccColor3B color = ccWHITE;  // default white
        // choose highest-priority role for color
        if (accountId == rl::DEV_ACCOUNT_ID) {
            color = {150, 255, 255};  // ArcticWoof cyan
        } else if (m_fields->isClassicAdmin) {
            color = {255, 170, 170};  // bright red
        } else if (m_fields->isPlatAdmin) {
            color = {255, 235, 161};  // bright orange
        } else if (m_fields->isLeaderboardAdmin) {
            color = {140, 255, 140};  // brighter green
        } else if (m_fields->isLeaderboardMod) {
            color = {183, 255, 183};  // bright green
        } else if (m_fields->isClassicMod) {
            color = {120, 200, 255};  // bright blue
        } else if (m_fields->isPlatMod) {
            color = {234, 255, 143};  // bright cyan
        } else if (m_fields->supporter) {
            color = {255, 200, 255};  // bright pink
        } else if (m_fields->booster) {
            color = {200, 200, 255};  // light purple
        }

        log::debug("Applying comment text color for role: {} in {} mode",
            m_fields->isClassicAdmin ? "admin"
            : m_fields->isClassicMod ? "mod"
            : m_fields->supporter    ? "supporter"
            : m_fields->booster      ? "booster"
                                     : "normal",
            m_compactMode ? "compact" : "non-compact");

        // check for prevter.comment_emojis
        if (Loader::get()->isModLoaded("prevter.comment_emojis")) {
            log::debug("prevter.comment_emojis mod detected, looking for custom text area");

            if (auto emojiTextArea = m_mainLayer->getChildByIDRecursive(
                    "prevter.comment_emojis/comment-text-area")) {
                typeinfo_cast<CCRGBAProtocol*>(emojiTextArea)->setColor(color);
            } else if (auto emojiLabel = m_mainLayer->getChildByIDRecursive(
                           "prevter.comment_emojis/comment-text-label")) {
                typeinfo_cast<CCRGBAProtocol*>(emojiLabel)->setColor(color);
            }
            return;
        }

        // vanilla text area/label
        if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont*>(
                m_mainLayer->getChildByIDRecursive(
                    "comment-text-label"))) {  // compact mode (easy face mode)
            log::debug("Found comment-text-label, applying color");
            typeinfo_cast<CCLabelBMFont*>(commentTextLabel)->setColor(color);
        } else if (auto textArea = m_mainLayer->getChildByIDRecursive(
                       "comment-text-area")) {
            typeinfo_cast<TextArea*>(textArea)->colorAllLabels(color);
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
            std::string(rl::BASE_API_URL) + "/profile");

        Ref<RLCommentCell> cellRef = this;  // commentcell ref

        m_fields->m_fetchTask.spawn(std::move(postTask), [cellRef, accountId](web::WebResponse response) {
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
                        if (auto userNameMenu = static_cast<CCMenu*>(
                                cellRef->m_mainLayer->getChildByIDRecursive(
                                    "username-menu"))) {
                            if (auto owner =
                                    userNameMenu->getChildByID("rl-comment-owner-badge"))
                                owner->removeFromParent();
                            if (auto badge = userNameMenu->getChildByID(
                                    "rl-comment-classic-admin-badge"))
                                badge->removeFromParent();
                            if (auto badge =
                                    userNameMenu->getChildByID("rl-comment-plat-admin-badge"))
                                badge->removeFromParent();
                            if (auto badge = userNameMenu->getChildByID(
                                    "rl-comment-classic-mod-badge"))
                                badge->removeFromParent();
                            if (auto badge =
                                    userNameMenu->getChildByID("rl-comment-plat-mod-badge"))
                                badge->removeFromParent();
                            if (auto badge =
                                    userNameMenu->getChildByID("rl-comment-lb-mod-badge"))
                                badge->removeFromParent();
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
                    if (auto userNameMenu = typeinfo_cast<CCMenu*>(
                            cellRef->m_mainLayer->getChildByIDRecursive(
                                "username-menu"))) {
                        if (auto owner =
                                userNameMenu->getChildByID("rl-comment-owner-badge"))
                            owner->removeFromParent();
                        if (auto badge = userNameMenu->getChildByID(
                                "rl-comment-classic-admin-badge"))
                            badge->removeFromParent();
                        if (auto badge =
                                userNameMenu->getChildByID("rl-comment-plat-admin-badge"))
                            badge->removeFromParent();
                        if (auto badge =
                                userNameMenu->getChildByID("rl-comment-classic-mod-badge"))
                            badge->removeFromParent();
                        if (auto badge =
                                userNameMenu->getChildByID("rl-comment-plat-mod-badge"))
                            badge->removeFromParent();
                        if (auto badge =
                                userNameMenu->getChildByID("rl-comment-lb-mod-badge"))
                            badge->removeFromParent();
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
            bool validCell = cellRef && cellRef->m_mainLayer && cellRef->m_backgroundLayer && cellRef->getParent();
            if (validCell && nameplate != 0 &&
                !Mod::get()->getSettingValue<bool>("disableNameplateInComment")) {
                std::string url = fmt::format(
                    "{}/nameplates/banner/nameplate_{}.png",
                    std::string(rl::BASE_API_URL),
                    nameplate);
                if (cellRef->m_compactMode) {
                    auto lazy = LazySprite::create(
                        {cellRef->m_backgroundLayer->getScaledContentSize()},
                        true);
                    lazy->loadFromUrl(url, CCImage::kFmtPng, true);
                    lazy->setAutoResize(true);
                    lazy->setPosition(
                        {cellRef->m_backgroundLayer->getScaledContentSize().width / 2,
                            cellRef->m_backgroundLayer->getScaledContentSize().height / 2});
                    cellRef->m_backgroundLayer->setOpacity(100);
                    cellRef->m_backgroundLayer->addChild(lazy, -1);

                    // add a background behind the comment text for better contrast with bright nameplates in compact mode
                    auto commentBg = NineSlice::create("square02_small.png");
                    auto commentText = cellRef->m_mainLayer->getChildByIDRecursive("comment-text-label");
                    if (commentText) {
                        commentBg->setInsets({5, 5, 5, 5});
                        commentBg->setContentSize(commentText->getScaledContentSize() + CCSize(5, 0));
                        commentBg->setPosition({commentText->getPosition().x - 2, commentText->getPosition().y});
                        commentBg->setOpacity(150);
                        commentBg->setAnchorPoint(commentText->getAnchorPoint());
                        cellRef->m_mainLayer->addChild(commentBg, -1);
                    } else {
                        log::warn("CommentCell compress mode: comment-text-label not found, skipping text background for nameplate");
                    }
                } else {
                    auto lazy = LazySprite::create(
                        {cellRef->m_backgroundLayer->getScaledContentSize() +
                            CCSize(425, 425)},
                        true);
                    lazy->loadFromUrl(url, CCImage::kFmtPng, true);
                    lazy->setAutoResize(true);
                    lazy->setPosition(
                        {-20,
                            cellRef->m_backgroundLayer->getScaledContentSize().height / 2});
                    cellRef->m_backgroundLayer->setOpacity(150);
                    cellRef->m_backgroundLayer->addChild(lazy, -1);
                }
            } else if (!validCell && nameplate != 0) {
                log::debug("Skipping nameplate for account {} because CommentCell was removed or invalid", accountId);
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
            cellRef->m_fields->nameplate = nameplate;

            log::debug(
                "User comment supporter={}, booster={}, classicMod={}, "
                "classicAdmin={}, leaderboardMod={}, platMod={}, platAdmin={}, "
                "nameplate={}",
                cellRef->m_fields->supporter,
                cellRef->m_fields->booster,
                cellRef->m_fields->isClassicMod,
                cellRef->m_fields->isClassicAdmin,
                cellRef->m_fields->isLeaderboardMod,
                cellRef->m_fields->isPlatMod,
                cellRef->m_fields->isPlatAdmin,
                cellRef->m_fields->nameplate);

            cellRef->loadBadgeForComment(accountId);
            cellRef->applyCommentTextColor(accountId);
            cellRef->applyStarGlow(accountId, stars, planets);
        });
        // Only update UI if it still exists
        if (cellRef && cellRef->m_mainLayer) {
            cellRef->loadBadgeForComment(accountId);
            cellRef->applyCommentTextColor(accountId);
            cellRef->applyStarGlow(accountId, cellRef->m_fields->stars, cellRef->m_fields->planets);
        }
    }

    void loadBadgeForComment(int accountId) {
        if (!m_mainLayer) {
            log::warn("main layer is null, cannot load badge for comment");
            return;
        }
        auto userNameMenu = typeinfo_cast<CCMenu*>(
            m_mainLayer->getChildByIDRecursive("username-menu"));
        if (!userNameMenu) {
            log::warn("username-menu not found in comment cell");
            return;
        }
        auto addBadgeItem = [&](CCSprite* sprite, int tag, const char* id) {
            if (!sprite)
                return;
            if (userNameMenu->getChildByID(id)) {
                return;  // already added
            }
            sprite->setScale(0.7f);
            auto btn = CCMenuItemSpriteExtra::create(
                sprite, this, menu_selector(RLCommentCell::onBadgeClicked));
            btn->setTag(tag);
            btn->setID(id);
            userNameMenu->addChild(btn);
        };

        // admins
        if (m_fields->isClassicAdmin) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgeAdmin01.png"_spr), 5, "rl-comment-classic-admin-badge:2");
        }
        if (m_fields->isPlatAdmin) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgePlatAdmin01.png"_spr), 7, "rl-comment-plat-admin-badge:2");
        }
        // mods
        if (m_fields->isClassicMod) {
            addBadgeItem(CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr),
                6,
                "rl-comment-classic-mod-badge:3");
        }
        if (m_fields->isPlatMod) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgePlatMod01.png"_spr), 8, "rl-comment-plat-mod-badge:3");
        }

        // leaderboard mod badge
        if (m_fields->isLeaderboardMod) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgelbMod01.png"_spr), 9, "rl-comment-lb-mod-badge:3");
        }
        if (m_fields->isLeaderboardAdmin) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgelbAdmin01.png"_spr), 11, "rl-comment-lb-admin-badge:2");
        }
        // supporter badge
        if (m_fields->supporter) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgeSupporter.png"_spr), 3, "rl-comment-supporter-badge:4");
        }

        // booster badge
        if (m_fields->booster) {
            addBadgeItem(
                CCSprite::createWithSpriteFrameName("RL_badgeBooster.png"_spr), 4, "rl-comment-booster-badge:4");
        }

        if (accountId == rl::DEV_ACCOUNT_ID) {  // ArcticWoof
            addBadgeItem(CCSprite::createWithSpriteFrameName("RL_badgeOwner.png"_spr),
                10,
                "rl-comment-owner-badge:1");
        }
        userNameMenu->updateLayout();
        applyCommentTextColor(accountId);
    }

    // show explanatory alert when a badge in a comment is tapped
    void onBadgeClicked(CCObject* sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        if (!btn)
            return;
        int tag = btn->getTag();
        switch (tag) {
            case 3:  // Supporters
            rl::showSupporterInfo();
                break;
            case 4:  // Boosters
            rl::showBoosterInfo();
                break;
            case 5:  // Classic Admins
            rl::showClassicAdminInfo();
                break;
            case 6:  // Classic Mods
            rl::showClassicModInfo();
                break;
            case 7:  // Plat Admins
            rl::showPlatAdminInfo();
                break;
            case 8:  // Plat Mods
            rl::showPlatModInfo();
                break;
            case 9:  // Leaderboard Mods
            rl::showLeaderboardModInfo();
                break;
            case 11:  // Leaderboard Admins
            rl::showLeaderboardAdminInfo();
                break;
            case 10:  // Owner
            rl::showOwnerInfo();
                break;
            default:
                break;
        }
    }

    void applyStarGlow(int accountId, int stars, int planets) {
        // skip if no stars/planets
        if (stars <= 0 && planets <= 0)
            return;
        // global disable
        if (Mod::get()->getSettingValue<bool>("disableCommentGlow")) {
            log::debug("Skipping star glow: global setting disabled");
            return;
        }

        if (m_fields->nameplate != 0 &&
            Mod::get()->getSettingValue<bool>("disableCommentGlowNameplate")) {
            if (!Mod::get()->getSettingValue<bool>("disableNameplateInComment")) {
                log::debug(
                    "Skipping star glow for account {} due to nameplate and setting",
                    accountId);
                return;
            }
        }

        if (!m_mainLayer)
            return;

        if (m_accountComment)
            return;  // no glow for account comments

        auto glowId = fmt::format("rl-comment-glow-{}", accountId);
        // don't create duplicate glow
        if (m_mainLayer->getChildByIDRecursive(glowId))
            return;

        auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
        if (!glow)
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
