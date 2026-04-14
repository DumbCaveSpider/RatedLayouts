#include "RLNewsAnnouncementPopup.hpp"
#include "../include/RLAchievements.hpp"
#include "../include/RLConstants.hpp"
#include "Geode/utils/general.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/General.hpp>
#include <Geode/ui/MDTextArea.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

static std::string formatAnnouncementTimestamp(std::string const& ts) {
    if (ts.empty()) {
        return {};
    }

    std::string datePart = ts;
    std::string timePart;

    auto spacePos = ts.find(' ');
    if (spacePos != std::string::npos) {
        datePart = ts.substr(0, spacePos);
        timePart = ts.substr(spacePos + 1);
    }
    auto tPos = datePart.find('T');
    if (tPos != std::string::npos) {
        timePart = datePart.substr(tPos + 1);
        datePart = datePart.substr(0, tPos);
    }

    if (!timePart.empty()) {
        auto tzPos = timePart.find_first_of("Zz+-");
        if (tzPos != std::string::npos) {
            timePart = timePart.substr(0, tzPos);
        }
        auto dotPos = timePart.find('.');
        if (dotPos != std::string::npos) {
            timePart = timePart.substr(0, dotPos);
        }
    }

    auto normalizeDate = [&](std::string const& date) -> std::string {
        if (date.size() >= 10) {
            if ((date[4] == '-' || date[4] == '/') &&
                (date[7] == '-' || date[7] == '/')) {
                auto year = date.substr(0, 4);
                auto month = date.substr(5, 2);
                auto day = date.substr(8, 2);
                if (!month.empty() && !day.empty() && !year.empty()) {
                    return month + "-" + day + "-" + year;
                }
            }
            if ((date[2] == '-' || date[2] == '/') &&
                (date[5] == '-' || date[5] == '/')) {
                auto month = date.substr(0, 2);
                auto day = date.substr(3, 2);
                auto year = date.substr(6, 4);
                if (!month.empty() && !day.empty() && !year.empty()) {
                    return month + "-" + day + "-" + year;
                }
            }
        }
        return {};
    };

    auto normalizedDate = normalizeDate(datePart);
    if (normalizedDate.empty()) {
        return {};
    }

    if (timePart.empty()) {
        return normalizedDate;
    }

    if (timePart.size() > 5 && timePart[2] == ':') {
        timePart = timePart.substr(0, 5);
    }

    return normalizedDate + " " + timePart;
}

RLNewsAnnouncementPopup* RLNewsAnnouncementPopup::create() {
    auto ret = new RLNewsAnnouncementPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RLNewsAnnouncementPopup::init() {
    if (!Popup::init(500.f, 280.f, "GJ_square02.png"))
        return false;

    setTitle("Rated Layouts Announcements");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);

    auto cs = m_mainLayer->getScaledContentSize();
    CCSize listSize = {160.f, cs.height - 80.f};
    m_listNode = cue::ListNode::create(listSize, {20, 25, 55, 255}, cue::ListBorderStyle::CommentsBlue);
    m_listNode->setPosition({25.f + listSize.width / 2.f, cs.height / 2.f - 10.f});
    m_mainLayer->addChild(m_listNode, 1);

    auto contentLayer = m_listNode->getScrollLayer()->m_contentLayer;
    if (contentLayer) {
        contentLayer->setLayout(ColumnLayout::create()
                ->setGap(0.f)
                ->setAutoGrowAxis(listSize.height)
                ->setAxisAlignment(AxisAlignment::Start)
                ->setAxisReverse(true));
    }

    m_bodyText = MDTextArea::create(
        "<cy>No Announcements</c>",
        {280.f, cs.height - 60.f});
    if (m_bodyText) {
        m_bodyText->setPosition({listSize.width + (cs.width - listSize.width - 60.f) / 2.f + 40.f,
            cs.height / 2.f - 10.f});
        m_bodyText->setAnchorPoint({0.5f, 0.5f});
        m_mainLayer->addChild(m_bodyText);
    }

    if (!Mod::get()->getSettingValue<bool>("disableScrollbar")) {
        auto scrollBar = Scrollbar::create(m_listNode->getScrollLayer());
        scrollBar->setPosition({listSize.width + 30.f, cs.height / 2.f - 10.f});
        scrollBar->setScale(0.9f);
        m_mainLayer->addChild(scrollBar, 3);
    }

    Ref<RLNewsAnnouncementPopup> self = this;
    m_fetchTask.spawn(
        web::WebRequest().get(std::string(rl::BASE_API_URL) + "/getAllAnnouncement"),
        [self](web::WebResponse const& res) {
            if (!self)
                return;

            if (!res.ok()) {
                self->showError("Failed to fetch announcements");
                return;
            }

            auto jsonRes = res.json();
            if (!jsonRes) {
                self->showError("Invalid announcement response");
                return;
            }

            auto root = jsonRes.unwrap();
            std::vector<AnnouncementEntry> announcements;

            auto parseItem = [&](auto const& item) {
                if (!item.isObject()) {
                    return;
                }
                AnnouncementEntry entry;
                if (auto idRes = item["id"].template as<int>(); idRes)
                    entry.id = idRes.unwrap();
                if (auto timestampRes = item["timestamp"].asString(); timestampRes)
                    entry.timestampLabel = formatAnnouncementTimestamp(timestampRes.unwrap());
                if (entry.timestampLabel.empty()) {
                    if (auto createdAtRes = item["createdAt"].asString(); createdAtRes)
                        entry.timestampLabel = formatAnnouncementTimestamp(createdAtRes.unwrap());
                }
                if (entry.timestampLabel.empty()) {
                    if (auto dateRes = item["date"].asString(); dateRes)
                        entry.timestampLabel = formatAnnouncementTimestamp(dateRes.unwrap());
                }
                if (entry.timestampLabel.empty())
                    entry.timestampLabel = "Announcement " + numToString(static_cast<int>(announcements.size() + 1));
                if (auto titleRes = item["title"].asString(); titleRes)
                    entry.title = titleRes.unwrap();
                if (entry.title.empty()) {
                    if (auto subjectRes = item["subject"].asString(); subjectRes)
                        entry.title = subjectRes.unwrap();
                }
                if (entry.title.empty()) {
                    if (auto headingRes = item["heading"].asString(); headingRes)
                        entry.title = headingRes.unwrap();
                }
                if (entry.title.empty()) {
                    if (auto nameRes = item["name"].asString(); nameRes)
                        entry.title = nameRes.unwrap();
                }
                if (auto bodyRes = item["body"].asString(); bodyRes)
                    entry.body = bodyRes.unwrap();
                if (entry.body.empty()) {
                    if (auto msgRes = item["message"].asString(); msgRes)
                        entry.body = msgRes.unwrap();
                }
                announcements.push_back(std::move(entry));
            };

            if (root.isArray()) {
                for (auto& item : root.asArray().unwrap()) {
                    parseItem(item);
                }
            } else if (root.isObject()) {
                auto arr = root["announcements"];
                if (!arr.isArray()) {
                    arr = root["data"];
                }
                if (arr.isArray()) {
                    for (auto& item : arr.asArray().unwrap()) {
                        parseItem(item);
                    }
                }
            }

            if (announcements.empty()) {
                self->showError("No announcements available.");
                return;
            }

            self->m_announcements = std::move(announcements);
            self->m_listNode->clear();
            for (int i = 0; i < static_cast<int>(self->m_announcements.size()); ++i) {
                self->addAnnouncementItem(self->m_announcements[i], i);
            }

            self->m_listNode->updateLayout();
            self->m_listNode->getScrollLayer()->m_contentLayer->updateLayout();
            self->m_listNode->scrollToTop();

            self->selectAnnouncement(0);
        });

    return true;
}

void RLNewsAnnouncementPopup::addAnnouncementItem(AnnouncementEntry const& entry, int index) {
    auto row = CCLayer::create();
    row->setContentSize({m_listNode->getContentSize().width, 20.f});

    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->setContentSize(row->getContentSize());

    auto buttonSpr = ButtonSprite::create(entry.timestampLabel.c_str(), "goldFont.fnt", "GJ_button_05.png");
    buttonSpr->setScale(0.5f);
    auto buttonItem = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(RLNewsAnnouncementPopup::onAnnouncementSelected));
    buttonItem->setPosition({row->getContentSize().width / 2.f, row->getContentSize().height / 2.f});
    buttonItem->setTag(index);
    menu->addChild(buttonItem);
    row->addChild(menu);

    m_listNode->addCell(row);
}

void RLNewsAnnouncementPopup::onAnnouncementSelected(CCObject* sender) {
    if (!sender)
        return;
    auto buttonItem = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!buttonItem)
        return;
    selectAnnouncement(buttonItem->getTag());
}

void RLNewsAnnouncementPopup::selectAnnouncement(int index) {
    if (index < 0 || index >= static_cast<int>(m_announcements.size()))
        return;

    auto const& entry = m_announcements[index];
    if (!entry.title.empty()) {
        updateContent(entry.body, entry.title);
    } else {
        updateContent(entry.body, entry.timestampLabel);
    }
}

void RLNewsAnnouncementPopup::updateContent(std::string const& content) {
    updateContent(content, {});
}

void RLNewsAnnouncementPopup::updateContent(std::string const& content, std::string const& title) {
    if (!m_bodyText) {
        return;
    }

    if (title.empty()) {
        m_bodyText->setString(content.c_str());
        return;
    }

    std::string display = title + "\n\n" + content;
    m_bodyText->setString(display.c_str());
    RLAchievements::onReward("misc_news");
}

void RLNewsAnnouncementPopup::showError(std::string const& message) {
    if (m_bodyText) {
        m_bodyText->setString(message.c_str());
    }
}
