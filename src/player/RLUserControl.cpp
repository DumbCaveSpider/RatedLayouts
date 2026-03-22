#include "RLUserControl.hpp"
#include "Geode/ui/Popup.hpp"

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

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
        text.c_str(), 200, true, "goldFont.fnt", bg.c_str(), 30.f, 1.f));
}

static std::string getResponseFailMessage(web::WebResponse const& response,
    std::string const& fallback) {
    auto message = response.string().unwrapOrDefault();
    if (!message.empty())
        return message;
    return fallback;
}

const int DEV_ACCOUNT_ID = 7689052;

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
    if (!Popup::init(380.f, 240.f, "GJ_square04.png"))
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
    usernameLabel->setScale(m_title->getScale());
    m_mainLayer->addChild(usernameLabel);

    auto optionsMenu = CCMenu::create();
    optionsMenu->setPosition({m_mainLayer->getContentSize().width / 2,
        m_mainLayer->getContentSize().height / 2 - 15});
    optionsMenu->setContentSize({m_mainLayer->getContentSize().width - 40, 150});
    optionsMenu->setLayout(RowLayout::create()
            ->setGap(6.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false));

    m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
    m_optionsMenu = optionsMenu;

    auto spinner = LoadingSpinner::create(60.f);
    spinner->setPosition({m_mainLayer->getContentSize().width / 2.f,
        m_mainLayer->getContentSize().height / 2.f});
    m_mainLayer->addChild(spinner, 5);
    m_spinner = spinner;

    // create action buttons (set and remove) for each option
    auto makeActionButton = [this](const std::string& text,
                                cocos2d::SEL_MenuHandler cb)
        -> std::pair<ButtonSprite*, CCMenuItemSpriteExtra*> {
        auto spr = ButtonSprite::create(text.c_str(), 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f);
        if (spr)
            spr->updateBGImage("GJ_button_01.png");
        auto item = CCMenuItemSpriteExtra::create(spr, this, cb);
        return {spr, item};
    };

    // Exclude action button
    auto [excludeSpr, excludeBtn] = makeActionButton(
        "Leaderboard Exclude", menu_selector(RLUserControl::onOptionAction));
    excludeBtn->setVisible(false);
    excludeBtn->setEnabled(false);
    optionsMenu->addChild(excludeBtn);
    m_userOptions["exclude"] = {excludeBtn, excludeSpr, false, false};

    // Blacklist action button
    auto [blackSpr, blackBtn] = makeActionButton(
        "Report Blacklist", menu_selector(RLUserControl::onOptionAction));
    blackBtn->setVisible(false);
    blackBtn->setEnabled(false);
    optionsMenu->addChild(blackBtn);
    m_userOptions["blacklisted"] = {blackBtn, blackSpr, false, false};

    // Whitelist action button
    auto [whitelistSpr, whitelistBtn] = makeActionButton(
        "Set/Remove Leaderboard Whitelist",
        menu_selector(RLUserControl::onOptionAction));
    whitelistBtn->setVisible(false);
    whitelistBtn->setEnabled(false);
    optionsMenu->addChild(whitelistBtn);
    m_userOptions["whitelist"] = {whitelistBtn, whitelistSpr, false, false};

    // Creator banned action button
    auto [bannedSpr, bannedBtn] = makeActionButton(
        "Creator Ban", menu_selector(RLUserControl::onOptionAction));
    bannedBtn->setVisible(false);
    bannedBtn->setEnabled(false);
    optionsMenu->addChild(bannedBtn);
    m_userOptions["bannedCreator"] = {bannedBtn, bannedSpr, false, false};

    // Wipe User Data button
    auto [wipeSpr, wipeBtn] = makeActionButton(
        "Wipe User Data", menu_selector(RLUserControl::onWipeAction));
    wipeBtn->setVisible(false);
    wipeBtn->setEnabled(false);
    optionsMenu->addChild(wipeBtn);
    m_wipeButton = wipeBtn;
    if (GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID) {
        wipeBtn->setVisible(true);
        wipeBtn->setEnabled(true);
    }

    // Promote/Demote buttons (dev-only)
    auto [lbModSpr, lbModBtn] = makeActionButton(
        "Promote to LB Mod", menu_selector(RLUserControl::onPromoteAction));
    lbModBtn->setVisible(false);
    lbModBtn->setEnabled(false);
    optionsMenu->addChild(lbModBtn);
    m_promoteLeaderboardModButton = lbModBtn;

    auto [classicModSpr, classicModBtn] = makeActionButton(
        "Promote to Classic Mod", menu_selector(RLUserControl::onPromoteAction));
    classicModBtn->setVisible(false);
    classicModBtn->setEnabled(false);
    optionsMenu->addChild(classicModBtn);
    m_promoteClassicModButton = classicModBtn;

    auto [classicAdminSpr, classicAdminBtn] =
        makeActionButton("Promote to Classic Admin",
            menu_selector(RLUserControl::onPromoteAction));
    classicAdminBtn->setVisible(false);
    classicAdminBtn->setEnabled(false);
    optionsMenu->addChild(classicAdminBtn);
    m_promoteClassicAdminButton = classicAdminBtn;

    auto [platModSpr, platModBtn] = makeActionButton(
        "Promote to Plat Mod", menu_selector(RLUserControl::onPromoteAction));
    platModBtn->setVisible(false);
    platModBtn->setEnabled(false);
    optionsMenu->addChild(platModBtn);
    m_promotePlatModButton = platModBtn;

    auto [platAdminSpr, platAdminBtn] = makeActionButton(
        "Promote to Plat Admin", menu_selector(RLUserControl::onPromoteAction));
    platAdminBtn->setVisible(false);
    platAdminBtn->setEnabled(false);
    optionsMenu->addChild(platAdminBtn);
    m_promotePlatAdminButton = platAdminBtn;

    auto [demoteSpr, demoteBtn] = makeActionButton(
        "Demote All", menu_selector(RLUserControl::onPromoteAction));
    demoteBtn->setVisible(false);
    demoteBtn->setEnabled(false);
    optionsMenu->addChild(demoteBtn);
    m_demoteButton = demoteBtn;

    // If there's no profile to load (no target account), show dev buttons
    // immediately
    if (m_targetAccountId <= 0 &&
        GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID) {
        setPromoBtnState(lbModBtn, "Promote to LB Mod", true);
        setPromoBtnState(classicModBtn, "Promote to Classic Mod", true);
        setPromoBtnState(classicAdminBtn, "Promote to Classic Admin", true);
        setPromoBtnState(platModBtn, "Promote to Plat Mod", true);
        setPromoBtnState(platAdminBtn, "Promote to Plat Admin", true);
        setPromoBtnState(demoteBtn, "Demote All", true);
        wipeBtn->setVisible(true);
        wipeBtn->setEnabled(false);
    }

    m_optionsLayout = static_cast<RowLayout*>(optionsMenu->getLayout());
    optionsMenu->updateLayout();
    m_mainLayer->addChild(optionsMenu);

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
                bool isPlatMod = false;
                bool isPlatAdmin = false;
                bool isWhitelisted = false;

                if (!response.ok()) {
                    log::warn("Profile fetch returned non-ok status: {}",
                        response.code());
                } else {
                    auto jsonRes = response.json();
                    if (!jsonRes) {
                        log::warn("Failed to parse JSON response for profile");
                    } else {
                        auto json = jsonRes.unwrap();
                        isExcluded = json["excluded"].asBool().unwrapOrDefault();
                        isBlacklisted = json["blacklisted"].asBool().unwrapOrDefault();
                        isBanned = json["banned"].asBool().unwrapOrDefault();
                        isLBMod = json["isLeaderboardMod"].asBool().unwrapOrDefault();
                        isClassicMod = json["isClassicMod"].asBool().unwrapOrDefault();
                        isClassicAdmin =
                            json["isClassicAdmin"].asBool().unwrapOrDefault();
                        isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
                        isPlatAdmin = json["isPlatAdmin"].asBool().unwrapOrDefault();
                        isWhitelisted = json["whitelisted"].asBool().unwrapOrDefault();
                    }
                }

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
                for (auto& kv : self->m_userOptions) {
                    auto& key = kv.first;
                    auto& opt = kv.second;
                    if (opt.actionButton) {
                        if (self->m_spinner)
                            self->m_spinner->setVisible(false);

                        bool showAction = true;
                        if (key == "bannedCreator") {
                            bool hasPerms =
                                (Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
                                    Mod::get()->getSavedValue<bool>("isPlatAdmin")) ||
                                GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID;
                            if (!hasPerms)
                                showAction = false;
                        }

                        if (key == "blacklisted") {
                            bool hasPerms =
                                Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
                                Mod::get()->getSavedValue<bool>("isPlatAdmin") ||
                                GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID;
                            if (!hasPerms)
                                showAction = false;
                        }

                        opt.actionButton->setVisible(showAction);

                        if (key == "exclude" && isWhitelisted) {
                            opt.actionButton->setEnabled(false);
                        } else {
                            opt.actionButton->setEnabled(showAction);
                        }
                    }
                }

                self->m_targetIsLeaderboardMod = isLBMod;
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
                            "Wipe User Data", 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f));
                        self->m_wipeButton->setEnabled(false);
                    } else {
                        // restore normal active appearance
                        self->m_wipeButton->setNormalImage(ButtonSprite::create(
                            "Wipe User Data", 200, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 1.f));
                        self->m_wipeButton->setEnabled(true);
                    }
                }

                // show or remove dev-only buttons depending on current account
                if (GJAccountManager::get()->m_accountID != DEV_ACCOUNT_ID) {
                    if (self->m_wipeButton) {
                        self->m_wipeButton->removeFromParentAndCleanup(true);
                        self->m_wipeButton = nullptr;
                    }
                    if (self->m_promoteLeaderboardModButton) {
                        self->m_promoteLeaderboardModButton->removeFromParentAndCleanup(
                            true);
                        self->m_promoteLeaderboardModButton = nullptr;
                    }
                    if (self->m_promoteClassicModButton) {
                        self->m_promoteClassicModButton->removeFromParentAndCleanup(true);
                        self->m_promoteClassicModButton = nullptr;
                    }
                    if (self->m_promoteClassicAdminButton) {
                        self->m_promoteClassicAdminButton->removeFromParentAndCleanup(
                            true);
                        self->m_promoteClassicAdminButton = nullptr;
                    }
                    if (self->m_promotePlatModButton) {
                        self->m_promotePlatModButton->removeFromParentAndCleanup(true);
                        self->m_promotePlatModButton = nullptr;
                    }
                    if (self->m_promotePlatAdminButton) {
                        self->m_promotePlatAdminButton->removeFromParentAndCleanup(true);
                        self->m_promotePlatAdminButton = nullptr;
                    }
                    if (self->m_demoteButton) {
                        self->m_demoteButton->removeFromParentAndCleanup(true);
                        self->m_demoteButton = nullptr;
                    }
                } else {
                    // developer: ensure buttons are visible and enabled now that
                    // loading finished
                    if (self->m_wipeButton) {
                        self->m_wipeButton->setVisible(true);
                        self->m_wipeButton->setEnabled(excludeOpt &&
                                                       excludeOpt->persisted);
                    }
                    auto enableIf = [&](CCMenuItemSpriteExtra* btn,
                                        const std::string& text,
                                        bool cond) {
                        if (btn) {
                            // set background according to enabled state
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
                    // demote is active if any role present
                    bool anyRole = self->m_targetIsLeaderboardMod ||
                                   self->m_targetIsClassicMod ||
                                   self->m_targetIsClassicAdmin ||
                                   self->m_targetIsPlatMod || self->m_targetIsPlatAdmin;
                    enableIf(self->m_demoteButton, "Demote All", anyRole);
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
                            getResponseFailMessage(response, "Failed to delete user"));
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

    if (GJAccountManager::get()->m_accountID != DEV_ACCOUNT_ID) {
        FLAlertLayer::create("Access Denied",
            "You do not have permission to perform this action.",
            "OK")
            ->show();
        return;
    }
    std::string actionText = "";
    std::string confirmText = "";
    std::string titleText = "";
    bool setLBMod = false;
    bool setClassicMod = false;
    bool setClassicAdmin = false;
    bool setPlatMod = false;
    bool setPlatAdmin = false;
    bool clearAll = false;

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
    } else if (sender == m_demoteButton) {
        actionText = "Demoting user...";
        confirmText = "clear all roles for this user";
        titleText = "Demote User?";
        clearAll = true;
    } else {
        return;  // unknown sender
    }

    createQuickPopup(
        titleText.c_str(),
        ("Are you sure you want to <cg>" + confirmText + "</c>?").c_str(),
        "Cancel",
        "Confirm",
        [this, setLBMod, setClassicMod, setClassicAdmin, setPlatMod, setPlatAdmin, clearAll, actionText](auto, bool yes) {
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
            setPromoBtnState(this->m_demoteButton, "Demote All", false);
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
            if (setClassicMod)
                jsonBody["isClassicMod"] = true;
            if (setClassicAdmin)
                jsonBody["isClassicAdmin"] = true;
            if (setPlatMod)
                jsonBody["isPlatMod"] = true;
            if (setPlatAdmin)
                jsonBody["isPlatAdmin"] = true;
            if (clearAll) {
                jsonBody["isLeaderboardMod"] = false;
                jsonBody["isClassicMod"] = false;
                jsonBody["isClassicAdmin"] = false;
                jsonBody["isPlatMod"] = false;
                jsonBody["isPlatAdmin"] = false;
            }

            log::info("Sending promoteUser request: {}", jsonBody.dump());

            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            Ref<RLUserControl> self = this;
            self->m_promoteUserTask.spawn(
                postReq.post("https://gdrate.arcticwoof.xyz/promoteUser"),
                [self, upopup, setLBMod, setClassicMod, setClassicAdmin, setPlatMod, setPlatAdmin, clearAll](web::WebResponse response) {
                    if (!self || !upopup)
                        return;

                    // re-enable UI
                    self->setAllOptionsEnabled(true);
                    setPromoBtnState(self->m_promoteLeaderboardModButton,
                        "Promote to LB Mod",
                        true);
                    setPromoBtnState(self->m_promoteClassicModButton,
                        "Promote to Classic Mod",
                        true);
                    setPromoBtnState(self->m_promoteClassicAdminButton,
                        "Promote to Classic Admin",
                        true);
                    setPromoBtnState(self->m_promotePlatModButton,
                        "Promote to Plat Mod",
                        true);
                    setPromoBtnState(self->m_promotePlatAdminButton,
                        "Promote to Plat Admin",
                        true);
                    setPromoBtnState(self->m_demoteButton, "Demote All", true);
                    if (self->m_wipeButton)
                        self->m_wipeButton->setEnabled(true);

                    if (!response.ok()) {
                        log::warn("promoteUser returned non-ok status: {}",
                            response.code());
                        upopup->showFailMessage(getResponseFailMessage(
                            response, "Failed to update user role"));
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
                        if (clearAll) {
                            self->m_targetIsLeaderboardMod = false;
                            self->m_targetIsClassicMod = false;
                            self->m_targetIsClassicAdmin = false;
                            self->m_targetIsPlatMod = false;
                            self->m_targetIsPlatAdmin = false;
                            upopup->showSuccessMessage("User roles cleared!");
                        } else {
                            if (setLBMod) {
                                self->m_targetIsLeaderboardMod = true;
                                upopup->showSuccessMessage("User promoted to LB Mod!");
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
                        bool anyRole = self->m_targetIsLeaderboardMod ||
                                       self->m_targetIsClassicMod ||
                                       self->m_targetIsClassicAdmin ||
                                       self->m_targetIsPlatMod ||
                                       self->m_targetIsPlatAdmin;
                        setPromoBtnState(self->m_promoteLeaderboardModButton,
                            "Promote to LB Mod",
                            !self->m_targetIsLeaderboardMod);
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
                        setPromoBtnState(self->m_demoteButton, "Demote All", anyRole);
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
                "Are you sure you want to <cg>{}</c> for user <cg>{}</c>?",
                actionDesc,
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
    if (opt->actionButton && opt->actionSprite) {
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
        } else if (key == "blacklisted") {
            if (desired) {
                text = "Remove Report Blacklist";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Report Blacklist";
                bg = "GJ_button_01.png";
            }
        } else if (key == "bannedCreator") {
            if (desired) {
                text = "Remove Creator Ban";
                bg = "GJ_button_02.png";
            } else {
                text = "Set Creator Ban";
                bg = "GJ_button_01.png";
            }
        } else if (key == "whitelist") {
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
            text.c_str(), 200, true, "goldFont.fnt", bg.c_str(), 30.f, 1.f);
        opt->actionButton->setNormalImage(opt->actionSprite);
    }

    if (m_optionsMenu) {
        m_optionsMenu->updateLayout();
    }

    // whenever whitelist is updated, exclude is locked and styled as disabled
    auto whitelistOpt = getOptionByKey("whitelist");
    auto excludeOpt = getOptionByKey("exclude");
    if (whitelistOpt && whitelistOpt->desired) {
        if (excludeOpt && excludeOpt->actionButton) {
            excludeOpt->actionButton->setEnabled(false);
            std::string excludeText = excludeOpt->desired ? "Remove Leaderboard Exclude" : "Set Leaderboard Exclude";
            excludeOpt->actionSprite = ButtonSprite::create(
                excludeText.c_str(), 200, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 1.f);
            excludeOpt->actionButton->setNormalImage(excludeOpt->actionSprite);
        }
    } else {
        setOptionEnabled("exclude", true);
        if (excludeOpt && excludeOpt->actionButton) {
            std::string excludeText = excludeOpt->desired ? "Remove Leaderboard Exclude" : "Set Leaderboard Exclude";
            std::string excludeBg = excludeOpt->desired ? "GJ_button_02.png" : "GJ_button_01.png";
            excludeOpt->actionSprite = ButtonSprite::create(
                excludeText.c_str(), 200, true, "goldFont.fnt", excludeBg.c_str(), 30.f, 1.f);
            excludeOpt->actionButton->setNormalImage(excludeOpt->actionSprite);
            excludeOpt->actionButton->setEnabled(true);
        }
    }

    if (GJAccountManager::get()->m_accountID == DEV_ACCOUNT_ID) {
        setPromoBtnState(m_promoteLeaderboardModButton, "Promote to LB Mod", !m_targetIsLeaderboardMod);
        setPromoBtnState(m_promoteClassicModButton, "Promote to Classic Mod", !m_targetIsClassicMod);
        setPromoBtnState(m_promoteClassicAdminButton, "Promote to Classic Admin", !m_targetIsClassicAdmin);
        setPromoBtnState(m_promotePlatModButton, "Promote to Plat Mod", !m_targetIsPlatMod);
        setPromoBtnState(m_promotePlatAdminButton, "Promote to Plat Admin", !m_targetIsPlatAdmin);
        bool anyRole = m_targetIsLeaderboardMod || m_targetIsClassicMod ||
                       m_targetIsClassicAdmin || m_targetIsPlatMod ||
                       m_targetIsPlatAdmin;
        if (m_demoteButton) {
            setPromoBtnState(m_demoteButton, "Demote All", anyRole);
        }
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
                    getResponseFailMessage(response, "Failed to update user"));
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
                    getResponseFailMessage(response, "Failed to update user"));
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
