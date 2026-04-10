#include "RLDonationPopup.hpp"
#include <argon/Argon.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include "RLBadgeRequestPopup.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"

#include <cstdlib>
#include <ctime>

using namespace geode::prelude;
using namespace rl;

const std::string badgeParticle = "20,2065,2,225,3,165,145,20a-1a2a0.3a8a90a180a15a0a5a5a0a0a0a0a0a0a10a1a0a0a0.768627a0a0.219608a0a0.768627a0a1a0a1a1a0a0a1a0a0.415686a0a0.996078a0a1a0a0.25a0a1a0a0a0a0a0a0a0a0a2a1a0a0a1a21a0a0a0a0a0a0a0a0a0a0a0a0a0a0;";

RLDonationPopup* RLDonationPopup::create() {
    auto ret = new RLDonationPopup();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLDonationPopup::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square07.png"))
        return false;

    const float mainLayerWidthSpacing = m_mainLayer->getContentSize().width - 25.f;

    // clipping node for rounded corners
    auto sStencil = NineSlice::create("GJ_square06.png");
    if (sStencil) {
        sStencil->setAnchorPoint({0.f, 0.f});
        sStencil->setPosition({0.f, 0.f});
        sStencil->setContentSize({m_mainLayer->getScaledContentSize()});
    }
    auto clip = CCClippingNode::create(sStencil);
    clip->setPosition({0.f, 0.f});
    clip->setContentSize({m_mainLayer->getScaledContentSize()});
    clip->setAlphaThreshold(0.01f);
    m_mainLayer->addChild(clip, -1);

    // border
    auto borderSprite = NineSlice::create("square02b_001.png");
    borderSprite->setContentSize({m_mainLayer->getScaledContentSize()});
    borderSprite->setColor({140, 0, 140});
    borderSprite->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height / 2.f});
    m_mainLayer->addChild(borderSprite, -2);

    // glow bg
    auto glowSprite = CCSprite::createWithSpriteFrameName("chest_glow_bg_001.png");
    glowSprite->setScaleX(20.f);
    glowSprite->setScaleY(20.f);
    glowSprite->setPosition({1600, 300});
    glowSprite->setOpacity(120);
    glowSprite->setBlendFunc({GL_SRC_ALPHA, GL_ONE});  // multiply blend
    clip->addChild(glowSprite);

    // title
    auto titleLabel = CCLabelBMFont::create("Support Rated Layouts!", "bigFont.fnt");
    titleLabel->setScale(0.75f);
    titleLabel->setColor({255, 148, 255});
    titleLabel->setPosition({m_mainLayer->getScaledContentSize().width / 2.f + 20.f, m_mainLayer->getScaledContentSize().height - 30.f});
    m_mainLayer->addChild(titleLabel);

    // supporter badge next to title
    auto badgeSpr = CCSprite::createWithSpriteFrameName("RL_badgeSupporter.png"_spr);
    badgeSpr->setScale(1.5f);
    badgeSpr->setRotation(-20.f);
    badgeSpr->setPosition({titleLabel->getPositionX() - titleLabel->getContentSize().width * 0.75f / 2 - 30.f, titleLabel->getPositionY()});
    m_mainLayer->addChild(badgeSpr);

    // remove any existing particles on this coin to avoid dupes
    const std::string& pStruct = badgeParticle;
    if (!pStruct.empty()) {
        if (auto existingP =
                badgeSpr->getChildByID("rating-particles")) {
            existingP->removeFromParent();
        }
        ParticleStruct pStruct;
        GameToolbox::particleStringToStruct(badgeParticle, pStruct);
        CCParticleSystemQuad* particle =
            GameToolbox::particleFromStruct(pStruct, nullptr, false);
        if (particle) {
            badgeSpr->addChild(particle, -1);
            particle->resetSystem();
            particle->setPosition(badgeSpr->getContentSize() /
                                  2.f);
            particle->setID("rating-particles"_spr);
            particle->updateWithNoTime();
        }
    }

    // title1
    auto heading1 = CCLabelBMFont::create("Get a Supporter Badge!", "goldFont.fnt");
    heading1->limitLabelWidth(mainLayerWidthSpacing, 0.75f, 0.7f);
    heading1->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 60.f});
    m_mainLayer->addChild(heading1);

    // desc1
    auto desc1 = CCLabelBMFont::create("Get a special badge shown to\nall players and a colored comment within Rated Layouts!", "bigFont.fnt");
    desc1->limitLabelWidth(mainLayerWidthSpacing, 0.5f, 0.3f);
    desc1->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 90.f});
    desc1->setAlignment(kCCTextAlignmentCenter);
    m_mainLayer->addChild(desc1);

    // title2
    auto heading2 = CCLabelBMFont::create("Get Exclusive Features", "goldFont.fnt");
    heading2->limitLabelWidth(mainLayerWidthSpacing, 0.75f, 0.7f);
    heading2->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 125.f});
    m_mainLayer->addChild(heading2);
    // desc2
    auto desc2 = CCLabelBMFont::create("Access exclusive and up-coming features added in Rated Layouts.\nAnd get early access to these new features!", "bigFont.fnt");
    desc2->limitLabelWidth(mainLayerWidthSpacing, 0.5f, 0.3f);
    desc2->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 155.f});
    desc2->setAlignment(kCCTextAlignmentCenter);
    m_mainLayer->addChild(desc2);

    // title3
    auto heading3 = CCLabelBMFont::create("Priority Layout Requests", "goldFont.fnt");
    heading3->limitLabelWidth(mainLayerWidthSpacing, 0.75f, 0.7f);
    heading3->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 190.f});
    m_mainLayer->addChild(heading3);

    // desc3
    auto desc3 = CCLabelBMFont::create("Request priority consideration for your layout\nsubmissions and get faster review from the team.", "bigFont.fnt");
    desc3->limitLabelWidth(mainLayerWidthSpacing, 0.5f, 0.3f);
    desc3->setPosition({m_mainLayer->getScaledContentSize().width / 2.f, m_mainLayer->getScaledContentSize().height - 220.f});
    desc3->setAlignment(kCCTextAlignmentCenter);
    m_mainLayer->addChild(desc3);

    // menu for da buttons yes
    auto donoMenu = CCMenu::create();
    donoMenu->setPosition({m_mainLayer->getContentWidth() / 2.f, 20});
    donoMenu->setContentSize({m_mainLayer->getContentWidth(), 40.f});
    donoMenu->setLayout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Center)->setAxisReverse(false));
    m_mainLayer->addChild(donoMenu);

    // badge request button
    auto getBadgeSpr = ButtonSprite::create("Get Badge?", "goldFont.fnt", "GJ_button_01.png");
    auto getBadgeBtn = CCMenuItemSpriteExtra::create(getBadgeSpr, this, menu_selector(RLDonationPopup::onGetBadge));
    donoMenu->addChild(getBadgeBtn);

    // open kofi link button
    auto kofiSpr = ButtonSprite::create("Donate via Ko-fi", "goldFont.fnt", "GJ_button_03.png");
    auto kofiBtn = CCMenuItemSpriteExtra::create(kofiSpr, this, menu_selector(RLDonationPopup::onClick));
    donoMenu->addChild(kofiBtn);

    // request supporter features
    auto requestSpr = ButtonSprite::create("Request Features", "goldFont.fnt", "GJ_button_05.png");
    auto requestBtn = CCMenuItemSpriteExtra::create(requestSpr, this, menu_selector(RLDonationPopup::onAccessBadge));  // same popup for now since we don't have a separate feature request system
    donoMenu->addChild(requestBtn);

    donoMenu->updateLayout();

    // floating blocks
    static bool _rl_rain_seeded = false;
    if (!_rl_rain_seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        _rl_rain_seeded = true;
    }

    const int BLOCK_COUNT = 30;
    const float margin = 0.f;
    const float WIDTH_BOUND = 460.f;
    const float HEIGHT_BOUND = 270.f;

    auto makeRain = [this, WIDTH_BOUND, HEIGHT_BOUND](CCSprite* spr, float minDur, float maxDur, float rotAmount, float alpha) {
        float startX = spr->getPositionX();
        // add a randomized vertical offset so each block starts at a different height
        float rnd = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float startY = HEIGHT_BOUND + spr->getContentSize().height * spr->getScale() + rnd * HEIGHT_BOUND;  // up to one extra bound above
        const float endY = -15.f;
        spr->setPosition({startX, startY});
        spr->setOpacity(static_cast<GLubyte>(alpha));

        // randomized duration
        float durRnd = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float dur = minDur + durRnd * (maxDur - minDur);

        // fade to low opacity while moving down, then instantly reset position and opacity
        auto moveToEnd = CCMoveTo::create(dur, {startX, endY});
        auto fadeToEnd = CCFadeTo::create(dur, 10.f);  // end opacity = 10
        auto spawn = CCSpawn::create(moveToEnd, fadeToEnd, nullptr);
        auto placeBack = CCMoveTo::create(0.f, {startX, startY});
        auto resetFade = CCFadeTo::create(0.f, static_cast<GLubyte>(spr->getOpacity()));
        auto seq = CCSequence::create(spawn, placeBack, resetFade, nullptr);

        spr->runAction(CCRepeatForever::create(seq));
        spr->runAction(CCRepeatForever::create(CCRotateBy::create(dur, rotAmount)));
    };

    for (int i = 0; i < BLOCK_COUNT; ++i) {
        auto spr = CCSprite::createWithSpriteFrameName("square_01_001.png");
        if (!spr) continue;

        float rnd = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float scale = 0.45f + rnd * 0.6f;  // 0.45 - 1.05
        float x = margin + rnd * (WIDTH_BOUND - margin * 2.f);
        float rot = -40.f + rnd * 80.f;     // -40..40
        float alpha = 150.f + rnd * 100.f;  // 150..250

        spr->setScale(scale);
        spr->setRotation(rot);
        spr->setPosition({x, 0.f});  // y will be set by makeRain
        clip->addChild(spr);

        // randomized timing and rotation direction
        float minDur = 3.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 3.0f;       // 3 - 6
        float maxDur = minDur + 10.0f;                                                   // spread
        float rotAmount = -60.f + (static_cast<float>(std::rand()) / RAND_MAX) * 120.f;  // -60..60
        float delay = (static_cast<float>(std::rand()) / RAND_MAX) * (minDur);           // staggered start

        makeRain(spr, minDur, maxDur, rotAmount, alpha);
    }

    return true;
}

void RLDonationPopup::onClick(CCObject* sender) {
    utils::web::openLinkInBrowser("https://ko-fi.com/arcticwoof");
}

void RLDonationPopup::onGetBadge(CCObject* sender) {
    auto popup = RLBadgeRequestPopup::create();
    if (popup) popup->show();
}

void RLDonationPopup::onAccessBadge(CCObject* sender) {
    m_uploadPopup = UploadActionPopup::create(nullptr, "Requesting Access...");
    m_uploadPopup->show();
    // argon my beloved <3
    auto accountData = argon::getGameAccountData();

    m_authTask.spawn(
        argon::startAuth(std::move(accountData)),
        [this](Result<std::string> res) {
            if (res.isOk()) {
                auto token = std::move(res).unwrap();
                log::info("token obtained: {}", token);
                Mod::get()->setSavedValue("argon_token", token);

                // json body
                matjson::Value jsonBody = matjson::Value::object();
                jsonBody["argonToken"] = token;
                jsonBody["accountId"] = GJAccountManager::get()->m_accountID;

                // verify the user's role
                auto postReq = web::WebRequest();
                postReq.bodyJSON(jsonBody);

                m_getAccessTask.spawn(
                    postReq.post(std::string(rl::BASE_API_URL) + "/getAccessSupporter"),
                    [this](web::WebResponse response) {
                        log::info("Received response from server");
                        if (!response.ok()) {
                            log::warn("Server returned non-ok status: {}",
                                response.code());
                            m_uploadPopup->showFailMessage(rl::getResponseFailMessage(
                                response, "Failed! Try again later."));
                            return;
                        }

                        auto jsonRes = response.json();
                        if (!jsonRes) {
                            log::warn("Failed to parse JSON response");
                            m_uploadPopup->showFailMessage(
                                "Failed to parse JSON response");
                            return;
                        }

                        auto json = jsonRes.unwrap();
                        bool isSupporter =
                            json["isSupporter"].asBool().unwrapOrDefault();
                        bool isBooster =
                            json["isBooster"].asBool().unwrapOrDefault();

                        Mod::get()->setSavedValue<bool>("isSupporter", isSupporter);
                        Mod::get()->setSavedValue<bool>("isBooster", isBooster);

                        if (isSupporter || isBooster) {
                            log::info("Granted Supporter Features");
                            m_uploadPopup->showSuccessMessage(
                                "Granted Supporter Features.");
                        } else {
                            m_uploadPopup->showFailMessage("You are not a Supporter or Booster.");
                        }
                    });
            } else {
                auto err = res.unwrapErr();
                log::warn("Auth failed: {}", err);
                Notification::create(err, NotificationIcon::Error)->show();
                argon::clearToken();
            }
        });
};
