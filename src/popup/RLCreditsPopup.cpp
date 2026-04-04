#include "RLCreditsPopup.hpp"
#include "../include/RLAchievements.hpp"
#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include "../include/RLConstants.hpp"

using namespace geode::prelude;
RLCreditsPopup* RLCreditsPopup::create() {
    auto ret = new RLCreditsPopup();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLCreditsPopup::init() {
    if (!Popup::init(440.f, 280.f))
        return false;
    setTitle("Rated Layouts Credits");

    m_listNode = cue::ListNode::create({350.f, 205.f});
    m_listNode->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f - 10.f});
    m_mainLayer->addChild(m_listNode);

    auto scrollLayer = m_listNode->getScrollLayer();
    if (!scrollLayer)
        return false;

    if (!Mod::get()->getSettingValue<bool>("disableScrollbar")) {
        auto scrollbar = Scrollbar::create(scrollLayer);
        scrollbar->setPosition({m_mainLayer->getContentSize().width - 35.f,
            (m_mainLayer->getContentSize().height / 2.f) - 5.f});
        scrollbar->setScale(0.9f);
        m_mainLayer->addChild(scrollbar);
    }

    // info button
    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.75f);
    auto infoBtn = CCMenuItemSpriteExtra::create(
        infoSpr, this, menu_selector(RLCreditsPopup::onInfo));
    infoBtn->setPosition({m_mainLayer->getContentSize().width,
        m_mainLayer->getContentSize().height - 3});
    m_buttonMenu->addChild(infoBtn);

    // create spinner
    auto contentLayer = scrollLayer->m_contentLayer;
    if (contentLayer) {
        auto layout = ColumnLayout::create();
        contentLayer->setLayout(layout);
        layout->setGap(0.f);
        layout->setAutoGrowAxis(0.f);
        layout->setAxisReverse(true);

        auto spinner = LoadingSpinner::create(48.f);
        spinner->setPosition(contentLayer->getContentSize() / 2);
        contentLayer->addChild(spinner);
        m_spinner = spinner;
    }

    // Fetch mod players
    Ref<RLCreditsPopup> self = this;
    web::WebRequest req;
    self->m_spinner = nullptr;  // keep consistent
    async::spawn(
        req.get(std::string(rl::BASE_API_URL) + "/getCredits"),
        [self](web::WebResponse res) {
            if (!self)
                return;
            if (!res.ok()) {
                log::warn("getCredits returned non-ok status: {}", res.code());
                if (self->m_spinner) {
                    self->m_spinner->removeFromParent();
                    self->m_spinner = nullptr;
                }
                Notification::create("Failed to fetch mod players",
                    NotificationIcon::Error)
                    ->show();
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                log::warn("Failed to parse mod players JSON");
                if (self->m_spinner) {
                    self->m_spinner->removeFromParent();
                    self->m_spinner = nullptr;
                }
                Notification::create("Invalid server response",
                    NotificationIcon::Error)
                    ->show();
                return;
            }

            auto json = jsonRes.unwrap();
            bool success = json["success"].asBool().unwrapOrDefault();
            if (!success) {
                log::warn("Server returned success=false for getMod");
                if (self->m_spinner) {
                    self->m_spinner->removeFromParent();
                    self->m_spinner = nullptr;
                }
                return;
            }

            RLAchievements::onReward("misc_credits");

            // populate players
            if (!self->m_listNode)
                return;
            auto scrollLayer = self->m_listNode->getScrollLayer();
            if (!scrollLayer || !scrollLayer->m_contentLayer)
                return;
            if (self->m_spinner) {
                self->m_spinner->removeFromParent();
                self->m_spinner = nullptr;
            }
            self->m_listNode->clear();

            auto addHeader = [&](std::string_view text) {  // header yesz
                auto content = self->m_listNode->getScrollLayer()->m_contentLayer;
                auto tableCell = TableViewCell::create();
                tableCell->setContentSize({340.f, 30.f});
                auto label =
                    CCLabelBMFont::create(std::string{text}.c_str(), "bigFont.fnt");
                // center the label in the cell
                label->setAnchorPoint({0.5f, 0.5f});
                const float contentW = tableCell->getContentSize().width;
                const float labelX = contentW / 2.f;
                label->limitLabelWidth(contentW - 120.f, 0.5f, 0.4f);
                label->setPosition({labelX, 15.f});

                tableCell->addChild(label);

                // badge sprite on the left of the label
                CCSprite* headerBadge = nullptr;
                if (text == "Platformer Layout Admins")
                    headerBadge = CCSprite::createWithSpriteFrameName(
                        "RL_badgePlatAdmin01.png"_spr);
                else if (text == "Platformer Layout Moderators")
                    headerBadge = CCSprite::createWithSpriteFrameName(
                        "RL_badgePlatMod01.png"_spr);
                else if (text == "Leaderboard Layout Admins")
                    headerBadge = CCSprite::createWithSpriteFrameName(
                        "RL_badgelbAdmin01.png"_spr);
                else if (text == "Leaderboard Layout Moderators")
                    headerBadge =
                        CCSprite::createWithSpriteFrameName("RL_badgelbMod01.png"_spr);
                else if (text == "Rated Layouts Owner")
                    headerBadge =
                        CCSprite::createWithSpriteFrameName("RL_badgeOwner.png"_spr);
                else if (text.find("Admin") != std::string::npos)
                    headerBadge =
                        CCSprite::createWithSpriteFrameName("RL_badgeAdmin01.png"_spr);
                else if (text.find("Moderator") != std::string::npos)
                    headerBadge =
                        CCSprite::createWithSpriteFrameName("RL_badgeMod01.png"_spr);
                else if (text == "Layout Supporters")
                    headerBadge = CCSprite::createWithSpriteFrameName(
                        "RL_badgeSupporter.png"_spr);
                else if (text == "Layout Boosters")
                    headerBadge =
                        CCSprite::createWithSpriteFrameName("RL_badgeBooster.png"_spr);
                const float gap = 8.f;
                float labelWidth = label->getContentSize().width * label->getScale();
                if (headerBadge) {
                    headerBadge->setScale(0.9f);
                    float badgeWidth =
                        headerBadge->getContentSize().width * headerBadge->getScale();
                    float badgeX =
                        labelX - (labelWidth / 2.f) - gap - (badgeWidth / 2.f);
                    headerBadge->setPosition({badgeX, 15.f});
                    tableCell->addChild(headerBadge);
                }

                auto headerMenu = CCMenu::create();
                headerMenu->setPosition({0, 0});
                auto infoSpr =
                    CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
                infoSpr->setScale(0.4f);
                auto infoBtn = CCMenuItemSpriteExtra::create(
                    infoSpr, self, menu_selector(RLCreditsPopup::onHeaderInfo));
                int infoTag = 0;
                if (text == "Layout Supporters")
                    infoTag = 3;
                else if (text == "Layout Boosters")
                    infoTag = 4;
                else if (text == "Classic Layout Admins")
                    infoTag = 5;
                else if (text == "Classic Layout Moderators")
                    infoTag = 6;
                else if (text == "Platformer Layout Admins")
                    infoTag = 7;
                else if (text == "Platformer Layout Moderators")
                    infoTag = 8;
                else if (text == "Leaderboard Layout Admins")
                    infoTag = 11;
                else if (text == "Leaderboard Layout Moderators")
                    infoTag = 9;
                else if (text == "Rated Layouts Owner")
                    infoTag = 10;
                infoBtn->setTag(infoTag);
                // place next to label
                float infoWidth =
                    infoSpr->getContentSize().width * infoSpr->getScale();
                float infoX = labelX + (labelWidth / 2.f) + gap + (infoWidth / 2.f);
                infoBtn->setPosition({infoX, 15.f});
                headerMenu->addChild(infoBtn);
                tableCell->addChild(headerMenu);

                if (self->m_listNode) {
                    self->m_listNode->addCell(tableCell);
                }
            };

            auto addPlayer = [&](const matjson::Value& userVal, bool isAdmin, bool isMod, bool isBooster, bool isSupporter, bool isPlat, bool isLeaderboard, bool isOwner) {
                if (!userVal.isObject())
                    return;

                int accountId = userVal["accountId"].asInt().unwrapOrDefault();
                std::string username =
                    userVal["username"].asString().unwrapOrDefault();
                int iconId = userVal["iconid"].asInt().unwrapOrDefault();
                int color1 = userVal["color1"].asInt().unwrapOrDefault();
                int color2 = userVal["color2"].asInt().unwrapOrDefault();
                int color3 = userVal["color3"].asInt().unwrapOrDefault();

                auto cell = TableViewCell::create();
                cell->setContentSize({340.f, 50.f});

                // color background according to role subtype
                auto bgSprite = CCSprite::create();
                bgSprite->setTextureRect(CCRectMake(0, 0, 360.f, 50.f));
                bgSprite->setPosition({170.f, 25.f});
                bgSprite->setOpacity(120);
                if (isOwner) {
                    bgSprite->setColor({150, 255, 255});  // cyan(boi) for owner
                } else if (isAdmin) {
                    if (isPlat) {
                        bgSprite->setColor({255, 160, 0});  // orange for plat admin
                    } else {
                        bgSprite->setColor({245, 107, 107});  // red for classic admin
                    }
                } else if (isMod) {
                    if (isLeaderboard) {
                        bgSprite->setColor({120, 220, 120});  // green for leaderboard mod
                    } else if (isPlat) {
                        bgSprite->setColor({0, 200, 200});  // cyan for plat mod
                    } else {
                        bgSprite->setColor({81, 147, 248});  // blue for classic mod
                    }
                } else if (isBooster) {
                    bgSprite->setColor({148, 93, 255});
                } else if (isSupporter) {
                    bgSprite->setColor({248, 86, 187});
                }
                cell->addChild(bgSprite);

                auto gm = GameManager::sharedState();
                auto player = SimplePlayer::create(iconId);
                player->updatePlayerFrame(iconId, IconType::Cube);
                player->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
                if (color3 != 0)
                    player->setGlowOutline(gm->colorForIdx(color3));
                player->setPosition({40.f, 25.f});
                cell->addChild(player);

                auto nameLabel =
                    CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
                nameLabel->setAnchorPoint({0.f, 0.5f});
                nameLabel->setScale(0.8f);
                nameLabel->setPosition({80.f, 25.f});

                auto menu = CCMenu::create();
                menu->setPosition({0, 0});
                auto accountButton = CCMenuItemSpriteExtra::create(
                    nameLabel, self, menu_selector(RLCreditsPopup::onAccountClicked));
                accountButton->setTag(accountId);
                accountButton->setPosition({70.f, 25.f});
                accountButton->setAnchorPoint({0.f, 0.5f});
                menu->addChild(accountButton);
                cell->addChild(menu);

                if (self->m_listNode) {
                    self->m_listNode->addCell(cell);
                }
            };
            if (json.contains("owner") && json["owner"].isArray()) {
                addHeader("Rated Layouts Owner");
                auto arr = json["owner"].asArray().unwrap();
                for (auto& val : arr) {
                    addPlayer(val, true, false, false, false, false, false, true);
                }
            }
            if (json.contains("classicAdmins") && json["classicAdmins"].isArray()) {
                addHeader("Classic Layout Admins");
                auto arr = json["classicAdmins"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, true, false, false, false, false, false, false);
            }
            if (json.contains("platAdmins") && json["platAdmins"].isArray()) {
                addHeader("Platformer Layout Admins");
                auto arr = json["platAdmins"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, true, false, false, false, true, false, false);
            }
            if (json.contains("classicModerators") &&
                json["classicModerators"].isArray()) {
                addHeader("Classic Layout Moderators");
                auto arr = json["classicModerators"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, false, true, false, false, false, false, false);
            }
            if (json.contains("platModerators") &&
                json["platModerators"].isArray()) {
                addHeader("Platformer Layout Moderators");
                auto arr = json["platModerators"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, false, true, false, false, true, false, false);
            }
            if (json.contains("leaderboardAdmins") && json["leaderboardAdmins"].isArray()) {
                addHeader("Leaderboard Layout Admins");
                auto arr = json["leaderboardAdmins"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, true, false, false, false, false, true, false);
            }
            if (json.contains("leaderboardModerators") &&
                json["leaderboardModerators"].isArray()) {
                addHeader("Leaderboard Layout Moderators");
                auto arr = json["leaderboardModerators"].asArray().unwrap();
                for (auto& val : arr)
                    addPlayer(val, false, true, false, false, false, true, false);
            }
            if (json.contains("supporters") && json["supporters"].isArray()) {
                addHeader("Layout Supporters");
                auto sup = json["supporters"].asArray().unwrap();
                for (auto& val : sup) {
                    addPlayer(val, false, false, false, true, false, false, false);
                }
            }

            if (json.contains("boosters") && json["boosters"].isArray()) {
                addHeader("Layout Boosters");
                auto boosters = json["boosters"].asArray().unwrap();
                for (auto& val : boosters) {
                    addPlayer(val, false, false, true, false, false, false, false);
                }
            }
            if (self->m_listNode) {
                self->m_listNode->updateLayout();
                if (auto listScroll = self->m_listNode->getScrollLayer()) {
                    listScroll->scrollToTop();
                }
            }
        });

    return true;
}

void RLCreditsPopup::onAccountClicked(CCObject* sender) {
    auto button = static_cast<CCMenuItem*>(sender);
    int accountId = button->getTag();
    ProfilePage::create(accountId, false)->show();
}

void RLCreditsPopup::onInfo(CCObject* sender) {
    MDPopup::create(
        "Becoming a Layout Moderator",
        "To become a **<cl>Classic</c>/<co>Platformer</c>/<cb>Leaderboard</c> "
        "Layout "
        "Moderator</c>**, you are required to join the <cl>Rated Layouts Discord "
        "Server</c> and be <cg>active in the community</c>.\n"
        "There's an <cl>application form</c> in the server that you can fill out "
        "and the Admins usually review these applications.\n"
        "### <cr>Begging for Layout Mod to ArcticWoof or any of the Layout "
        "Admins will be ignored and lower your chances of becoming a mod.</c>\n"
        "If you have any questions about the application process or the role, "
        "feel free to ask in the <cl>Rated Layouts Discord Server</c>.\n"
        "All promotion will be decided by <cf>ArcticWoof</c> and is usually "
        "announced in the server."
        "\r\n\r\n---\r\n\r\n"
        "### Moderator Responsibilities\n"
        "- Moderators are expected to be <cg>active in the community</c> and "
        "help maintain the quality of the <cl>Rated Layouts</c>.\n"
        "- This includes <co>suggesting levels, rating levels</c> and "
        "<cy>providing "
        "feedback</c> to level creators.\n"
        "- Moderators may also be asked to help with <cg>managing the "
        "community</c>, such as moderating the leaderboard section or assisting "
        "with events.\n"
        "\r\n\r\n---\r\n\r\n"
        "If you are <cg>interested in becoming a layout moderator</c>, make sure "
        "to "
        "join the <cl>Rated Layouts Discord Server</c> and apply in the "
        "application form!",
        "OK")
        ->show();
}

void RLCreditsPopup::onHeaderInfo(CCObject* sender) {
    auto btn = static_cast<CCMenuItem*>(sender);
    if (!btn)
        return;
    int tag = btn->getTag();
    switch (tag) {
        case 3:  // Supporters
            rl::showSupporterInfo();
            break;
        case 4:  // Boosters
            rl::showBoosterInfo();
            break;
        case 5:  // Classic Admins
            rl::showClassicAdminInfo();
            break;
        case 6:  // Classic Mods
            rl::showClassicModInfo();
            break;
        case 7:  // Plat Admins
            rl::showPlatAdminInfo();
            break;
        case 8:  // Plat Mods
            rl::showPlatModInfo();
            break;
        case 9:  // Leaderboard Mods
            rl::showLeaderboardModInfo();
            break;
        case 11:  // Leaderboard Admins
            rl::showLeaderboardAdminInfo();
            break;
        case 10:  // Owner
            rl::showOwnerInfo();
            break;
        default:
            break;
    }
}
