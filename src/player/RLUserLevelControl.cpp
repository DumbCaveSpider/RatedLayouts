#include "RLUserLevelControl.hpp"
#include "../include/RLNetworkUtils.hpp"
#include "../include/RLConstants.hpp"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Popup.hpp"
#include <cue/ListNode.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/Geode.hpp>
#include <fmt/format.h>

using namespace geode::prelude;

RLUserLevelControl* RLUserLevelControl::create(int accountId) {
    auto ret = new RLUserLevelControl();
    ret->m_targetId = accountId;

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
};

bool RLUserLevelControl::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square02.png"))
        return false;
    setTitle("Rated Layouts User Level Mod Panel");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);
    m_noElasticity = true;

    // show target user's name in title if possible
    std::string username =
        fmt::format("Target: {}",
            GameLevelManager::sharedState()->tryGetUsername(m_targetId));
    if (!username.empty()) {
        m_usernameLabel = CCLabelBMFont::create(username.c_str(), "bigFont.fnt");
        m_usernameLabel->setPosition(m_title->getPositionX(),
            m_title->getPositionY() - 20);
        m_usernameLabel->limitLabelWidth(m_mainLayer->getContentWidth(), .5f, 0.3f);
        m_usernameLabel->setString(username.c_str());
        m_mainLayer->addChild(m_usernameLabel, 3);
    }

    auto cs = m_mainLayer->getContentSize();

    auto listWidth = cs.width - 80.f;
    auto listHeight = 200.f;

    m_listNode = cue::ListNode::create({listWidth, listHeight}, {191, 114, 62, 255}, cue::ListBorderStyle::CommentsBlue);
    m_listNode->setAnchorPoint({0.5f, 0.5f});
    m_listNode->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f - 15.f});
    m_listNode->getScrollLayer()->m_contentLayer->setLayout(ColumnLayout::create()
            ->setGap(0.f)
            ->setAutoGrowAxis(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End));

    m_mainLayer->addChild(m_listNode, 2);

    m_listSpinner = LoadingSpinner::create(48.f);
    if (m_listSpinner) {
        m_listSpinner->setPosition(m_listNode->getPosition());
        m_listSpinner->setVisible(false);
        m_mainLayer->addChild(m_listSpinner, 5);
    }

    // paging arrows
    auto prevSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    m_prevPageButton = CCMenuItemSpriteExtra::create(
        prevSprite, this, menu_selector(RLUserLevelControl::onPrevPage));
    m_prevPageButton->setPosition({20, m_mainLayer->getContentHeight() / 2.f - 5.f});
    m_buttonMenu->addChild(m_prevPageButton);

    auto nextSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    nextSprite->setFlipX(true);
    m_nextPageButton = CCMenuItemSpriteExtra::create(
        nextSprite, this, menu_selector(RLUserLevelControl::onNextPage));
    m_nextPageButton->setPosition({cs.width - 20, m_mainLayer->getContentHeight() / 2.f - 5.f});
    m_buttonMenu->addChild(m_nextPageButton);

    m_prevPageButton->setVisible(false);
    m_nextPageButton->setVisible(false);

    // toggle button to show platformer completions
    auto platIcon =
        CCSprite::createWithSpriteFrameName("RL_planetBig.png"_spr);
    auto platOffSpr = EditorButtonSprite::create(
        platIcon, EditorBaseColor::Gray, EditorBaseSize::Normal);
    auto platOnSpr = EditorButtonSprite::create(
        platIcon, EditorBaseColor::Cyan, EditorBaseSize::Normal);
    m_togglePlatButton = CCMenuItemSpriteExtra::create(
        platOffSpr, this, menu_selector(RLUserLevelControl::onPlanetFilter));
    if (m_togglePlatButton) {
        m_togglePlatButton->setPosition({cs.width - 22, m_mainLayer->getContentHeight() - 80});
        m_buttonMenu->addChild(m_togglePlatButton);
    }

    m_filterPlat = false;

    fetchCompletionList(0);

    return true;
}

void RLUserLevelControl::removeLevel(int levelId) {
    if (levelId <= 0)
        return;

    createQuickPopup(
        "Remove Completion",
        fmt::format("<cr>Remove</c> <cc>{}</c> from <cg>{}'s completed levels</c>?", levelId, GameLevelManager::sharedState()->tryGetUsername(m_targetId)),
        "Cancel",
        "Remove",
        [this, levelId](auto, bool yes) {
            if (!yes) return;

            auto upopup = UploadActionPopup::create(nullptr, "Removing level completion...");
            upopup->show();

            auto token = Mod::get()->getSavedValue<std::string>("argon_token");
            if (token.empty()) {
                upopup->showFailMessage("Authentication token not found");
                return;
            }

            matjson::Value body = matjson::Value::object();
            body["accountId"] = GJAccountManager::get()->m_accountID;
            body["argonToken"] = token;
            body["targetAccountId"] = m_targetId;
            body["targetLevelId"] = levelId;

            auto req = web::WebRequest();
            req.bodyJSON(body);
            Ref<RLUserLevelControl> self = this;
            self->m_removeLevelTask.spawn(
                req.post(std::string(rl::BASE_API_URL) + "/deleteCompletionLevel"),
                [self, upopup](web::WebResponse res) {
                    if (!self || !upopup)
                        return;

                    if (!res.ok()) {
                        upopup->showFailMessage(
                            rl::getResponseFailMessage(res, "Failed to remove level"));
                        return;
                    }

                    auto jsonRes = res.json();
                    if (!jsonRes) {
                        upopup->showFailMessage("Invalid server response");
                        return;
                    }

                    auto json = jsonRes.unwrap();
                    bool success = json["success"].asBool().unwrapOrDefault();
                    if (success) {
                        upopup->showSuccessMessage("Level removed!");
                        if (self) {
                            self->fetchCompletionList(self->m_page);
                        }
                    } else {
                        upopup->showFailMessage("Failed to remove level");
                    }
                });
        });
}

void RLUserLevelControl::onRemoveLevel(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!item)
        return;

    int levelId = item->getTag();
    removeLevel(levelId);
}

void RLUserLevelControl::fetchCompletionList(int page) {
    m_page = page;
    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        Notification::create("Cannot get completed levels: auth missing", NotificationIcon::Warning)->show();
        return;
    }

    if (m_listNode)
        m_listNode->clear();
    if (m_listSpinner)
        m_listSpinner->setVisible(true);

    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = token;
    body["targetAccountId"] = m_targetId;
    body["page"] = page;
    body["isPlat"] = m_filterPlat;

    auto req = web::WebRequest();
    req.bodyJSON(body);

    Ref<RLUserLevelControl> self = this;
    self->m_getCompletionTask.spawn(
        req.post(std::string(rl::BASE_API_URL) + "/getCompletionList"),
        [self](web::WebResponse res) {
            if (!self)
                return;
            if (!res.ok()) {
                log::warn("getCompletionList returned non-ok status: {}", res.code());
                Notification::create("Failed to fetch completion list", NotificationIcon::Error)->show();
                self->updateCompletionPaging(0);
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                Notification::create("Invalid completion list response", NotificationIcon::Error)->show();
                self->updateCompletionPaging(0);
                return;
            }

            auto json = jsonRes.unwrap();
            if (!json.contains("completed") || json["completed"].type() != matjson::Type::Array) {
                if (self->m_listNode)
                    self->m_listNode->clear();
                self->updateCompletionPaging(0);
                return;
            }

            std::vector<int> completedIds;
            for (auto v : json["completed"]) {
                if (auto id = v.as<int>(); id) {
                    completedIds.push_back(id.unwrap());
                }
            }

            if (!completedIds.empty()) {
                std::string levelIds;
                bool first = true;
                for (auto id : completedIds) {
                    if (!first)
                        levelIds += ",";
                    levelIds += numToString(id);
                    first = false;
                }
                auto glm = GameLevelManager::get();
                if (glm) {
                    glm->m_levelManagerDelegate = self;
                    auto searchObj = GJSearchObject::create(SearchType::Type19, levelIds.c_str());
                    glm->getOnlineLevels(searchObj);
                }
            } else {
                if (self) {
                    self->populateCompletionLevels(nullptr);
                }
            }
        });
}

void RLUserLevelControl::populateCompletionLevels(cocos2d::CCArray* levels) {
    if (!m_listNode)
        return;

    m_listNode->clear();
    if (m_emptyLabel) {
        m_emptyLabel->removeFromParent();
        m_emptyLabel = nullptr;
    }

    if (!levels || levels->count() == 0) {
        m_emptyLabel = CCLabelBMFont::create("No completion levels returned", "goldFont.fnt");
        if (m_emptyLabel) {
            m_emptyLabel->setAnchorPoint({0.5f, 0.5f});
            m_emptyLabel->setPosition({m_listNode->getContentSize().width / 2.f,
                m_listNode->getContentSize().height / 2.f});
                m_emptyLabel->limitLabelWidth(m_listNode->getContentSize().width - 20, .7f, .5f);
            m_listNode->addChild(m_emptyLabel, 3);
        }
        if (m_listSpinner) {
            m_listSpinner->setVisible(false);
        }

        updateCompletionPaging(0);
        return;
    }

    int index = 0;
    for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
        if (!level)
            continue;

        float cellH = 50.f;
        auto cell = new LevelCell("RLLevelCell", m_listNode->getContentSize().width, cellH);
        cell->autorelease();
        cell->m_compactView = true;
        cell->loadFromLevel(level);
        cell->setContentSize({m_listNode->getContentSize().width, cellH});
        cell->setAnchorPoint({0.f, 1.f});
        cell->updateBGColor(index);
        m_listNode->addCell(cell);

        if (cell->m_mainMenu) {
            // disable the view button and use its spot for the remove button
            auto viewBtn = cell->m_mainLayer->getChildByIDRecursive("view-button");
            CCPoint removePos = viewBtn->getPosition();
            if (viewBtn) {
                removePos = viewBtn->getPosition();
                viewBtn->removeFromParent();
            }

            auto removeBtnSpr = CCSprite::createWithSpriteFrameName("RL_cross.png"_spr);
            auto removeBtn = CCMenuItemSpriteExtra::create(
                removeBtnSpr, this, menu_selector(RLUserLevelControl::onRemoveLevel));
            if (removeBtn) {
                removeBtn->setTag(level->m_levelID);
                removeBtn->setPosition(removePos);
                cell->m_mainMenu->addChild(removeBtn);
            }
        }
        index++;
    }

    if (m_listSpinner)
        m_listSpinner->setVisible(false);

    m_listNode->getScrollLayer()->m_contentLayer->updateLayout();
    m_listNode->scrollToTop();

    updateCompletionPaging(index);
}

void RLUserLevelControl::updateCompletionPaging(int resultCount) {
    if (m_prevPageButton)
        m_prevPageButton->setVisible(m_page > 0);
    if (m_nextPageButton)
        m_nextPageButton->setVisible(resultCount >= m_perPage);
}

void RLUserLevelControl::loadLevelsFinished(cocos2d::CCArray* levels, char const* key) {
    (void)key;
    populateCompletionLevels(levels);
}

void RLUserLevelControl::loadLevelsFailed(char const* key) {
    (void)key;
    if (m_listNode)
        m_listNode->clear();
    updateCompletionPaging(0);
}

void RLUserLevelControl::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) {
    (void)type;
    loadLevelsFinished(levels, key);
}

void RLUserLevelControl::loadLevelsFailed(char const* key, int type) {
    (void)type;
    loadLevelsFailed(key);
}

void RLUserLevelControl::onPlanetFilter(CCObject* sender) {
    (void)sender;

    m_filterPlat = !m_filterPlat;

    if (m_togglePlatButton) {
        auto icon = CCSprite::createWithSpriteFrameName("RL_planetBig.png"_spr);
        auto sprite = EditorButtonSprite::create(
            icon, m_filterPlat ? EditorBaseColor::Cyan : EditorBaseColor::Gray, EditorBaseSize::Normal);
        m_togglePlatButton->setNormalImage(sprite);
    }

    fetchCompletionList(0);
}

void RLUserLevelControl::onPrevPage(CCObject* sender) {
    if (m_page <= 0)
        return;
    fetchCompletionList(m_page - 1);
}

void RLUserLevelControl::onNextPage(CCObject* sender) {
    fetchCompletionList(m_page + 1);
}
