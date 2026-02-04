#include <Geode/Geode.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>
#include "custom/RLAchievements.hpp"

using namespace geode::prelude;

class $modify(SupportLayer) {
      struct Fields {
            async::TaskHolder<web::WebResponse> m_getAccessTask;
            async::TaskHolder<Result<std::string>> m_authTask;
            ~Fields() {
                  m_getAccessTask.cancel();
                  m_authTask.cancel();
            }
      };
      void onRequestAccess(CCObject* sender) {  // i assume that no one will ever get gd mod xddd
            auto popup = UploadActionPopup::create(nullptr, "Requesting Access...");
            popup->show();
            // argon my beloved <3
            argon::AuthOptions options;
            options.progress = [](argon::AuthProgress progress) {
                log::debug("auth progress: {}", argon::authProgressToString(progress));
            };

            Ref<SupportLayer> self = this;
            Ref<UploadActionPopup> upopup = popup;

            m_fields->m_authTask.spawn(
                argon::startAuth(std::move(options)),
                [self, upopup, this](Result<std::string> res) {
                    if (!self || !upopup) return;
                    if (!res) {
                        log::warn("Auth failed: {}", res.unwrapErr());
                        Notification::create(res.unwrapErr(), NotificationIcon::Error)
                            ->show();
                        return;
                    }
                    auto token = std::move(res).unwrap();
                    log::debug("token obtained: {}", token);
                    Mod::get()->setSavedValue("argon_token", token);

                    // json body
                    matjson::Value jsonBody = matjson::Value::object();
                    jsonBody["argonToken"] = token;
                    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

                    // verify the user's role
                    auto postReq = web::WebRequest();
                    postReq.bodyJSON(jsonBody);

                    m_fields->m_getAccessTask.spawn(
                        postReq.post("https://gdrate.arcticwoof.xyz/getAccess"),
                        [self, upopup, this](web::WebResponse response) {
                            if (!self || !upopup) return;
                            log::info("Received response from server");
                            if (!response.ok()) {
                                log::warn("Server returned non-ok status: {}", response.code());
                                return;
                            }

                            auto jsonRes = response.json();
                            if (!jsonRes) {
                                log::warn("Failed to parse JSON response");
                                return;
                            }

                            auto json = jsonRes.unwrap();
                            int role = json["role"].asInt().unwrapOrDefault();
                            Mod::get()->setSavedValue<int>("role", role);
                            if (role == 1) {
                                log::info("Granted Layout Mod role");
                                upopup->showSuccessMessage("Granted Layout Mod.");
                                RLAchievements::onReward("misc_moderator");
                            } else if (role == 2) {
                                log::info("Granted Layout Admin role");
                                upopup->showSuccessMessage("Granted Layout Admin.");
                            } else {
                                upopup->showFailMessage("Nothing Happened.");
                            }
                        });

                });
      }
};