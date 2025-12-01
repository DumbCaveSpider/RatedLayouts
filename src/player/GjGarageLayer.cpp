#include <capeling.garage-stats-menu/include/StatsDisplayAPI.h>

#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class $modify(GJGarageLayer) {
      struct Fields {
            CCNode* myStatItem = nullptr;
            int storedStars = 0;
      };

      bool init() {
            if (!GJGarageLayer::init())
                  return false;

            fetchProfileData(GJAccountManager::get()->m_accountID);

            auto statMenu = this->getChildByID("capeling.garage-stats-menu/stats-menu");

            auto starSprite = CCSprite::create("rlStarIconMed.png"_spr);
            auto myStatItem = StatsDisplayAPI::getNewItem(
                "blueprint-stars"_spr, starSprite,
                m_fields->storedStars, 0.54f);

            m_fields->myStatItem = myStatItem;

            if (statMenu) {
                  statMenu->addChild(myStatItem);
                  statMenu->updateLayout();
            }

            return true;
      }

      void fetchProfileData(int accountId) {
            log::info("Fetching profile data for account ID: {}", accountId);
            // argon my beloved <3
            std::string token;
            auto res = argon::startAuth(
                [](Result<std::string> res) {
                      if (!res) {
                            log::warn("Auth failed: {}", res.unwrapErr());
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
                  return;
            }

            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["argonToken"] =
                Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["accountId"] = accountId;

            // web request
            auto postReq = web::WebRequest();
            postReq.bodyJSON(jsonBody);
            auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");

            // Handle the response
            postTask.listen([this](web::WebResponse* response) {
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

                  int points = json["points"].asInt().unwrapOrDefault();
                  int stars = json["stars"].asInt().unwrapOrDefault();

                  log::info("Profile data - points: {}, stars: {}", points, stars);
                  // store the values into the saved value
                  Mod::get()->setSavedValue("stars", stars);
                  Mod::get()->setSavedValue("points", points);
                  m_fields->storedStars = stars;
            });
      }
};