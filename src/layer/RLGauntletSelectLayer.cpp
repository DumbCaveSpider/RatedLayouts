#include "RLGauntletSelectLayer.hpp"
#include "../include/RLConstants.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/BoomScrollLayer.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <filesystem>
#include <unordered_set>

static std::filesystem::path gauntletCompletedPath() {
    return dirs::getModsSaveDir() / Mod::get()->getID() / "gauntlet_completed.json";
}

static std::unordered_set<int> getCompletedGauntletsSet() {
    std::unordered_set<int> out;
    auto path = gauntletCompletedPath();
    if (!std::filesystem::exists(path)) {
        return out;
    }

    auto existing = utils::file::readString(utils::string::pathToString(path));
    if (!existing) {
        return out;
    }

    auto parsed = matjson::parse(existing.unwrap());
    if (!parsed || !parsed.unwrap().isObject()) {
        return out;
    }

    auto root = parsed.unwrap();
    auto completed = root["completed_gauntlets"];
    if (!completed.isArray()) {
        return out;
    }

    for (auto& v : completed.asArray().unwrap()) {
        int id = v.asInt().unwrapOr(-1);
        if (id > 0) {
            out.insert(id);
        }
    }
    return out;
}

static matjson::Value loadGauntletCompletedJson() {
    auto path = gauntletCompletedPath();
    if (!std::filesystem::exists(path)) {
        return matjson::Value::object();
    }

    auto existing = utils::file::readString(utils::string::pathToString(path));
    if (!existing) {
        return matjson::Value::object();
    }

    auto parsed = matjson::parse(existing.unwrap());
    if (!parsed || !parsed.unwrap().isObject()) {
        return matjson::Value::object();
    }

    return parsed.unwrap();
}

static std::unordered_set<int> getCompletedLevelsForGauntlet(int gauntletId) {
    std::unordered_set<int> out;
    auto data = loadGauntletCompletedJson();
    if (!data.isObject()) {
        return out;
    }

    auto gauntletLevels = data["gauntlet_levels"];
    if (!gauntletLevels.isObject()) {
        return out;
    }

    auto levelArr = gauntletLevels[std::to_string(gauntletId)];
    if (!levelArr.isArray()) {
        return out;
    }

    for (auto& v : levelArr.asArray().unwrap()) {
        int lid = v.asInt().unwrapOr(-1);
        if (lid > 0) {
            out.insert(lid);
        }
    }
    return out;
}

static bool isGauntletLevelCompleted(int levelId) {
    if (levelId <= 0) {
        return false;
    }

    auto glm = GameLevelManager::sharedState();
    auto gsm = GameStatsManager::sharedState();
    if (!glm || !gsm) {
        return false;
    }

    auto searchObj = GJSearchObject::create(SearchType::Search, fmt::format("{}", levelId));
    auto stored = glm->getStoredOnlineLevels(searchObj->getKey());
    if (!stored || stored->count() == 0) {
        return false;
    }

    for (unsigned int si = 0; si < stored->count(); ++si) {
        auto g = static_cast<GJGameLevel*>(stored->objectAtIndex(si));
        if (!g || static_cast<int>(g->m_levelID) != levelId) {
            continue;
        }
        return gsm->hasCompletedLevel(g);
    }

    return false;
}

#include "RLAnnouncementPopup.hpp"
#include "RLGauntletLevelsLayer.hpp"

using namespace geode::prelude;

RLGauntletSelectLayer* RLGauntletSelectLayer::create() {
    auto ret = new RLGauntletSelectLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RLGauntletSelectLayer::init() {
    if (!CCLayer::init())
        return false;

    auto bg = createLayerBG();
    addChild(bg, -1);
    bg->setColor({50, 50, 50});

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // title
    auto titleSprite =
        CCSprite::createWithSpriteFrameName("RL_titleGauntlet.png"_spr);
    titleSprite->setPosition({winSize.width / 2, winSize.height - 30});
    titleSprite->setScale(.7f);
    this->addChild(titleSprite, 10);

    // crap
    addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    this->addChild(menu, 1);

    addBackButton(this, BackButtonStyle::Green);

    this->setKeypadEnabled(true);

    m_loadingCircle = LoadingSpinner::create(100.f);
    m_loadingCircle->setPosition(winSize / 2);
    this->addChild(m_loadingCircle);

    // @geode-ignore(unknown-resource)
    auto announceSpr =
        CCSprite::createWithSpriteFrameName("RL_gauntletBtn01.png"_spr);
    announceSpr->setScale(0.7f);
    auto announceBtn = CCMenuItemSpriteExtra::create(
        announceSpr, this, menu_selector(RLGauntletSelectLayer::onInfoButton));
    announceBtn->setPosition({25, 25});
    menu->addChild(announceBtn);
    fetchGauntlets();
    return true;
}

void RLGauntletSelectLayer::onInfoButton(CCObject* sender) {
    auto announcement = RLAnnouncementPopup::create();
    announcement->show();
}

void RLGauntletSelectLayer::onGauntletButtonClick(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!item)
        return;
    int index = item->getTag();

    if (index >= 0 && index < static_cast<int>(m_gauntletsArray.size())) {
        auto gauntlet = m_gauntletsArray[index];
        auto layer = RLGauntletLevelsLayer::create(gauntlet);
        auto scene = CCScene::create();
        scene->addChild(layer);
        CCDirector::sharedDirector()->pushScene(
            CCTransitionFade::create(0.5f, scene));
    }
}

void RLGauntletSelectLayer::fetchGauntlets() {
    web::WebRequest request;
    m_gauntletsTask.spawn(
        request.get(std::string(rl::BASE_API_URL) + "/getGauntlets"),
        [this](web::WebResponse const& response) {
            if (response.ok()) {
                auto jsonRes = response.json();
                if (jsonRes.isOk()) {
                    this->onGauntletsFetched(jsonRes.unwrap());
                } else {
                    log::error("Failed to parse JSON: {}", jsonRes.unwrapErr());
                    Notification::create("Failed to parse gauntlets data",
                        NotificationIcon::Error)
                        ->show();
                }
            } else {
                log::error("Failed to fetch gauntlets: {}",
                    response.string().unwrapOr("Unknown error"));
                Notification::create("Failed to fetch gauntlets",
                    NotificationIcon::Error)
                    ->show();
            }
        });
}

void RLGauntletSelectLayer::onGauntletsFetched(matjson::Value const& json) {
    m_loadingCircle->setVisible(false);
    createGauntletButtons(json);
}

void RLGauntletSelectLayer::createGauntletButtons(
    matjson::Value const& gauntlets) {
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    if (m_scrollLayer) {
        m_scrollLayer->removeFromParent();
        m_scrollLayer = nullptr;
    }

    if (!gauntlets.isArray()) {
        log::error("Expected gauntlets to be an array");
        return;
    }

    auto gauntletsArray = gauntlets.asArray().unwrap();
    auto completedGauntlets = getCompletedGauntletsSet();

    // Store gauntlets for later reference
    m_gauntletsArray.clear();
    m_gauntletButtons.clear();
    for (auto& g : gauntletsArray) {
        m_gauntletsArray.push_back(g);
    }

    auto pages = CCArray::create();
    pages->retain();

    CCLayer* page = nullptr;
    CCMenu* pageMenu = nullptr;

    for (size_t i = 0; i < gauntletsArray.size(); i++) {
        if (i % m_pageSize == 0) {
            page = CCLayer::create();
            page->setContentSize(winSize);
            pageMenu = CCMenu::create();
            pageMenu->setLayout(RowLayout::create()->setGap(3.f)->setAxisAlignment(AxisAlignment::Center));
            pageMenu->setPosition({page->getContentSize().width / 2, winSize.height / 2 + 35});
            page->addChild(pageMenu);
            pages->addObject(page);
        }

        auto gauntlet = gauntletsArray[i];
        if (!gauntlet.isObject())
            continue;

        int gauntletId = gauntlet["id"].asInt().unwrapOr(0);
        std::string gauntletName = gauntlet["name"].asString().unwrapOr("Unknown");

        auto gauntletBg = NineSlice::create("GJ_squareB_01.png");
        gauntletBg->setContentSize({110, 240});

        // name label with line breaks at spaces
        std::string formattedName = gauntletName;
        std::replace(formattedName.begin(), formattedName.end(), ' ', '\n');
        auto nameLabel =
            CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
        auto nameLabelShadow =
            CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
        nameLabel->setAlignment(kCCTextAlignmentCenter);
        nameLabel->setPosition({gauntletBg->getContentSize().width / 2,
            gauntletBg->getContentSize().height - 15});
        nameLabel->setAnchorPoint({0.5f, 1.0f});
        nameLabel->setScale(0.5f);

        nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
        nameLabelShadow->setPosition(
            {nameLabel->getPositionX() + 2, nameLabel->getPositionY() - 2});
        nameLabelShadow->setAnchorPoint({0.5f, 1.0f});
        nameLabelShadow->setColor({0, 0, 0});
        nameLabelShadow->setOpacity(60);
        nameLabelShadow->setScale(0.5f);

        gauntletBg->addChild(nameLabel, 3);
        gauntletBg->addChild(nameLabelShadow, 2);

        // difficulty range label (min-max) with star icon to the right
        int minDiff = gauntlet["min_difficulty"].asInt().unwrapOr(0);
        int maxDiff = gauntlet["max_difficulty"].asInt().unwrapOr(0);
        bool gauntletComplete = completedGauntlets.count(gauntletId) > 0;

        if (gauntletComplete) {
            auto completeIconShadow = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
            if (completeIconShadow) {
                completeIconShadow->setColor({0, 0, 0});
                completeIconShadow->setOpacity(120);
                completeIconShadow->setScale(1.1f);
                completeIconShadow->setPosition({gauntletBg->getContentSize().width / 2 + 2, 48});
                completeIconShadow->setAnchorPoint({0.5f, 0.5f});
                gauntletBg->addChild(completeIconShadow, 2);
            }

            auto completeIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
            if (completeIcon) {
                completeIcon->setScale(1.1f);
                completeIcon->setPosition({gauntletBg->getContentSize().width / 2, 50});
                completeIcon->setAnchorPoint({0.5f, 0.5f});
                gauntletBg->addChild(completeIcon, 3);
            }
        } else if (minDiff > 0 || maxDiff > 0) {
            std::string diffText = fmt::format("{}-{}", minDiff, maxDiff);

            auto diffLabel = CCLabelBMFont::create(diffText.c_str(), "bigFont.fnt");
            auto diffLabelShadow =
                CCLabelBMFont::create(diffText.c_str(), "bigFont.fnt");
            diffLabelShadow->setColor({0, 0, 0});
            diffLabelShadow->setOpacity(60);
            if (diffLabel) {
                diffLabel->setAlignment(kCCTextAlignmentCenter);
                diffLabel->setScale(0.45f);
                diffLabel->setPosition(
                    {gauntletBg->getContentSize().width / 2 + 10, 50});
                diffLabel->setAnchorPoint({1.f, 0.5f});
                diffLabelShadow->setAlignment(kCCTextAlignmentCenter);
                diffLabelShadow->setAnchorPoint({1.f, 0.5f});
                diffLabelShadow->setScale(0.45f);
                diffLabelShadow->setPosition(
                    {diffLabel->getPositionX() + 2, diffLabel->getPositionY() - 2});

                gauntletBg->addChild(diffLabel, 3);
                gauntletBg->addChild(diffLabelShadow, 2);

                auto diffStar =
                    CCSprite::createWithSpriteFrameName("RL_starSmall.png"_spr);
                auto diffStarShadow =
                    CCSprite::createWithSpriteFrameName("RL_starSmall.png"_spr);
                if (diffStar && diffStarShadow) {
                    diffStar->setAnchorPoint({0.f, 0.5f});
                    diffStar->setPosition({gauntletBg->getContentSize().width / 2 + 15,
                        diffLabel->getPositionY()});

                    diffStarShadow->setAnchorPoint({0.f, 0.5f});
                    diffStarShadow->setPosition(
                        {diffStar->getPositionX() + 2, diffStar->getPositionY() - 2});
                    diffStarShadow->setColor({0, 0, 0});
                    diffStarShadow->setOpacity(60);
                    gauntletBg->addChild(diffStarShadow, 2);
                    gauntletBg->addChild(diffStar, 3);
                }
            }
        }

        std::string spriteName = fmt::format("RL_gauntlet-{}.png"_spr, gauntletId);
        auto gauntletSprite =
            CCSprite::createWithSpriteFrameName(spriteName.c_str());
        auto gauntletSpriteShadow =
            CCSprite::createWithSpriteFrameName(spriteName.c_str());
        gauntletSpriteShadow->setColor({0, 0, 0});
        gauntletSpriteShadow->setOpacity(50);
        gauntletSpriteShadow->setScaleY(1.2f);

        int r = gauntlet["r"].asInt().unwrapOr(255);
        int g = gauntlet["g"].asInt().unwrapOr(255);
        int b = gauntlet["b"].asInt().unwrapOr(255);
        gauntletBg->setColor({static_cast<GLubyte>(r), static_cast<GLubyte>(g), static_cast<GLubyte>(b)});
        gauntletBg->addChild(gauntletSprite, 3);
        gauntletBg->addChild(gauntletSpriteShadow, 2);
        gauntletSprite->setPosition({gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height / 2 + 10});
        gauntletSpriteShadow->setPosition(
            {gauntletSprite->getPositionX(), gauntletSprite->getPositionY() - 6});

        int totalLevels = 0;
        int completedLevels = 0;
        std::vector<int> levelIdsList;
        auto levelIds = gauntlet["levelids"];
        if (!levelIds.isString()) {
            levelIds = gauntlet["levelIds"];
        }

        if (levelIds.isString()) {
            auto levelIdsStr = levelIds.asString().unwrapOr("");
            size_t start = 0;
            while (start < levelIdsStr.size()) {
                auto comma = levelIdsStr.find(',', start);
                auto token = levelIdsStr.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                auto first = token.find_first_not_of(" \t\n\r");
                auto last = token.find_last_not_of(" \t\n\r");
                if (first != std::string::npos && last != std::string::npos) {
                    token = token.substr(first, last - first + 1);
                }
                if (!token.empty()) {
                    int levelId = std::atoi(token.c_str());
                    if (levelId > 0) {
                        levelIdsList.push_back(levelId);
                    }
                }
                if (comma == std::string::npos) {
                    break;
                }
                start = comma + 1;
            }
        } else if (levelIds.isArray()) {
            for (auto& levelValue : levelIds.asArray().unwrap()) {
                int levelId = levelValue.asInt().unwrapOr(0);
                if (levelId > 0) {
                    levelIdsList.push_back(levelId);
                }
            }
        }

        totalLevels = static_cast<int>(levelIdsList.size());
        auto completedLevelsSet = getCompletedLevelsForGauntlet(gauntletId);
        for (auto levelId : levelIdsList) {
            if (completedLevelsSet.count(levelId) > 0 || isGauntletLevelCompleted(levelId)) {
                completedLevels++;
            }
        }

        auto progressText = fmt::format("{}/{}", completedLevels, totalLevels);
        auto progressLabel = CCLabelBMFont::create(progressText.c_str(), "bigFont.fnt");
        auto progressShadow = CCLabelBMFont::create(progressText.c_str(), "bigFont.fnt");
        progressLabel->setAlignment(kCCTextAlignmentCenter);
        progressLabel->setAnchorPoint({0.5f, 0.5f});
        progressLabel->setScale(0.4f);
        progressLabel->setPosition({gauntletSprite->getPosition().x, gauntletSprite->getPosition().y - 40});

        progressShadow->setAlignment(kCCTextAlignmentCenter);
        progressShadow->setAnchorPoint({0.5f, 0.5f});
        progressShadow->setScale(0.4f);
        progressShadow->setPosition({progressLabel->getPositionX() + 2, progressLabel->getPositionY() - 2});
        progressShadow->setColor({0, 0, 0});
        progressShadow->setOpacity(100);

        gauntletBg->addChild(progressShadow, 2);
        gauntletBg->addChild(progressLabel, 3);

        auto button = CCMenuItemSpriteExtra::create(
            gauntletBg, this, menu_selector(RLGauntletSelectLayer::onGauntletButtonClick));
        button->setTag(static_cast<int>(i));
        button->m_scaleMultiplier = 1.05f;

        m_gauntletButtons.push_back(button);
        if (pageMenu) {
            pageMenu->addChild(button);
            pageMenu->updateLayout();
        }
    }

    m_scrollLayer = BoomScrollLayer::create(pages, 0, false);
    if (m_scrollLayer) {
        m_scrollLayer->setPosition({0, -45});
        this->addChild(m_scrollLayer);
    }

    int pageCount = pages->count();
    if (pageCount > 1) {
        auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        if (prevSpr)
            prevSpr->setFlipX(true);

        m_prevPageBtn = CCMenuItemSpriteExtra::create(
            prevSpr, this, menu_selector(RLGauntletSelectLayer::onPrevPage));
        m_nextPageBtn = CCMenuItemSpriteExtra::create(
            nextSpr, this, menu_selector(RLGauntletSelectLayer::onNextPage));

        m_prevPageBtn->setPosition({25, winSize.height / 2.f});
        m_nextPageBtn->setPosition({winSize.width - 25, winSize.height / 2.f});

        auto navMenu = CCMenu::create();
        navMenu->setPosition({0, 0});
        navMenu->addChild(m_prevPageBtn);
        navMenu->addChild(m_nextPageBtn);
        this->addChild(navMenu, 2);

        updatePage();
    }

    pages->release();
}

void RLGauntletSelectLayer::onPrevPage(CCObject* sender) {
    if (!m_scrollLayer)
        return;

    int totalPages = m_scrollLayer->getTotalPages();
    if (totalPages <= 1)
        return;

    int page = m_scrollLayer->m_page;
    if (page > 0) {
        page--;
        m_scrollLayer->moveToPage(page);
        updatePage();
    }
}

void RLGauntletSelectLayer::onNextPage(CCObject* sender) {
    if (!m_scrollLayer)
        return;

    int totalPages = m_scrollLayer->getTotalPages();
    if (totalPages <= 1)
        return;

    int page = m_scrollLayer->m_page;
    if (page < totalPages - 1) {
        page++;
        m_scrollLayer->moveToPage(page);
        updatePage();
    }
}

void RLGauntletSelectLayer::updatePage() {
    if (!m_scrollLayer)
        return;

    int totalPages = m_scrollLayer->getTotalPages();
    int page = m_scrollLayer->m_page;
    if (page < 0)
        page = 0;
    if (page >= totalPages)
        page = std::max(0, totalPages - 1);

    if (m_prevPageBtn) {
        m_prevPageBtn->setEnabled(page > 0);
        m_prevPageBtn->setOpacity(page > 0 ? 255 : 120);
    }
    if (m_nextPageBtn) {
        m_nextPageBtn->setEnabled(page < totalPages - 1);
        m_nextPageBtn->setOpacity(page < totalPages - 1 ? 255 : 120);
    }
}
void RLGauntletSelectLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}
