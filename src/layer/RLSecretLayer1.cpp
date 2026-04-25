#include <Geode/Geode.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "../include/RLConstants.hpp"
#include "../include/RLDialogIcons.hpp"
#include "RLSecretLayer1.hpp"
#include "RLRubiesCodePopup.hpp"
#include "../include/RLAchievements.hpp"
#include <filesystem>

using namespace geode::prelude;

static std::filesystem::path redeemedCodesPath() {
    return dirs::getModsSaveDir() / Mod::get()->getID() / "redeemed_codes.json";
}

static matjson::Value loadRedeemedCodesJson() {
    auto path = redeemedCodesPath();
    if (!std::filesystem::exists(path)) {
        return matjson::Value::array();
    }

    auto existing = utils::file::readString(utils::string::pathToString(path));
    if (!existing) {
        return matjson::Value::array();
    }

    auto parsed = matjson::parse(existing.unwrap());
    if (!parsed || !parsed.unwrap().isArray()) {
        return matjson::Value::array();
    }

    return parsed.unwrap();
}

static void saveRedeemedCodesJson(matjson::Value const& entries) {
    auto path = redeemedCodesPath();
    std::filesystem::create_directories(path.parent_path());
    auto jsonString = entries.dump();
    auto writeRes = utils::file::writeString(utils::string::pathToString(path), jsonString);
    if (!writeRes) {
        log::warn("Failed to write redeemed codes data to {}", path.string());
    }
}

static bool isCodeRedeemed(std::string const& code) {
    if (code.empty()) {
        return false;
    }

    auto data = loadRedeemedCodesJson();
    if (!data.isArray()) {
        return false;
    }

    auto arr = data.asArray().unwrap();
    for (auto& entry : arr) {
        if (entry.isString() && entry.asString().unwrap() == code) {
            return true;
        }
    }
    return false;
}

static void addRedeemedCode(std::string const& code) {
    if (code.empty()) {
        return;
    }

    auto data = loadRedeemedCodesJson();
    if (!data.isArray()) {
        data = matjson::Value::array();
    }

    auto& arr = data.asArray().unwrap();
    for (auto& entry : arr) {
        if (entry.isString() && entry.asString().unwrap() == code) {
            return;
        }
    }

    arr.push_back(matjson::Value(code));
    saveRedeemedCodesJson(data);
}

const std::string oracleFloatingStr = "25,2065,2,2715,3,2265,155,2,156,8,145,25a-1a1a0.3a19a90a0a35a0a30a20a0a0a0a0a0a0a30a1a0a0a0.392157a0a0.0392157a0a0.74902a0a1a0a5a1a0a0a1a0a0.4a0a0.45098a0a1a0a0.11a0a1a0a0a0a0a0a0a0a0a2a1a0a0a1a26a0a0a0a0a0a0a0a0a0a0a0a0a0a0;";
const std::string crystalBallStr = "25,2065,2,2865,3,2265,155,2,156,8,145,25a-1a1a0.3a-1a90a0a0a0a0a0a0a0a0a0a0a0a100a1a0a0a0.0392157a0a0.564706a0a0.74902a0a1a0a0a1a0a0a0.4a0a0.976471a0a1a0a1a0a1a0a1a0a1a0a0a0a-360a0a1a2a1a0a0a1a185a0a0a0a0a0a0a0a0a0a0a0a0a0a0;";

RLSecretLayer1* RLSecretLayer1::create() {
    auto layer = new RLSecretLayer1();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool RLSecretLayer1::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    addBackButton(this, BackButtonStyle::Green);
    auto bg = createLayerBG();
    bg->setColor({50, 20, 20});
    this->addChild(bg);

    // the oracle title
    auto title = CCLabelBMFont::create("The Observatory", "goldFont.fnt");
    title->setPosition({winSize.width / 2, winSize.height - 20});
    this->addChild(title);

    // text input thingy
    m_rewardInput = TextInput::create(150.f, "...");
    m_rewardInput->setScale(1.3f);
    m_rewardInput->setPosition({winSize.width / 2, winSize.height / 2 + 60});
    m_rewardInput->setID("reward-input");
    this->addChild(m_rewardInput);

    // redeem button
    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    this->addChild(menu);

    m_oracleSpr = CCSprite::createWithSpriteFrameName("RL_oracle.png"_spr);
    m_oracleSpr->setScale(1.2f);
    m_oracleBtn = CCMenuItemSpriteExtra::create(
        m_oracleSpr, this, menu_selector(RLSecretLayer1::onRedeem));
    m_oracleBtn->setPosition({winSize.width / 2, winSize.height / 2 - 35});
    menu->addChild(m_oracleBtn);

    // oracle particle
    ParticleStruct oracleFloating;
    GameToolbox::particleStringToStruct(oracleFloatingStr, oracleFloating);
    m_oracleParticle = GameToolbox::particleFromStruct(oracleFloating, nullptr, false);
    if (m_oracleParticle) {
        m_oracleSpr->addChild(m_oracleParticle, -1);
        m_oracleParticle->resetSystem();
        m_oracleParticle->setPosition(
            m_oracleSpr->getContentSize() / 2.f);
        m_oracleParticle->setScale(1.2f);
        m_oracleParticle->setID("floating-particles"_spr);
        m_oracleParticle->update(0.15f);
    }

    // crystal ball particle
    ParticleStruct crystalBall;
    GameToolbox::particleStringToStruct(crystalBallStr, crystalBall);
    m_crystalParticle = GameToolbox::particleFromStruct(crystalBall, nullptr, false);
    if (m_crystalParticle) {
        m_oracleSpr->addChild(m_crystalParticle, 1);
        m_crystalParticle->resetSystem();
        m_crystalParticle->setPosition(
            m_oracleSpr->getContentSize() / 2.f);
        m_crystalParticle->setPositionY(m_crystalParticle->getPositionY() - 29);
        m_crystalParticle->setScale(.5f);
        m_crystalParticle->setID("floating-particles"_spr);
        m_crystalParticle->update(0.15f);
        m_crystalParticle->setVisible(false);
    }

    if (rl::isUserOwner()) {
        // secret button for adding new codes
        auto secretSpr = CCSprite::createWithSpriteFrameName("RL_bigRuby.png"_spr);
        secretSpr->setScale(0.7f);
        secretSpr->setColor({0, 0, 0});
        secretSpr->setOpacity(150);
        auto secretBtn = CCMenuItemSpriteExtra::create(
            secretSpr, this, menu_selector(RLSecretLayer1::onSecretPopup));
        secretBtn->setPosition({winSize.width - 30, 30});
        menu->addChild(secretBtn);
    }

    this->setKeypadEnabled(true);
    return true;
}

void RLSecretLayer1::onSecretPopup(CCObject* sender) {
    auto popup = RLRubiesCodePopup::create();
    popup->show();
}

void RLSecretLayer1::onExitTransitionDidStart() {
    CCLayer::onExitTransitionDidStart();
    GameManager::sharedState()->playMenuMusic();
}

void RLSecretLayer1::onEnterTransitionDidFinish() {
    CCLayer::onEnterTransitionDidFinish();
    FMODAudioEngine::sharedEngine()->playMusic("secretLoopRL.mp3"_spr, true, 0.f, 0);
    // first time?
    if (!Mod::get()->getSavedValue<bool>("oracleFirstTime")) {
        Mod::get()->setSavedValue("oracleFirstTime", true);
        DialogObject* dialog1 = DialogObject::create("The Oracle", "A <cg>visitor</c>?<d100> <cy>Surely not</c>?", 2, 1.f, false, ccWHITE);
        DialogObject* dialog2 = DialogObject::create("The Oracle", "...!", 1, 1.f, false, ccWHITE);
        DialogObject* dialog3 = DialogObject::create("The Oracle", "By the <s100><cr>Red Comet</c></s>! Your <cg>arrival</c> has been <co>lingering</c> on the <cy>firmament above for ages</c>!", 3, 1.f, false, ccWHITE);
        DialogObject* dialog4 = DialogObject::create("The Oracle", "...", 4, 1.f, false, ccWHITE);
        DialogObject* dialog5 = DialogObject::create("The Oracle", "Do excuse me.", 5, 1.f, false, ccWHITE);
        DialogObject* dialog6 = DialogObject::create("The Oracle", "I inquire that you <cg>learn to read</c> the <cp>Cosmos</c>, that you may find <cy>riches unbound</c>!", 1, 1.f, false, ccWHITE);
        DialogObject* dialog7 = DialogObject::create("The Oracle", "Now then.", 2, 1.f, false, ccWHITE);
        DialogObject* dialog8 = DialogObject::create("The Oracle", "I shall watch your <co>exploits</c> most <cg>attentively</c>. <cl>Enjoy</c>.", 1, 1.f, false, ccWHITE);

        auto dialogArray = CCArray::create();
        dialogArray->addObject(dialog1);
        dialogArray->addObject(dialog2);
        dialogArray->addObject(dialog3);
        dialogArray->addObject(dialog4);
        dialogArray->addObject(dialog5);
        dialogArray->addObject(dialog6);
        dialogArray->addObject(dialog7);
        dialogArray->addObject(dialog8);

        auto dialog = DialogLayer::createWithObjects(dialogArray, 4);
        this->addChild(dialog, 100);
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialog1->m_characterFrame);
    }
}

void RLSecretLayer1::onRedeem(CCObject* sender) {
    if (m_isRedeeming || !m_rewardInput) {
        return;
    }

    std::string code = m_rewardInput->getString();
    std::transform(code.begin(), code.end(), code.begin(), ::tolower);

    if (code.empty()) {
        DialogObject* dialogObj = nullptr;
        dialogObj = DialogObject::create("The Oracle", "<cr>Mimicking</c> the void of the <cp>Cosmos</c>, <cg>are we</c>?", 1, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialogObj->m_characterFrame);
        return;
    }

    std::string argonToken = Mod::get()->getSavedValue<std::string>("argon_token");
    if (argonToken.empty()) {
        DialogObject* dialogObj = nullptr;
        dialogObj = DialogObject::create("The Oracle", "Something went wrong...", 3, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectCustomIcon(dialog, "RL_dialogIcon_00.png"_spr);
        Notification::create("Argon authentication missing")->show();
        return;
    }

    m_isRedeeming = true;
    m_redeemCode = code;
    m_oracleBtn->setEnabled(false);
    m_crystalParticle->setVisible(true);

    FMODAudioEngine::sharedEngine()->playEffect("unlockGauntlet.ogg");

    // fade in the text label while redeeming
    if (m_textLabel) {
        m_textLabel->removeFromParent();
        m_textLabel = nullptr;
    }
    m_textLabel = CCLabelBMFont::create("Let's consult the Cosmos", "gjFont15.fnt");
    m_textLabel->setOpacity(0);
    m_textLabel->setScale(0.5f);
    m_textLabel->setColor({150, 0, 150});
    m_textLabel->setPosition({CCDirector::sharedDirector()->getWinSize().width / 2,
        CCDirector::sharedDirector()->getWinSize().height - 60});
    this->addChild(m_textLabel, 1);
    m_textLabel->runAction(CCFadeIn::create(0.5f));

    // Fade out the input and fade in the spinner while waiting
    if (m_rewardInput) {
        if (auto bg = m_rewardInput->getBGSprite()) {
            bg->setOpacity(90);
            bg->runAction(CCFadeTo::create(.5f, 0));
        }
        if (auto inputNode = m_rewardInput->getInputNode()) {
            if (auto label = inputNode->getTextLabel()) {
                label->runAction(CCFadeOut::create(.5f));
            }
        }
        m_rewardInput->runAction(CCSequence::create(
            CCDelayTime::create(.45f),
            CCCallFunc::create(this, callfunc_selector(RLSecretLayer1::hideRewardInput)),
            nullptr));
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }
    m_spinner = LoadingSpinner::create(40.f);
    m_spinner->getSpinner()->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
    m_spinner->setPosition(m_rewardInput->getPosition());
    m_spinner->setOpacity(0);
    this->addChild(m_spinner, 100);
    m_spinner->runAction(CCSequence::create(
        CCDelayTime::create(.25f),
        CCFadeIn::create(.5f),
        nullptr));

    auto delay = CCDelayTime::create(1.5f);
    auto call = CCCallFunc::create(this, callfunc_selector(RLSecretLayer1::startRedeemRequest));
    this->runAction(CCSequence::create(delay, call, nullptr));
}

void RLSecretLayer1::startRedeemRequest() {
    if (!m_isRedeeming || m_redeemCode.empty()) {
        finishRedeem();
        return;
    }

    std::string code = m_rewardInput->getString();
    std::transform(code.begin(), code.end(), code.begin(), ::tolower);

    Ref<RLSecretLayer1> self = this;

    std::string argonToken = Mod::get()->getSavedValue<std::string>("argon_token");
    if (argonToken.empty()) {
        DialogObject* dialogObj = nullptr;
        dialogObj = DialogObject::create("The Oracle", "Something went wrong...", 3, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialogObj->m_characterFrame);
        Notification::create("Argon authentication missing")->show();
        finishRedeem();
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = argonToken;
    body["code"] = m_redeemCode;

    auto req = web::WebRequest();
    req.bodyJSON(body);

    if (code == "spire" && !Mod::get()->getSavedValue<bool>("hasCode")) {  // if u reading this, bruh
        Mod::get()->setSavedValue("hasCode", true);
        addRedeemedCode(code);
        if (self->m_textLabel) {
            self->m_textLabel->setString("Something has aligned...");
            self->m_textLabel->setColor({150, 100, 0});
        }
        DialogObject* dialog1 = DialogObject::create("The Oracle", "The <co>Planets align</c>. You read the <cp>Cosmos</c> and <cg>found the way to what you spoke.</c>", 2, 1.f, false, ccWHITE);
        DialogObject* dialog2 = DialogObject::create("The Oracle", "Now seek your <cg>words</c> outside of this place, <s100><cr>it</c></s> is <cy>waiting.</c>", 1, 1.f, false, ccWHITE);

        auto dialogArray = CCArray::create();
        dialogArray->addObject(dialog1);
        dialogArray->addObject(dialog2);

        auto dialog = DialogLayer::createWithObjects(dialogArray, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialog1->m_characterFrame);
        self->finishRedeem();
        RLAchievements::onReward("misc_spire");
        return;
    }

    if (code == "modpls") {
        DialogObject* dialog1 = DialogObject::create("The Oracle", "No...", 2, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialog1, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialog1->m_characterFrame);
        self->finishRedeem();
    }

    if (code == "goog") {
        DialogObject* dialog1 = DialogObject::create("The Oracle", "goog", 2, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialog1, nullptr, 4);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        rl::setDialogObjectIcon(dialog, dialog1->m_characterFrame);
        self->finishRedeem();
    }

    async::spawn(req.post(std::string(rl::BASE_API_URL) + "/getRubiesReward"),
        [self, code](web::WebResponse res) {
            if (!self) {
                return;
            }

            if (!res.ok() || isCodeRedeemed(code)) {
                if (self->m_textLabel) {
                    self->m_textLabel->setString("The Cosmos rejects your request");
                    self->m_textLabel->setColor({150, 0, 0});
                }
                self->finishRedeem();
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                DialogObject* dialogObj = nullptr;
                dialogObj = DialogObject::create("The Oracle", "Something went wrong...", 0, 1.f, false, ccWHITE);

                auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 4);
                dialog->addToMainScene();
                dialog->animateInRandomSide();

                rl::setDialogObjectIcon(dialog, dialogObj->m_characterFrame);
                Notification::create("Redeem request failed with invalid response.")->show();
                log::warn("Redeem request failed with invalid JSON response.");
                self->finishRedeem();
                return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOr(false);
            if (!success) {
                if (self->m_textLabel) {
                    self->m_textLabel->setString("The Cosmos rejects your request");
                    self->m_textLabel->setColor({150, 0, 0});
                }
                self->finishRedeem();
                return;
            }

            int reward = json["reward"].asInt().unwrapOr(0);
            if (reward <= 0) {
                if (self->m_textLabel) {
                    self->m_textLabel->setString("The Cosmos rejects your request");
                    self->m_textLabel->setColor({150, 0, 0});
                }
                self->finishRedeem();
                return;
            }

            if (self->m_textLabel) {
                self->m_textLabel->setString("The Cosmos approves of your wisdom");
                self->m_textLabel->setColor({0, 150, 0});
                RLAchievements::onReward("misc_code");
            }

            addRedeemedCode(self->m_redeemCode);

            // Reward the player with rubies and show currency animation
            int oldRubies = Mod::get()->getSavedValue<int>("rubies", 0);
            Mod::get()->setSavedValue<int>("rubies", oldRubies + reward);

            if (auto rewardLayer = CurrencyRewardLayer::create(
                    0, 0, 0, reward, CurrencySpriteType::Star, 0, CurrencySpriteType::Star, 0, self->m_oracleBtn->getPosition(), CurrencyRewardType::Default, 0.0, 1.0)) {
                if (rewardLayer->m_diamondsLabel) {
                    rewardLayer->m_diamonds = 0;
                    rewardLayer->incrementDiamondsCount(oldRubies);
                }

                std::string rubyFrameName = "RL_bigRuby.png"_spr;
                std::string rubyCurrency = "RL_currencyRuby.png"_spr;
                auto rubyDisplayFrame =
                    CCSpriteFrameCache::sharedSpriteFrameCache()
                        ->spriteFrameByName(rubyFrameName.c_str());
                auto rubyCurrencyFrame =
                    CCSpriteFrameCache::sharedSpriteFrameCache()
                        ->spriteFrameByName(rubyCurrency.c_str());

                CCTexture2D* rubyTexture = nullptr;
                CCTexture2D* rubyCurrencyTexture = nullptr;
                if (!rubyDisplayFrame) {
                    rubyTexture =
                        CCTextureCache::sharedTextureCache()->addImage(
                            (rubyFrameName).c_str(), false);
                    if (rubyTexture) {
                        rubyDisplayFrame = CCSpriteFrame::createWithTexture(
                            rubyTexture,
                            {{0, 0}, rubyTexture->getContentSize()});
                    }
                } else {
                    rubyTexture = rubyDisplayFrame->getTexture();
                }

                if (!rubyCurrencyFrame) {
                    rubyCurrencyTexture =
                        CCTextureCache::sharedTextureCache()->addImage(
                            (rubyCurrency).c_str(), false);
                    if (rubyCurrencyTexture) {
                        rubyCurrencyFrame =
                            CCSpriteFrame::createWithTexture(
                                rubyCurrencyTexture,
                                {{0, 0},
                                    rubyCurrencyTexture->getContentSize()});
                    }
                } else {
                    rubyCurrencyTexture = rubyCurrencyFrame->getTexture();
                }

                if (rewardLayer->m_diamondsSprite && rubyDisplayFrame) {
                    rewardLayer->m_diamondsSprite->setDisplayFrame(
                        rubyDisplayFrame);
                }
                if (rewardLayer->m_currencyBatchNode && rubyCurrencyTexture) {
                    rewardLayer->m_currencyBatchNode->setTexture(
                        rubyCurrencyTexture);
                }

                for (auto sprite :
                    CCArrayExt<CurrencySprite>(rewardLayer->m_objects)) {
                    if (!sprite)
                        continue;
                    if (sprite->m_burstSprite)
                        sprite->m_burstSprite->setVisible(false);
                    if (auto child = sprite->getChildByIndex(0))
                        child->setVisible(false);
                    if (sprite->m_spriteType == CurrencySpriteType::Diamond) {
                        if (rubyCurrencyFrame)
                            sprite->setDisplayFrame(rubyCurrencyFrame);
                        if (rubyCurrencyTexture &&
                            rewardLayer->m_currencyBatchNode) {
                            rewardLayer->m_currencyBatchNode->setTexture(
                                rubyCurrencyTexture);
                        }
                    }
                }

                self->addChild(rewardLayer, 100);
                FMODAudioEngine::sharedEngine()->playEffect(
                    // @geode-ignore(unknown-resource)
                    "gold02.ogg");
            }
            Notification::create(
                std::string("Received ") +
                    numToString(reward) + " rubies!",
                CCSprite::createWithSpriteFrameName("RL_bigRuby.png"_spr))
                ->show();
            self->finishRedeem();
        });
}

void RLSecretLayer1::finishRedeem() {
    m_isRedeeming = false;
    m_redeemCode.clear();
    m_oracleBtn->setEnabled(true);
    m_crystalParticle->setVisible(false);
    m_rewardInput->setString("");

    if (m_rewardInput) {
        // Ensure the input itself is visible again
        m_rewardInput->setVisible(true);
        m_rewardInput->setEnabled(true);

        // Fade the background sprite (the main visible portion of the input)
        if (auto bg = m_rewardInput->getBGSprite()) {
            bg->setVisible(true);
            bg->setOpacity(0);
            bg->runAction(CCFadeTo::create(.5f, 90));
        }
        if (auto inputNode = m_rewardInput->getInputNode()) {
            if (auto label = inputNode->getTextLabel()) {
                label->setOpacity(0);
                label->runAction(CCFadeIn::create(.5f));
            }
        }
    }

    if (m_spinner) {
        m_spinner->runAction(CCSequence::create(
            CCFadeOut::create(.5f),
            CCCallFunc::create(this, callfunc_selector(RLSecretLayer1::cleanupSpinner)),
            nullptr));
    }

    if (m_textLabel) {
        m_textLabel->runAction(CCSequence::create(
            CCDelayTime::create(3.0f),
            CCFadeOut::create(.5f),
            nullptr));
    }
}

void RLSecretLayer1::hideRewardInput() {
    if (m_rewardInput) {
        m_rewardInput->setVisible(false);
        m_rewardInput->setEnabled(false);
    }
}

void RLSecretLayer1::cleanupSpinner() {
    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }
}

void RLSecretLayer1::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}
