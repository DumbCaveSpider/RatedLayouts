#include "include/RLAchievements.hpp"
#include "include/RLConstants.hpp"
#include "include/RLNetworkUtils.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class $modify(RLSupportLayer, SupportLayer) {
    struct Fields {
        async::TaskHolder<web::WebResponse> m_getAccessTask;
        async::TaskHolder<Result<std::string>> m_authTask;
        CCMenu* m_argonMenu = nullptr;
        ~Fields() {
            m_getAccessTask.cancel();
            m_authTask.cancel();
        }
    };

    void customSetup() {
        // show the argon button for user with a role
        if (rl::isUserHasPerms() || rl::isUserOwner()) {
            m_fields->m_argonMenu = CCMenu::create();
            m_fields->m_argonMenu->setPosition({0, 0});
            m_listLayer->addChild(m_fields->m_argonMenu, 10);
            auto argonBtnSpr = ButtonSprite::create("Argon", 25, true, "bigFont.fnt", "GJ_button_04.png", 25.f, 1.f);
            auto argonBtn = CCMenuItemSpriteExtra::create(
                argonBtnSpr, this, menu_selector(RLSupportLayer::onGetArgon));
            argonBtn->setPosition({328, 55});
            m_fields->m_argonMenu->addChild(argonBtn);
        }

        SupportLayer::customSetup();
    }

    void onGetArgon(CCObject* sender) {
        m_uploadPopup = UploadActionPopup::create(nullptr, "Obtaining Token...");
        m_uploadPopup->show();
        auto accountData = argon::getGameAccountData();
        m_fields->m_authTask.spawn(
            argon::startAuth(std::move(accountData)),
            [this](Result<std::string> res) {
                if (res.isOk()) {
                    auto token = std::move(res).unwrap();
                    utils::clipboard::write(token);
                    m_uploadPopup->showSuccessMessage(
                        "Token copied to clipboard.");
                } else {
                    auto err = res.unwrapErr();
                    log::warn("Auth failed: {}", err);
                    m_uploadPopup->showFailMessage(err);
                    argon::clearToken();
                }
            });
    }

    void onRequestAccess(CCObject* sender) {  // i assume that no one will ever get gd mod xddd
        m_uploadPopup = UploadActionPopup::create(nullptr, "Requesting Access...");
        m_uploadPopup->show();
        // argon my beloved <3
        auto accountData = argon::getGameAccountData();

        Ref<RLSupportLayer> self = this;

        m_fields->m_authTask.spawn(
            argon::startAuth(std::move(accountData)),
            [self, this](Result<std::string> res) {
                if (!self)
                    return;

                if (res.isOk()) {
                    auto token = std::move(res).unwrap();
                    log::info("token obtained: {}", token);
                    Mod::get()->setSavedValue("argon_token", token);

                    // json body
                    matjson::Value jsonBody = matjson::Value::object();
                    jsonBody["argonToken"] = token;
                    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

                    // verify the user's role
                    auto postReq = web::WebRequest();
                    postReq.bodyJSON(jsonBody);

                    m_fields->m_getAccessTask.spawn(
                        postReq.post(std::string(rl::BASE_API_URL) + "/getAccess"),
                        [self, this](web::WebResponse response) {
                            if (!self)
                                return;
                            log::info("Received response from server");
                            if (!response.ok()) {
                                log::warn("Server returned non-ok status: {}",
                                    response.code());
                                m_uploadPopup->showFailMessage(rl::getResponseFailMessage(
                                    response, "Failed! Try again later."));
                                return;
                            }

                            auto jsonRes = response.json();
                            if (!jsonRes) {
                                log::warn("Failed to parse JSON response");
                                m_uploadPopup->showFailMessage(
                                    "Failed to parse JSON response");
                                return;
                            }

                            auto json = jsonRes.unwrap();
                            bool isClassicMod =
                                json["isClassicMod"].asBool().unwrapOrDefault();
                            bool isClassicAdmin =
                                json["isClassicAdmin"].asBool().unwrapOrDefault();
                            bool isLeaderboardMod =
                                json["isLeaderboardMod"].asBool().unwrapOrDefault();
                            bool isLeaderboardAdmin =
                                json["isLeaderboardAdmin"].asBool().unwrapOrDefault();
                            bool isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
                            bool isPlatAdmin =
                                json["isPlatAdmin"].asBool().unwrapOrDefault();

                            Mod::get()->setSavedValue<bool>("isClassicMod", isClassicMod);
                            Mod::get()->setSavedValue<bool>("isClassicAdmin",
                                isClassicAdmin);
                            Mod::get()->setSavedValue<bool>("isLeaderboardMod",
                                isLeaderboardMod);
                            Mod::get()->setSavedValue<bool>("isLeaderboardAdmin",
                                isLeaderboardAdmin);
                            Mod::get()->setSavedValue<bool>("isPlatMod", isPlatMod);
                            Mod::get()->setSavedValue<bool>("isPlatAdmin", isPlatAdmin);

                            int roleCount = 0;
                            roleCount += isClassicMod ? 1 : 0;
                            roleCount += isClassicAdmin ? 1 : 0;
                            roleCount += isLeaderboardMod ? 1 : 0;
                            roleCount += isLeaderboardAdmin ? 1 : 0;
                            roleCount += isPlatMod ? 1 : 0;
                            roleCount += isPlatAdmin ? 1 : 0;

                            if (roleCount > 1) {
                                log::info("Granted Multiple roles");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Layout roles.");
                            } else if (isClassicMod) {
                                log::info("Granted Layout Mod role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Classic Layout Mod.");
                            } else if (isClassicAdmin) {
                                log::info("Granted Layout Admin role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Classic Layout Admin.");
                            } else if (isLeaderboardMod) {
                                log::info("Granted Leaderboard Layout Mod role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Leaderboard Layout Mod.");
                            } else if (isLeaderboardAdmin) {
                                log::info("Granted Leaderboard Layout Admin role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Leaderboard Layout Admin.");
                            } else if (isPlatMod) {
                                log::info("Granted Platformer Layout Mod role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Platformer Layout Mod.");
                            } else if (isPlatAdmin) {
                                log::info("Granted Platformer Admin role");
                                m_uploadPopup->showSuccessMessage(
                                    "Granted Platformer Layout Admin.");
                            } else {
                                m_uploadPopup->showFailMessage("Nothing Happened.");
                            }
                        });
                } else {
                    auto err = res.unwrapErr();
                    log::warn("Auth failed: {}", err);
                    Notification::create(err, NotificationIcon::Error)->show();
                    argon::clearToken();
                }
            });
    }
};
