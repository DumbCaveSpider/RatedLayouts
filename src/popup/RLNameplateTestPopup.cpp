#include "RLNameplateTestPopup.hpp"
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/utils/file.hpp>
#include <algorithm>
#include "Geode/utils/general.hpp"

using namespace geode::prelude;

arc::Future<void> RLNameplateTestPopup::pickAndLoadPng() {
    auto popup = WeakRef(this);
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({"PNG Image", {"*.png"}});

    auto notify = [&](std::string message) {
        Loader::get()->queueInMainThread([message = std::move(message)]() {
            Notification::create(message.c_str(), NotificationIcon::Warning)->show();
        });
    };

    auto result = co_await geode::utils::file::pick(
        geode::utils::file::PickMode::OpenFile,
        options);

    if (!result) {
        notify("Failed to open file picker");
        co_return;
    }

    auto maybePath = result.unwrap();
    if (!maybePath) {
        co_return;
    }

    auto path = *maybePath;

    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension != ".png") {
        notify("Please select a PNG file");
        co_return;
    }

    auto binaryResult = geode::utils::file::readBinary(path);
    if (!binaryResult) {
        notify("Failed to read file");
        co_return;
    }

    CCImage image;
    if (!image.initWithImageFile(path.string().c_str())) {
        notify("Unable to decode image");
        co_return;
    }

    if (image.getWidth() != 1500 || image.getHeight() != 150) {
        notify("Image must be 1500x150, got " + numToString(image.getWidth()) + "x" + numToString(image.getHeight()));
        co_return;
    }

    // imma goog this an create a new lazysprite with this one :speak:
    Loader::get()->queueInMainThread([popup, path = std::move(path)]() {
        if (auto self = popup.lock()) {
            if (auto existing = self->m_nameplateLazy) {
                auto parent = existing->getParent();
                auto zorder = existing->getZOrder();
                auto position = existing->getPosition();
                auto anchor = existing->getAnchorPoint();
                auto size = existing->getContentSize();

                existing->removeFromParent();

                auto replacement = LazySprite::create(size, false);
                replacement->setAutoResize(true);
                replacement->setPosition(position);
                replacement->setAnchorPoint(anchor);

                if (parent) {
                    parent->addChild(replacement, zorder);
                }

                self->m_nameplateLazy = replacement;
                self->m_nameplateLazy->loadFromFile(path, CCImage::kFmtPng, true);
            }
        }
    });

    co_return;
}

RLNameplateTestPopup* RLNameplateTestPopup::create() {
    auto popup = new RLNameplateTestPopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool RLNameplateTestPopup::init() {
    if (!Popup::init(440, 130, "GJ_square01.png"))
        return false;

    this->setTitle("Nameplate Test");

    m_leaderboardListNode = cue::ListNode::create({355.f, 40.f});
    m_leaderboardListNode->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f});
    m_mainLayer->addChild(m_leaderboardListNode);

    // leaderboard item
    auto rowContainer = CCLayer::create();
    rowContainer->setContentSize({356.f, 40.f});

    m_nameplateLazy = LazySprite::create({356.f, 40.f}, false);
    m_nameplateLazy->setAutoResize(true);
    m_nameplateLazy->setPosition({rowContainer->getContentWidth() / 2.f, 20.f});
    rowContainer->addChild(m_nameplateLazy, -1);

    // Rank label
    auto rankLabel =
        CCLabelBMFont::create("1", "goldFont.fnt");
    rankLabel->setScale(0.5f);
    rankLabel->setPosition({15.f, 20.f});
    rankLabel->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(rankLabel, 2);

    auto accountLabel = CCLabelBMFont::create("Player", "goldFont.fnt");
    accountLabel->setAnchorPoint({0.f, 0.5f});
    accountLabel->setScale(0.7f);
    accountLabel->setPosition({80.f, 20.f});
    accountLabel->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(accountLabel, 2);

    auto gm = GameManager::sharedState();
    auto player = SimplePlayer::create(1);
    player->updatePlayerFrame(1, IconType::Cube);
    player->setColors(gm->colorForIdx(1), gm->colorForIdx(2));
    player->setPosition({55.f, 20.f});
    player->setScale(0.75f);
    rowContainer->addChild(player, 2);

    auto scoreLabelText = CCLabelBMFont::create(
        fmt::format("{}", GameToolbox::pointsToString(69420)).c_str(),
        "bigFont.fnt");
    scoreLabelText->setScale(0.5f);
    scoreLabelText->setPosition({320.f, 20.f});
    scoreLabelText->setAnchorPoint({1.f, 0.5f});
    rowContainer->addChild(scoreLabelText, 2);

    auto iconSprite = CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
    iconSprite->setScale(0.65f);
    iconSprite->setPosition({325.f, 20.f});
    iconSprite->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(iconSprite, 2);

    m_leaderboardListNode->addCell(rowContainer);
    m_leaderboardListNode->getScrollLayer()->m_disableMovement = true;

    auto pickSprite = ButtonSprite::create("Pick Nameplate", 120, true, "goldFont.fnt", "GJ_button_01.png", 25.f, .7f);
    auto pickMenuItem = CCMenuItemSpriteExtra::create(
        pickSprite, this, menu_selector(RLNameplateTestPopup::onPickImage));
    pickMenuItem->setPosition({m_mainLayer->getContentSize().width / 2, 25.f});
    m_buttonMenu->addChild(pickMenuItem);

    return true;
}

void RLNameplateTestPopup::onPickImage(CCObject* sender) {
    async::spawn(this->pickAndLoadPng());
}