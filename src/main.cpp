#include "Geode/loader/Mod.hpp"
#include "custom/RLAchievements.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/SupportLayer.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

static std::string getResponseFailMessage(web::WebResponse const &response,
                                          std::string const &fallback) {
  auto message = response.string().unwrapOrDefault();
  if (!message.empty())
    return message;
  return fallback;
}

$execute {
  // clear old cache on game start to prevent stale data issues
  auto saveDir = dirs::getModsSaveDir();
  auto userCache = saveDir / "user_role_cache.json";
  auto levelCache = saveDir / "level_ratings_cache.json";

  bool deletedAny = false;

  auto userCachePathStr = geode::utils::string::pathToString(userCache);
  auto levelCachePathStr = geode::utils::string::pathToString(levelCache);

  if (utils::file::readString(userCachePathStr)) {
    auto writeRes = utils::file::writeString(userCachePathStr, "{}");
    if (!writeRes) {
      log::warn("Failed to clear user cache file: {}", userCachePathStr);
    }
    deletedAny = true;
  }
  if (utils::file::readString(levelCachePathStr)) {
    auto writeRes = utils::file::writeString(levelCachePathStr, "{}");
    if (!writeRes) {
      log::warn("Failed to clear level cache file: {}", levelCachePathStr);
    }
    deletedAny = true;
    log::debug("cleared cache files");
  }
};

class $modify(SupportLayer) {
  struct Fields {
    async::TaskHolder<web::WebResponse> m_getAccessTask;
    async::TaskHolder<Result<std::string>> m_authTask;
    ~Fields() {
      m_getAccessTask.cancel();
      m_authTask.cancel();
    }
  };

  void onRequestAccess(
      CCObject *sender) { // i assume that no one will ever get gd mod xddd
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
                    upopup->showFailMessage(getResponseFailMessage(
                        response, "Failed! Try again later."));
                    return;
                  }

                  auto jsonRes = response.json();
                  if (!jsonRes) {
                    log::warn("Failed to parse JSON response");
                    return;
                  }

                  auto json = jsonRes.unwrap();
                  bool isClassicMod =
                      json["isClassicMod"].asBool().unwrapOrDefault();
                  bool isClassicAdmin =
                      json["isClassicAdmin"].asBool().unwrapOrDefault();
                  bool isLeaderboardMod =
                      json["isLeaderboardMod"].asBool().unwrapOrDefault();
                  bool isPlatMod = json["isPlatMod"].asBool().unwrapOrDefault();
                  bool isPlatAdmin =
                      json["isPlatAdmin"].asBool().unwrapOrDefault();

                  Mod::get()->setSavedValue<bool>("isClassicMod", isClassicMod);
                  Mod::get()->setSavedValue<bool>("isClassicAdmin",
                                                  isClassicAdmin);
                  Mod::get()->setSavedValue<bool>("isLeaderboardMod",
                                                  isLeaderboardMod);
                  Mod::get()->setSavedValue<bool>("isPlatMod", isPlatMod);
                  Mod::get()->setSavedValue<bool>("isPlatAdmin", isPlatAdmin);

                  if (isClassicMod) {
                    log::info("Granted Layout Mod role");
                    upopup->showSuccessMessage("Granted Classic Layout Mod.");
                  } else if (isClassicAdmin) {
                    log::info("Granted Layout Admin role");
                    upopup->showSuccessMessage("Granted Classic Layout Admin.");
                  } else if (isLeaderboardMod) {
                    log::info("Granted Leaderboard Layout Mod role");
                    upopup->showSuccessMessage(
                        "Granted Leaderboard Layout Mod.");
                  } else if (isPlatMod) {
                    log::info("Granted Platformer Layout Mod role");
                    upopup->showSuccessMessage(
                        "Granted Platformer Layout Mod.");
                  } else if (isPlatAdmin) {
                    log::info("Granted Platformer Admin role");
                    upopup->showSuccessMessage(
                        "Granted Platformer Layout Admin.");
                  } else {
                    upopup->showFailMessage("Nothing Happened.");
                  }

                  if (isClassicMod || isPlatMod || isLeaderboardMod) {
                    RLAchievements::onReward("misc_moderator");
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