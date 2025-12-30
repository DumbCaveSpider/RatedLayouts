#include <Geode/Geode.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>
#include <BadgesAPI.hpp>

using namespace geode::prelude;

class $modify(SupportLayer)
{
  void onRequestAccess(
      CCObject *sender)
  { // i assume that no one will ever get gd mod xddd
    // argon my beloved <3
    std::string token;
    auto res = argon::startAuth(
        [](Result<std::string> res)
        {
          if (!res)
          {
            log::warn("Auth failed: {}", res.unwrapErr());
            Notification::create(res.unwrapErr(), NotificationIcon::Error)
                ->show();
          }
          auto token = std::move(res).unwrap();
          log::debug("token obtained: {}", token);
          Mod::get()->setSavedValue("argon_token", token);
        },
        [](argon::AuthProgress progress)
        {
          log::debug("auth progress: {}",
                     argon::authProgressToString(progress));
        });
    if (!res)
    {
      log::warn("Failed to start auth attempt: {}", res.unwrapErr());
      Notification::create(res.unwrapErr(), NotificationIcon::Error)->show();
    }

    // json boody crap
    matjson::Value jsonBody = matjson::Value::object();
    jsonBody["argonToken"] =
        Mod::get()->getSavedValue<std::string>("argon_token");
    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

    // verify the user's role
    auto postReq = web::WebRequest();
    postReq.bodyJSON(jsonBody);
    auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/access");

    // handle the response
    postTask.listen([this](web::WebResponse *response)
                    {
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
        Notification::create("Granted Layout Mod.", NotificationIcon::Success)
            ->show();
      } else if (role == 2) {
        log::info("Granted Layout Admin role");
        Notification::create("Granted Layout Admin.", NotificationIcon::Success)
            ->show();
      } else {
        Notification::create("Nothing happened.", NotificationIcon::Error)
            ->show();
      } });
  }
};