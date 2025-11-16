#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "ModRatePopup.hpp"

using namespace geode::prelude;

class $modify(RLLevelInfoLayer, LevelInfoLayer)
{
    bool init(GJGameLevel *level, bool challenge)
    {
        if (!LevelInfoLayer::init(level, challenge))
            return false;

        int starRatings = level->m_stars;
        bool legitCompleted = level->m_isCompletionLegitimate;
        auto leftMenu = this->getChildByID("left-side-menu");
        bool isPlatformer = this->m_level->isPlatformer();

        log::debug("isPlatformer = {}, starRatings = {}, legitCompleted = {}", isPlatformer, starRatings, legitCompleted);

        if (Mod::get()->getSavedValue<int>("role") == 1)
        {
            // add a mod button
            auto iconSprite = CCSprite::create("rlStarIconBig.png"_spr);
            CCSprite *buttonSprite = nullptr;

            if ((isPlatformer && starRatings != 0) || starRatings != 0)
            {
                buttonSprite = CCSpriteGrayscale::create("rlStarIconBig.png"_spr);
            }
            else
            {
                buttonSprite = CCSprite::create("rlStarIconBig.png"_spr);
            }

            auto modButtonSpr = CircleButtonSprite::create(
                buttonSprite,
                CircleBaseColor::Cyan,
                CircleBaseSize::Medium);

            auto modButtonItem = CCMenuItemSpriteExtra::create(
                modButtonSpr,
                this,
                menu_selector(RLLevelInfoLayer::onModButton));
            modButtonItem->setID("mod-button");

            leftMenu->addChild(modButtonItem);
        }
        else if (Mod::get()->getSavedValue<int>("role") == 2)
        {
            // add an admin button
            CCSprite *buttonSprite = nullptr;

            if ((isPlatformer && starRatings != 0) || starRatings != 0)
            {
                buttonSprite = CCSpriteGrayscale::create("rlStarIconBig.png"_spr);
            }
            else
            {
                buttonSprite = CCSprite::create("rlStarIconBig.png"_spr);
            }

            auto modButtonSpr = CircleButtonSprite::create(
                buttonSprite,
                CircleBaseColor::Blue,
                CircleBaseSize::Medium);

            auto modButtonItem = CCMenuItemSpriteExtra::create(
                modButtonSpr,
                this,
                menu_selector(RLLevelInfoLayer::onAdminButton));
            modButtonItem->setID("admin-button");

            leftMenu->addChild(modButtonItem);
        }

        leftMenu->updateLayout();

        // fetch rating data
        int levelId = this->m_level->m_levelID;
        log::info("Fetching rating data for level ID: {}", levelId);

        if (this->getChildByID("stars-icon") != nullptr)
        {
            auto getReq = web::WebRequest();
            auto getTask = getReq.get(fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

            getTask.listen([this](web::WebResponse *response)
                           {
            log::info("Received rating response from server");
            
            if (!response->ok())
            {
                log::warn("Server returned non-ok status: {}", response->code());
                return;
            }
            
            auto jsonRes = response->json();
            if (!jsonRes)
            {
                log::warn("Failed to parse JSON response");
                return;
            }
            
            auto json = jsonRes.unwrap();
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();
            
            log::info("difficulty: {}, featured: {}", difficulty, featured);
            
            // Map difficulty
            int difficultyLevel = 0;
            bool isDemon = false;
            switch (difficulty)
            {
            case 1:
                difficultyLevel = -1;
                break;
            case 2:
                difficultyLevel = 1;
                break;
            case 3:
                difficultyLevel = 2;
                break;
            case 4:
            case 5:
                difficultyLevel = 3;
                break;
            case 6:
            case 7:
                difficultyLevel = 4;
                break;
            case 8:
            case 9:
                difficultyLevel = 5;
                break;
            case 10:
                difficultyLevel = 7;
                isDemon = true;
                break;
            case 11:
                difficultyLevel = 8;
                isDemon = true;
                break;
            case 12:
                difficultyLevel = 6;
                isDemon = true;
                break;
            case 13:
                difficultyLevel = 9;
                isDemon = true;
                break;
            case 14:
                difficultyLevel = 10;
                isDemon = true;
                break;
            default:
                difficultyLevel = 0;
                break;
            }
            
            // Update the existing difficulty sprite
            auto difficultySprite = this->getChildByID("difficulty-sprite");
            if (difficultySprite)
            {
                auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);
                sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);

                // star icon
                auto newStarIcon = CCSprite::create("rlStarIcon.png"_spr);
                newStarIcon->setPosition({difficultySprite->getContentSize().width / 2 + 5, -7});
                newStarIcon->setScale(0.53f); // im trying to be exact here alright
                newStarIcon->setID("rl-star-icon");
                difficultySprite->addChild(newStarIcon);

                // star value label
                auto starLabelValue = CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
                starLabelValue->setPosition({newStarIcon->getPositionX() - 7, newStarIcon->getPositionY()});
                starLabelValue->setScale(0.4f);
                starLabelValue->setAnchorPoint({1.0f, 0.5f});
                starLabelValue->setAlignment(kCCTextAlignmentRight);
                starLabelValue->setID("rl-star-label");

                if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(this->m_level->m_levelID))
                {
                    starLabelValue->setColor({ 0, 150, 255 }); // cyan
                }
                difficultySprite->addChild(starLabelValue);

                if (isDemon)
                {
                    sprite->setPositionY(sprite->getPositionY() + 20); // wth is the demon offset cuz of the long text thing
                } else
                {
                    sprite->setPositionY(sprite->getPositionY() + 10);
                }
            }
            
            // Update featured coin visibility
            if (difficultySprite)
            {
                auto featuredCoin = difficultySprite->getChildByID("featured-coin");
                if (featured == 1)
                {
                    if (!featuredCoin)
                    {
                        auto newFeaturedCoin = CCSprite::create("rlfeaturedCoin.png"_spr);
                        newFeaturedCoin->setPosition({difficultySprite->getContentSize().width / 2, difficultySprite->getContentSize().height / 2});
                        newFeaturedCoin->setID("featured-coin");
                        difficultySprite->addChild(newFeaturedCoin, -1);
                    }
                }
                else
                {
                    if (featuredCoin)
                    {
                        featuredCoin->removeFromParent();
                    }
                }
            } });
        }

        return true;
    };

    void levelUpdateFinished(GJGameLevel *level, UpdateResponse response)
    {
        LevelInfoLayer::levelUpdateFinished(level, response);

        int starRatings = level->m_stars;
        if (starRatings)
        {
            return;
        }
        // Fetch rating data from server
        int levelId = this->m_level->m_levelID;
        log::info("Fetching rating data for level ID on update: {}", levelId);

        auto getReq = web::WebRequest();
        auto getTask = getReq.get(fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId));

        getTask.listen([this](web::WebResponse *response)
                       {
            log::info("Received rating response from server on level update");
            
            if (!response->ok())
            {
                log::warn("Server returned non-ok status: {}", response->code());
                return;
            }
            
            auto jsonRes = response->json();
            if (!jsonRes)
            {
                log::warn("Failed to parse JSON response");
                return;
            }
            
            auto json = jsonRes.unwrap();
            int difficulty = json["difficulty"].asInt().unwrapOrDefault();
            int featured = json["featured"].asInt().unwrapOrDefault();
            
            log::info("Rating data - difficulty: {}, featured: {}", difficulty, featured);
            
            // Map difficulty to difficultyLevel
            int difficultyLevel = 0;
            bool isDemon = false;
            switch (difficulty)
            {
            case 1:
                difficultyLevel = -1;
                break;
            case 2:
                difficultyLevel = 1;
                break;
            case 3:
                difficultyLevel = 2;
                break;
            case 4:
            case 5:
                difficultyLevel = 3;
                break;
            case 6:
            case 7:
                difficultyLevel = 4;
                break;
            case 8:
            case 9:
                difficultyLevel = 5;
                break;
            case 10:
                difficultyLevel = 7;
                isDemon = true;
                break;
            case 11:
                difficultyLevel = 8;
                isDemon = true;
                break;
            case 12:
                difficultyLevel = 6;
                isDemon = true;
                break;
            case 13:
                difficultyLevel = 9;
                isDemon = true;
                break;
            case 14:
                difficultyLevel = 10;
                isDemon = true;
                break;
            default:
                difficultyLevel = 0;
                break;
            }
            
            // update difficulty
            auto difficultySprite = this->getChildByID("difficulty-sprite");
            if (difficultySprite)
            {
                auto sprite = static_cast<GJDifficultySprite*>(difficultySprite);
                sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);

                if (isDemon)
                {
                    sprite->setPositionY(sprite->getPositionY() + 20);
                } else
                {
                    sprite->setPositionY(sprite->getPositionY() + 10);
                }

                                // update stars
                auto starIcon = difficultySprite->getChildByID("rl-star-icon");
                auto starLabel = difficultySprite->getChildByID("rl-star-label");

                if (!starIcon)
                {
                    starIcon = CCSprite::create("rlStarIcon.png"_spr);
                    starIcon->setPosition({difficultySprite->getContentSize().width / 2 + 5, -7});
                    starIcon->setScale(0.53f);
                    starIcon->setID("rl-star-icon");
                    difficultySprite->addChild(starIcon);
                }
                else
                {
                    starIcon->setPositionX(difficultySprite->getContentSize().width / 2 + 5);
                }

                if (!starLabel)
                {
                    starLabel = CCLabelBMFont::create(numToString(difficulty).c_str(), "bigFont.fnt");
                    starLabel->setID("rl-star-label");
                    difficultySprite->addChild(starLabel);
                }

                if (starLabel)
                {
                    auto starLabelFont = static_cast<CCLabelBMFont*>(starLabel);
                    starLabelFont->setString(numToString(difficulty).c_str());
                    starLabel->setPosition({starIcon->getPositionX() - 7, starIcon->getPositionY()});
                    starLabel->setScale(0.4f);
                    starLabel->setAnchorPoint({1.0f, 0.5f});
                    
                    // did u beat this legit? lol
                    if (GameStatsManager::sharedState()->hasCompletedOnlineLevel(this->m_level->m_levelID))
                    {
                        starLabelFont->setColor({0, 150, 255}); // cyan
                    }
                }
            }
            
            // Update featured coin visibility
            if (difficultySprite)
            {
                auto featuredCoin = difficultySprite->getChildByID("featured-coin");
                if (featured == 1)
                {
                    if (!featuredCoin)
                    {
                        auto newFeaturedCoin = CCSprite::create("rlfeaturedCoin.png"_spr);
                        newFeaturedCoin->setPosition({0, 0});
                        newFeaturedCoin->setScale(1.1f);
                        newFeaturedCoin->setID("featured-coin");
                        difficultySprite->addChild(newFeaturedCoin, -1);
                    }
                }
                else if (featured == 0)
                {
                    if (featuredCoin)
                    {
                        featuredCoin->removeFromParent();
                    }
                }
            } });
    }

    void onModButton(CCObject *sender)
    {
        int starRatings = this->m_level->m_stars;

        if ((this->m_level->isPlatformer() && starRatings != 0) || starRatings != 0)
        {
            FLAlertLayer::create(
                "Action Unavailable",
                "You cannot perform this action on <cy>platformer or rated levels</c>.",
                "OK")
                ->show();
            return;
        }
        requestStatus(GJAccountManager::get()->m_accountID);

        if (Mod::get()->getSavedValue<int>("role") == 1)
        {
            log::info("Mod button clicked!");
            auto popup = ModRatePopup::create("Mod: Rate Layout", this->m_level);
            popup->show();
        }
        else
        {
            Notification::create("You do not have the required role to perform this action", NotificationIcon::Error)->show();
        }
    }

    void onAdminButton(CCObject *sender)
    {
        int starRatings = this->m_level->m_stars;

        if ((this->m_level->isPlatformer() && starRatings != 0) || starRatings != 0)
        {
            FLAlertLayer::create(
                "Action Unavailable",
                "You cannot perform this action on <cy>platformer or rated levels</c>.",
                "OK")
                ->show();
            return;
        }
        requestStatus(GJAccountManager::get()->m_accountID);

        if (Mod::get()->getSavedValue<int>("role") == 2)
        {
            log::info("Admin button clicked!");
            auto popup = ModRatePopup::create("Admin: Rate Layout", this->m_level);
            popup->show();
        }
        else
        {
            Notification::create("You do not have the required role to perform this action", NotificationIcon::Error)->show();
        }
    }

    void requestStatus(int accountId)
    {
        // argon my beloved <3
        std::string token;
        auto res = argon::startAuth([](Result<std::string> res)
                                    {
        if (!res) {
            log::warn("Auth failed: {}", res.unwrapErr());
            Notification::create(res.unwrapErr(), NotificationIcon::Error)->show();
        }
        auto token = std::move(res).unwrap();
        log::debug("token obtained: {}", token);
        Mod::get()->setSavedValue("argon_token", token); }, [](argon::AuthProgress progress)
                                    { log::debug("auth progress: {}", argon::authProgressToString(progress)); });
        if (!res)
        {
            log::warn("Failed to start auth attempt: {}", res.unwrapErr());
            Notification::create(res.unwrapErr(), NotificationIcon::Error)->show();
        }

        // json boody crap
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
        jsonBody["accountId"] = accountId;

        // verify the user's role
        auto postReq = web::WebRequest();
        postReq.bodyJSON(jsonBody);
        auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/access");

        // handle the response
        postTask.listen([this](web::WebResponse *response)
                        {
            log::info("Received response from server");

            if (!response->ok())
            {
                log::warn("Server returned non-ok status: {}", response->code());
                return;
            }

            auto jsonRes = response->json();
            if (!jsonRes)
            {
                log::warn("Failed to parse JSON response");
                return;
            }

            auto json = jsonRes.unwrap();
            int role = json["role"].asInt().unwrapOrDefault();
            
            // role check lol
            if (role == 1) {
                log::info("User has mod role");
                Mod::get()->setSavedValue<int>("role", role);
            } else if (role == 2) {
                log::info("User has admin role. Enable featured layouts");
                Mod::get()->setSavedValue<int>("role", role);
            } else {
                Mod::get()->setSavedValue<int>("role", 0);
            } });
    }
};