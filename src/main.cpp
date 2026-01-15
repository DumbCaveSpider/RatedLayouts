#include <Geode/Geode.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class $modify(SupportLayer) {
      struct Fields { utils::web::WebTask m_getAccessTask; ~Fields() { m_getAccessTask.cancel(); } };
      void onRequestAccess(CCObject* sender) {  // i assume that no one will ever get gd mod xddd
            auto popup = UploadActionPopup::create(nullptr, "Requesting Access...");
            popup->show();
            // argon my beloved <3
            std::string token;
            auto res = argon::startAuth(
                [](Result<std::string> res) {
                      if (!res) {
                            log::warn("Auth failed: {}", res.unwrapErr());
                            Notification::create(res.unwrapErr(), NotificationIcon::Error)
                                ->show();
                            return;
                      }
                      auto token = std::move(res).unwrap();
                      log::debug("token obtained: {}", token);
                      Mod::get()->setSavedValue("argon_token", token);
                },
                [](argon::AuthProgress progress) {
                      log::debug("auth progress: {}",
                                 argon::authProgressToString(progress));
                });
            if (!res) {
                  log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                  Notification::create(res.unwrapErr(), NotificationIcon::Error)->show();
                  return;
            }

            // json boody crap
            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] =
                Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

            // verify the user's role
            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            m_fields->m_getAccessTask = postReq.post("https://gdrate.arcticwoof.xyz/getAccess");

            // handle the response
            Ref<SupportLayer> self = this;
            Ref<UploadActionPopup> upopup = popup;
            m_fields->m_getAccessTask.listen([self, upopup](web::WebResponse* response) {
                  if (!self || !upopup) return;
                  log::info("Received response from server");

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
                  Mod::get()->setSavedValue<int>("role", role);
                  // role check lol
                  if (role == 1) {
                        log::info("Granted Layout Mod role");
                        upopup->showSuccessMessage("Granted Layout Mod.");
                  } else if (role == 2) {
                        log::info("Granted Layout Admin role");
                        upopup->showSuccessMessage("Granted Layout Admin.");
                  } else {
                        upopup->showFailMessage("Nothing Happened.");
                  }
            });
      }
};