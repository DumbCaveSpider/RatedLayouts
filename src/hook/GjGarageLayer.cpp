#include "../include/RLConstants.hpp"
#include <capeling.garage-stats-menu/include/StatsDisplayAPI.h>

#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class $modify(GJGarageLayer) {
    struct Fields {
        CCNode* myStatItem = nullptr;
        CCNode* statMenu = nullptr;

        CCNode* starsValue = nullptr;
        CCNode* planetsValue = nullptr;
        CCNode* coinsValue = nullptr;
        CCNode* votesValue = nullptr;
        CCNode* pointsValue = nullptr;
        int m_stars = 0;
        int m_planets = 0;
        int m_coins = 0;
        int m_votes = 0;
        int m_points = 0;
        async::TaskHolder<web::WebResponse> m_profileTask;
        async::TaskHolder<Result<std::string>> m_authTask;
        ~Fields() {
            m_profileTask.cancel();
            m_authTask.cancel();
        }
    };

    bool init() {
        if (!GJGarageLayer::init())
            return false;

        m_fields->statMenu =
            this->getChildByID("capeling.garage-stats-menu/stats-menu");
        fetchProfile(GJAccountManager::get()->m_accountID);

        return true;
    }

    void fetchProfile(int accountId) {
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["argonToken"] =
            Mod::get()->getSavedValue<std::string>("argon_token");
        jsonBody["accountId"] = accountId;

        auto postReq = web::WebRequest();
        postReq.bodyJSON(jsonBody);
        m_fields->m_profileTask.cancel();
        m_fields->m_profileTask.spawn(
            postReq.post(std::string(rl::BASE_API_URL) + "/profile"),
            [this](web::WebResponse response) {
                log::info("Received response from server");

                if (!response.ok()) {
                    // log::warn("Server returned non-ok status: {}", response.code());
                    return;
                }

                auto jsonRes = response.json();
                if (!jsonRes) {
                    log::warn("Failed to parse JSON response");
                    return;
                }

                auto json = jsonRes.unwrap();

                int planets = json["planets"].asInt().unwrapOrDefault();
                int stars = json["stars"].asInt().unwrapOrDefault();
                int coins = json["coins"].asInt().unwrapOrDefault();
                int votes = json["votes"].asInt().unwrapOrDefault();
                int points = json["points"].asInt().unwrapOrDefault();

                log::info("Profile data - points: {}, stars: {}", points, stars);
                m_fields->m_stars = stars;
                m_fields->m_planets = planets;
                m_fields->m_coins = coins;
                m_fields->m_votes = votes;
                m_fields->m_points = points;

                // sparks
                auto starSprite =
                    CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
                auto starsValue = StatsDisplayAPI::getNewItem(
                    "rl-sparks"_spr, starSprite, m_fields->m_stars, 0.54f);
                starsValue->setID("rl-stars-value");
                m_fields->statMenu->addChild(starsValue);

                // planets
                auto planetSprite =
                    CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
                auto planetsValue =
                    StatsDisplayAPI::getNewItem("planets-collected"_spr, planetSprite, m_fields->m_planets, 0.54f);
                planetsValue->setID("rl-planets-value");
                m_fields->statMenu->addChild(planetsValue);

                // coints
                auto coinsSprite =
                    CCSprite::createWithSpriteFrameName("RL_BlueCoinSmall.png"_spr);
                auto coinsValue = StatsDisplayAPI::getNewItem(
                    "coins-collected"_spr, coinsSprite, coins, 0.54f);
                coinsValue->setID("rl-coins-value");
                m_fields->statMenu->addChild(coinsValue);

                // votes
                auto votesSprite =
                    CCSprite::createWithSpriteFrameName("RL_commVote01.png"_spr);

                auto votesValue = StatsDisplayAPI::getNewItem(
                    "votes-collected"_spr, votesSprite, votes, 0.54f);
                votesValue->setID("rl-votes-value");
                m_fields->statMenu->addChild(votesValue);

                if (m_fields->m_points > 0) {
                    // points
                    auto pointsSprite = CCSprite::createWithSpriteFrameName(
                        "RL_blueprintPoint01.png"_spr);
                    auto pointsValue = StatsDisplayAPI::getNewItem(
                        "points-collected"_spr, pointsSprite, m_fields->m_points, 0.54f);
                    pointsValue->setID("rl-points-value");
                    m_fields->statMenu->addChild(pointsValue);
                }

                if (auto menu = typeinfo_cast<CCMenu*>(m_fields->statMenu)) {
                    menu->updateLayout();
                }
            });
    }
};
