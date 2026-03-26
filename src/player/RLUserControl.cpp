#include "RLUserControl.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"
#include "Geode/ui/Popup.hpp"

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

const int buttonWidth = 250.f;

// helper to update a promotion-style button background and enabled state
static void setPromoBtnState(CCMenuItemSpriteExtra* btn,
    const std::string& text,
    bool enable) {
    if (!btn)
        return;
    btn->setEnabled(enable);
    btn->setVisible(true);
    std::string bg = enable ? "GJ_button_01.png" : "GJ_button_04.png";
    btn->setNormalImage(ButtonSprite::create(
        text.c_str(), buttonWidth, true, "goldFont.fnt", bg.c_str(), 30.f, 1.f));
}

CCMenuItemSpriteExtra* RLUserControl::createUserOptionButton(
    const std::string& text,
    cocos2d::SEL_MenuHandler cb) {
    auto spr = ButtonSprite::create(text.c_str(), buttonWidth, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
    if (spr)
        spr->updateBGImage("GJ_button_01.png");

    auto item = CCMenuItemSpriteExtra::create(spr, this, cb);
    return item;
}

void RLUserControl::createOptionButtons() {
    if (!m_optionsMenu)
        return;

    // helper to create styled action buttons
    auto makeActionButton = [this](const std::string& text,
                                cocos2d::SEL_MenuHandler cb)
        -> std::pair<ButtonSprite*, CCMenuItemSpriteExtra*> {
        auto spr = ButtonSprite::create(text.c_str(), buttonWidth, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
        if (spr)
            spr->updateBGImage("GJ_button_01.png");
        auto item = CCMenuItemSpriteExtra::create(spr, this, cb);
        return {spr, item};
    };

    // options
    auto addOption = [&](const std::string& key, const std::string& text) {
        auto [spr, btn] = makeActionButton(text, menu_selector(RLUserControl::onOptionAction));
        if (!btn)
            return;
        btn->setVisible(false);
        btn->setEnabled(false);
        m_optionsMenu->addChild(btn);
        m_userOptions[key] = {btn, spr, false, false};
    };

    // promotion
    auto addPromote = [&](CCMenuItemSpriteExtra*& target,
                          const std::string& text,
                          cocos2d::SEL_MenuHandler cb) {
        auto [spr, btn] = makeActionButton(text, cb);
        if (!btn)
            return;
        btn->setVisible(false);
        btn->setEnabled(false);
        m_optionsMenu->addChild(btn);
        target = btn;
    };

    // single options button
    addOption("exclude", "Leaderboard Exclude");
    addOption("blacklisted", "Report Blacklist");
    addOption("whitelist", "Set/Remove Leaderboard Whitelist");
    addOption("bannedCreator", "Creator Ban");

    // Promotion/demotion buttons
    addPromote(m_promoteLeaderboardModButton, "Promote to LB Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_promoteClassicModButton, "Promote to Classic Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_promoteClassicAdminButton, "Promote to Classic Admin", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_promotePlatModButton, "Promote to Plat Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_promotePlatAdminButton, "Promote to Plat Admin", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_promoteLeaderboardAdminButton, "Promote to LB Admin", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demoteClassicModButton, "Demote Classic Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demoteClassicAdminButton, "Demote Classic Admin", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demotePlatModButton, "Demote Plat Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demotePlatAdminButton, "Demote Plat Admin", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demoteLBModButton, "Demote LB Mod", menu_selector(RLUserControl::onPromoteAction));
    addPromote(m_demoteLBAdminButton, "Demote LB Admin", menu_selector(RLUserControl::onPromoteAction));

    // dev only buttons
    if (rl::isUserOwner()) {
        auto [spr, btn] = makeActionButton("Wipe User Data", menu_selector(RLUserControl::onWipeAction));
        if (btn) {
            btn->setVisible(false);
            btn->setEnabled(false);
            m_optionsMenu->addChild(btn);
            m_wipeButton = btn;
        }
    }
}

void RLUserControl::rebuildUserOptions() {
    if (!m_optionsMenu)
        return;

    std::unordered_map<std::string, std::pair<bool, bool>> oldState;
    for (auto& kv : m_userOptions) {
        oldState[kv.first] = {kv.second.persisted, kv.second.desired};
    }

    // remove everything from parent and reset pointers
    m_optionsMenu->removeAllChildrenWithCleanup(true);
    m_userOptions.clear();
    m_wipeButton = nullptr;
    m_promoteLeaderboardModButton = nullptr;
    m_promoteLeaderboardAdminButton = nullptr;
    m_promoteClassicModButton = nullptr;
    m_promoteClassicAdminButton = nullptr;
    m_promotePlatModButton = nullptr;
    m_promotePlatAdminButton = nullptr;
    m_demoteClassicModButton = nullptr;
    m_demoteClassicAdminButton = nullptr;
    m_demotePlatModButton = nullptr;
    m_demotePlatAdminButton = nullptr;
    m_demoteLBModButton = nullptr;
    m_demoteLBAdminButton = nullptr;

    createOptionButtons();

    m_isRebuildingOptions = true;
    for (auto& kv : oldState) {
        auto key = kv.first;
        auto persisted = kv.second.first;
        auto desired = kv.second.second;
        if (auto opt = getOptionByKey(key)) {
            opt->persisted = persisted;
            opt->desired = desired;
            setOptionState(key, desired, true);
        }
    }
    m_isRebuildingOptions = false;

    updateOptionVisibility();
    if (m_optionsMenu)
        m_optionsMenu->updateLayout();
}

void RLUserControl::updateOptionVisibility() {
    auto whitelistOpt = getOptionByKey("whitelist");
    auto excludeOpt = getOptionByKey("exclude");
    bool isWhitelistActive = whitelistOpt && whitelistOpt->desired;
    bool isExcludeActive = excludeOpt && excludeOpt->desired;

    for (auto& kv : m_userOptions) {
        auto key = kv.first;
        auto& opt = kv.second;
        if (!opt.actionButton)
            continue;

        if (rl::isUserOwner()) {
            opt.actionButton->setVisible(true);
            opt.actionButton->setEnabled(true);
            continue;
        }

        bool show = false;
        if (key == "exclude") {
            show = rl::isUserLeaderboardAdmin() || rl::isUserLeaderboardMod();
        } else if (key == "whitelist") {
            show = rl::isUserLeaderboardAdmin();
        } else if (key == "blacklisted" || key == "bannedCreator") {
            show = rl::isUserClassicAdmin() || rl::isUserPlatformerAdmin();
        }

        opt.actionButton->setVisible(show);

        bool enabled = show;
        if ((key == "exclude" && isWhitelistActive) || (key == "whitelist" && isExcludeActive)) {
            enabled = false;
        }

        opt.actionButton->setEnabled(enabled);


        if (key == "exclude" && isWhitelistActive) {
            setOptionEnabled("exclude", false);
        }
        if (key == "whitelist" && isExcludeActive) {
            setOptionEnabled("whitelist", false);
        }
    }

    if (m_wipeButton) {
        m_wipeButton->setVisible(rl::isUserOwner());
        m_wipeButton->setEnabled(rl::isUserOwner());
    }

    // promote/demote button visibility by permission level
    auto adminOrOwnerOnly = [&](CCMenuItemSpriteExtra* btn, bool cond) {
        if (!btn)
            return;
        btn->setVisible(cond);
        btn->setEnabled(cond);
    };

    auto modOrOwnerOnly = [&](CCMenuItemSpriteExtra* btn, bool cond) {
        if (!btn)
            return;
        btn->setVisible(cond);
        btn->setEnabled(cond);
    };

    adminOrOwnerOnly(m_promoteClassicAdminButton, rl::isUserOwner());
    adminOrOwnerOnly(m_promotePlatAdminButton, rl::isUserOwner());
    adminOrOwnerOnly(m_promoteLeaderboardAdminButton, rl::isUserOwner());

    modOrOwnerOnly(m_promoteClassicModButton, rl::isUserClassicAdmin() || rl::isUserOwner());
    modOrOwnerOnly(m_promotePlatModButton, rl::isUserPlatformerAdmin() || rl::isUserOwner());
    modOrOwnerOnly(m_promoteLeaderboardModButton, rl::isUserLeaderboardAdmin() || rl::isUserOwner());

    adminOrOwnerOnly(m_demoteClassicAdminButton, rl::isUserOwner());
    adminOrOwnerOnly(m_demotePlatAdminButton, rl::isUserOwner());
    adminOrOwnerOnly(m_demoteLBAdminButton, rl::isUserOwner());

    modOrOwnerOnly(m_demoteClassicModButton, rl::isUserClassicAdmin() || rl::isUserOwner());
    modOrOwnerOnly(m_demotePlatModButton, rl::isUserPlatformerAdmin() || rl::isUserOwner());
    modOrOwnerOnly(m_demoteLBModButton, rl::isUserLeaderboardAdmin() || rl::isUserOwner());

    // Final exclusive lock enforcement: if one option is active, the other is disabled.
    if (whitelistOpt && whitelistOpt->desired) {
        setOptionEnabled("exclude", false);
        if (excludeOpt && excludeOpt->actionButton)
            excludeOpt->actionButton->setEnabled(false);
    }
    if (excludeOpt && excludeOpt->desired) {
        setOptionEnabled("whitelist", false);
        if (whitelistOpt && whitelistOpt->actionButton)
            whitelistOpt->actionButton->setEnabled(false);
    }
}

RLUserControl* RLUserControl::create(int accountId) {
    auto ret = new RLUserControl();
    ret->m_targetAccountId = accountId;

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLUserControl::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square04.png"))
        return false;
    setTitle("Rated Layouts User Mod Panel");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);
    m_noElasticity = true;

    std::string username =
        GameLevelManager::get()->tryGetUsername(m_targetAccountId);
    if (username.empty()) {
        username = "Unknown User";
    }
    // username label
    auto usernameLabel = CCLabelBMFont::create(
        ("Target: " + username).c_str(), "bigFont.fnt", m_mainLayer->getContentSize().width - 40, kCCTextAlignmentCenter);
    usernameLabel->setPosition(
        {m_title->getPositionX(), m_title->getPositionY() - 20});
    usernameLabel->limitLabelWidth(m_mainLayer->getContentWidth(), .5f, 0.3f);
    m_mainLayer->addChild(usernameLabel);

    auto optionsMenu = CCMenu::create();
    optionsMenu->setPosition({m_mainLayer->getContentSize().width / 2,
        m_mainLayer->getContentSize().height / 2 - 15});
    optionsMenu->setContentSize({m_mainLayer->getContentSize().width - 60, 190});
    optionsMenu->setLayout(RowLayout::create()
            ->setGap(6.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false));

    m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
    m_optionsMenu = optionsMenu;

    auto menuBg = NineSlice::create("geode.loader/white-square.png");
    menuBg->setContentSize({optionsMenu->getContentSize().width + 5,
        optionsMenu->getContentSize().height + 10});
    menuBg->setPosition(optionsMenu->getPosition());
    menuBg->setOpacity(200);
    menuBg->setColor({0, 0, 0});
    m_mainLayer->addChild(menuBg, 1);

    auto spinner = LoadingSpinner::create(60.f);
    spinner->setPosition({m_mainLayer->getContentSize().width / 2.f,
        m_mainLayer->getContentSize().height / 2.f});
    m_mainLayer->addChild(spinner, 5);
    m_spinner = spinner;

    // create and configure buttons for the options menu
    createOptionButtons();

    // If there's no profile to load (no target account), show dev buttons
    // immediately
    if (m_targetAccountId <= 0 && rl::isUserOwner()) {
        setPromoBtnState(m_promoteLeaderboardModButton, "Promote to LB Mod", true);
        setPromoBtnState(m_promoteClassicModButton, "Promote to Classic Mod", true);
        setPromoBtnState(m_promoteClassicAdminButton, "Promote to Classic Admin", true);
        setPromoBtnState(m_promotePlatModButton, "Promote to Plat Mod", true);
        setPromoBtnState(m_promotePlatAdminButton, "Promote to Plat Admin", true);
        setPromoBtnState(m_promoteLeaderboardAdminButton, "Promote to LB Admin", true);
        setPromoBtnState(m_demoteClassicModButton, "Demote Classic Mod", true);
        setPromoBtnState(m_demoteClassicAdminButton, "Demote Classic Admin", true);
        setPromoBtnState(m_demotePlatModButton, "Demote Platformer Mod", true);
        setPromoBtnState(m_demotePlatAdminButton, "Demote Platformer Admin", true);
        setPromoBtnState(m_demoteLBModButton, "Demote LB Mod", true);
        setPromoBtnState(m_demoteLBAdminButton, "Demote LB Admin", true);
        if (m_wipeButton) {
            m_wipeButton->setVisible(true);
            m_wipeButton->setEnabled(false);
        }
    }

    m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
    optionsMenu->updateLayout();
    m_mainLayer->addChild(optionsMenu, 2);

    optionsMenu->updateLayout();

    // fetch profile data to determine initial excluded state
    if (m_targetAccountId > 0) {
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["argonToken"] =
            Mod::get()->getSavedValue<std::string>("argon_token");
        jsonBody["accountId"] = m_targetAccountId;

        auto postReq = web::WebRequest();
        postReq.bodyJSON(jsonBody);
        Ref<RLUserControl> self = this;
        m_profileTask.spawn(
            postReq.post("https://gdrate.arcticwoof.xyz/profile"),
            [self](web::WebResponse response) {
                if (!self)
                    return;  // popup destroyed or otherwise invalid

                // default values if any part of the fetch fails
                bool isExcluded = false;
                bool isBlacklisted = false;
                bool isBanned = false;
                bool isLBMod = false;
                bool isClassicMod = false;
                bool isClassicAdmin = false;
                bool isLbAdmin = false;
                bool isPlatMod = false;
                bool isPlatAdmin = false;
                bool isWhitelisted = false;

                if (!response.ok()) {
                    log::warn("Profile fetch returned non-ok status: {}",
                        response.code());
                    return;
                }
                auto jsonRes = response.json();
                if (!jsonRes) {
                    log::warn("Failed to parse JSON response for profile");
                    return;
                }

                auto json = jsonRes.unwrap();

                isExcluded = json["excluded"].asBool().unwrapOrDefault();
                isBlacklisted = json["blacklisted"].asBool().unwrapOrDefault();
                isBanned = json["banned"].asBool().unwrapOrDefault();
                isLBMod = json["isLeaderboardMod"].asBool().unwrapOrDefault();
                isClassicMod = json["isClassicMod"].asBool().unwrapOrDefault();
                isClassicAdmin =
                    json["isClassicAdmin"].asBool().unwrapOrDefault();
                isLbAdmin =
                    json["isLeaderboardAdmin"].asBool().unwrapOrDefault();
                isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
                isPlatAdmin = json["isPlatAdmin"].asBool().unwrapOrDefault();
                isWhitelisted = json["whitelisted"].asBool().unwrapOrDefault();

                // Apply option states
                self->m_isInitializing = true;
                self->setOptionState("exclude", isExcluded, true);
                self->setOptionState("blacklisted", isBlacklisted, true);
                self->setOptionState("bannedCreator", isBanned, true);
                self->setOptionState("whitelist", isWhitelisted, true);

                // ensure exclude is disabled while whitelist is set
                if (isWhitelisted) {
                    self->setOptionEnabled("exclude", false);
                }

                // show and enable option buttons now that profile loading has
                // finished (successfully or not)
                if (self->m_spinner)
                    self->m_spinner->setVisible(false);

                self->updateOptionVisibility();

                self->m_targetIsLeaderboardMod = isLBMod;
                self->m_targetIsLeaderboardAdmin = isLbAdmin;
                self->m_targetIsClassicMod = isClassicMod;
                self->m_targetIsClassicAdmin = isClassicAdmin;
                self->m_targetIsPlatMod = isPlatMod;
                self->m_targetIsPlatAdmin = isPlatAdmin;

                // Update wipe button appearance and enabled state based on exclude
                // state
                auto excludeOpt = self->getOptionByKey("exclude");
                if (self->m_wipeButton) {
                    if (!excludeOpt || !excludeOpt->persisted) {
                        self->m_wipeButton->setNormalImage(ButtonSprite::create(
                            "Wipe User Data", buttonWidth, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                        self->m_wipeButton->setEnabled(false);
                    } else {
                        // restore normal active appearance
                        self->m_wipeButton->setNormalImage(ButtonSprite::create(
                            "Wipe User Data", buttonWidth, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                        self->m_wipeButton->setEnabled(true);
                    }
                }

                if (self->m_wipeButton) {
                    self->m_wipeButton->setVisible(rl::isUserOwner());
                }
                if (!(rl::isUserLeaderboardAdmin() || rl::isUserOwner()) && self->m_promoteLeaderboardModButton) {
                    self->m_promoteLeaderboardModButton->removeFromParentAndCleanup(true);
                    self->m_promoteLeaderboardModButton = nullptr;
                }
                if (!(rl::isUserClassicAdmin() || rl::isUserOwner()) && self->m_promoteClassicModButton) {
                    self->m_promoteClassicModButton->removeFromParentAndCleanup(true);
                    self->m_promoteClassicModButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_promoteClassicAdminButton) {
                    self->m_promoteClassicAdminButton->removeFromParentAndCleanup(true);
                    self->m_promoteClassicAdminButton = nullptr;
                }
                if (!(rl::isUserPlatformerAdmin() || rl::isUserOwner()) && self->m_promotePlatModButton) {
                    self->m_promotePlatModButton->removeFromParentAndCleanup(true);
                    self->m_promotePlatModButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_promotePlatAdminButton) {
                    self->m_promotePlatAdminButton->removeFromParentAndCleanup(true);
                    self->m_promotePlatAdminButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_promoteLeaderboardAdminButton) {
                    self->m_promoteLeaderboardAdminButton->removeFromParentAndCleanup(true);
                    self->m_promoteLeaderboardAdminButton = nullptr;
                }
                if (!(rl::isUserClassicAdmin() || rl::isUserOwner()) && self->m_demoteClassicModButton) {
                    self->m_demoteClassicModButton->removeFromParentAndCleanup(true);
                    self->m_demoteClassicModButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_demoteClassicAdminButton) {
                    self->m_demoteClassicAdminButton->removeFromParentAndCleanup(true);
                    self->m_demoteClassicAdminButton = nullptr;
                }
                if (!(rl::isUserPlatformerAdmin() || rl::isUserOwner()) && self->m_demotePlatModButton) {
                    self->m_demotePlatModButton->removeFromParentAndCleanup(true);
                    self->m_demotePlatModButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_demotePlatAdminButton) {
                    self->m_demotePlatAdminButton->removeFromParentAndCleanup(true);
                    self->m_demotePlatAdminButton = nullptr;
                }
                if (!(rl::isUserLeaderboardAdmin() || rl::isUserOwner()) && self->m_demoteLBModButton) {
                    self->m_demoteLBModButton->removeFromParentAndCleanup(true);
                    self->m_demoteLBModButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_demoteLBAdminButton) {
                    self->m_demoteLBAdminButton->removeFromParentAndCleanup(true);
                    self->m_demoteLBAdminButton = nullptr;
                }
                if (!rl::isUserOwner() && self->m_wipeButton) {
                    self->m_wipeButton->removeFromParentAndCleanup(true);
                    self->m_wipeButton = nullptr;
                } else {
                    auto enableIf = [&](CCMenuItemSpriteExtra* btn,
                                        const std::string& text,
                                        bool cond) {
                        if (btn) {
                            setPromoBtnState(btn, text, cond);
                        }
                    };
                    enableIf(self->m_promoteLeaderboardModButton, "Promote to LB Mod", !self->m_targetIsLeaderboardMod);
                    enableIf(self->m_promoteClassicModButton, "Promote to Classic Mod", !self->m_targetIsClassicMod);
                    enableIf(self->m_promoteClassicAdminButton,
                        "Promote to Classic Admin",
                        !self->m_targetIsClassicAdmin);
                    enableIf(self->m_promotePlatModButton, "Promote to Plat Mod", !self->m_targetIsPlatMod);
                    enableIf(self->m_promotePlatAdminButton, "Promote to Plat Admin", !self->m_targetIsPlatAdmin);
                    enableIf(self->m_promoteLeaderboardAdminButton, "Promote to LB Admin", !self->m_targetIsLeaderboardAdmin);

                    enableIf(self->m_demoteClassicModButton, "Demote Classic Mod", self->m_targetIsClassicMod);
                    enableIf(self->m_demoteClassicAdminButton, "Demote Classic Admin", self->m_targetIsClassicAdmin);
                    enableIf(self->m_demotePlatModButton, "Demote Platformer Mod", self->m_targetIsPlatMod);
                    enableIf(self->m_demotePlatAdminButton, "Demote Platformer Admin", self->m_targetIsPlatAdmin);
                    enableIf(self->m_demoteLBModButton, "Demote LB Mod", self->m_targetIsLeaderboardMod);
                    enableIf(self->m_demoteLBAdminButton, "Demote LB Admin", self->m_targetIsLeaderboardAdmin);
                }

                if (self->m_optionsMenu)
                    self->m_optionsMenu->updateLayout();
                self->m_isInitializing = false;
            });
    }

    return true;
}
void RLUserControl::onWipeAction(CCObject* sender) {
    if (m_isInitializing)
        return;
    if (!m_wipeButton)
        return;

    auto excludeOpt = getOptionByKey("exclude");
    if (!excludeOpt || !excludeOpt->persisted) {
        FLAlertLayer::create("Cannot Wipe User",
            "You can only wipe users who are <cr>excluded from "
            "the leaderboard</c>.",
            "OK")
            ->show();
        return;
    }

    // Confirm wipe
    std::string title = fmt::format("Wipe user {}?", m_targetAccountId);
    std::string body =
        fmt::format(
            "Are you sure you want to <cr>wipe</c> the data for user "
            "<cg>{}</c>?\n<cy>This action is irreversible.</c>",
            m_targetAccountId);
    geode::createQuickPopup(
        title.c_str(), body.c_str(), "No", "Wipe", [this](auto, bool yes) {
            if (!yes)
                return;

            auto popup = UploadActionPopup::create(nullptr, "Wiping user data...");
            popup->show();
            Ref<UploadActionPopup> upopup = popup;

            // Get token
            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                upopup->showFailMessage("Authentication token not found");
                return;
            }

            // disable UI and show spinner
            this->setAllOptionsEnabled(false);
            if (this->m_wipeButton)
                this->m_wipeButton->setEnabled(false);
            if (this->m_spinner)
                this->m_spinner->setVisible(true);

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
            jsonBody["argonToken"] = token;
            jsonBody["targetAccountId"] = this->m_targetAccountId;

            log::info("Sending deleteUser request: {}", jsonBody.dump());

            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            Ref<RLUserControl> self = this;
            self->m_deleteUserTask.spawn(
                postReq.post("https://gdrate.arcticwoof.xyz/deleteUser"),
                [self, upopup](web::WebResponse response) {
                    if (!self || !upopup)
                        return;

                    // re-enable UI
                    self->setAllOptionsEnabled(true);
                    if (self->m_wipeButton)
                        self->m_wipeButton->setEnabled(true);

                    if (!response.ok()) {
                        log::warn("deleteUser returned non-ok status: {}",
                            response.code());
                        upopup->showFailMessage(
                            rl::getResponseFailMessage(response, "Failed to delete user"));
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                        return;
                    }

                    auto jsonRes = response.json();
                    if (!jsonRes) {
                        log::warn("Failed to parse deleteUser response");
                        upopup->showFailMessage("Invalid server response");
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                        return;
                    }

                    auto json = jsonRes.unwrap();
                    bool success = json["success"].asBool().unwrapOrDefault();
                    if (success) {
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                        upopup->showSuccessMessage("User data wiped!");
                    } else {
                        upopup->showFailMessage("Failed to delete user");
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                    }
                });
        });
}

void RLUserControl::onPromoteAction(CCObject* sender) {
    if (m_isInitializing)
        return;

    std::string actionText = "";
    std::string confirmText = "";
    std::string titleText = "";
    bool setLBMod = false;
    bool setLBAdmin = false;
    bool setClassicMod = false;
    bool setClassicAdmin = false;
    bool setPlatMod = false;
    bool setPlatAdmin = false;
    bool demoteClassicMod = false;
    bool demoteClassicAdmin = false;
    bool demotePlatMod = false;
    bool demotePlatAdmin = false;
    bool demoteLBMod = false;
    bool demoteLBAdmin = false;

    bool targetHasLB = m_targetIsLeaderboardMod || m_targetIsLeaderboardAdmin;
    bool targetHasClassic = m_targetIsClassicMod || m_targetIsClassicAdmin;
    bool targetHasPlat = m_targetIsPlatMod || m_targetIsPlatAdmin;

    if (sender == m_promoteLeaderboardModButton) {
        actionText = "Promoting to LB Mod...";
        confirmText = "promote this user to leaderboard mod";
        titleText = "Promote User?";
        setLBMod = true;
    } else if (sender == m_promoteClassicModButton) {
        actionText = "Promoting to Classic Mod...";
        confirmText = "promote this user to classic mod";
        titleText = "Promote User?";
        setClassicMod = true;
    } else if (sender == m_promoteClassicAdminButton) {
        actionText = "Promoting to Classic Admin...";
        confirmText = "promote this user to classic admin";
        titleText = "Promote User?";
        setClassicAdmin = true;
    } else if (sender == m_promotePlatModButton) {
        actionText = "Promoting to Plat Mod...";
        confirmText = "promote this user to plat mod";
        titleText = "Promote User?";
        setPlatMod = true;
    } else if (sender == m_promotePlatAdminButton) {
        actionText = "Promoting to Plat Admin...";
        confirmText = "promote this user to plat admin";
        titleText = "Promote User?";
        setPlatAdmin = true;
    } else if (sender == m_promoteLeaderboardAdminButton) {
        actionText = "Promoting to LB Admin...";
        confirmText = "promote this user to LB admin";
        titleText = "Promote User?";
        setLBAdmin = true;
    } else if (sender == m_demoteClassicModButton) {
        actionText = "Demoting Classic mod...";
        confirmText = "demote this user from classic mod role";
        titleText = "Demote Classic Mod?";
        demoteClassicMod = true;
    } else if (sender == m_demoteClassicAdminButton) {
        actionText = "Demoting Classic admin...";
        confirmText = "demote this user from classic admin role";
        titleText = "Demote Classic Admin?";
        demoteClassicAdmin = true;
    } else if (sender == m_demotePlatModButton) {
        actionText = "Demoting Platformer mod...";
        confirmText = "demote this user from platformer mod role";
        titleText = "Demote Platformer Mod?";
        demotePlatMod = true;
    } else if (sender == m_demotePlatAdminButton) {
        actionText = "Demoting Platformer admin...";
        confirmText = "demote this user from platformer admin role";
        titleText = "Demote Platformer Admin?";
        demotePlatAdmin = true;
    } else if (sender == m_demoteLBModButton) {
        actionText = "Demoting LB mod...";
        confirmText = "demote this user from leaderboard mod role";
        titleText = "Demote LB Mod?";
        demoteLBMod = true;
    } else if (sender == m_demoteLBAdminButton) {
        actionText = "Demoting LB admin...";
        confirmText = "demote this user from leaderboard admin role";
        titleText = "Demote LB Admin?";
        demoteLBAdmin = true;
    } else {
        return;  // unknown sender
    }

    createQuickPopup(
        titleText.c_str(),
        ("Are you sure you want to <cg>" + confirmText + "</c>?").c_str(),
        "Cancel",
        "Confirm",
        [this, setLBMod, setLBAdmin, setClassicMod, setClassicAdmin, setPlatMod, setPlatAdmin, demoteClassicMod, demoteClassicAdmin, demotePlatMod, demotePlatAdmin, demoteLBMod, demoteLBAdmin, targetHasLB, targetHasClassic, targetHasPlat, actionText](auto, bool yes) {
            if (!yes)
                return;

            auto popup = UploadActionPopup::create(nullptr, actionText.c_str());
            popup->show();
            Ref<UploadActionPopup> upopup = popup;

            // Get token
            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                upopup->showFailMessage("Authentication token not found");
                return;
            }

            // disable UI and show spinner
            this->setAllOptionsEnabled(false);
            setPromoBtnState(this->m_promoteLeaderboardModButton,
                "Promote to LB Mod",
                false);
            setPromoBtnState(this->m_promoteLeaderboardAdminButton,
                "Promote to LB Admin",
                false);
            setPromoBtnState(this->m_promoteClassicModButton,
                "Promote to Classic Mod",
                false);
            setPromoBtnState(this->m_promoteClassicAdminButton,
                "Promote to Classic Admin",
                false);
            setPromoBtnState(this->m_promotePlatModButton, "Promote to Plat Mod", false);
            setPromoBtnState(this->m_promotePlatAdminButton,
                "Promote to Plat Admin",
                false);
            setPromoBtnState(this->m_demoteClassicModButton, "Demote Classic Mod", false);
            setPromoBtnState(this->m_demoteClassicAdminButton, "Demote Classic Admin", false);
            setPromoBtnState(this->m_demotePlatModButton, "Demote Platformer Mod", false);
            setPromoBtnState(this->m_demotePlatAdminButton, "Demote Platformer Admin", false);
            setPromoBtnState(this->m_demoteLBModButton, "Demote LB Mod", false);
            setPromoBtnState(this->m_demoteLBAdminButton, "Demote LB Admin", false);
            if (this->m_wipeButton)
                this->m_wipeButton->setEnabled(false);
            if (this->m_spinner)
                this->m_spinner->setVisible(true);

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
            jsonBody["argonToken"] = token;
            jsonBody["targetAccountId"] = this->m_targetAccountId;
            if (setLBMod)
                jsonBody["isLeaderboardMod"] = true;
            if (setLBAdmin)
                jsonBody["isLeaderboardAdmin"] = true;
            if (setClassicMod)
                jsonBody["isClassicMod"] = true;
            if (setClassicAdmin)
                jsonBody["isClassicAdmin"] = true;
            if (setPlatMod)
                jsonBody["isPlatMod"] = true;
            if (setPlatAdmin)
                jsonBody["isPlatAdmin"] = true;
            if (demoteClassicMod)
                jsonBody["isClassicMod"] = false;
            if (demoteClassicAdmin)
                jsonBody["isClassicAdmin"] = false;
            if (demotePlatMod)
                jsonBody["isPlatMod"] = false;
            if (demotePlatAdmin)
                jsonBody["isPlatAdmin"] = false;
            if (demoteLBMod)
                jsonBody["isLeaderboardMod"] = false;
            if (demoteLBAdmin)
                jsonBody["isLeaderboardAdmin"] = false;

            log::info("Sending promoteUser request: {}", jsonBody.dump());

            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            Ref<RLUserControl> self = this;
            self->m_promoteUserTask.spawn(
                postReq.post("https://gdrate.arcticwoof.xyz/promoteUser"),
                [self, upopup, setLBMod, setLBAdmin, setClassicMod, setClassicAdmin, setPlatMod, setPlatAdmin, demoteClassicMod, demoteClassicAdmin, demotePlatMod, demotePlatAdmin, demoteLBMod, demoteLBAdmin, targetHasClassic, targetHasPlat, targetHasLB](web::WebResponse response) {
                    if (!self || !upopup)
                        return;

                    // re-enable UI
                    self->setAllOptionsEnabled(true);
                    auto enableBtnIf = [&](CCMenuItemSpriteExtra* btn, const char* text, bool cond) {
                        if (btn)
                            setPromoBtnState(btn, text, cond);
                    };

                    enableBtnIf(self->m_promoteLeaderboardModButton, "Promote to LB Mod", rl::isUserLeaderboardAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_promoteLeaderboardAdminButton, "Promote to LB Admin", rl::isUserOwner());
                    enableBtnIf(self->m_promoteClassicModButton, "Promote to Classic Mod", rl::isUserClassicAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_promoteClassicAdminButton, "Promote to Classic Admin", rl::isUserOwner());
                    enableBtnIf(self->m_promotePlatModButton, "Promote to Plat Mod", rl::isUserPlatformerAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_promotePlatAdminButton, "Promote to Plat Admin", rl::isUserOwner());
                    enableBtnIf(self->m_demoteClassicModButton, "Demote Classic Mod", rl::isUserClassicAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_demoteClassicAdminButton, "Demote Classic Admin", rl::isUserOwner());
                    enableBtnIf(self->m_demotePlatModButton, "Demote Platformer Mod", rl::isUserPlatformerAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_demotePlatAdminButton, "Demote Platformer Admin", rl::isUserOwner());
                    enableBtnIf(self->m_demoteLBModButton, "Demote LB Mod", rl::isUserLeaderboardAdmin() || rl::isUserOwner());
                    enableBtnIf(self->m_demoteLBAdminButton, "Demote LB Admin", rl::isUserOwner());
                    if (self->m_wipeButton)
                        self->m_wipeButton->setEnabled(true);

                    if (!response.ok()) {
                        log::warn("promoteUser returned non-ok status: {}",
                            response.code());

                        if (response.code() == 403) {
                            upopup->showFailMessage(rl::getResponseFailMessage(
                                response, "Permission denied"));
                        }

                        // enforce only safe UI refresh path on 4xx/5xx response.
                        if (response.code() != 403) {
                            self->updateOptionVisibility();
                        }

                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                        return;
                    }

                    auto jsonRes = response.json();
                    if (!jsonRes) {
                        log::warn("Failed to parse promoteUser response");
                        upopup->showFailMessage("Invalid server response");
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                        return;
                    }

                    auto json = jsonRes.unwrap();
                    bool success = json["success"].asBool().unwrapOrDefault();
                    if (success) {
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);

                        // update local state based on which action was taken
                        if (demoteClassicMod) {
                            self->m_targetIsClassicMod = false;
                            upopup->showSuccessMessage("User demoted from Classic mod!");
                        } else if (demoteClassicAdmin) {
                            self->m_targetIsClassicAdmin = false;
                            upopup->showSuccessMessage("User demoted from Classic admin!");
                        } else if (demotePlatMod) {
                            self->m_targetIsPlatMod = false;
                            upopup->showSuccessMessage("User demoted from Platformer mod!");
                        } else if (demotePlatAdmin) {
                            self->m_targetIsPlatAdmin = false;
                            upopup->showSuccessMessage("User demoted from Platformer admin!");
                        } else if (demoteLBMod) {
                            self->m_targetIsLeaderboardMod = false;
                            upopup->showSuccessMessage("User demoted from LB mod!");
                        } else if (demoteLBAdmin) {
                            self->m_targetIsLeaderboardAdmin = false;
                            upopup->showSuccessMessage("User demoted from LB admin!");
                        } else {
                            if (setLBMod) {
                                self->m_targetIsLeaderboardMod = true;
                                upopup->showSuccessMessage("User promoted to LB Mod!");
                            }
                            if (setLBAdmin) {
                                self->m_targetIsLeaderboardAdmin = true;
                                upopup->showSuccessMessage("User promoted to LB Admin!");
                            }
                            if (setClassicMod) {
                                self->m_targetIsClassicMod = true;
                                upopup->showSuccessMessage("User promoted to Classic Mod!");
                            }
                            if (setClassicAdmin) {
                                self->m_targetIsClassicAdmin = true;
                                upopup->showSuccessMessage(
                                    "User promoted to Classic Admin!");
                            }
                            if (setPlatMod) {
                                self->m_targetIsPlatMod = true;
                                upopup->showSuccessMessage("User promoted to Plat Mod!");
                            }
                            if (setPlatAdmin) {
                                self->m_targetIsPlatAdmin = true;
                                upopup->showSuccessMessage("User promoted to Plat Admin!");
                            }
                        }

                        // refresh the developer buttons to reflect new state
                        setPromoBtnState(self->m_promoteLeaderboardModButton,
                            "Promote to LB Mod",
                            !self->m_targetIsLeaderboardMod);
                        setPromoBtnState(self->m_promoteLeaderboardAdminButton,
                            "Promote to LB Admin",
                            !self->m_targetIsLeaderboardAdmin);
                        setPromoBtnState(self->m_promoteClassicModButton,
                            "Promote to Classic Mod",
                            !self->m_targetIsClassicMod);
                        setPromoBtnState(self->m_promoteClassicAdminButton,
                            "Promote to Classic Admin",
                            !self->m_targetIsClassicAdmin);
                        setPromoBtnState(self->m_promotePlatModButton,
                            "Promote to Plat Mod",
                            !self->m_targetIsPlatMod);
                        setPromoBtnState(self->m_promotePlatAdminButton,
                            "Promote to Plat Admin",
                            !self->m_targetIsPlatAdmin);
                        setPromoBtnState(self->m_demoteClassicModButton, "Demote Classic Mod", self->m_targetIsClassicMod);
                        setPromoBtnState(self->m_demoteClassicAdminButton, "Demote Classic Admin", self->m_targetIsClassicAdmin);
                        setPromoBtnState(self->m_demotePlatModButton, "Demote Platformer Mod", self->m_targetIsPlatMod);
                        setPromoBtnState(self->m_demotePlatAdminButton, "Demote Platformer Admin", self->m_targetIsPlatAdmin);
                        setPromoBtnState(self->m_demoteLBModButton, "Demote LB Mod", self->m_targetIsLeaderboardMod);
                        setPromoBtnState(self->m_demoteLBAdminButton, "Demote LB Admin", self->m_targetIsLeaderboardAdmin);
                    } else {
                        upopup->showFailMessage("Failed to update user role");
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);
                    }
                });
        });
}

void RLUserControl::onOptionAction(CCObject* sender) {
    if (m_isInitializing)
        return;

    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!item)
        return;
    if (!item->isEnabled())
        return;

    for (auto& kv : m_userOptions) {
        auto& key = kv.first;
        auto& opt = kv.second;
        if (opt.actionButton == item) {
            bool newDesired = !opt.persisted;

            std::string actionDesc;
            if (key == "exclude") {
                actionDesc = newDesired ? "set leaderboard exclude"
                                        : "remove leaderboard exclude";
            } else if (key == "blacklisted") {
                actionDesc =
                    newDesired ? "set report blacklist" : "remove report blacklist";
            } else if (key == "bannedCreator") {
                actionDesc = newDesired ? "set creator ban" : "remove creator ban";
            } else if (key == "whitelist") {
                actionDesc = newDesired ? "set leaderboard whitelist" : "remove leaderboard whitelist";
            } else {
                actionDesc = newDesired ? "apply this change" : "remove this change";
            }

            std::string title = "Confirm Change";
            std::string body = fmt::format(
                "Are you sure you want to <cy>{}</c> for user <cg>{} ({})</c>?",
                actionDesc,
                GameLevelManager::sharedState()->tryGetUsername(m_targetAccountId),
                m_targetAccountId);

            Ref<RLUserControl> self = this;
            geode::createQuickPopup(title.c_str(), body.c_str(), "Cancel", "Confirm", [self, key, newDesired](auto, bool yes) {
                if (!yes)
                    return;
                auto currentOpt = self->getOptionByKey(key);
                if (!currentOpt)
                    return;

                currentOpt->desired = newDesired;
                self->setOptionState(key, newDesired, false);
                self->applySingleOption(key, newDesired);
            });

            break;
        }
    }
}

RLUserControl::OptionState*
RLUserControl::getOptionByKey(const std::string& key) {
    auto it = m_userOptions.find(key);
    if (it == m_userOptions.end())
        return nullptr;
    return &it->second;
}

void RLUserControl::setOptionState(const std::string& key, bool desired, bool updatePersisted) {
    auto opt = getOptionByKey(key);
    if (!opt)
        return;
    opt->desired = desired;
    if (updatePersisted)
        opt->persisted = desired;

    // update action button visuals and label depending on desired state
    if (opt->actionButton) {
        std::string text;
        std::string bg;
        if (key == "exclude") {
            if (desired) {
                text = "Remove Leaderboard Exclude";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Leaderboard Exclude";
                bg = "GJ_button_01.png";
            }
        }
        if (key == "blacklisted") {
            if (desired) {
                text = "Remove Report Blacklist";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Report Blacklist";
                bg = "GJ_button_01.png";
            }
        }
        if (key == "bannedCreator") {
            if (desired) {
                text = "Remove Creator Ban";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Creator Ban";
                bg = "GJ_button_01.png";
            }
        }
        if (key == "whitelist") {
            if (desired) {
                text = "Remove Leaderboard Whitelist";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Leaderboard Whitelist";
                bg = "GJ_button_01.png";
            }
        }

        // create new sprite and replace normal image so label/background update
        opt->actionSprite = ButtonSprite::create(
            text.c_str(), buttonWidth, true, "goldFont.fnt", bg.c_str(), 30.f, 1.f);
        opt->actionButton->setNormalImage(opt->actionSprite);
    }

    if (m_optionsMenu) {
        m_optionsMenu->updateLayout();
    }

    // whenever whitelist is updated, exclude is locked and styled as disabled
    auto whitelistOpt = getOptionByKey("whitelist");
    auto excludeOpt = getOptionByKey("exclude");
    if (whitelistOpt && whitelistOpt->desired) {
        setOptionEnabled("exclude", false);
        if (excludeOpt && excludeOpt->actionButton) {
            excludeOpt->actionButton->setEnabled(false);
            std::string excludeText = excludeOpt->desired ? "Remove Leaderboard Exclude" : "Set Leaderboard Exclude";
            excludeOpt->actionSprite = ButtonSprite::create(
                excludeText.c_str(), buttonWidth, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            excludeOpt->actionButton->setNormalImage(excludeOpt->actionSprite);
        }
    } else {
        setOptionEnabled("exclude", true);
        if (excludeOpt && excludeOpt->actionButton) {
            std::string excludeText = excludeOpt->desired ? "Remove Leaderboard Exclude" : "Set Leaderboard Exclude";
            std::string excludeBg = excludeOpt->desired ? "GJ_button_02.png" : "GJ_button_01.png";
            excludeOpt->actionSprite = ButtonSprite::create(
                excludeText.c_str(), buttonWidth, true, "goldFont.fnt", excludeBg.c_str(), 30.f, 1.f);
            excludeOpt->actionButton->setNormalImage(excludeOpt->actionSprite);
            excludeOpt->actionButton->setEnabled(true);
        }
    }

    // whenever exclude is updated, whitelist is locked and styled as disabled
    if (excludeOpt && excludeOpt->desired) {
        setOptionEnabled("whitelist", false);
        if (whitelistOpt && whitelistOpt->actionButton) {
            whitelistOpt->actionButton->setEnabled(false);
            std::string whitelistText = whitelistOpt->desired ? "Remove Leaderboard Whitelist" : "Set Leaderboard Whitelist";
            whitelistOpt->actionSprite = ButtonSprite::create(
                whitelistText.c_str(), buttonWidth, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            whitelistOpt->actionButton->setNormalImage(whitelistOpt->actionSprite);
        }
    } else {
        if (whitelistOpt) {
            setOptionEnabled("whitelist", true);
            if (whitelistOpt->actionButton) {
                std::string whitelistText = whitelistOpt->desired ? "Remove Leaderboard Whitelist" : "Set Leaderboard Whitelist";
                std::string whitelistBg = whitelistOpt->desired ? "GJ_button_02.png" : "GJ_button_01.png";
                whitelistOpt->actionSprite = ButtonSprite::create(
                    whitelistText.c_str(), buttonWidth, true, "goldFont.fnt", whitelistBg.c_str(), 30.f, 1.f);
                whitelistOpt->actionButton->setNormalImage(whitelistOpt->actionSprite);
                whitelistOpt->actionButton->setEnabled(true);
            }
        }
    }

    // enforce lock state for gray-out behavior explicitly
    if (whitelistOpt && whitelistOpt->desired && excludeOpt && excludeOpt->actionButton) {
        excludeOpt->actionButton->setEnabled(false);
    }
    if (excludeOpt && excludeOpt->desired && whitelistOpt && whitelistOpt->actionButton) {
        whitelistOpt->actionButton->setEnabled(false);
    }

    if (rl::isUserOwner()) {
        setPromoBtnState(m_promoteClassicAdminButton, "Promote to Classic Admin", !m_targetIsClassicAdmin);
        setPromoBtnState(m_promotePlatAdminButton, "Promote to Platformer Admin", !m_targetIsPlatAdmin);
        setPromoBtnState(m_promoteLeaderboardAdminButton, "Promote to LB Admin", !m_targetIsLeaderboardAdmin);

        setPromoBtnState(m_demoteClassicAdminButton, "Demote Classic Admin", m_targetIsClassicAdmin);
        setPromoBtnState(m_demotePlatAdminButton, "Demote Platformer Admin", m_targetIsPlatAdmin);
        setPromoBtnState(m_demoteLBAdminButton, "Demote LB Admin", m_targetIsLeaderboardAdmin);
    }
    if (rl::isUserClassicAdmin() || rl::isUserOwner()) {
        setPromoBtnState(m_promoteClassicModButton, "Promote to Classic Mod", !m_targetIsClassicMod);
        setPromoBtnState(m_demoteClassicModButton, "Demote Classic Mod", m_targetIsClassicMod);
    }
    if (rl::isUserPlatformerAdmin() || rl::isUserOwner()) {
        setPromoBtnState(m_promotePlatModButton, "Promote to Platformer Mod", !m_targetIsPlatMod);
        setPromoBtnState(m_demotePlatModButton, "Demote Platformer Mod", m_targetIsPlatMod);
    }
    if (rl::isUserLeaderboardAdmin() || rl::isUserOwner()) {
        setPromoBtnState(m_promoteLeaderboardModButton, "Promote to LB Mod", !m_targetIsLeaderboardMod);
        setPromoBtnState(m_demoteLBModButton, "Demote LB Mod", m_targetIsLeaderboardMod);
    }

    if (!m_isRebuildingOptions && !m_isInitializing) {
        updateOptionVisibility();
        if (m_optionsMenu)
            m_optionsMenu->updateLayout();
    }
}

void RLUserControl::setOptionEnabled(const std::string& key, bool enabled) {
    auto opt = getOptionByKey(key);
    if (!opt)
        return;
    if (opt->actionButton)
        opt->actionButton->setEnabled(enabled);
}

void RLUserControl::setAllOptionsEnabled(bool enabled) {
    for (auto& kv : m_userOptions) {
        auto& opt = kv.second;
        if (opt.actionButton)
            opt.actionButton->setEnabled(enabled);
    }
}

void RLUserControl::applySingleOption(const std::string& key, bool value) {
    auto opt = getOptionByKey(key);
    if (!opt)
        return;

    auto popup =
        UploadActionPopup::create(nullptr, fmt::format("Applying {}...", key));
    popup->show();
    Ref<UploadActionPopup> upopup = popup;

    // get token
    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        upopup->showFailMessage("Authentication token not found");
        // revert visual to persisted
        setOptionState(key, opt->persisted, true);
        return;
    }

    // disable this option's button while applying and show center spinner
    setOptionEnabled(key, false);
    if (m_spinner)
        m_spinner->setVisible(true);

    matjson::Value jsonBody = matjson::Value::object();
    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
    jsonBody["argonToken"] = token;
    jsonBody["targetAccountId"] = m_targetAccountId;
    jsonBody[key] = value;

    log::info("Applying option {}={} for account {}", key, value ? "true" : "false", m_targetAccountId);

    auto postReq = web::WebRequest();
    postReq.bodyJSON(jsonBody);
    Ref<RLUserControl> self = this;
    m_setUserTask.spawn(
        postReq.post("https://gdrate.arcticwoof.xyz/setUser"),
        [self, key, value, upopup](web::WebResponse response) {
            if (!self || !upopup)
                return;
            // re-enable buttons
            self->setOptionEnabled(key, true);

            if (!response.ok()) {
                log::warn("setUser returned non-ok status: {}", response.code());
                upopup->showFailMessage(
                    rl::getResponseFailMessage(response, "Failed to update user"));
                // revert visual to persisted
                auto currentOpt = self->getOptionByKey(key);
                if (currentOpt) {
                    self->m_isInitializing = true;
                    self->setOptionState(key, currentOpt->persisted, true);
                    self->m_isInitializing = false;
                }
                if (self->m_spinner)
                    self->m_spinner->setVisible(false);
                self->setOptionEnabled(key, true);
                return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("Failed to parse setUser response");
                upopup->showFailMessage("Invalid server response");
                auto currentOpt = self->getOptionByKey(key);
                if (currentOpt) {
                    self->m_isInitializing = true;
                    self->setOptionState(key, currentOpt->persisted, true);
                    self->m_isInitializing = false;
                }
                if (self->m_spinner)
                    self->m_spinner->setVisible(false);
                self->setOptionEnabled(key, true);
                return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (success) {
                auto currentOpt = self->getOptionByKey(key);
                if (currentOpt) {
                    currentOpt->persisted = value;
                    currentOpt->desired = value;
                    self->m_isInitializing = true;
                    self->setOptionState(key, value, true);
                    self->m_isInitializing = false;
                }
                if (self->m_spinner)
                    self->m_spinner->setVisible(false);
                self->setOptionEnabled(key, true);
                upopup->showSuccessMessage("User has been updated!");
            } else {
                upopup->showFailMessage(
                    rl::getResponseFailMessage(response, "Failed to update user"));
                auto currentOpt = self->getOptionByKey(key);
                if (currentOpt) {
                    self->m_isInitializing = true;
                    self->setOptionState(key, currentOpt->persisted, true);
                    self->m_isInitializing = false;
                }
                if (self->m_spinner)
                    self->m_spinner->setVisible(false);
                self->setOptionEnabled(key, true);
            }
        });
}
