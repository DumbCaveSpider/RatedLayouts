#include "RLRubiesCodePopup.hpp"
#include "RLAddCodePopup.hpp"
#include "../include/RLConstants.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <cue/ListNode.hpp>
#include <vector>

using namespace geode::prelude;
using namespace cue;

RLRubiesCodePopup* RLRubiesCodePopup::create() {
    auto popup = new RLRubiesCodePopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool RLRubiesCodePopup::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square02.png"))
        return false;

    setTitle("Oracle Rubies Codes");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    CCSize listSize = {
        m_mainLayer->getScaledContentSize().width - 50.f,
        m_mainLayer->getScaledContentSize().height - 80.f,
    };

    m_listNode = cue::ListNode::create(listSize, {20, 25, 55, 255}, cue::ListBorderStyle::CommentsBlue);
    m_listNode->setPosition({m_mainLayer->getContentSize().width / 2.f,
        m_mainLayer->getContentSize().height / 2.f});
    m_mainLayer->addChild(m_listNode, 1);

    m_scrollLayer = m_listNode->getScrollLayer();
    if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
        auto contentLayer = m_scrollLayer->m_contentLayer;
        auto layout = ColumnLayout::create();
        layout->setGap(0.f);
        layout->setAutoGrowAxis(listSize.height);
        layout->setAxisReverse(true);
        layout->setAxisAlignment(AxisAlignment::End);
        contentLayer->setLayout(layout);
    }

    auto scrollBar = Scrollbar::create(m_listNode->getScrollLayer());
    if (scrollBar) {
        scrollBar->setPosition({m_listNode->getContentSize().width + 10.f, m_listNode->getContentSize().height / 2.f});
        m_listNode->addChild(scrollBar);
    }

    auto newSpr = ButtonSprite::create("New", "goldFont.fnt", "GJ_button_01.png");
    auto newBtn = CCMenuItemSpriteExtra::create(newSpr, this, menu_selector(RLRubiesCodePopup::onNewCode));
    newBtn->setPosition({m_mainLayer->getContentSize().width / 2.f, 20.f});
    if (m_buttonMenu)
        m_buttonMenu->addChild(newBtn);

    this->fetchCodes();

    return true;
}

void RLRubiesCodePopup::refreshCodes() {
    if (!m_listNode)
        return;

    m_codeItems.clear();
    m_listNode->clear();

    if (m_listNode) {
        m_listNode->updateLayout();
        m_listNode->scrollToTop();
    }

    this->fetchCodes();
}

void RLRubiesCodePopup::fetchCodes() {
    if (!m_listNode)
        return;

    auto loadingSpinner = LoadingSpinner::create(50.f);
    loadingSpinner->setPosition(m_listNode->getContentSize() / 2.f);
    m_listNode->addChild(loadingSpinner);

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    auto accountId = GJAccountManager::get()->m_accountID;
    if (token.empty()) {
        Notification::create("Argon token missing", NotificationIcon::Error)->show();
        if (loadingSpinner)
            loadingSpinner->removeFromParent();
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountId;
    body["argonToken"] = token;

    Ref<RLRubiesCodePopup> self = this;
    m_fetchTask.spawn(web::WebRequest().bodyJSON(body).post(std::string(rl::BASE_API_URL) + "/getRubiesCode"),
        [self, loadingSpinner](web::WebResponse response) {
            if (!self)
                return;
            if (!response.ok()) {
                Notification::create("Failed to fetch rubies codes", NotificationIcon::Error)->show();
                log::warn("getRubiesCode request failed {}", response.code());
                if (loadingSpinner)
                    loadingSpinner->removeFromParent();
                return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("getRubiesCode response not json");
                if (loadingSpinner)
                    loadingSpinner->removeFromParent();
                return;
            }

            auto root = jsonRes.unwrap();
            auto items = std::vector<matjson::Value>{};
            if (root.isArray()) {
                items = root.asArray().unwrap();
            } else if (root.isObject() && root["codes"].isArray()) {
                items = root["codes"].asArray().unwrap();
            }

            self->m_codeItems.clear();
            int idx = 0;
            for (auto& item : items) {
                if (!item.isObject())
                    continue;
                std::string code = item["code"].asString().unwrapOrDefault();
                int reward = item["reward"].asInt().unwrapOrDefault();
                int totalRedeemed = item["redeems"].asInt().unwrapOrDefault();
                if (code.empty())
                    continue;

                ccColor4B bgColor = (idx % 2 == 0) ? ccColor4B{55, 70, 140, 255} : ccColor4B{40, 50, 100, 255};
                auto row = CCLayerColor::create(bgColor, self->m_listNode->getContentSize().width, 50.f);
                row->setAnchorPoint({0.f, 0.f});
                row->setPosition({0.f, 0.f});

                auto codeLabel = CCLabelBMFont::create(code.c_str(), "goldFont.fnt");
                codeLabel->setScale(0.8f);
                codeLabel->limitLabelWidth(200.f, 0.8f, 0.5f);

                auto codeBtn = CCMenuItemSpriteExtra::create(codeLabel, self, menu_selector(RLRubiesCodePopup::onCodeLabelClick));
                codeBtn->setTag(idx);
                codeBtn->setAnchorPoint({0.f, 0.5f});
                codeBtn->setPosition({10.f, 25.f});

                auto codeMenu = CCMenu::create(codeBtn, nullptr);
                codeMenu->setPosition({0.f, 0.f});
                codeMenu->setAnchorPoint({0.f, 0.f});
                codeMenu->setContentSize(row->getContentSize());
                row->addChild(codeMenu);

                float codeWidth = codeLabel->getContentSize().width * codeLabel->getScale();
                float iconX = 10.f + codeWidth + 10.f;

                auto rubySpr = CCSprite::createWithSpriteFrameName("RL_bigRuby.png"_spr);
                if (rubySpr) {
                    rubySpr->setScale(0.5f);
                    rubySpr->setAnchorPoint({0.f, 0.5f});
                    rubySpr->setPosition({iconX, 25.f});
                    row->addChild(rubySpr);
                    iconX += rubySpr->getContentSize().width * rubySpr->getScale() + 8.f;
                }

                auto rewardLabel = CCLabelBMFont::create(fmt::format("{}", reward).c_str(), "bigFont.fnt");
                rewardLabel->limitLabelWidth(50.f, 0.5f, 0.4f);
                rewardLabel->setAnchorPoint({0.f, 0.5f});
                rewardLabel->setPosition({iconX, 25.f});
                row->addChild(rewardLabel);
                self->m_codeItems.push_back({static_cast<int>(item["id"].asInt().unwrapOrDefault()), code, fmt::format("{}", reward)});

                auto redeemedLabel = CCLabelBMFont::create(fmt::format("Redeemed: {}", totalRedeemed).c_str(), "chatFont.fnt");
                redeemedLabel->setScale(0.5f);
                redeemedLabel->setAnchorPoint({0.5f, 0.f});
                redeemedLabel->setPosition({row->getContentWidth() / 2, 1.f});
                row->addChild(redeemedLabel);

                auto checkSpr = CCSprite::createWithSpriteFrameName("RL_check.png"_spr);
                auto crossSpr = CCSprite::createWithSpriteFrameName("RL_cross.png"_spr);
                auto checkBtn = CCMenuItemSpriteExtra::create(checkSpr, self, menu_selector(RLRubiesCodePopup::onEditCode));
                auto crossBtn = CCMenuItemSpriteExtra::create(crossSpr, self, menu_selector(RLRubiesCodePopup::onDeleteCode));
                if (checkBtn) {
                    checkBtn->setTag(idx);
                    checkBtn->setPosition({self->m_listNode->getContentSize().width - 70.f, 25.f});
                }
                if (crossBtn) {
                    crossBtn->setTag(idx);
                    crossBtn->setPosition({self->m_listNode->getContentSize().width - 28.f, 25.f});
                }

                auto actionMenu = CCMenu::create();
                actionMenu->setPosition({0.f, 0.f});
                actionMenu->setContentSize(row->getContentSize());
                actionMenu->setAnchorPoint({0.f, 0.f});
                if (checkBtn) actionMenu->addChild(checkBtn);
                if (crossBtn) actionMenu->addChild(crossBtn);
                row->addChild(actionMenu);

                if (self->m_listNode) {
                    self->m_listNode->addCell(row);
                }
                idx++;
            }

            if (idx == 0 && self->m_listNode) {
                auto noDataLabel = CCLabelBMFont::create("No rubies codes found", "goldFont.fnt");
                noDataLabel->setScale(0.45f);
                noDataLabel->setPosition(self->m_listNode->getContentSize() / 2.f);
                self->m_listNode->addChild(noDataLabel);
            }

            if (loadingSpinner)
                loadingSpinner->removeFromParent();

            if (self->m_listNode) {
                self->m_listNode->updateLayout();
            }
            self->m_listNode->getScrollLayer()->m_contentLayer->updateLayout();
            self->m_listNode->scrollToTop();
            self->m_title->setString(fmt::format("Oracle Rubies Codes ({})", idx).c_str());
        });
}

void RLRubiesCodePopup::onEditCode(CCObject* sender) {
    if (!sender)
        return;
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn)
        return;

    int idx = btn->getTag();
    if (idx < 0 || static_cast<size_t>(idx) >= m_codeItems.size())
        return;

    auto const& item = m_codeItems[idx];
    Ref<RLRubiesCodePopup> self = this;
    auto popup = RLAddCodePopup::create(item.code, item.reward, item.id, [self]() {
        if (self)
            self->refreshCodes();
    });
    if (popup)
        popup->show();
}

void RLRubiesCodePopup::onDeleteCode(CCObject* sender) {
    if (!sender)
        return;
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn)
        return;

    int idx = btn->getTag();
    if (idx < 0 || static_cast<size_t>(idx) >= m_codeItems.size())
        return;

    auto const& item = m_codeItems[idx];

    geode::createQuickPopup(
        "Delete Rubies Code",
        fmt::format("Delete code <cg>{}</c> ({} rubies)?", item.code, item.reward).c_str(),
        "Cancel",
        "Delete",
        [self = this, idx, item](auto, bool yes) {
            if (!yes)
                return;

            UploadActionPopup* upopup = UploadActionPopup::create(nullptr, "Deleting code...");
            upopup->show();

            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                upopup->showFailMessage("Argon token missing");
                return;
            }

            auto accountId = GJAccountManager::get()->m_accountID;
            if (accountId != rl::DEV_ACCOUNT_ID || accountId != rl::NOVA_ACCOUNT_ID) {
                upopup->showFailMessage("Not authorized");
                return;
            }

            matjson::Value body = matjson::Value::object();
            body["accountId"] = accountId;
            body["argonToken"] = token;
            body["id"] = item.id;

            self->m_deleteTask.spawn(
                web::WebRequest().bodyJSON(body).post(std::string(rl::BASE_API_URL) + "/deleteRubiesCode"),
                [self, upopup](web::WebResponse response) {
                    if (!self || !upopup)
                        return;

                    if (!response.ok()) {
                        upopup->showFailMessage("Failed to delete code");
                        return;
                    }

                    auto jsonRes = response.json();
                    if (!jsonRes) {
                        upopup->showFailMessage("Invalid server response");
                        return;
                    }

                    auto json = jsonRes.unwrap();
                    bool success = json["success"].asBool().unwrapOrDefault();
                    if (!success) {
                        auto msg = json["message"].asString().unwrapOrDefault();
                        if (msg.empty())
                            msg = "Failed to delete code";
                        upopup->showFailMessage(msg);
                        return;
                    }

                    upopup->showSuccessMessage("Code deleted");
                    if (self)
                        self->refreshCodes();
                });
        });
}

void RLRubiesCodePopup::onNewCode(CCObject* sender) {
    Ref<RLRubiesCodePopup> self = this;
    auto popup = RLAddCodePopup::create("", "", -1, [self]() {
        if (self)
            self->refreshCodes();
    });
    if (popup)
        popup->show();
}

void RLRubiesCodePopup::onCodeLabelClick(CCObject* sender) {
    if (!sender)
        return;
    auto btn = static_cast<CCMenuItemLabel*>(sender);
    if (!btn)
        return;

    int idx = btn->getTag();
    if (idx < 0 || static_cast<size_t>(idx) >= m_codeItems.size())
        return;

    auto const& item = m_codeItems[idx];
    utils::clipboard::write(item.code);
    Notification::create("Code copied to clipboard", NotificationIcon::Info)->show();
}