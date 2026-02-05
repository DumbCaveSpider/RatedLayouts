#include "custom/RLAchievements.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>


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
  void onRequestAccess(CCObject *sender) { // i assume that no one will ever get gd mod xddd
    auto popup = UploadActionPopup::create(nullptr, "Requesting Access...");
    popup->show();
    // argon my beloved <3
    auto accountData = argon::getGameAccountData();

    Ref<SupportLayer> self = this;
    Ref<UploadActionPopup> upopup = popup;

    m_fields->m_authTask.spawn(
        argon::startAuth(std::move(accountData)),
        [self, upopup, this](Result<std::string> res) {
          if (!self || !upopup)
            return;

          if (res.isOk()) {
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
                  if (!self || !upopup)
                    return;
                  log::info("Received response from server");
                  if (!response.ok()) {
                    log::warn("Server returned non-ok status: {}",
                              response.code());
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
          } else {
            auto err = res.unwrapErr();
            log::warn("Auth failed: {}", err);

            // If account data is invalid, attempt interactive auth as a fallback
            if (err.find("Invalid account data") != std::string::npos) {
              log::info("Falling back to interactive auth due to invalid account data");
              argon::AuthOptions options;
              options.progress = [](argon::AuthProgress progress) {
                log::debug("auth progress: {}", argon::authProgressToString(progress));
              };

              m_fields->m_authTask.spawn(
                  argon::startAuth(std::move(options)),
                  [self, upopup, this](Result<std::string> res2) {
                    if (!self || !upopup) return;
                    if (res2.isOk()) {
                      auto token = std::move(res2).unwrap();
                      log::debug("token obtained (fallback): {}", token);
                      Mod::get()->setSavedValue("argon_token", token);
                      // Trigger same access check as above
                      matjson::Value jsonBody = matjson::Value::object();
                      jsonBody["argonToken"] = token;
                      jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

                      auto postReq = web::WebRequest();
                      postReq.bodyJSON(jsonBody);

                      m_fields->m_getAccessTask.spawn(
                          postReq.post("https://gdrate.arcticwoof.xyz/getAccess"),
                          [self, upopup, this](web::WebResponse response) {
                            if (!self || !upopup) return;
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
                              upopup->showSuccessMessage("Granted Layout Mod.");
                              RLAchievements::onReward("misc_moderator");
                            } else if (role == 2) {
                              upopup->showSuccessMessage("Granted Layout Admin.");
                            } else {
                              upopup->showFailMessage("Nothing Happened.");
                            }
                          });
                    } else {
                      log::warn("Interactive auth also failed: {}", res2.unwrapErr());
                      Notification::create(res2.unwrapErr(), NotificationIcon::Error)->show();
                      argon::clearToken();
                    }
                  });
            } else {
              Notification::create(err, NotificationIcon::Error)->show();
              argon::clearToken();
            }
          }
        });
  }
};