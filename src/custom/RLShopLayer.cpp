#include "RLShopLayer.hpp"
#include "../include/RLDialogIcons.hpp"
#include "../utils/RLNameplateItem.hpp"
#include "Geode/ui/General.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/Popup.hpp"
#include "Geode/utils/async.hpp"
#include "Geode/utils/random.hpp"
#include "RLBuyItemPopup.hpp"
#include "RLSecretLayer1.hpp"
#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <fmt/format.h>

using namespace geode::prelude;
using namespace rl;

RLShopLayer* RLShopLayer::create() {
    auto layer = new RLShopLayer();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool RLShopLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    addBackButton(this, BackButtonStyle::Pink);

    // bg
    auto shopBGSpr = CCSprite::createWithSpriteFrameName("RL_shopBG.png"_spr);
    auto bgSize = shopBGSpr->getTextureRect().size;

    shopBGSpr->setAnchorPoint({0.0f, 0.0f});
    shopBGSpr->setScaleX((winSize.width + 10.0f) / bgSize.width);
    shopBGSpr->setScaleY((winSize.height + 10.0f) / bgSize.height);
    shopBGSpr->setPosition({-5.0f, -5.0f});
    this->addChild(shopBGSpr, -3);

    // desk
    auto deckSpr = CCSprite::createWithSpriteFrameName("RL_storeDesk.png"_spr);
    deckSpr->setPosition({winSize.width / 2, 95});
    this->addChild(deckSpr);

    // ruby counter
    auto rubySpr = CCSprite::createWithSpriteFrameName("RL_rubiesIcon.png"_spr);
    rubySpr->setPosition({winSize.width - 20, winSize.height - 20});
    rubySpr->setScale(0.7f);
    this->addChild(rubySpr);

    // lyout creator menu
    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    this->addChild(menu, -1);

    // layout creator (clickable)
    auto gm = GameManager::sharedState();
    auto shopkeeperIcon =
        CCSprite::createWithSpriteFrameName("RL_arcticwoof.png"_spr);
    shopkeeperIcon->setScale(2.f);

    auto shopkeeperItem = CCMenuItemSpriteExtra::create(
        shopkeeperIcon, this, menu_selector(RLShopLayer::onLayoutCreator));
    shopkeeperItem->setPosition(
        {winSize.width / 2 - 120, winSize.height / 2 + 65});
    shopkeeperItem->m_scaleMultiplier = 1.02;
    menu->addChild(shopkeeperItem);

    // ruby counter label
    auto rubyLabel =
        CCCounterLabel::create(Mod::get()->getSavedValue<int>("rubies"),
            "bigFont.fnt",
            FormatterType::Integer);
    rubyLabel->setPosition(
        {rubySpr->getPositionX() - 15, rubySpr->getPositionY()});
    rubyLabel->setAnchorPoint({1.0f, 0.5f});
    rubyLabel->setScale(0.6f);
    m_rubyLabel = rubyLabel;
    this->addChild(rubyLabel);

    // ruby shop sign
    auto shopSignSpr =
        CCSprite::createWithSpriteFrameName("RL_shopSign_001.png"_spr);
    shopSignSpr->setPosition({winSize.width / 2 + 60, winSize.height - 45});
    shopSignSpr->setScale(1.2f);
    this->addChild(shopSignSpr, -2);

    // PLUSHIESS
    auto plushiesSpr =
        CCSprite::createWithSpriteFrameName("RL_plushpile.png"_spr);
    plushiesSpr->setPosition({winSize.width / 2 - 10, winSize.height / 2 + 28});
    plushiesSpr->setAnchorPoint({0.5f, 0.f});
    this->addChild(plushiesSpr, -2);

    // random sign image
    std::vector<std::string> signFrames = {
        "signImage_00.png"_spr, "signImage_01.png"_spr, "signImage_02.png"_spr, "signImage_03.png"_spr, "signImage_04.png"_spr, "signImage_05.png"_spr, "signImage_06.png"_spr, "signImage_07.png"_spr, "signImage_08.png"_spr};
    static geode::utils::random::Generator signGen = [] {
        geode::utils::random::Generator g;
        g.seed(geode::utils::random::secureU64());
        return g;
    }();

    int signIndex = signGen.generate<int>(0, static_cast<int>(signFrames.size()));
    auto signSpr =
        CCSprite::createWithSpriteFrameName(signFrames[signIndex].c_str());
    if (signSpr) {
        signSpr->setPosition({19, 32});
        signSpr->setRotation(-10);
        plushiesSpr->addChild(signSpr, 1);
    }
    // button to reset
    auto resetBtnSpr = ButtonSprite::create(
        "Reset Rubies", 80, true, "goldFont.fnt", "GJ_button_06.png", 25.f, .7f);
    auto resetBtn = CCMenuItemSpriteExtra::create(
        resetBtnSpr, this, menu_selector(RLShopLayer::onResetRubies));
    resetBtn->setPosition(
        {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 30});
    menu->addChild(resetBtn);

    // button to unequip nameplate
    auto unequipSpr = ButtonSprite::create("Unequip", 80, true, "goldFont.fnt", "GJ_button_06.png", 25.f, .7f);
    auto unequipBtn = CCMenuItemSpriteExtra::create(
        unequipSpr, this, menu_selector(RLShopLayer::onUnequipNameplate));
    unequipBtn->setPosition(
        {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 60});
    menu->addChild(unequipBtn);

    // open submission form
    auto submitSpr = ButtonSprite::create("Submission", 80, true, "goldFont.fnt", "GJ_button_01.png", 25.f, .7f);
    auto submitBtn = CCMenuItemSpriteExtra::create(
        submitSpr, this, menu_selector(RLShopLayer::onForm));
    submitBtn->setPosition(
        {rubyLabel->getPositionX() - 30, rubyLabel->getPositionY() - 90});
    menu->addChild(submitBtn);

    // bottom left redeem button
    auto orcaleSpr = CCSprite::createWithSpriteFrameName("RL_observatoryDoor.png"_spr);
    orcaleSpr->setColor({50, 50, 50});
    orcaleSpr->setOpacity(150);
    orcaleSpr->setScale(1.25f);
    auto redeemBtn = CCMenuItemSpriteExtra::create(
        orcaleSpr, this, menu_selector(RLShopLayer::onRedeemLayer));
    redeemBtn->setPosition({20, 25});
    menu->addChild(redeemBtn);

    // shop item menu
    auto shopMenu = CCMenu::create();
    shopMenu->setPosition({deckSpr->getContentSize().width / 2,
        deckSpr->getContentSize().height / 2});
    shopMenu->setContentSize({deckSpr->getContentSize().width - 40,
        deckSpr->getContentSize().height - 37});
    // arrange rows vertically
    shopMenu->setLayout(ColumnLayout::create()
            ->setGap(-15.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAxisReverse(true));

    // two horizontal row menus
    auto rowH = [&](void) {
        auto m = CCMenu::create();
        m->setPosition(
            {m->getContentSize().width / 2.f, m->getContentSize().height / 2.f});
        m->setContentSize({shopMenu->getContentSize().width,
            (shopMenu->getContentSize().height - 8.f) / 2.f});
        m->setLayout(RowLayout::create()
                ->setGap(40.f)
                ->setAxisAlignment(AxisAlignment::Center)
                ->setAxisReverse(false));
        m->updateLayout();
        return m;
    };

    m_shopRow1 = rowH();
    m_shopRow2 = rowH();

    m_shopItems.clear();
    m_shopRow1->setAnchorPoint({0.5f, 0.5f});
    m_shopRow2->setAnchorPoint({0.5f, 0.5f});

    // compute vertical positions to match the ColumnLayout that used to be
    auto shopMenuCS = shopMenu->getContentSize();
    float rowHeight = m_shopRow1->getContentSize().height;
    const float gap = 20.f;
    const float centerY = shopMenu->getPositionY();
    const float offset = (rowHeight / 2.f) + (gap / 2.f);

    m_shopRow1->setPosition(
        {shopMenu->getPositionX(),
            centerY + offset - 16});  // the magic number is for my ocd
    m_shopRow2->setPosition({shopMenu->getPositionX(), centerY - offset + 3});
    deckSpr->addChild(m_shopRow1, 1);
    deckSpr->addChild(m_shopRow2, 1);

    // pagination controls
    m_pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
    if (m_pageLabel) {
        m_pageLabel->setScale(0.5f);
        m_pageLabel->setPosition({shopMenu->getPositionX(), 3.f});
        m_pageLabel->setAnchorPoint({0.5f, 0.f});
        deckSpr->addChild(m_pageLabel, 2);
    }

    // pagination menu thingy
    auto pageMenu = CCMenu::create();
    pageMenu->setPosition({0, 0});
    pageMenu->setContentSize(deckSpr->getContentSize());
    deckSpr->addChild(pageMenu, 2);

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    if (prevSpr) {
        m_prevPageBtn = CCMenuItemSpriteExtra::create(
            prevSpr, this, menu_selector(RLShopLayer::onPrevPage));
        if (m_prevPageBtn) {
            m_prevPageBtn->setPosition({-10, pageMenu->getContentSize().height / 2});
            pageMenu->addChild(m_prevPageBtn);
        }
    }

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setFlipX(true);
    if (nextSpr) {
        m_nextPageBtn = CCMenuItemSpriteExtra::create(
            nextSpr, this, menu_selector(RLShopLayer::onNextPage));
        if (m_nextPageBtn) {
            m_nextPageBtn->setPosition({pageMenu->getContentSize().width + 10,
                pageMenu->getContentSize().height / 2});
            pageMenu->addChild(m_nextPageBtn);
        }
    }

    loadShopPage(0);

    shopMenu->updateLayout();
    deckSpr->addChild(shopMenu);
    this->setKeypadEnabled(true);
    return true;
}

void RLShopLayer::onForm(CCObject* sender) {
    createQuickPopup("Nameplate Submission Form",
        "You will be redirected to the <cl>Nameplate Submission "
        "Form</c> in your web browser.\n<cy>Continue?</c>",
        "No",
        "Yes",
        [](auto, bool yes) {
            if (!yes)
                return;
            Notification::create("Opening a new link to the browser",
                NotificationIcon::Info)
                ->show();
            utils::web::openLinkInBrowser(
                "https://forms.gle/3UU5JJE1XrfwPK5u7");
        });
}

// play the dum audio lol
void RLShopLayer::onEnterTransitionDidFinish() {
    CCLayer::onEnterTransitionDidFinish();
    FMODAudioEngine::sharedEngine()->playMusic("rubyShop.mp3"_spr, true, 0.f, 0);
    refreshRubyLabel();
}

void RLShopLayer::onExitTransitionDidStart() {
    CCLayer::onExitTransitionDidStart();
    GameManager::sharedState()->playMenuMusic();
}

void RLShopLayer::onLayoutCreator(CCObject* sender) {
    // gen random
    static geode::utils::random::Generator gen = [] {
        geode::utils::random::Generator g;
        g.seed(geode::utils::random::secureU64());  // seed once
        return g;
    }();

    int v = gen.generate<int>(0, 16);
    uint64_t raw = gen.next();
    DialogObject* dialogObj = nullptr;
    std::string response = "Can I help you?";
    log::debug("Random value: {}, raw: {}", v, raw);
    switch (v) {
        case 1:
            response = "I got all of the <cg>nameplates</c> in stock!";
            break;
        case 2:
            response =
                "<cg>Layout Creator</c>? <cl>Well he kinda run away when I "
                "arrived</c>, odd fella but oh well...";
            break;
        case 3:
            response =
                "The plushies are <cr>NOT FOR SALE</c>. I just keep them here "
                "because they are cute.";
            break;
        case 4:
            response =
                "<cl>Darkore</c>, that weird kid that put this <cg>awesome "
                "music</c> in the shop? Truely peak bud :)";
            break;
        case 5:
            response =
                "Someone must have break into the <cg>front door</c> while "
                "<cl>I was away...</c>";
            break;
        case 6:
            response = "Are you gonna buy something? <cy>Or just keep annoying me?</c>";
            break;
        case 7:
            response =
                "Everyone <cr>hates me</c>,<d100> but I'm their <cf>God</c>...<d050> I mean...<d100> just a <cg>normal shopkeeper</c>.";
            break;
        case 8:
            response =
                "<co><d030>s<d030>i<d030>g<d030>h<d030>.<d030>.<d030>.<d030></c> Is my own decision <cr>bad</c>...";
            break;
        case 9:
            response =
                "Cooking some <cg>new nameplates</c> for you all! <cl>Can't wait for you to see them</c>!";
            break;
        case 10:
            response = "I'm <cr>lurking</c> on <cl>every move</c> you do...";
            break;
        case 11:
            response =
                "<cg>Fun fact about me!</c> I actually <co>suck at making "
                "gameplay</c>.";
            break;
        case 12:
            response =
                "If you giving me a <cr>nameplate</c>, at least put an "
                "<cg>effort on it</c>...";
            break;
        case 13:
            response =
                "Ask <cp>The Oracle</c> about <cf>me</c>! That would be funny.";
            break;
        case 14:
            response =
                "Would you want to buy my entire shop for <cr>100k Rubies?</c> I know "
                "someone is <cy>interested</c> :P";
            break;
        case 15:
            response = "Give me <cr>10k Rubies</c> and a <cl>high-end car</c>!";
            break;
        case 16:
            response = "I heard there's <cf>The Spire</c> nearby, but I don't know how to get in there...";
            break;
        default:
            response = "Weh!";
            break;
    }
    dialogObj = DialogObject::create("ArcticWoof", response.c_str(), 1, 1.f, false, ccWHITE);

    auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
    dialog->addToMainScene();
    dialog->animateInRandomSide();

    rl::setDialogObjectCustomIcon(dialog, "RL_dialogIconAW.png"_spr);
}

void RLShopLayer::onBuyItem(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = item->getTag();
    RLNameplateInfo info;
    if (!RLNameplateItem::getInfo(idx, info)) {
        log::warn("RLShopLayer: no nameplate info for index {}", idx);
        return;
    }

    // open buy popup with creator/price information
    RLBuyItemPopup::create(info.index, info.creatorId, info.creatorUsername, info.iconUrl, info.value, this)
        ->show();
}

void RLShopLayer::onUnequipNameplate(CCObject* sender) {
    createQuickPopup(
        "Unequip Nameplate",
        "Are you sure you want to <cr>unequip your current "
        "nameplate</c>?\n<cy>You can "
        "re-equip it later from this shop page.</c>",
        "No",
        "Yes",
        [this](auto, bool yes) {
            if (!yes)
                return;

            // show a spinner/popup while we call the backend
            auto popupRef =
                UploadActionPopup::create(nullptr, "Unequipping nameplate...");
            popupRef->show();

            // validate token
            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                popupRef->showFailMessage("Argon auth missing");
                return;
            }

            // build JSON body (same format as RLBuyItemPopup::onApply)
            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
            jsonBody["argonToken"] = token;
            jsonBody["index"] = 0;

            auto req = web::WebRequest();
            req.bodyJSON(jsonBody);

            Ref<UploadActionPopup> upRef = popupRef;
            Ref<RLShopLayer> self = this;
            async::spawn(
                req.post("https://gdrate.arcticwoof.xyz/setNameplate"),
                [self, upRef](web::WebResponse res) {
                    if (!upRef)
                        return;
                    if (!res.ok()) {
                        log::warn("Failed to unequip nameplate on server: {}",
                            res.code());
                        upRef->showFailMessage(
                            "Failed to unequip nameplate on server.");
                        return;
                    }
                    auto jsonRes = res.json();
                    if (!jsonRes) {
                        upRef->showFailMessage("Invalid server response.");
                        return;
                    }
                    auto json = jsonRes.unwrap();
                    bool success = json["success"].asBool().unwrapOrDefault();
                    if (!success) {
                        upRef->showFailMessage(json["message"].asString().unwrapOr(
                            "Failed to unequip nameplate."));
                        return;
                    }

                    Mod::get()->setSavedValue<int>("selected_nameplate", 0);
                    upRef->showSuccessMessage("Nameplate unequipped!");

                    if (self) {
                        self->updateShopPage();
                    }
                });
        });
}

void RLShopLayer::updateShopPage() {
    // clear rows first
    if (m_shopRow1)
        m_shopRow1->removeAllChildrenWithCleanup(true);
    if (m_shopRow2)
        m_shopRow2->removeAllChildrenWithCleanup(true);

    // add current items
    int totalItems = static_cast<int>(m_shopItems.size());
    for (int i = 0; i < totalItems; ++i) {
        const auto& s = m_shopItems[i];
        auto item = RLNameplateItem::create(s.idx, s.price, s.creatorId, s.creatorUsername, s.iconUrl, this, menu_selector(RLShopLayer::onBuyItem));
        item->setTag(s.idx);
        if (i < 4) {
            m_shopRow1->addChild(item);
        } else {
            m_shopRow2->addChild(item);
        }
    }

    if (m_shopRow1)
        m_shopRow1->updateLayout();
    if (m_shopRow2)
        m_shopRow2->updateLayout();

    // update page UI
    if (m_pageLabel) {
        m_pageLabel->setString(
            fmt::format("{}/{}", m_shopPage + 1, m_totalPages).c_str());
    }
    if (m_prevPageBtn) {
        m_prevPageBtn->setEnabled(m_shopPage > 0);
        m_prevPageBtn->setOpacity(m_shopPage > 0 ? 255 : 120);
    }
    if (m_nextPageBtn) {
        m_nextPageBtn->setEnabled(m_shopPage < m_totalPages - 1);
        m_nextPageBtn->setOpacity(m_shopPage < m_totalPages - 1 ? 255 : 120);
    }

    // ensure parent recomputes layout
    if (m_shopRow1 && m_shopRow2) {
        if (m_shopRow1->getParent()) {
            static_cast<CCNode*>(m_shopRow1->getParent())->updateLayout();
        }
    }
}

void RLShopLayer::refreshRubyLabel() {
    if (!m_rubyLabel)
        return;
    int val = Mod::get()->getSavedValue<int>("rubies", 0);
    m_rubyLabel->setTargetCount(val);
    m_rubyLabel->updateCounter(0.25f);
}

void RLShopLayer::onPrevPage(CCObject* sender) {
    if (m_shopPage > 0) {
        loadShopPage(m_shopPage - 1);
    }
}

void RLShopLayer::onNextPage(CCObject* sender) {
    if (m_shopPage < m_totalPages - 1) {
        loadShopPage(m_shopPage + 1);
    }
}

void RLShopLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}

void RLShopLayer::loadShopPage(int page) {
    m_shopPage = page;
    matjson::Value body = matjson::Value::object();
    body["page"] = page + 1;
    body["amount"] = 8;

    auto req = web::WebRequest();
    req.bodyJSON(body);

    Ref<RLShopLayer> self = this;
    async::spawn(
        req.post("https://gdrate.arcticwoof.xyz/getNameplates"),
        [self](web::WebResponse res) {
            if (!self)
                return;
            if (!res.ok()) {
                log::warn("Failed to fetch nameplates: {}", res.code());
                Notification::create("Failed to load shop", NotificationIcon::Warning)
                    ->show();
                return;
            }
            auto jsonRes = res.json();
            if (!jsonRes) {
                Notification::create("Invalid server response",
                    NotificationIcon::Warning)
                    ->show();
                return;
            }
            auto json = jsonRes.unwrap();
            self->m_shopItems.clear();

            // server may return object with nameplates/items array or raw array
            if (json.isObject()) {
                auto itemsVal =
                    json.contains("nameplates") ? json["nameplates"] : json["items"];
                if (itemsVal.isArray()) {
                    auto arr = itemsVal.asArray().unwrap();
                    for (auto& it : arr) {
                        ShopItem si;
                        si.idx = it["index"].asInt().unwrapOrDefault();
                        si.price = it["price"].asInt().unwrapOrDefault();
                        si.creatorId = it["accountId"].asInt().unwrapOrDefault();
                        si.creatorUsername =
                            it["username"].asString().unwrapOrDefault();
                        si.iconUrl = "https://gdrate.arcticwoof.xyz" + it["url"].asString().unwrapOrDefault();

                        self->m_shopItems.push_back(si);
                    }
                }
                self->m_totalPages = json["totalPages"].asInt().unwrapOrDefault();
            }
            self->updateShopPage();
        });
}

void RLShopLayer::onResetRubies(CCObject* sender) {
    if (Mod::get()->getSavedValue<int>("rubies") <= 0) {
        Notification::create("You don't have any rubies to reset!",
            NotificationIcon::Warning)
            ->show();
        return;
    }
    createQuickPopup(
        "Reset Rubies",
        "Are you sure you want to <cr>reset your "
        "rubies</c> and <co>all your brought cosmetics</c>?\n"
        "<cy>This will clear all your rubies but you can reclaim rubies back "
        "from any completed rated layouts.</c>",
        "No",
        "Yes",
        [this](auto, bool yes) {
            if (!yes)
                return;
            // clear the data from rubies
            auto rubyPath = dirs::getModsSaveDir() / Mod::get()->getID() /
                            "rubies_collected.json";

            if (utils::file::readString(rubyPath)) {
                auto writeRes = utils::file::writeString(rubyPath, "{}");
                if (!writeRes) {
                    log::warn("Failed to clear ruby cache file: {}", rubyPath);
                }
            }

            auto ownedPath =
                dirs::getModsSaveDir() / Mod::get()->getID() / "owned_items.json";
            if (utils::file::readString(ownedPath)) {
                auto writeRes2 = utils::file::writeString(ownedPath, "[]");
                if (!writeRes2) {
                    log::warn("Failed to clear owned items file: {}", ownedPath);
                }
            }
            Mod::get()->setSavedValue<int>("selected_nameplate", 0);

            if (Mod::get()->getSavedValue<int>("rubies") > 0) {
                Mod::get()->setSavedValue<int>("rubies", 0);
                Notification::create("Rubies have been reset!",
                    NotificationIcon::Info)
                    ->show();
                FMODAudioEngine::sharedEngine()->playEffect(
                    "geode.loader/newNotif02.ogg");
            }
            m_rubyLabel->setTargetCount(0);
            m_rubyLabel->updateCounter(0.5f);

            this->updateShopPage();
        });
}

void RLShopLayer::onRedeemLayer(CCObject* sender) {
    auto searchLayer = RLSecretLayer1::create();
    auto scene = CCScene::create();
    scene->addChild(searchLayer);
    auto transitionFade = CCTransitionFade::create(0.5f, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
}
