#include <capeling.garage-stats-menu/include/StatsDisplayAPI.h>

#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class $modify(GJGarageLayer) {
      struct Fields {
            CCNode* myStatItem = nullptr;
            CCNode* statMenu = nullptr;
            CCNode* starsValue = nullptr;
            CCNode* planetsValue = nullptr;
            int storedStars = 0;
            int storedPlanets = 0;
      };

      bool init() {
            if (!GJGarageLayer::init())
                  return false;

            // keep a reference to the stat menu; we'll populate it once we have profile data
            m_fields->statMenu = this->getChildByID("capeling.garage-stats-menu/stats-menu");

            // start fetching profile data; we'll create the stat item in the callback
            fetchProfileData(GJAccountManager::get()->m_accountID);

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
            auto self = this;
            postTask.listen([self](web::WebResponse* response) {
                  if (!self) return;
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
                  int planets = json["planets"].asInt().unwrapOrDefault();
                  int stars = json["stars"].asInt().unwrapOrDefault();

                  log::info("Profile data - points: {}, stars: {}", points, stars);
                  self->m_fields->storedStars = stars;

                  // create/update the stat item now that we have the value
                  log::debug("Updating stat item with {} stars", stars);
                  if (self->m_fields->statMenu) {
                        // if an existing stat item exists, remove it first
                        if (self->m_fields->starsValue) {
                              self->m_fields->starsValue->removeFromParent();
                              self->m_fields->starsValue = nullptr;
                        }
                        if (self->m_fields->planetsValue) {
                              self->m_fields->planetsValue->removeFromParent();
                              self->m_fields->planetsValue = nullptr;
                        }

                        auto starSprite = CCSprite::create("RL_starMed.png"_spr);
                        auto starsValue = StatsDisplayAPI::getNewItem(
                            "blueprint-stars"_spr, starSprite,
                            self->m_fields->storedStars, 0.54f);

                        self->m_fields->starsValue = starsValue;
                        self->m_fields->statMenu->addChild(starsValue);

                        auto planetSprite = CCSprite::create("RL_planetMed.png"_spr);
                        auto planetsValue = StatsDisplayAPI::getNewItem(
                            "planets-collected"_spr, planetSprite,
                            planets, 0.54f);
                        self->m_fields->planetsValue = planetsValue;
                        self->m_fields->statMenu->addChild(planetsValue);
                        // call updateLayout if available
                        if (auto menu = typeinfo_cast<CCMenu*>(self->m_fields->statMenu)) {
                              menu->updateLayout();
                        }
                  } else {
                        log::warn("Stat menu not found; cannot display blueprint-stars stat");
                  }
            });
      }
};