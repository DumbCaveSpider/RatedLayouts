#include "RLModsNotesPopup.hpp"
#include <Geode/binding/CCSpriteGrayscale.hpp>
#include "../include/RLConstants.hpp"

static std::string mapRatingToLevel(int rating) {
    switch (rating) {
        case 1:
            return "Auto";
        case 2:
            return "Easy";
        case 3:
            return "Normal";
        case 4:
        case 5:
            return "Hard";
        case 6:
        case 7:
            return "Harder";
        case 8:
        case 9:
            return "Insane";
        case 10:
            return "Easy Demon";
        case 15:
            return "Medium Demon";
        case 20:
            return "Hard Demon";
        case 25:
            return "Insane Demon";
        case 30:
            return "Extreme Demon";
        default:
            return "N/A";
    }
}

static std::string mapFeaturedToName(int featured) {
    switch (featured) {
        case 0:
            return "Base";
        case 1:
            return "Featured";
        case 2:
            return "Epic";
        case 3:
            return "Legendary";
        default:
            return "N/A";
    }
}

RLModsNotesPopup* RLModsNotesPopup::create(GJGameLevel* level) {
    RLModsNotesPopup* popup = new RLModsNotesPopup();
    if (popup) {
        popup->m_level = level;
        if (popup->init()) {
            popup->autorelease();
            return popup;
        }
    }
    delete popup;
    return nullptr;
}

bool RLModsNotesPopup::init() {
    if (!Popup::init(440.f, 290.f, "GJ_square01.png"))
        return false;

    setTitle("Rated Layouts Moderators Sent Notes");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);
    m_title->setPositionY(m_title->getPositionY() + 2.f);

    // prepare scrollable list for notes directly on main layer
    auto listWidth = m_mainLayer->getScaledContentSize().width - 40.f;
    auto listHeight = m_mainLayer->getScaledContentSize().height - 60.f;
    CCSize listSize = {listWidth, listHeight};

    m_listNode = cue::ListNode::create(listSize, {20, 25, 55, 255}, cue::ListBorderStyle::Comments);
    m_listNode->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f});
    m_mainLayer->addChild(m_listNode, 1);

    auto loadingSpinner = LoadingSpinner::create(50.f);
    loadingSpinner->setPosition(m_listNode->getContentSize() / 2.f);
    m_listNode->addChild(loadingSpinner);

    m_scrollLayer = m_listNode->getScrollLayer();

    if (!Mod::get()->getSettingValue<bool>("disableScrollbar")) {
        auto scrollBar = Scrollbar::create(m_scrollLayer);
        scrollBar->setPosition({m_mainLayer->getContentSize().width - 14.f,
            m_mainLayer->getContentSize().height / 2.f - 5.f});
        scrollBar->setScale(0.9f);
        m_mainLayer->addChild(scrollBar, 3);
    }

    auto contentLayer = m_scrollLayer->m_contentLayer;
    if (contentLayer) {
        auto layout = ColumnLayout::create();
        contentLayer->setLayout(layout);
        layout->setGap(0.f);
        layout->setAutoGrowAxis(listHeight);
        layout->setAxisAlignment(AxisAlignment::End);
        layout->setAxisReverse(true);
    }

    // fetch notes from server
    auto req = web::WebRequest();
    req.param("levelId", numToString(static_cast<int>(m_level->m_levelID)));
    Ref<RLModsNotesPopup> self = this;
    Ref<ScrollLayer> scrollRef = m_scrollLayer;
    Ref<CCNode> contentRef = m_scrollLayer->m_contentLayer;
    Ref<LoadingSpinner> spinnerRef = loadingSpinner;
    // im googing with ref<> rn

    m_getNotesTask.spawn(
        req.get(std::string(rl::BASE_API_URL) + "/getNotes"),
        [self, scrollRef, contentRef, spinnerRef](web::WebResponse response) {
            if (!self)
                return;
            log::info("received notes response");

            if (!scrollRef || !spinnerRef) {
                log::warn("ui elements vanished before response");
                return;
            }

            if (!response.ok()) {
                Notification::create("Failed to fetch mod notes",
                    NotificationIcon::Error)
                    ->show();
                log::warn("notes request failed {}", response.code());
                return;
            }
            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("notes response not json");
                return;
            }
            auto root = jsonRes.unwrap();
            log::debug("notes root: {}", root.dump());
            if (!root.isObject())
                return;
            auto notesVal = root["notes"];
            if (!notesVal.isArray())
                return;
            auto arr = notesVal.asArray().unwrap();
            log::debug("notes array length {}", arr.size());
            int idx = 0;
            for (auto& val : arr) {
                if (!val.isObject())
                    continue;
                std::string user = val["username"].asString().unwrapOrDefault();
                std::string time = val["createdAt"].asString().unwrapOrDefault();
                std::string note = val["note"].asString().unwrapOrDefault();
                int difficulty = val["difficulty"].asInt().unwrapOrDefault();
                int featured = val["featured"].asInt().unwrapOrDefault();
                bool isRejected = val["isRejected"].asBool().unwrapOrDefault();

                // username label
                auto usernameLabel =
                    CCLabelBMFont::create(user.c_str(), "goldFont.fnt");
                auto timeLabel = CCLabelBMFont::create(time.c_str(), "chatFont.fnt");
                float labelHeight = usernameLabel->getContentSize().height;
                const float areaHeight = 40.f;
                float totalH = labelHeight + areaHeight + 8.f;

                // alternate background color
                ccColor4B bgColor = (idx % 2 == 0) ? ccColor4B{55, 70, 140, 255} : ccColor4B{40, 50, 100, 255};
                auto layer = CCLayerColor::create(bgColor, scrollRef->getContentSize().width, totalH);

                usernameLabel->setAnchorPoint({0.f, .5f});
                usernameLabel->setScale(.6f);
                usernameLabel->setPosition({15.f, totalH - 15.f});
                layer->addChild(usernameLabel);

                timeLabel->setAnchorPoint({1.f, .5f});
                timeLabel->setScale(.5f);
                timeLabel->setPosition(
                    {layer->getContentWidth() - 10.f, totalH - 15.f});
                layer->addChild(timeLabel);

                // bg thingy
                auto noteBg = NineSlice::create("square02b_001.png");
                noteBg->setPosition(
                    {layer->getContentSize().width / 2, totalH / 2 - 10});
                noteBg->setContentSize(
                    {layer->getContentSize().width - 20.f, areaHeight});
                noteBg->setColor({0, 0, 0});
                noteBg->setOpacity(75);
                layer->addChild(noteBg);
                // note label
                self->m_notes.push_back(note);
                auto noteDisplay = note;
                if (noteDisplay.size() > 80) {
                    noteDisplay = noteDisplay.substr(0, 80) + "...";
                }

                auto noteLabel = CCLabelBMFont::create(noteDisplay.c_str(), "chatFont.fnt");
                noteLabel->limitLabelWidth(noteBg->getContentSize().width - 20.f, 1.f, .1f);

                auto noteButton = CCMenuItemSpriteExtra::create(noteLabel, self, menu_selector(RLModsNotesPopup::onNoteLabelClick));
                noteButton->setTag(idx);
                noteButton->setEnabled(false);
                noteButton->m_scaleMultiplier = 1.05f;
                noteButton->setPosition({noteBg->getContentSize().width / 2, noteBg->getContentSize().height / 2});

                if (note.size() > 80) {
                    auto infoSpr = CCSpriteGrayscale::createWithSpriteFrameName("RL_info01.png"_spr);
                    infoSpr->setScale(0.3f);
                    infoSpr->setOpacity(150);
                    infoSpr->setAnchorPoint({1.f, 1.f});
                    infoSpr->setPosition({noteBg->getContentSize().width - 5.f, noteBg->getContentSize().height - 5.f});
                    noteButton->setEnabled(true);
                    noteBg->addChild(infoSpr);
                }

                auto noteMenu = CCMenu::create(noteButton, nullptr);
                noteMenu->setPosition({0, 0});
                noteMenu->setContentSize(noteBg->getContentSize());
                noteMenu->setAnchorPoint({0, 0});
                noteBg->addChild(noteMenu);

                auto difficultyLabel = CCLabelBMFont::create(
                    fmt::format("{} ({})", mapRatingToLevel(difficulty), mapFeaturedToName(featured))
                        .c_str(),
                    "bigFont.fnt");
                difficultyLabel->setPosition(
                    {layer->getContentSize().width / 2, totalH - 15.f});
                difficultyLabel->setScale(.3f);
                // if rejected, say rejected lol
                switch (featured) {
                    case 1:
                        difficultyLabel->setColor({255, 255, 50});
                        break;
                    case 2:
                        difficultyLabel->setColor({50, 255, 255});
                        break;
                    case 3:
                        difficultyLabel->setColor({255, 50, 255});
                        break;
                    default:
                        break;
                }
                if (isRejected) {
                    difficultyLabel->setString("Rejected");
                    difficultyLabel->setColor({255, 50, 50});
                }
                layer->addChild(difficultyLabel);

                if (self->m_listNode) {
                    self->m_listNode->addCell(layer);
                }
                if (spinnerRef) {
                    spinnerRef->removeFromParent();
                }
                idx++;
            }
            if (idx == 0) {
                auto noNotesLabel =
                    CCLabelBMFont::create("No notes from moderators", "goldFont.fnt");
                noNotesLabel->setPosition(self->m_listNode->getContentSize() / 2.f);
                noNotesLabel->setScale(0.6f);
                self->m_listNode->addChild(noNotesLabel);
                if (spinnerRef) {
                    spinnerRef->removeFromParent();
                }
            }
            if (contentRef) {
                contentRef->updateLayout();
                scrollRef->scrollToTop();
            }
        });

    return true;
}

void RLModsNotesPopup::onNoteLabelClick(CCObject* sender) {
    auto button = static_cast<CCMenuItemLabel*>(sender);
    if (!button)
        return;

    int idx = button->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_notes.size()))
        return;

    auto note = m_notes[idx];
    MDPopup::create("Moderator Note", note.c_str(), "OK")->show();
}
