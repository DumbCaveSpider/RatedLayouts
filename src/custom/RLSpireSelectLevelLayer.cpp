#include <Geode/Geode.hpp>
#include <cue/RepeatingBackground.hpp>
#include "Geode/cocos/script_support/CCScriptSupport.h"
#include "Geode/ui/MDPopup.hpp"
#include "Geode/utils/general.hpp"
#include "RLSpireSelectLevelLayer.hpp"
#include <unordered_set>
#include <filesystem>

using namespace geode::prelude;

static std::filesystem::path spireCompletedLevelPath() {
    return dirs::getModsSaveDir() / Mod::get()->getID() / "spire_completed_levels.json";
}

static std::unordered_set<int> getSpireCompletedLevelSet() {
    std::unordered_set<int> out;
    auto path = spireCompletedLevelPath();

    if (!std::filesystem::exists(path)) {
        return out;
    }

    auto existing = utils::file::readString(utils::string::pathToString(path));
    if (!existing) {
        return out;
    }

    auto parsed = matjson::parse(existing.unwrap());
    if (!parsed) {
        return out;
    }

    auto root = parsed.unwrap();
    if (!root.isArray()) {
        return out;
    }

    for (auto& v : root.asArray().unwrap()) {
        int id = v.asInt().unwrapOr(-1);
        if (id > 0) {
            out.insert(id);
        }
    }

    return out;
}

static void saveSpireCompletedLevelSet(std::unordered_set<int> const& set) {
    auto path = spireCompletedLevelPath();
    std::filesystem::create_directories(path.parent_path());

    matjson::Value root = matjson::Value::array();
    for (auto id : set) {
        root.asArray().unwrap().push_back(matjson::Value(id));
    }

    auto jsonString = root.dump();
    auto writeRes = utils::file::writeString(utils::string::pathToString(path), jsonString);
    if (!writeRes) {
        log::warn("Failed to write spire completed levels to {}", path.string());
    }
}

const std::string doorOpenParticle = "30,2065,2,2715,3,2265,155,2,156,8,145,50a-1a1a0.3a38a0a180a0a0a0a0a0a0a0a0a0a0a50a1a0a0a0.701961a0a1a0a1a0a1a0a0a1a0a0a0.176471a0a0.290196a0a0.807843a0a1a0a1a0a0.35a0a30a0a0a0a-200a0a1a2a1a0a0a1a158a0a0a0a0a0a0a0a0a0a0a0a0a0a0;";

RLSpireSelectLevelLayer* RLSpireSelectLevelLayer::create() {
    auto layer = new RLSpireSelectLevelLayer();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool RLSpireSelectLevelLayer::isSpireLevelCompleted(int levelId) {
    if (levelId <= 0) {
        return false;
    }

    auto localSet = getSpireCompletedLevelSet();
    if (localSet.count(levelId) > 0) {
        return true;
    }

    // fallback to existing game completion data
    auto glm = GameLevelManager::sharedState();
    auto gsm = GameStatsManager::sharedState();
    if (!glm || !gsm) {
        return false;
    }

    auto searchObj = GJSearchObject::create(SearchType::Search, numToString(levelId));
    auto stored = glm->getStoredOnlineLevels(searchObj->getKey());
    if (!stored) {
        return false;
    }

    for (unsigned int i = 0; i < stored->count(); ++i) {
        auto gjl = static_cast<GJGameLevel*>(stored->objectAtIndex(i));
        if (gjl && static_cast<int>(gjl->m_levelID) == levelId) {
            bool completed = gsm->hasCompletedLevel(gjl);
            if (completed) {
                RLSpireSelectLevelLayer::setSpireLevelCompleted(levelId);
            }
            return completed;
        }
    }

    return false;
}

void RLSpireSelectLevelLayer::setSpireLevelCompleted(int levelId) {
    if (levelId <= 0) {
        return;
    }

    auto set = getSpireCompletedLevelSet();
    if (set.insert(levelId).second) {
        saveSpireCompletedLevelSet(set);
    }
}

void RLSpireSelectLevelLayer::updateNavArrowState() {
    m_allCompleted = true;
    if (m_levelIds.empty()) {
        m_allCompleted = false;
    } else {
        for (unsigned int i = 0; i < m_levelIds.size(); ++i) {
            int levelId = m_levelIds[i];
            if (levelId <= 0 || i >= m_levelCompleted.size() || !m_levelCompleted[i]) {
                m_allCompleted = false;
                break;
            }
        }
    }

    if (m_navArrowUp) {
        auto arrowSpr = typeinfo_cast<CCSprite*>(m_navArrowUp->getNormalImage());
        if (arrowSpr) {
            arrowSpr->setColor(m_allCompleted ? ccWHITE : ccc3(50, 50, 50));
        }
    }

    if (m_navArrowDown) {
        bool canGoDown = m_spireRoomIndex > 1;
        m_navArrowDown->setEnabled(canGoDown);
        m_navArrowDown->setVisible(canGoDown);
        auto downSpr = typeinfo_cast<CCSprite*>(m_navArrowDown->getNormalImage());
        if (downSpr) {
            downSpr->setColor(canGoDown ? ccWHITE : ccc3(50, 50, 50));
        }
    }
}

void RLSpireSelectLevelLayer::showRoomTransition() {
    if (m_isRoomTransitionActive) {
        return;
    }

    if (m_levelsMenu) {
        m_levelsMenu->removeAllChildrenWithCleanup(true);
    }
    m_levelIds.clear();
    m_levelCompleted.clear();

    if (!m_transitionLayer) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        m_transitionLayer = CCLayerColor::create({0, 0, 0, 0}, winSize.width, winSize.height);
        m_transitionLayer->setPosition({0, 0});
        this->addChild(m_transitionLayer, 5);
    }

    m_transitionLayer->setOpacity(0);
    m_transitionLayer->setVisible(true);
    m_transitionLayer->runAction(CCFadeTo::create(0.3f, 255));

    if (!m_loadingSpinner) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        m_loadingSpinner = LoadingSpinner::create(60.f);
        if (m_loadingSpinner) {
            m_loadingSpinner->setPosition({winSize.width / 2.0f, winSize.height / 2.0f});
            this->addChild(m_loadingSpinner, 6);
        }
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
        m_loadingSpinner->setOpacity(0);
        m_loadingSpinner->runAction(CCFadeTo::create(0.3f, 255));
    }

    m_isRoomTransitionActive = true;
    this->runAction(CCSequence::create(CCDelayTime::create(0.6f), CCCallFunc::create(this, callfunc_selector(RLSpireSelectLevelLayer::onRoomTransitionFetch)), nullptr));
}

void RLSpireSelectLevelLayer::onRoomTransitionFetch() {
    if (!m_isRoomTransitionActive) {
        return;
    }

    fetchSpireLevels();
}

void RLSpireSelectLevelLayer::completeRoomTransition() {
    if (!m_transitionLayer)
        return;

    auto fadeOutTransition = CCFadeTo::create(0.3f, 0);
    auto hideAction = CCCallFunc::create(this, callfunc_selector(RLSpireSelectLevelLayer::onRoomTransitionComplete));

    m_transitionLayer->runAction(CCSequence::create(CCDelayTime::create(.3f), fadeOutTransition, hideAction, nullptr));
    if (m_loadingSpinner) {
        m_loadingSpinner->runAction(CCSequence::create(CCDelayTime::create(1.0f), CCFadeTo::create(0.3f, 0), nullptr));
    }
}

void RLSpireSelectLevelLayer::rewardRoomTransition() {
    if (m_spireRoomIndex < 2) {
        return;
    }

    int highestExplored = Mod::get()->getSavedValue<int>("highestSpireRoomExplored", 0);
    if (m_spireRoomIndex <= highestExplored) {
        return;
    }

    Mod::get()->setSavedValue("highestSpireRoomExplored", m_spireRoomIndex);

    const int reward = 5000;
    int oldRubies = Mod::get()->getSavedValue<int>("rubies", 0);
    Mod::get()->setSavedValue<int>("rubies", oldRubies + reward);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto rewardPos = ccp(winSize.width / 2.0f, winSize.height / 2.0f);

    if (auto rewardLayer = CurrencyRewardLayer::create(
            0,
            0,
            0,
            reward,
            CurrencySpriteType::Star,
            0,
            CurrencySpriteType::Star,
            0,
            rewardPos,
            CurrencyRewardType::Default,
            0.0,
            1.0)) {
        if (rewardLayer->m_diamondsLabel) {
            rewardLayer->m_diamonds = 0;
            rewardLayer->incrementDiamondsCount(oldRubies);
        }

        std::string rubyFrameName = "RL_bigRuby.png"_spr;
        std::string rubyCurrency = "RL_currencyRuby.png"_spr;
        auto rubyDisplayFrame =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(rubyFrameName.c_str());
        auto rubyCurrencyFrame =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(rubyCurrency.c_str());

        CCTexture2D* rubyTexture = nullptr;
        CCTexture2D* rubyCurrencyTexture = nullptr;
        if (!rubyDisplayFrame) {
            rubyTexture =
                CCTextureCache::sharedTextureCache()->addImage((rubyFrameName).c_str(), false);
            if (rubyTexture) {
                rubyDisplayFrame = CCSpriteFrame::createWithTexture(
                    rubyTexture, {{0, 0}, rubyTexture->getContentSize()});
            }
        } else {
            rubyTexture = rubyDisplayFrame->getTexture();
        }

        if (!rubyCurrencyFrame) {
            rubyCurrencyTexture =
                CCTextureCache::sharedTextureCache()->addImage((rubyCurrency).c_str(), false);
            if (rubyCurrencyTexture) {
                rubyCurrencyFrame = CCSpriteFrame::createWithTexture(
                    rubyCurrencyTexture,
                    {{0, 0}, rubyCurrencyTexture->getContentSize()});
            }
        } else {
            rubyCurrencyTexture = rubyCurrencyFrame->getTexture();
        }

        if (rewardLayer->m_diamondsSprite && rubyDisplayFrame) {
            rewardLayer->m_diamondsSprite->setDisplayFrame(rubyDisplayFrame);
        }
        if (rewardLayer->m_currencyBatchNode && rubyCurrencyTexture) {
            rewardLayer->m_currencyBatchNode->setTexture(rubyCurrencyTexture);
        }

        for (auto sprite : CCArrayExt<CurrencySprite>(rewardLayer->m_objects)) {
            if (!sprite) continue;
            if (sprite->m_burstSprite) sprite->m_burstSprite->setVisible(false);
            if (auto child = sprite->getChildByIndex(0)) child->setVisible(false);
            if (sprite->m_spriteType == CurrencySpriteType::Diamond) {
                if (rubyCurrencyFrame) sprite->setDisplayFrame(rubyCurrencyFrame);
                if (rubyCurrencyTexture && rewardLayer->m_currencyBatchNode) {
                    rewardLayer->m_currencyBatchNode->setTexture(rubyCurrencyTexture);
                }
            }
        }

        this->addChild(rewardLayer, 100);
        FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
    }

    Notification::create(
        std::string("Received ") + numToString(reward) + " rubies!",
        CCSprite::createWithSpriteFrameName("RL_bigRuby.png"_spr))
        ->show();
}

void RLSpireSelectLevelLayer::onRoomTransitionComplete() {
    if (m_transitionLayer) {
        m_transitionLayer->removeFromParent();
        m_transitionLayer = nullptr;
    }
    if (m_loadingSpinner) {
        m_loadingSpinner->removeFromParent();
        m_loadingSpinner = nullptr;
    }

    if (m_didAdvanceRoom) {
        rewardRoomTransition();
        m_didAdvanceRoom = false;
    }

    if (!Mod::get()->getSavedValue<bool>("hasEnteredSpire")) {
        Mod::get()->setSavedValue("hasEnteredSpire", true);
        DialogObject* dialog1 = DialogObject::create("The Oracle", "This <cl>place</c>... I believe <cr><s100>it</s></c> is <cy>waiting for you here</c>.", 1, 1.f, false, ccWHITE);
        DialogObject* dialog2 = DialogObject::create("The Oracle", "<cg>Good luck</c>.", 0, 1.f, false, ccWHITE);

        auto dialogArray = CCArray::create();
        dialogArray->addObject(dialog1);
        dialogArray->addObject(dialog2);

        auto dialog = DialogLayer::createWithObjects(dialogArray, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        auto orSprite = CCSprite::createWithSpriteFrameName("RL_dialogIconOracle.png"_spr);
        orSprite->setPosition(dialog->m_characterSprite->getPosition());
        dialog->m_characterSprite->setVisible(false);
        dialog->m_mainLayer->addChild(orSprite, 1);
    }

    m_isRoomTransitionActive = false;
}

bool RLSpireSelectLevelLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    addBackButton(this, BackButtonStyle::Pink);

    m_levelsMenu = CCMenu::create();
    m_levelsMenu->setPosition({0, 0});
    this->addChild(m_levelsMenu, -1);

    m_infoMenu = CCMenu::create();
    m_infoMenu->setPosition({0, 0});
    this->addChild(m_infoMenu, 5);

    m_roomLabel = CCLabelBMFont::create(fmt::format("Room {}", m_spireRoomIndex).c_str(), "goldFont.fnt");
    if (m_roomLabel) {
        m_roomLabel->setScale(0.5f);
        m_roomLabel->setAnchorPoint({0.f, .5f});
        m_roomLabel->setPosition({50.f, winSize.height - 25.f});
        m_roomLabel->setColor({255, 255, 255});
        this->addChild(m_roomLabel, 1);
    }

    // back border on the left side
    auto leftBorder = CCLayerColor::create({0, 0, 0, 255}, 125, winSize.height);
    leftBorder->setPosition({0, 0});
    this->addChild(leftBorder, 0);
    auto leftGlow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
    leftGlow->setPosition({145, -500});
    leftGlow->setScaleY(1);
    leftGlow->setScaleX(10);
    leftGlow->setRotation(90);
    leftGlow->setColor({0, 0, 0});
    leftBorder->addChild(leftGlow, 1);

    // back border on the right side
    auto rightBorder = CCLayerColor::create({0, 0, 0, 255}, 125, winSize.height);
    rightBorder->setPosition({winSize.width - 125, 0});
    this->addChild(rightBorder, 0);
    auto rightGlow = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
    rightGlow->setPosition({-20, 800});
    rightGlow->setScaleY(1);
    rightGlow->setScaleX(10);
    rightGlow->setRotation(-90);
    rightGlow->setColor({0, 0, 0});
    rightBorder->addChild(rightGlow, 1);

    // info thingy yappa
    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.75f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(RLSpireSelectLevelLayer::onInfoClick));
    infoBtn->setPosition({winSize.width - 25, winSize.height - 25});
    m_infoMenu->addChild(infoBtn);

    this->setKeypadEnabled(true);
    this->scheduleUpdate();

    showRoomTransition();

    return true;
}

void RLSpireSelectLevelLayer::onInfoClick(CCObject*) {
    MDPopup::create("The Spire",
        "<cf>The Spire</c> is tower-themed <co>Platformer-focus</c> user created <cl>Rated Layouts</c> levels.\n\n"
        "Explore the Spire and find forsaken lore beyond the <cp>Cosmos</c>.\n\n"
        "Each <co>room</c> contains <cl>5 platformer layouts</c>. Complete them to unlock the next room, when you completed a room, you are rewarded <cr>5000 rubies</c>.\n\n"
        "These levels are hand-picked by <cf>ArcticWoof</c> and usually relates to <cf>The Spire</c> and it's <cr>lore</c>. <cg>Check out the Spire regularly for new rooms!</c>",
        "OK")
        ->show();
}

void RLSpireSelectLevelLayer::fetchSpireLevels() {
    Ref<RLSpireSelectLevelLayer> self = this;
    m_fetchTask.spawn(web::WebRequest()
                          .param("index", numToString(m_spireRoomIndex))
                          .get("https://gdrate.arcticwoof.xyz/getSpireLevels"),
        [self](web::WebResponse const& res) {
            if (!self) return;

            if (!res.ok()) {
                Notification::create("Failed to load spire levels", NotificationIcon::Warning)->show();
                self->keyBackClicked();
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                Notification::create("Bad spire level data", NotificationIcon::Warning)->show();
                self->keyBackClicked();
                return;
            }

            auto json = jsonRes.unwrap();
            self->applySpireLevelsJson(json);
        });
}

void RLSpireSelectLevelLayer::applySpireLevelsJson(matjson::Value const& json) {
    int bgIndex = 6;
    if (json.contains("bgIndex")) {
        auto opt = json["bgIndex"].asInt();
        if (opt) {
            bgIndex = opt.unwrap();
        }
    }
    bgIndex = std::clamp(bgIndex, 0, 9);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    char bgNameBuf[64];
    std::sprintf(bgNameBuf, "game_bg_%02d_001.png", bgIndex);
    m_bg = cue::RepeatingBackground::create(bgNameBuf, .75f, cue::RepeatMode::Both);
    if (m_bg) {
        m_bg->setSpeed(0.f);
        m_bg->setColor({0, 50, 100});
        this->addChild(m_bg, -5);
    }

    m_levelIds.clear();
    m_levelDifficulty.clear();
    if (json.contains("levels") && json["levels"].isArray()) {
        auto levels = json["levels"];
        for (unsigned int i = 0; i < levels.size() && i < 5; ++i) {
            int levelId = 0;
            int difficulty = 0;
            if (levels[i].contains("levelid")) {
                auto opt = levels[i]["levelid"].asInt();
                if (opt)
                    levelId = opt.unwrap();
            }
            if (levels[i].contains("difficulty")) {
                auto diffOpt = levels[i]["difficulty"].asInt();
                if (diffOpt)
                    difficulty = diffOpt.unwrap();
            }
            m_levelIds.push_back(levelId);
            m_levelDifficulty.push_back(difficulty);
        }
    }

    // ensure we have 5 entries, fill with no-level ids if missing
    while (m_levelIds.size() < 5) {
        m_levelIds.push_back(0);
    }

    if (m_roomLabel) {
        m_roomLabel->setString(fmt::format("Room {}", m_spireRoomIndex).c_str());
    }

    m_levelCompleted.clear();
    m_levelCompleted.resize(m_levelIds.size());
    for (unsigned int i = 0; i < m_levelIds.size(); ++i) {
        if (m_levelIds[i] > 0) {
            m_levelCompleted[i] = isLevelCompleted(m_levelIds[i]);
        } else {
            m_levelCompleted[i] = false;
        }
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->removeFromParent();
        m_loadingSpinner = nullptr;
    }

    createSpireDoors();
    updateNavArrowState();
    completeRoomTransition();
}

bool RLSpireSelectLevelLayer::isLevelCompleted(int levelId) {
    return RLSpireSelectLevelLayer::isSpireLevelCompleted(levelId);
}

void RLSpireSelectLevelLayer::createSpireDoors() {
    if (!m_levelsMenu)
        return;

    m_levelsMenu->removeAllChildrenWithCleanup(true);
    m_navArrowUp = nullptr;
    m_navArrowDown = nullptr;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    float yMin = 80.0f;
    float yMax = winSize.height - 80.0f;
    float step = (yMax - yMin) / 4.0f;

    float xOffset[5] = {-80.f, 80.0f, -80.0f, 80.0f, 0.0f};
    if (!m_navArrowUp) {
        auto arrowSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        if (arrowSprite) {
            m_navArrowUp = CCMenuItemSpriteExtra::create(arrowSprite, this, menu_selector(RLSpireSelectLevelLayer::onNavArrowUpClick));
            m_navArrowUp->setPosition({winSize.width / 2.0f, winSize.height - 20.0f});
            m_navArrowUp->setRotation(-90);
            m_levelsMenu->addChild(m_navArrowUp, 10);
        }
    }

    if (!m_navArrowDown) {
        auto arrowDownSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        if (arrowDownSprite) {
            m_navArrowDown = CCMenuItemSpriteExtra::create(arrowDownSprite, this, menu_selector(RLSpireSelectLevelLayer::onNavArrowDownClick));
            m_navArrowDown->setPosition({winSize.width / 2.0f, 20.0f});
            m_navArrowDown->setRotation(90);
            m_levelsMenu->addChild(m_navArrowDown, 10);
        }
    }

    for (int i = 0; i < 5; ++i) {
        int levelId = m_levelIds[i];
        bool previouslyCompleted = (i == 0) || (i > 0 && m_levelCompleted[i - 1]);
        bool thisCompleted = (levelId > 0) && m_levelCompleted[i];
        bool unlocked = levelId > 0 && (previouslyCompleted || thisCompleted);

        const char* spriteFrame = unlocked ? "RL_spireDoor_unlocked.png"_spr : "RL_spireDoor_locked.png"_spr;
        auto doorSprite = CCSprite::createWithSpriteFrameName(spriteFrame);
        if (!doorSprite)
            continue;

        float yPos = yMin + i * step;
        float xPos = (winSize.width / 2.0f) + xOffset[i];
        auto doorItem = CCMenuItemSpriteExtra::create(doorSprite, this, menu_selector(RLSpireSelectLevelLayer::onSpireDoorClick));
        doorItem->setPosition({xPos, yPos});
        doorItem->setTag(levelId);
        doorItem->setEnabled(unlocked);

        m_levelsMenu->addChild(doorItem, 5);

        if (unlocked) {
            // add particle effect for unlocked doors
            ParticleStruct doorParticle;
            GameToolbox::particleStringToStruct(doorOpenParticle, doorParticle);
            auto particleNode = GameToolbox::particleFromStruct(doorParticle, nullptr, false);
            if (particleNode) {
                doorItem->addChild(particleNode, 3);
                particleNode->setScale(1.5f);
                particleNode->setPosition({doorSprite->getContentSize().width / 2, doorSprite->getContentSize().height / 2 - 12});
                particleNode->update(0.15f);
            }

            // door number label
            int doorLevelNumber = ((m_spireRoomIndex - 1) * 5) + (i + 1);
            auto numberLabel = CCLabelBMFont::create(fmt::format("{}", doorLevelNumber).c_str(), "gjFont15.fnt");
            numberLabel->setScale(0.6f);
            numberLabel->setColor({100, 150, 230});
            numberLabel->setPosition({doorSprite->getContentSize().width / 2, doorSprite->getContentSize().height / 2 - 13});
            numberLabel->setAnchorPoint({0.5f, 0.5f});
            numberLabel->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            numberLabel->setOpacity(255);
            doorItem->addChild(numberLabel, 2);

            int theMagicNumberThatNoOneAsked = 1;
            // difficulty label
            int levelDiff = (i < m_levelDifficulty.size()) ? m_levelDifficulty[i] : 0;
            auto diffLabel = CCLabelBMFont::create(fmt::format("{}", levelDiff).c_str(), "bigFont.fnt");
            diffLabel->setScale(0.35f);
            diffLabel->setPosition({doorSprite->getContentSize().width / 2 + theMagicNumberThatNoOneAsked, doorSprite->getContentSize().height / 2 + 9});
            diffLabel->setAnchorPoint({0.f, 0.5f});
            diffLabel->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
            diffLabel->setOpacity(230);
            diffLabel->limitLabelWidth(10, .35f, .1f);
            doorItem->addChild(diffLabel, 2);

            // planet sprite
            auto planetSpr = CCSprite::createWithSpriteFrameName("RL_planetSmall.png"_spr);
            planetSpr->setPosition({diffLabel->getPositionX() - theMagicNumberThatNoOneAsked, diffLabel->getPositionY()});
            planetSpr->setAnchorPoint({1.f, 0.5f});
            planetSpr->setScale(.8f);
            doorItem->addChild(planetSpr);
        }
    }
}

void RLSpireSelectLevelLayer::refreshDoorStates() {
    if (!m_levelsMenu || m_levelIds.empty())
        return;

    for (unsigned int i = 0; i < m_levelIds.size(); ++i) {
        auto child = static_cast<CCNode*>(m_levelsMenu->getChildren()->objectAtIndex(i));
        auto menuItem = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
        if (!menuItem)
            continue;

        bool previouslyCompleted = (i == 0) || (i > 0 && m_levelCompleted[i - 1]);
        bool thisCompleted = (i < m_levelCompleted.size() && m_levelCompleted[i]);
        int levelId = m_levelIds[i];
        bool unlocked = levelId > 0 && (previouslyCompleted || thisCompleted);

        auto normalNode = menuItem->getNormalImage();
        if (normalNode) {
            normalNode->removeFromParent();
        }

        const char* spriteFrame = unlocked ? "RL_spireDoor_unlocked.png"_spr : "RL_spireDoor_locked.png"_spr;
        auto doorSprite = CCSprite::createWithSpriteFrameName(spriteFrame);
        if (doorSprite) {
            menuItem->setNormalImage(doorSprite);
        }
        menuItem->setEnabled(unlocked);

        if (unlocked) {
            // add particle effect for unlocked doors
            ParticleStruct doorParticle;
            GameToolbox::particleStringToStruct(doorOpenParticle, doorParticle);
            auto particleNode = GameToolbox::particleFromStruct(doorParticle, nullptr, false);
            if (particleNode) {
                menuItem->addChild(particleNode, 1);
                particleNode->setScale(1.5f);
                particleNode->setPosition({doorSprite->getContentSize().width / 2, doorSprite->getContentSize().height / 2 - 13});
                particleNode->update(0.15f);
            }
        }
    }

    updateNavArrowState();
}

void RLSpireSelectLevelLayer::onSpireDoorClick(CCObject* sender) {
    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!menuItem)
        return;

    int levelId = menuItem->getTag();
    if (levelId <= 0) {
        Notification::create("No level data", NotificationIcon::Warning)->show();
        return;
    }

    auto index = -1;
    for (size_t i = 0; i < m_levelIds.size(); ++i) {
        if (m_levelIds[i] == levelId) {
            index = static_cast<int>(i);
            break;
        }
    }

    if (index < 0) {
        Notification::create("Level not configured", NotificationIcon::Warning)->show();
        return;
    }

    if (index > 0 && !(index - 1 < static_cast<int>(m_levelCompleted.size()) && m_levelCompleted[index - 1])) {
        Notification::create("Complete previous level first", NotificationIcon::Warning)->show();
        return;
    }

    // Use searchObject and LevelInfoLayer
    auto searchObj = GJSearchObject::create(SearchType::Search, numToString(levelId));
    auto key = std::string(searchObj->getKey());
    auto glm = GameLevelManager::sharedState();
    if (!glm)
        return;

    auto stored = glm->getStoredOnlineLevels(key.c_str());
    if (stored && stored->count() > 0) {
        auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
        if (level && level->m_levelID == levelId) {
            auto scene = LevelInfoLayer::scene(level, false);
            auto transitionFade = CCTransitionFade::create(0.5f, scene);
            CCDirector::sharedDirector()->pushScene(transitionFade);
            return;
        }
    }

    m_pendingKey = key;
    m_pendingLevelId = levelId;
    m_pendingTimeout = 10.0;

    glm->getOnlineLevels(searchObj);
}

void RLSpireSelectLevelLayer::onNavArrowUpClick(CCObject* sender) {
    if (!m_navArrowUp) {
        return;
    }

    if (!m_allCompleted) {
        DialogObject* dialogObj = nullptr;
        dialogObj = DialogObject::create("The Oracle", "You <cr>cannot go further</c>. You must <cg>surpass the ones that face you here first</c>.", 1, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        auto orSprite =
            CCSprite::createWithSpriteFrameName("RL_dialogIconOracle.png"_spr);
        orSprite->setPosition(dialog->m_characterSprite->getPosition());
        dialog->m_mainLayer->addChild(orSprite, 1);
        dialog->m_characterSprite->setVisible(false);
        return;
    }

    // when all levels in current room are completed, go to next room
    m_didAdvanceRoom = true;
    m_spireRoomIndex++;
    showRoomTransition();
}

void RLSpireSelectLevelLayer::onNavArrowDownClick(CCObject* sender) {
    if (!m_navArrowDown) {
        return;
    }

    m_spireRoomIndex = std::max(1, m_spireRoomIndex - 1);
    showRoomTransition();
}

void RLSpireSelectLevelLayer::update(float dt) {
    CCLayer::update(dt);

    if (!m_pendingKey.empty()) {
        auto glm = GameLevelManager::sharedState();
        if (glm) {
            auto stored = glm->getStoredOnlineLevels(m_pendingKey.c_str());
            if (stored && stored->count() > 0) {
                auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
                if (level && level->m_levelID == m_pendingLevelId) {
                    auto scene = LevelInfoLayer::scene(level, false);
                    auto transitionFade = CCTransitionFade::create(0.5f, scene);
                    CCDirector::sharedDirector()->pushScene(transitionFade);

                    m_pendingKey.clear();
                    m_pendingLevelId = -1;
                    m_pendingTimeout = 0.0;
                    return;
                }
            }
        }

        m_pendingTimeout -= dt;
        if (m_pendingTimeout <= 0.0) {
            if (m_levelsMenu) {
                auto children = m_levelsMenu->getChildren();
                for (unsigned int i = 0; i < children->count(); ++i) {
                    auto child = static_cast<CCNode*>(children->objectAtIndex(i));
                    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
                }
            }
            Notification::create("Level not found", NotificationIcon::Warning)->show();
            m_pendingKey.clear();
            m_pendingLevelId = -1;
            m_pendingTimeout = 0.0;
        }
    }
}

void RLSpireSelectLevelLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}