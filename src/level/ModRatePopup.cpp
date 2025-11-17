#include "ModRatePopup.hpp"

bool ModRatePopup::setup(std::string title, GJGameLevel *level)
{
    m_title = title;
    m_level = level;
    m_difficultySprite = nullptr;
    m_isDemonMode = false;
    m_isFeatured = false;
    m_selectedRating = -1;
    m_levelId = -1;
    m_accountId = 0;

    // get the level ID ya
    if (level)
    {
        m_levelId = level->m_levelID;
        m_accountId = level->m_accountID;
    }

    // title
    auto titleLabel = CCLabelBMFont::create(m_title.c_str(), "bigFont.fnt");
    titleLabel->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 20});
    m_mainLayer->addChild(titleLabel);

    // menu time
    auto menuButtons = CCMenu::create();
    menuButtons->setPosition({0, 0});

    // normal and demon buttons
    m_normalButtonsContainer = CCMenu::create();
    m_normalButtonsContainer->setPosition({0, 0});
    m_demonButtonsContainer = CCMenu::create();
    m_demonButtonsContainer->setPosition({0, 0});

    // demon buttons only (ratings 10, 15, 20, 25, 30)
    float startX = 50.f;
    float buttonSpacing = 55.f;
    float firstRowY = 110.f;

    // normal difficulty buttons (1-9)
    for (int i = 1; i <= 9; i++)
    {
        auto buttonBg = CCSprite::create("GJ_button_04.png");
        auto buttonLabel = CCLabelBMFont::create(std::to_string(i).c_str(), "bigFont.fnt");
        buttonLabel->setScale(0.75f);
        buttonLabel->setPosition(buttonBg->getContentSize() / 2);
        buttonBg->addChild(buttonLabel);
        buttonBg->setID("button-bg-" + std::to_string(i));

        auto ratingButtonItem = CCMenuItemSpriteExtra::create(
            buttonBg,
            this,
            menu_selector(ModRatePopup::onRatingButton));

        if (i <= 5)
        {
            ratingButtonItem->setPosition({startX + (i - 1) * buttonSpacing, firstRowY});
        }
        else
        {
            ratingButtonItem->setPosition({startX + (i - 6) * buttonSpacing, firstRowY - 55.f});
        }
        ratingButtonItem->setTag(i);
        ratingButtonItem->setID("rating-button-" + std::to_string(i));
        m_normalButtonsContainer->addChild(ratingButtonItem);
    }

    // demon difficulty buttons (10, 15, 20, 25, 30)
    std::vector<int> demonRatings = {10, 15, 20, 25, 30};

    for (int idx = 0; idx < demonRatings.size(); idx++)
    {
        int rating = demonRatings[idx];
        auto buttonBg = CCSprite::create("GJ_button_04.png");
        auto buttonLabel = CCLabelBMFont::create(std::to_string(rating).c_str(), "bigFont.fnt");
        buttonLabel->setScale(0.75f);
        buttonLabel->setPosition(buttonBg->getContentSize() / 2);
        buttonBg->addChild(buttonLabel);
        buttonBg->setID("button-bg-" + std::to_string(rating));

        auto ratingButtonItem = CCMenuItemSpriteExtra::create(
            buttonBg,
            this,
            menu_selector(ModRatePopup::onRatingButton));

        ratingButtonItem->setPosition({startX + idx * buttonSpacing, firstRowY});
        ratingButtonItem->setTag(rating);
        ratingButtonItem->setID("rating-button-" + std::to_string(rating));
        m_demonButtonsContainer->addChild(ratingButtonItem);
    }

    menuButtons->addChild(m_normalButtonsContainer);
    menuButtons->addChild(m_demonButtonsContainer);
    m_demonButtonsContainer->setVisible(false);

    // demon toggle
    auto offDemonSprite = CCSpriteGrayscale::createWithSpriteFrameName("GJ_demonIcon_001.png");
    auto onDemonSprite = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
    auto demonToggle = CCMenuItemToggler::create(
        offDemonSprite,
        onDemonSprite,
        this,
        menu_selector(ModRatePopup::onToggleDemon));

    demonToggle->setPosition({m_mainLayer->getContentSize().width, 0});
    demonToggle->setScale(1.2f);
    menuButtons->addChild(demonToggle);

    // submit button
    int userRole = Mod::get()->getSavedValue<int>("role", 0);
    float submitButtonX = (userRole == 2) ? m_mainLayer->getContentSize().width / 2 - 65 : m_mainLayer->getContentSize().width / 2;

    auto submitButtonSpr = ButtonSprite::create("Submit", 1.f);
    auto submitButtonItem = CCMenuItemSpriteExtra::create(
        submitButtonSpr,
        this,
        menu_selector(ModRatePopup::onSubmitButton));

    submitButtonItem->setPosition({submitButtonX, 0});
    menuButtons->addChild(submitButtonItem);

    // unrate button (only for admins)
    if (userRole == 2)
    {
        auto unreateSpr = ButtonSprite::create("Unrate", 1.f);
        auto unrateButtonItem = CCMenuItemSpriteExtra::create(
            unreateSpr,
            this,
            menu_selector(ModRatePopup::onUnrateButton));

        unrateButtonItem->setPosition({m_mainLayer->getContentSize().width / 2 + 65, 0});
        menuButtons->addChild(unrateButtonItem);

        // info button for admin
        auto infoButton = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
            this,
            menu_selector(ModRatePopup::onInfoButton));
        infoButton->setPosition({m_mainLayer->getContentSize().width, m_mainLayer->getContentSize().height});
        menuButtons->addChild(infoButton);
    }

    // toggle between featured or stars only
    auto offSprite = CCSpriteGrayscale::create("rlfeaturedCoin.png"_spr);
    auto onSprite = CCSprite::create("rlfeaturedCoin.png"_spr);
    auto toggleFeatured = CCMenuItemToggler::create(
        offSprite,
        onSprite,
        this,
        menu_selector(ModRatePopup::onToggleFeatured));

    toggleFeatured->setPosition({0, 0});
    menuButtons->addChild(toggleFeatured);

    m_mainLayer->addChild(menuButtons);

    // difficulty sprite on the right side (NA face by default)
    m_difficultyContainer = CCNode::create();
    m_difficultyContainer->setPosition({m_mainLayer->getContentSize().width - 50.f, 90.f});
    m_difficultySprite = GJDifficultySprite::create(0, (GJDifficultyName)-1);
    m_difficultySprite->setPosition({0, 0});
    m_difficultySprite->setScale(1.2f);
    m_difficultyContainer->addChild(m_difficultySprite);
    m_mainLayer->addChild(m_difficultyContainer);

    // featured score textbox (created conditionally based on role)
    m_featuredScoreInput = geode::TextInput::create(100.f, "Score");
    m_featuredScoreInput->setPosition({300.f, 40.f});
    m_featuredScoreInput->setVisible(false);
    m_featuredScoreInput->setID("featured-score-input");
    m_mainLayer->addChild(m_featuredScoreInput);

    return true;
}

void ModRatePopup::onInfoButton(CCObject *sender)
{
    // matjson payload

    matjson::Value jsonBody = matjson::Value::object();
    jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
    jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
    jsonBody["levelId"] = m_levelId;

    auto postReq = web::WebRequest();
    postReq.bodyJSON(jsonBody);
    auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/level");

    postTask.listen([this](web::WebResponse *response)
    {
        log::info("Received response from server");

        if (!response->ok())
        {
            log::warn("Server returned non-ok status: {}", response->code());
            Notification::create("Failed to fetch level info", NotificationIcon::Error)->show();
            return;
        }

        auto jsonRes = response->json();
        if (!jsonRes)
        {
            log::warn("Failed to parse JSON response");
            Notification::create("Invalid server response", NotificationIcon::Error)->show();
            return;
        }

        auto json = jsonRes.unwrap();
        int levelId = json["levelId"].asInt().unwrapOrDefault();
        double averageDifficulty = json["averageDifficulty"].asDouble().unwrapOrDefault();
        int suggestedTotal = json["suggestedTotal"].asInt().unwrapOrDefault();
        int suggestedFeatured = json["suggestedFeatured"].asInt().unwrapOrDefault();

        std::string infoText = fmt::format(
            "Level ID: {}\n"
            "Average Difficulty: {:.1f}\n"
            "Total Suggested: {}\n"
            "Total Suggested Featured: {}",
            levelId, averageDifficulty, suggestedTotal, suggestedFeatured
        );

        FLAlertLayer::create(
            "Level Status Info",
            infoText,
            "OK")
            ->show();
    });
}

void ModRatePopup::onSubmitButton(CCObject *sender)
{
        log::info("Submitting - Difficulty: {}, Featured: {}, Demon: {}",
                  m_selectedRating, m_isFeatured ? 1 : 0, m_isDemonMode ? 1 : 0);

        // Get argon token
        auto token = Mod::get()->getSavedValue<std::string>("argon_token");
        if (token.empty())
        {
            log::error("Failed to get user token");
            Notification::create("Authentication token not found", NotificationIcon::Error)->show();
            return;
        }

        // matjson payload
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
        jsonBody["argonToken"] = token;
        jsonBody["levelId"] = m_levelId;
        jsonBody["levelOwnerId"] = m_accountId;
        jsonBody["difficulty"] = m_selectedRating;
        jsonBody["featured"] = m_isFeatured ? 1 : 0;

        // add featured score if featured mode is enabled
        if (m_isFeatured && m_featuredScoreInput)
        {
            auto scoreStr = m_featuredScoreInput->getString();
            if (!scoreStr.empty())
            {
                int score = numFromString<int>(scoreStr).unwrapOr(0);
                jsonBody["featuredScore"] = score;
            }
        }

        log::info("Sending request: {}", jsonBody.dump());

        auto postReq = web::WebRequest();
        postReq.bodyJSON(jsonBody);
        auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/rate");

        postTask.listen([this](web::WebResponse *response)
                        {
        log::info("Received response from server");
        
        if (!response->ok())
        {
            log::warn("Server returned non-ok status: {}", response->code());
            Notification::create("Failed to rate layout", NotificationIcon::Error)->show();
            return;
        }
        
        auto jsonRes = response->json();
        if (!jsonRes)
        {
            log::warn("Failed to parse JSON response");
            Notification::create("Invalid server response", NotificationIcon::Error)->show();
            return;
        }
        
        auto json = jsonRes.unwrap();
        bool success = json["success"].asBool().unwrapOrDefault();
        
        if (success)
        {
            log::info("Rate submission successful!");
            Notification::create("Level submitted successfully!",NotificationIcon::Success)->show();
            this->onClose(nullptr);
        }
        else
        {
            log::warn("Rate submission failed: success is false");
            Notification::create("Failed to submit level", NotificationIcon::Error)->show();
        } });
}

void ModRatePopup::onUnrateButton(CCObject *sender)
{
        log::info("Unrate button clicked");

        // Get argon token
        auto token = Mod::get()->getSavedValue<std::string>("argon_token");
        if (token.empty())
        {
            log::error("Failed to get user token");
            Notification::create("Authentication token not found", NotificationIcon::Error)->show();
            return;
        }
        // account ID
        auto accountId = GJAccountManager::get()->m_accountID;

        // matjson payload
        matjson::Value jsonBody = matjson::Value::object();
        jsonBody["accountId"] = accountId;
        jsonBody["argonToken"] = token;
        jsonBody["levelId"] = m_levelId;

        log::info("Sending unrate request: {}", jsonBody.dump());

        auto postReq = web::WebRequest();
        postReq.bodyJSON(jsonBody);
        auto postTask = postReq.post("https://gdrate.arcticwoof.xyz/unrate");

        postTask.listen([this](web::WebResponse *response)
                        {
        log::info("Received response from server");
        
        if (!response->ok())
        {
            log::warn("Server returned non-ok status: {}", response->code());
            Notification::create("Failed to unrate layout", NotificationIcon::Error)->show();
            return;
        }
        
        auto jsonRes = response->json();
        if (!jsonRes)
        {
            log::warn("Failed to parse JSON response");
            Notification::create("Invalid server response", NotificationIcon::Error)->show();
            return;
        }
        
        auto json = jsonRes.unwrap();
        bool success = json["success"].asBool().unwrapOrDefault();
        
        if (success)
        {
            log::info("Unrate submission successful!");
            Notification::create("Layout unrated successfully!",NotificationIcon::Success)->show();
            this->onClose(nullptr);
        }
        else
        {
            log::warn("Unrate submission failed: success is false");
            Notification::create("Failed to unrate layout", NotificationIcon::Error)->show();
        } });
}

void ModRatePopup::onToggleFeatured(CCObject *sender)
{
        // Check if user has admin role
        int userRole = Mod::get()->getSavedValue<int>("role", 0);

        m_isFeatured = !m_isFeatured;

        auto existingCoin = m_difficultyContainer->getChildByID("featured-coin");
        if (existingCoin)
        {
            existingCoin->removeFromParent(); // could do setVisible false but whatever
        }

        if (m_isFeatured)
        {
            auto featuredCoin = CCSprite::create("rlfeaturedCoin.png"_spr);
            featuredCoin->setPosition({0, 0});
            featuredCoin->setScale(1.2f);
            featuredCoin->setID("featured-coin");
            m_difficultyContainer->addChild(featuredCoin, -1);
            // score only for admin
            if (userRole == 2)
            {
                m_featuredScoreInput->setVisible(true);
            }
        }
        else
        {
            m_featuredScoreInput->setVisible(false);
        }
}

void ModRatePopup::onToggleDemon(CCObject *sender)
{
        m_isDemonMode = !m_isDemonMode;

        m_normalButtonsContainer->setVisible(!m_isDemonMode);
        m_demonButtonsContainer->setVisible(m_isDemonMode);
}

void ModRatePopup::onRatingButton(CCObject *sender)
{
        auto button = static_cast<CCMenuItemSpriteExtra *>(sender);
        int rating = button->getTag();

        // reset the bg of the previously selected button
        if (m_selectedRating != -1)
        {
            CCMenu *prevContainer = (m_selectedRating <= 9) ? m_normalButtonsContainer : m_demonButtonsContainer;
            auto prevButton = prevContainer->getChildByID("rating-button-" + std::to_string(m_selectedRating));
            if (prevButton)
            {
                auto prevButtonItem = static_cast<CCMenuItemSpriteExtra *>(prevButton);
                auto prevButtonBg = CCSprite::create("GJ_button_04.png");
                auto prevButtonLabel = CCLabelBMFont::create(std::to_string(m_selectedRating).c_str(), "bigFont.fnt");
                prevButtonLabel->setPosition(prevButtonBg->getContentSize() / 2);
                prevButtonLabel->setScale(0.75f);
                prevButtonBg->addChild(prevButtonLabel);
                prevButtonBg->setID("button-bg-" + std::to_string(m_selectedRating));
                prevButtonItem->setNormalImage(prevButtonBg);
            }
        }

        auto currentButton = static_cast<CCMenuItemSpriteExtra *>(sender);
        auto currentButtonBg = CCSprite::create("GJ_button_01.png");
        auto currentButtonLabel = CCLabelBMFont::create(std::to_string(rating).c_str(), "bigFont.fnt");
        currentButtonLabel->setPosition(currentButtonBg->getContentSize() / 2);
        currentButtonLabel->setScale(0.75f);
        currentButtonBg->addChild(currentButtonLabel);
        currentButtonBg->setID("button-bg-" + std::to_string(rating));
        currentButton->setNormalImage(currentButtonBg);

        m_selectedRating = button->getTag();

        updateDifficultySprite(rating);
}

void ModRatePopup::updateDifficultySprite(int rating)
{
        if (m_difficultySprite)
        {
            m_difficultySprite->removeFromParent();
        }

        int difficultyLevel;

        switch (rating)
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
            break;
        case 15:
            difficultyLevel = 8;
            break;
        case 20:
            difficultyLevel = 6;
            break;
        case 25:
            difficultyLevel = 9;
            break;
        case 30:
            difficultyLevel = 10;
            break;
        default:
            difficultyLevel = 0;
            break;
        }

        m_difficultySprite = GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Short);
        m_difficultySprite->setPosition({0, 0});
        m_difficultySprite->setScale(1.2f);
        m_difficultyContainer->addChild(m_difficultySprite);
}

ModRatePopup *ModRatePopup::create(std::string title, GJGameLevel *level)
{
        auto ret = new ModRatePopup();

        if (ret && ret->initAnchored(380.f, 180.f, title, level, "GJ_square02.png"))
        {
            ret->autorelease();
            return ret;
        };

        CC_SAFE_DELETE(ret);
        return nullptr;
};