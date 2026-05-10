#include "include/RLConstants.hpp"
#include "include/RLNetworkUtils.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <Geode/utils/file.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    GJUserScore* userScore = GJUserScore::create();
    userScore->m_accountID = GJAccountManager::get()->m_accountID;
    if (userScore->m_modBadge != 0) {
        log::debug("User has mod badge: {}", userScore->m_modBadge);
        Mod::get()->setSettingValue<bool>("disableRLReq", true);
    } else {
        log::debug("User does not have a mod badge");
    }

    if (rl::isGDPS()) {
        log::debug("Running on GDPS, disabling Rated Layouts requests");
        Mod::get()->setSettingValue<bool>("disableRLReq", true);
    }
};

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
        SupportLayer::customSetup();

        if (!Mod::get()->getSettingValue<bool>("disableRLReq")) {
            m_fields->m_argonMenu = CCMenu::create();
            m_fields->m_argonMenu->setPosition({0, 0});
            m_listLayer->addChild(m_fields->m_argonMenu, 10);

            auto keyBtnSpr = ButtonSprite::create("Key", 25, true, "bigFont.fnt", "GJ_button_04.png", 25.f, 1.f);
            auto keyBtn = CCMenuItemSpriteExtra::create(
                keyBtnSpr, this, menu_selector(RLSupportLayer::onImportKey));
            keyBtn->setPosition({328, 55});
            m_fields->m_argonMenu->addChild(keyBtn);
            
            auto blueReqBtnSpr = ButtonSprite::create("RL Req", 25, true, "bigFont.fnt", "GJ_button_04.png", 25.f, 1.f);
            auto blueReqBtn = CCMenuItemSpriteExtra::create(
                blueReqBtnSpr, this, menu_selector(RLSupportLayer::onRLRequest));
            blueReqBtn->setPosition({328, 25});
            m_fields->m_argonMenu->addChild(blueReqBtn);

            if (rl::isUserHasPerms() || rl::isUserOwner()) {
                auto argonBtnSpr = ButtonSprite::create("Argon", 25, true, "bigFont.fnt", "GJ_button_04.png", 25.f, 1.f);
                auto argonBtn = CCMenuItemSpriteExtra::create(
                    argonBtnSpr, this, menu_selector(RLSupportLayer::onGetArgon));
                argonBtn->setPosition({328, 85});
                m_fields->m_argonMenu->addChild(argonBtn);
            }
        }
    }

    void onRLRequest(CCObject* sender) {
        m_uploadPopup = UploadActionPopup::create(nullptr, "Loading...");
        m_uploadPopup->show();
        
        auto accountData = argon::getGameAccountData();
        Ref<RLSupportLayer> self = this;

        m_fields->m_authTask.spawn(
            argon::startAuth(std::move(accountData)),
            [self, this](Result<std::string> res) {
                if (!self) return;

                if (res.isOk()) {
                    auto token = std::move(res).unwrap();
                    log::info("token obtained: {}", token);
                    Mod::get()->setSavedValue("argon_token", token);

                    matjson::Value jsonBody = matjson::Value::object();                                            
                    jsonBody["argonToken"] = token;
                    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
                    
                    auto masterKey = Mod::get()->getSavedValue<std::string>("masterKey");
                    if (!masterKey.empty()) {
                        jsonBody["masterKey"] = masterKey;
                    }

                    auto postReq = web::WebRequest();
                    postReq.bodyJSON(jsonBody);

                    m_fields->m_getAccessTask.spawn(
                        postReq.post(std::string(rl::BASE_API_URL) + "/getAccess"),
                        [self, this](web::WebResponse response) {
                            if (!self) return;
                            
                            if (!response.ok()) {
                                m_uploadPopup->showFailMessage(rl::getResponseFailMessage(
                                    response, "Failed! Try again later."));
                                return;
                            }

                            auto jsonRes = response.json();
                            if (!jsonRes) {
                                m_uploadPopup->showFailMessage("Failed to parse JSON response");
                                return;
                            }

                            auto json = jsonRes.unwrap();
                            bool isClassicMod = json["isClassicMod"].asBool().unwrapOrDefault();
                            bool isClassicAdmin = json["isClassicAdmin"].asBool().unwrapOrDefault();
                            bool isLeaderboardMod = json["isLeaderboardMod"].asBool().unwrapOrDefault();
                            bool isLeaderboardAdmin = json["isLeaderboardAdmin"].asBool().unwrapOrDefault();
                            bool isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
                            bool isPlatAdmin = json["isPlatAdmin"].asBool().unwrapOrDefault();
                            bool isOwner = json["isOwner"].asBool().unwrapOrDefault();

                            Mod::get()->setSavedValue<bool>("isClassicMod", isClassicMod);
                            Mod::get()->setSavedValue<bool>("isClassicAdmin", isClassicAdmin);
                            Mod::get()->setSavedValue<bool>("isLeaderboardMod", isLeaderboardMod);
                            Mod::get()->setSavedValue<bool>("isLeaderboardAdmin", isLeaderboardAdmin);
                            Mod::get()->setSavedValue<bool>("isPlatMod", isPlatMod);
                            Mod::get()->setSavedValue<bool>("isPlatAdmin", isPlatAdmin);
                            Mod::get()->setSavedValue<bool>("isOwner", isOwner);

                            int roleCount = (isClassicMod ? 1 : 0) + (isClassicAdmin ? 1 : 0) + 
                                            (isLeaderboardMod ? 1 : 0) + (isLeaderboardAdmin ? 1 : 0) + 
                                            (isPlatMod ? 1 : 0) + (isPlatAdmin ? 1 : 0) + (isOwner ? 1 : 0);

                            if (roleCount > 1) {
                                m_uploadPopup->showSuccessMessage("Granted Layout roles.");
                            } else if (isClassicMod) {
                                m_uploadPopup->showSuccessMessage("Granted Classic Layout Mod.");
                            } else if (isClassicAdmin) {
                                m_uploadPopup->showSuccessMessage("Granted Classic Layout Admin.");
                            } else if (isLeaderboardMod) {
                                m_uploadPopup->showSuccessMessage("Granted Leaderboard Layout Mod.");
                            } else if (isLeaderboardAdmin) {
                                m_uploadPopup->showSuccessMessage("Granted Leaderboard Layout Admin.");
                            } else if (isPlatMod) {
                                m_uploadPopup->showSuccessMessage("Granted Platformer Layout Mod.");
                            } else if (isPlatAdmin) {
                                m_uploadPopup->showSuccessMessage("Granted Platformer Layout Admin.");
                            } else if (isOwner) {
                                m_uploadPopup->showSuccessMessage("Granted Owner role.");
                            } else {
                                m_uploadPopup->showFailMessage("Failed! Nothing found.");
                            }
                        });
                } else {
                    auto err = res.unwrapErr();
                    Notification::create(err, NotificationIcon::Error)->show();
                    argon::clearToken();
                    if (m_uploadPopup) m_uploadPopup->onClose(nullptr);
                }
            });
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
                    m_uploadPopup->showSuccessMessage("Token copied to clipboard.");
                } else {
                    auto err = res.unwrapErr();
                    m_uploadPopup->showFailMessage(err);
                    argon::clearToken();
                }
            });
    }

    void onImportKey(CCObject* sender) {
        m_uploadPopup = UploadActionPopup::create(nullptr, "Importing key...");
        m_uploadPopup->show();

        Ref<RLSupportLayer> self = this;
        async::spawn([self]() -> arc::Future<void> {
            if (!self) co_return;

            geode::utils::file::FilePickOptions options;
            options.filters.push_back({"Private Key", {"*.ppk", "*.pem"}});

            auto result = co_await geode::utils::file::pick(geode::utils::file::PickMode::OpenFile, options);

            if (!self) co_return;
            if (!result) {
                Loader::get()->queueInMainThread([self]() {
                    if (self && self->m_uploadPopup) self->m_uploadPopup->showFailMessage("Failed to open file picker");
                });
                co_return;
            }

            auto maybePath = result.unwrap();
            if (!maybePath) {
                Loader::get()->queueInMainThread([self]() {
                    if (self && self->m_uploadPopup) self->m_uploadPopup->showFailMessage("No file selected");
                });
                co_return;
            }

            auto path = *maybePath;
            auto textRes = utils::file::readString(utils::string::pathToString(path));
            if (!textRes) {
                Loader::get()->queueInMainThread([self]() {
                    if (self && self->m_uploadPopup) self->m_uploadPopup->showFailMessage("Failed to read key file");
                });
                co_return;
            }

            auto key = textRes.unwrap();
            std::vector<uint8_t> keyBytes(key.begin(), key.end());
            web::MultipartForm form;
            form.file("privateKey", keyBytes, path.filename().string(), "application/octet-stream");

            auto postReq = web::WebRequest();
            postReq.bodyMultipart(form);

            auto response = co_await postReq.post(std::string(rl::BASE_API_URL) + "/masterKey");
            if (!self) co_return;

            if (!response.ok()) {
                std::string errorMsg = rl::getResponseFailMessage(response, "Failed to upload private key.");
                Loader::get()->queueInMainThread([self, errorMsg]() {
                    if (self && self->m_uploadPopup) self->m_uploadPopup->showFailMessage(errorMsg);
                });
                co_return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                Loader::get()->queueInMainThread([self]() {
                    if (self && self->m_uploadPopup) self->m_uploadPopup->showFailMessage("Invalid server response.");
                });
                co_return;
            }

            auto json = jsonRes.unwrap();
            std::string returnedKey = json["masterKey"].asString().unwrapOr("");

            Loader::get()->queueInMainThread([self, returnedKey = std::move(returnedKey)]() {
                if (!self) return;
                Mod::get()->setSavedValue("masterKey", returnedKey);
                if (self->m_uploadPopup) {
                    self->m_uploadPopup->showSuccessMessage("Private key accepted.");
                    clipboard::write(returnedKey);
                }
            });
            co_return;
        });
    }

    void onRequestAccess(CCObject* sender) {
        SupportLayer::onRequestAccess(sender);
    }
};
