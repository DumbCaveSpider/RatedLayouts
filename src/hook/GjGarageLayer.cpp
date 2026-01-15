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
            CCNode* coinsValue = nullptr;
            int storedStars = 0;
            int storedPlanets = 0;
            int storedCoins = 0;
            utils::web::WebTask profileTask;
            ~Fields() {
                  profileTask.cancel();
            }
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
            m_fields->profileTask = postReq.post("https://gdrate.arcticwoof.xyz/profile");

            // Handle the response
            m_fields->profileTask.listen([this](web::WebResponse* response) {
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
                  int coins = json["coins"].asInt().unwrapOrDefault();

                  log::info("Profile data - points: {}, stars: {}", points, stars);
                  m_fields->storedStars = stars;

                  // create/update the stat item now that we have the value
                  log::debug("Updating stat item with {} stars", stars);
                  if (m_fields->statMenu) {
                        // if an existing stat item exists, remove it first
                        if (m_fields->starsValue) {
                              m_fields->starsValue->removeFromParent();
                              m_fields->starsValue = nullptr;
                        }
                        if (m_fields->planetsValue) {
                              m_fields->planetsValue->removeFromParent();
                              m_fields->planetsValue = nullptr;
                        }
                        if (m_fields->coinsValue) {
                              m_fields->coinsValue->removeFromParent();
                              m_fields->coinsValue = nullptr;
                        }

                        auto starSprite = CCSprite::create("RL_starMed.png"_spr);
                        auto starsValue = StatsDisplayAPI::getNewItem(
                            "blueprint-stars"_spr, starSprite,
                            m_fields->storedStars, 0.54f);

                        m_fields->starsValue = starsValue;
                        starsValue->setID("rl-stars-value");
                        m_fields->statMenu->addChild(starsValue);

                        auto planetSprite = CCSprite::create("RL_planetMed.png"_spr);
                        auto planetsValue = StatsDisplayAPI::getNewItem(
                            "planets-collected"_spr, planetSprite,
                            planets, 0.54f);
                        m_fields->planetsValue = planetsValue;
                        planetsValue->setID("rl-planets-value");
                        m_fields->statMenu->addChild(planetsValue);

                        auto coinsSprite = CCSprite::create("RL_BlueCoinSmall.png"_spr);
                        auto coinsValue = StatsDisplayAPI::getNewItem(
                            "coins-collected"_spr, coinsSprite,
                            coins, 0.54f);
                        m_fields->coinsValue = coinsValue;
                        coinsValue->setID("rl-coins-value");
                        m_fields->statMenu->addChild(coinsValue);

                        // call updateLayout if available
                        if (auto menu = typeinfo_cast<CCMenu*>(m_fields->statMenu)) {
                              menu->updateLayout();
                        }
                  } else {
                        log::warn("Stat menu not found; cannot display blueprint-stars stat");
                  }
            });
      }
};