#include "RLLeaderboardLayer.hpp"
#include "../include/RLAchievements.hpp"
#include "../include/RLLayerBackground.hpp"
#include <cue/RepeatingBackground.hpp>
#include "../include/RLConstants.hpp"

bool RLLeaderboardLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // create if moving bg disabled
    rl::addLayerBackground(this);

    addSideArt(this, SideArt::All, SideArtStyle::Layer, false);

    auto backMenu = CCMenu::create();
    backMenu->setPosition({0, 0});

    addBackButton(this, BackButtonStyle::Pink);

    this->fetchLeaderboard(1, 100);

    auto const listWidth = 356.f;
    auto const listHeight = 220.f;

    m_userListNode = cue::ListNode::create({listWidth, listHeight}, {191, 114, 62, 255}, cue::ListBorderStyle::SlimLevels);
    m_userListNode->setAnchorPoint({0.5f, 0.5f});
    m_userListNode->setPosition({winSize.width / 2 - 5, winSize.height / 2 - 5.f});
    this->addChild(m_userListNode, 5);
    m_scrollLayer = m_userListNode->getScrollLayer();

    if (!Mod::get()->getSettingValue<bool>("disableScrollbar")) {
        auto scrollBar = Scrollbar::create(m_userListNode->getScrollLayer());
        scrollBar->setPosition({m_userListNode->getContentSize().width + 24.f,
            m_userListNode->getContentSize().height / 2});
        scrollBar->setContentHeight(m_userListNode->getContentSize().height - 20);
        m_userListNode->addChild(scrollBar, 10);
    }

    auto contentLayer = m_userListNode->getScrollLayer()->m_contentLayer;
    if (contentLayer) {
        auto layout = ColumnLayout::create();
        contentLayer->setLayout(layout);
        layout->setGap(0.f);
        layout->setAutoGrowAxis(0.f);
        layout->setAxisReverse(true);

        auto spinner = LoadingSpinner::create(100.f);
        spinner->setPosition(m_userListNode->getContentSize() / 2);
        m_userListNode->addChild(spinner);
        m_spinner = spinner;
    }

    auto typeMenu = CCMenu::create();
    typeMenu->setPosition({0, -2});
    typeMenu->setContentSize(m_userListNode->getContentSize());

    auto starsTab = TabButton::create(
        TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Sparks", this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
    float centerX = m_userListNode->getContentSize().width / 2.f;
    const float tabY = 247.f;
    const float spacing = 80.f;

    starsTab->setTag(1);
    starsTab->toggle(true);
    starsTab->setScale(0.8f);
    starsTab->setPosition({centerX - 2.f * spacing, tabY});
    typeMenu->addChild(starsTab);
    m_starsTab = starsTab;

    auto planetsTab = TabButton::create(
        TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Planets", this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
    planetsTab->setTag(3);
    planetsTab->toggle(false);
    planetsTab->setScale(0.8f);
    planetsTab->setPosition({centerX - 1.f * spacing, tabY});
    typeMenu->addChild(planetsTab);
    m_planetsTab = planetsTab;

    auto creatorTab = TabButton::create(
        TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Creator", this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
    creatorTab->setTag(2);
    creatorTab->toggle(false);
    creatorTab->setScale(0.8f);
    creatorTab->setPosition({centerX, tabY});
    typeMenu->addChild(creatorTab);
    m_creatorTab = creatorTab;

    // Top Coins tab (type 4)
    auto coinsTab = TabButton::create(
        TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Coins", this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
    coinsTab->setTag(4);
    coinsTab->toggle(false);
    coinsTab->setScale(0.8f);
    coinsTab->setPosition({centerX + 1.f * spacing, tabY});
    typeMenu->addChild(coinsTab);
    m_coinsTab = coinsTab;

    auto votesTab = TabButton::create(
        TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Top Votes", this, menu_selector(RLLeaderboardLayer::onLeaderboardTypeButton));
    votesTab->setTag(5);
    votesTab->toggle(false);
    votesTab->setScale(0.8f);
    votesTab->setPosition({centerX + 2.f * spacing, tabY});
    typeMenu->addChild(votesTab);
    m_votesTab = votesTab;

    if (Mod::get()->getSettingValue<bool>("disableCreatorPoints") == true) {
        if (m_creatorTab) {
            m_creatorTab->setEnabled(false);
            m_creatorTab->setVisible(false);
        }
    }

    m_userListNode->addChild(typeMenu);

    // info button at the bottom left
    auto infoMenu = CCMenu::create();
    infoMenu->setPosition({0, 0});
    auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoButtonSpr->setScale(0.7f);
    auto infoButton = CCMenuItemSpriteExtra::create(
        infoButtonSpr, this, menu_selector(RLLeaderboardLayer::onInfoButton));
    infoButton->setPosition({25, 25});
    infoMenu->addChild(infoButton);
    this->addChild(infoMenu);

    // refresh button at the bottom right
    auto refreshSpr = CCSprite::createWithSpriteFrameName("RL_refresh01.png"_spr);
    m_refreshBtn = CCMenuItemSpriteExtra::create(
        refreshSpr, this, menu_selector(RLLeaderboardLayer::onRefreshButton));
    m_refreshBtn->setPosition({winSize.width - 35, 35});
    infoMenu->addChild(m_refreshBtn);

    this->scheduleUpdate();
    this->setKeypadEnabled(true);

    return true;
}

void RLLeaderboardLayer::onInfoButton(CCObject* sender) {
    MDPopup::create(
        "Rated Layouts Leaderboard",
        "The leaderboard shows the top players in <cb>Rated Layouts</c> based "
        "on <cl>Sparks</c>, <co>Planets</c>, <cb>Blue Coins</c>, <cf>Blueprint Points</c> and <cg>Votes</c>. You can view each category by selecting the tabs.\n\n"
        "- <cl>Sparks</c> are earned by completing a <cb>Classic Rated Layouts</c> level and are only counted when beaten legitimately.\n"
        "- <co>Planets</c> are earned by completing a <cb>Platformer Rated Layouts</c> level and are only counted when beaten legitimately.\n"
        "- <cb>Blue Coins</c> are earned by collecting them while playing in Rated Layouts levels.\n"
        "- <cf>Blueprint Points</c> are earned based on how many rated layout levels you have in your account, and users who are excluded "
        "won't be affected by this leaderboard.\n\n"
        "Getting a <cs>Rated</c> layout earns you 1 point, <cg>Featured</c> levels earn you 2 points, <cp>Epic</c> "
        "levels earn you 3 points, and <cd>Legendary</c> levels earn you 4 points.\n\n"
        "- <cg>Votes</c> are earned by voting in the <cb>Community Votes</c>. Each vote is earned per level.\n\n"
        "### Any <cr>unfair</c> means of obtaining these stats <cy>(eg. instant complete, noclipping, secret way)</c> will result in an <cr>exclusion from the leaderboard and there will be NO APPEALS!</c> Each completion is <co>publicly logged</c> for this purpose.\n\n",
        "OK")
        ->show();
}

void RLLeaderboardLayer::onAccountClicked(CCObject* sender) {
    auto button = static_cast<CCMenuItem*>(sender);
    int accountId = button->getTag();
    ProfilePage::create(accountId, false)->show();
}

void RLLeaderboardLayer::onRefreshButton(CCObject* sender) {
    // determine active tab type
    int type = 1;
    if (m_starsTab && m_starsTab->isToggled()) {
        type = 1;
    } else if (m_planetsTab && m_planetsTab->isToggled()) {
        type = 3;
    } else if (m_creatorTab && m_creatorTab->isToggled()) {
        type = 2;
    } else if (m_coinsTab && m_coinsTab->isToggled()) {
        type = 4;
    } else if (m_votesTab && m_votesTab->isToggled()) {
        type = 5;
    }

    auto contentLayer = m_scrollLayer ? m_scrollLayer->m_contentLayer : nullptr;
    if (contentLayer) {
        // clear the list and show a spinner
        if (m_userListNode) {
            m_userListNode->clear();
        }

        if (m_spinner) {
            m_spinner->removeFromParent();
            m_spinner = nullptr;
        }
        auto spinner = LoadingSpinner::create(100.f);
        spinner->setPosition(m_userListNode->getContentSize() / 2);
        m_userListNode->addChild(spinner);
        m_spinner = spinner;
    }

    this->fetchLeaderboard(type, 100);
}

void RLLeaderboardLayer::onLeaderboardTypeButton(CCObject* sender) {
    auto button = static_cast<TabButton*>(sender);
    int type = button->getTag();

    if (type == 1 && !m_starsTab->isToggled()) {
        m_starsTab->toggle(true);
        m_planetsTab->toggle(false);
        m_creatorTab->toggle(false);
        if (m_coinsTab)
            m_coinsTab->toggle(false);
    } else if (type == 3 && !m_planetsTab->isToggled()) {
        m_starsTab->toggle(false);
        m_planetsTab->toggle(true);
        m_creatorTab->toggle(false);
        if (m_coinsTab)
            m_coinsTab->toggle(false);
    } else if (type == 2 && !m_creatorTab->isToggled()) {
        m_starsTab->toggle(false);
        m_planetsTab->toggle(false);
        m_creatorTab->toggle(true);
        if (m_coinsTab)
            m_coinsTab->toggle(false);
        if (m_votesTab)
            m_votesTab->toggle(false);
    } else if (type == 4 && m_coinsTab && !m_coinsTab->isToggled()) {
        m_starsTab->toggle(false);
        m_planetsTab->toggle(false);
        m_creatorTab->toggle(false);
        m_coinsTab->toggle(true);
        if (m_votesTab)
            m_votesTab->toggle(false);
    } else if (type == 5 && m_votesTab && !m_votesTab->isToggled()) {
        m_starsTab->toggle(false);
        m_planetsTab->toggle(false);
        m_creatorTab->toggle(false);
        if (m_coinsTab)
            m_coinsTab->toggle(false);
        m_votesTab->toggle(true);
    }

    auto contentLayer = m_scrollLayer->m_contentLayer;
    if (contentLayer) {
        contentLayer->removeAllChildrenWithCleanup(true);
        if (m_spinner) {
            m_spinner->removeFromParent();
            m_spinner = nullptr;
        }
        auto spinner = LoadingSpinner::create(100.f);
        spinner->setPosition(m_userListNode->getContentSize() / 2);
        m_userListNode->addChild(spinner);
        m_spinner = spinner;
    }

    this->fetchLeaderboard(type, 100);
}

void RLLeaderboardLayer::fetchLeaderboard(int type, int amount) {
    Ref<RLLeaderboardLayer> self = this;
    auto request = web::WebRequest().param("type", type).param("amount", amount);
    async::spawn(request.get(std::string(rl::BASE_API_URL) + "/getScore"),
        [self](web::WebResponse response) {
            if (!self)
                return;
            if (!response.ok()) {
                log::warn("Server returned non-ok status: {}",
                    response.code());
                Notification::create("Failed to fetch leaderboard",
                    NotificationIcon::Error)
                    ->show();
                return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("Failed to parse JSON response");
                Notification::create("Invalid server response",
                    NotificationIcon::Error)
                    ->show();
                return;
            }

            auto json = jsonRes.unwrap();
            // log::info("Leaderboard data: {}", json.dump());

            bool success = json["success"].asBool().unwrapOrDefault();
            if (!success) {
                log::warn("Server returned success: false");
                return;
            }

            if (json.contains("users") && json["users"].isArray()) {
                auto users = json["users"].asArray().unwrap();
                self->populateLeaderboard(users);
                if (self->m_scrollLayer)
                    self->m_scrollLayer->scrollToTop();
            } else {
                log::warn("No users array in response");
            }
        });
}

void RLLeaderboardLayer::populateLeaderboard(
    const std::vector<matjson::Value>& users) {
    if (!m_scrollLayer)
        return;

    auto contentLayer = m_scrollLayer->m_contentLayer;
    if (!contentLayer)
        return;

    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }

    if (m_userListNode) {
        m_userListNode->clear();
    }

    int rank = 1;
    for (const auto& userValue : users) {
        if (!userValue.isObject())
            continue;

        int accountId = userValue["accountId"].asInt().unwrapOrDefault();
        int score = userValue["score"].asInt().unwrapOrDefault();
        int nameplateId = userValue["nameplate"].asInt().unwrapOr(0);

        auto rowContainer = CCLayer::create();
        rowContainer->setContentSize({356.f, 40.f});

        int currentAccountID = GJAccountManager::sharedState()->m_accountID;

        CCSprite* bgSprite = CCSprite::create();
        bgSprite->setTextureRect(CCRectMake(0, 0, 356.f, 40.f));
        if (accountId == currentAccountID) {
            bgSprite->setColor({230, 150, 10});
        } else if (rank % 2 == 1) {
            bgSprite->setColor({161, 88, 44});
        } else {
            bgSprite->setColor({194, 114, 62});
        }
        bgSprite->setPosition({178.f, 20.f});
        rowContainer->addChild(bgSprite, 0);

        if (nameplateId != 0 &&
            !Mod::get()->getSettingValue<bool>("disableNameplate")) {
            std::string url = fmt::format(
                "{}/nameplates/banner/nameplate_{}.png",
                std::string(rl::BASE_API_URL),
                nameplateId);
            auto lazy = LazySprite::create({bgSprite->getScaledContentSize() + CCSize(25, 25)}, false);
            lazy->loadFromUrl(url, CCImage::kFmtPng, true);
            lazy->setAutoResize(true);
            lazy->setPosition(
                {bgSprite->getPositionX(), bgSprite->getPositionY()});
            bgSprite->setOpacity(50);
            rowContainer->addChild(lazy, -1);
        }

        // glow for top 3
        if (rank == 1) {
            auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
            glow->setPosition({100.f, 40.5f});
            glow->setRotation(90);
            glow->setAnchorPoint({0.f, 0.5f});
            glow->setScale(5.f);
            glow->setColor({255, 215, 0});
            rowContainer->addChild(glow, 1);
        } else if (rank == 2) {
            auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
            glow->setPosition({100.f, 40.5f});
            glow->setRotation(90);
            glow->setAnchorPoint({0.f, 0.5f});
            glow->setScale(5.f);
            glow->setColor({192, 192, 192});
            rowContainer->addChild(glow, 1);
        } else if (rank == 3) {
            auto glow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
            glow->setPosition({100.f, 40.5f});
            glow->setRotation(90);
            glow->setAnchorPoint({0.f, 0.5f});
            glow->setScale(5.f);
            glow->setColor({205, 127, 50});
            rowContainer->addChild(glow, 1);
        }

        // Rank label
        auto rankLabel =
            CCLabelBMFont::create(fmt::format("{}", rank).c_str(), "goldFont.fnt");
        rankLabel->setScale(0.5f);
        rankLabel->setPosition({15.f, 20.f});
        rankLabel->setAnchorPoint({0.f, 0.5f});
        rowContainer->addChild(rankLabel, 2);

        auto username = userValue["username"].asString().unwrapOrDefault();
        auto accountLabel = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
        accountLabel->setAnchorPoint({0.f, 0.5f});
        accountLabel->setScale(0.7f);

        int iconId = userValue["iconid"].asInt().unwrapOrDefault();
        int color1 = userValue["color1"].asInt().unwrapOrDefault();
        int color2 = userValue["color2"].asInt().unwrapOrDefault();
        int color3 = userValue["color3"].asInt().unwrapOrDefault();

        auto gm = GameManager::sharedState();
        auto player = SimplePlayer::create(iconId);
        player->updatePlayerFrame(iconId, IconType::Cube);
        player->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
        if (color3 != 0) {  // no color 3? no glow
            player->setGlowOutline(gm->colorForIdx(color3));
        }
        player->setPosition({55.f, 20.f});
        player->setScale(0.75f);
        rowContainer->addChild(player, 2);

        auto buttonMenu = CCMenu::create();
        buttonMenu->setPosition({0, 0});

        auto accountButton = CCMenuItemSpriteExtra::create(
            accountLabel, this, menu_selector(RLLeaderboardLayer::onAccountClicked));
        accountButton->setTag(accountId);
        accountButton->setPosition({80.f, 20.f});
        accountButton->setAnchorPoint({0.f, 0.5f});

        buttonMenu->addChild(accountButton);
        rowContainer->addChild(buttonMenu, 2);

        auto scoreLabelText = CCLabelBMFont::create(
            fmt::format("{}", GameToolbox::pointsToString(score)).c_str(),
            "bigFont.fnt");
        scoreLabelText->setScale(0.5f);
        scoreLabelText->setPosition({320.f, 20.f});
        scoreLabelText->setAnchorPoint({1.f, 0.5f});
        rowContainer->addChild(scoreLabelText, 2);

        const bool isStar = m_starsTab->isToggled();
        const bool isPlanets = m_planetsTab && m_planetsTab->isToggled();
        const bool isCoins = m_coinsTab && m_coinsTab->isToggled();
        const bool isVotes = m_votesTab && m_votesTab->isToggled();
        const char* iconName =
            isStar ? "RL_starMed.png"_spr
                   : (isPlanets ? "RL_planetMed.png"_spr
                                : (isCoins ? "RL_BlueCoinSmall.png"_spr
                                           : (isVotes ? "RL_commVote01.png"_spr
                                                      : "RL_blueprintPoint01.png"_spr)));
        auto iconSprite = CCSprite::createWithSpriteFrameName(iconName);
        iconSprite->setScale(0.65f);
        iconSprite->setPosition({325.f, 20.f});
        iconSprite->setAnchorPoint({0.f, 0.5f});
        rowContainer->addChild(iconSprite, 2);

        if (m_userListNode) {
            m_userListNode->addCell(rowContainer);
            m_userListNode->getScrollLayer()->m_contentLayer->updateLayout();
        }

        if (accountId == currentAccountID) {
            accountLabel->setColor({0, 255, 255});
            rankLabel->setColor({0, 255, 255});
            RLAchievements::onReward("misc_leaderboard");  // gg
        }

        rank++;
    }
}

RLLeaderboardLayer* RLLeaderboardLayer::create() {
    auto ret = new RLLeaderboardLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void RLLeaderboardLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}
