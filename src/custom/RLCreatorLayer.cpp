#include "RLCreatorLayer.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/async.hpp>
#include <cue/RepeatingBackground.hpp>
#include <optional>

#include "../custom/RLAchievements.hpp"
#include "../custom/RLAchievementsPopup.hpp"
#include "../level/RLEventLayouts.hpp"
#include "../level/RLNotificationOverlay.hpp"
#include "../level/RLSelectSends.hpp"
#include "RLAddDialogue.hpp"
#include "RLAnnouncementPopup.hpp"
#include "RLCreditsPopup.hpp"
#include "RLDonationPopup.hpp"
#include "RLGauntletSelectLayer.hpp"
#include "RLLeaderboardLayer.hpp"
#include "RLLevelBrowserLayer.hpp"
#include "RLSearchLayer.hpp"
#include "ccTypes.h"

struct ModInfo {
  std::string message;
  std::string status;
  std::string serverVersion;
  std::string modVersion;
};

static arc::Future<std::optional<ModInfo>> fetchModInfoAsync() {
  log::debug("Fetching mod info from API");
  co_return co_await []() -> arc::Future<std::optional<ModInfo>> {
    auto req = web::WebRequest();
    auto response = co_await req.get("https://gdrate.arcticwoof.xyz/v1/");
    if (!response.ok()) {
      log::warn("Failed to fetch mod info from server");
      co_return std::nullopt;
    }

    auto jsonRes = response.json();
    if (!jsonRes) {
      log::warn("Failed to parse mod info JSON");
      co_return std::nullopt;
    }

    auto json = jsonRes.unwrap();
    ModInfo info;

    if (json.contains("message")) {
      if (auto m = json["message"].asString(); m)
        info.message = m.unwrap();
    }
    if (json.contains("status")) {
      if (auto s = json["status"].asString(); s)
        info.status = s.unwrap();
    }
    if (json.contains("serverVersion")) {
      if (auto sv = json["serverVersion"].asString(); sv)
        info.serverVersion = sv.unwrap();
    }
    if (json.contains("modVersion")) {
      if (auto mv = json["modVersion"].asString(); mv)
        info.modVersion = mv.unwrap();
    }

    log::debug("ModInfo fetched: status={}, serverVersion={}, modVersion={}, "
               "message={}",
               info.status, info.serverVersion, info.modVersion, info.message);
    co_return info;
  }();
}

using namespace geode::prelude;

bool RLCreatorLayer::init() {
  if (!CCLayer::init())
    return false;

  // quick achievements for custom bg
  if (Mod::get()->getSettingValue<int>("backgroundType") != 1 ||
      Mod::get()->getSettingValue<int>("floorType") != 1) {
    RLAchievements::onReward("misc_custom_bg");
  }

  auto winSize = CCDirector::sharedDirector()->getWinSize();

  // add the new Notification Overlay to OverlayManager
  if (auto overlayMgr = OverlayManager::get()) {
    if (auto noti = RLNotificationOverlay::create()) {
      if (!overlayMgr->getChildByID("rl-notification-overlay")) {
        noti->setID("rl-notification-overlay");
        overlayMgr->addChild(noti, 10);
      }
    }
  }

  // create if moving bg disabled
  if (Mod::get()->getSettingValue<bool>("disableBackground") == true) {
    auto bg = createLayerBG();
    bg->setColor(
        Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground"));
    addChild(bg, -1);
  } else {
    auto value = Mod::get()->getSettingValue<int>("backgroundType");
    std::string bgIndex = (value >= 1 && value <= 9)
                              ? ("0" + numToString(value))
                              : numToString(value);
    std::string bgName = "game_bg_" + bgIndex + "_001.png";
    auto bg = cue::RepeatingBackground::create(bgName.c_str(), 1.f,
                                               cue::RepeatMode::X);
    bg->setColor(
        Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground"));
    addChild(bg, -1);
  }

  addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

  auto backMenu = CCMenu::create();
  backMenu->setPosition({0, 0});

  auto backButtonSpr =
      CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
  CCMenuItemSpriteExtra *backButton = CCMenuItemSpriteExtra::create(
      backButtonSpr, this, menu_selector(RLCreatorLayer::onBackButton));
  backButton->setPosition({25, winSize.height - 25});
  backMenu->addChild(backButton);

  auto modSettingsBtnSprite = CircleButtonSprite::createWithSpriteFrameName(
      // @geode-ignore(unknown-resource)
      "geode.loader/settings.png", 1.f, CircleBaseColor::Blue,
      CircleBaseSize::Medium);
  modSettingsBtnSprite->setScale(0.75f);
  auto settingsButton = CCMenuItemSpriteExtra::create(
      modSettingsBtnSprite, this,
      menu_selector(RLCreatorLayer::onSettingsButton));
  settingsButton->setPosition({winSize.width - 25, winSize.height - 25});
  backMenu->addChild(settingsButton);
  this->addChild(backMenu);

  auto mainMenu = CCMenu::create();
  mainMenu->setPosition({winSize.width / 2, winSize.height / 2 - 10});
  mainMenu->setContentSize({400.f, 240.f});
  mainMenu->setLayout(RowLayout::create()
                          ->setGap(10.f)
                          ->setGrowCrossAxis(true)
                          ->setCrossAxisOverflow(false));

  this->addChild(mainMenu);

  auto title = CCSprite::createWithSpriteFrameName("RL_title.png"_spr);
  title->setPosition({winSize.width / 2, winSize.height / 2 + 120});
  this->addChild(title);

  auto featuredSpr =
      CCSprite::createWithSpriteFrameName("RL_featured01.png"_spr);
  auto featuredItem = CCMenuItemSpriteExtra::create(
      featuredSpr, this, menu_selector(RLCreatorLayer::onFeaturedLayouts));
  featuredItem->setID("featured-button");
  mainMenu->addChild(featuredItem);

  auto leaderboardSpr =
      CCSprite::createWithSpriteFrameName("RL_leaderboard01.png"_spr);
  auto leaderboardItem = CCMenuItemSpriteExtra::create(
      leaderboardSpr, this, menu_selector(RLCreatorLayer::onLeaderboard));
  leaderboardItem->setID("leaderboard-button");
  mainMenu->addChild(leaderboardItem);

  // gauntlet coming soon
  auto gauntletSpr =
      CCSprite::createWithSpriteFrameName("RL_gauntlets01.png"_spr);
  auto gauntletItem = CCMenuItemSpriteExtra::create(
      gauntletSpr, this, menu_selector(RLCreatorLayer::onLayoutGauntlets));
  gauntletItem->setID("gauntlet-button");
  mainMenu->addChild(gauntletItem);

  auto sentSpr = CCSprite::createWithSpriteFrameName("RL_sent01.png"_spr);
  auto sentItem = CCMenuItemSpriteExtra::create(
      sentSpr, this, menu_selector(RLCreatorLayer::onSentLayouts));
  sentItem->setID("sent-layouts-button");
  mainMenu->addChild(sentItem);

  auto searchSpr = CCSprite::createWithSpriteFrameName("RL_search01.png"_spr);
  auto searchItem = CCMenuItemSpriteExtra::create(
      searchSpr, this, menu_selector(RLCreatorLayer::onSearchLayouts));
  searchItem->setID("search-layouts-button");
  mainMenu->addChild(searchItem);

  auto dailySpr = CCSprite::createWithSpriteFrameName("RL_daily01.png"_spr);
  auto dailyItem = CCMenuItemSpriteExtra::create(
      dailySpr, this, menu_selector(RLCreatorLayer::onDailyLayouts));
  dailyItem->setID("daily-layouts-button");
  mainMenu->addChild(dailyItem);

  auto weeklySpr = CCSprite::createWithSpriteFrameName("RL_weekly01.png"_spr);
  auto weeklyItem = CCMenuItemSpriteExtra::create(
      weeklySpr, this, menu_selector(RLCreatorLayer::onWeeklyLayouts));
  weeklyItem->setID("weekly-layouts-button");
  mainMenu->addChild(weeklyItem);

  auto monthlySpr = CCSprite::createWithSpriteFrameName("RL_monthly01.png"_spr);
  auto monthlyItem = CCMenuItemSpriteExtra::create(
      monthlySpr, this, menu_selector(RLCreatorLayer::onMonthlyLayouts));
  monthlyItem->setID("monthly-layouts-button");
  mainMenu->addChild(monthlyItem);

  CCSprite *unknownSpr =
      CCSpriteGrayscale::createWithSpriteFrameName("RL_unknownBtn.png"_spr);
  auto unknownItem = CCMenuItemSpriteExtra::create(
      unknownSpr, this, menu_selector(RLCreatorLayer::onUnknownButton));
  unknownItem->setID("unknown-button");
  mainMenu->addChild(unknownItem);

  mainMenu->updateLayout();

  // button menu
  auto infoMenu = CCMenu::create();
  infoMenu->setPosition({0, 0});

  // info button
  auto infoButtonSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoButtonSpr->setScale(0.7f);
  auto infoButton = CCMenuItemSpriteExtra::create(
      infoButtonSpr, this, menu_selector(RLCreatorLayer::onInfoButton));
  infoButton->setPosition({25, 25});
  infoMenu->addChild(infoButton);

  // discord thingy
  auto discordIconSpr =
      CCSprite::createWithSpriteFrameName("RL_discord01.png"_spr);
  discordIconSpr->setScale(0.7f);
  auto discordIconBtn = CCMenuItemSpriteExtra::create(
      discordIconSpr, this, menu_selector(RLCreatorLayer::onDiscordButton));
  discordIconBtn->setPosition(
      {infoButton->getPositionX(), infoButton->getPositionY() + 40});
  infoMenu->addChild(discordIconBtn);

  // news button above discord
  // @geode-ignore(unknown-resource)
  auto annouceSpr = CCSprite::createWithSpriteFrameName("RL_news01.png"_spr);
  annouceSpr->setScale(0.7f);
  auto annouceBtn = CCMenuItemSpriteExtra::create(
      annouceSpr, this, menu_selector(RLCreatorLayer::onAnnoucementButton));
  annouceBtn->setPosition(
      {infoButton->getPositionX(), infoButton->getPositionY() + 80});
  infoMenu->addChild(annouceBtn);
  m_newsIconBtn = annouceBtn;

  auto achievementSpr =
      CCSprite::createWithSpriteFrameName("RL_achievements01.png"_spr);
  achievementSpr->setScale(0.7f);
  auto achievementItem = CCMenuItemSpriteExtra::create(
      achievementSpr, this,
      menu_selector(RLCreatorLayer::onAchievementsButton));
  achievementItem->setID("achievements-button");
  achievementItem->setPosition(
      {infoButton->getPositionX(), infoButton->getPositionY() + 120});
  infoMenu->addChild(achievementItem);

  // news button above discord
  // @geode-ignore(unknown-resource)
  auto browserSpr = CCSprite::createWithSpriteFrameName("RL_browser01.png"_spr);
  browserSpr->setScale(0.7f);
  auto browserBtn = CCMenuItemSpriteExtra::create(
      browserSpr, this, menu_selector(RLCreatorLayer::onBrowserButton));
  browserBtn->setPosition(
      {infoButton->getPositionX(), infoButton->getPositionY() + 160});
  infoMenu->addChild(browserBtn);
  m_newsIconBtn = browserBtn;

  // @geode-ignore(unknown-resource)
  auto badgeSpr =
      CCSprite::createWithSpriteFrameName("geode.loader/updates-failed.png");
  if (badgeSpr) {
    // position top-right of the icon
    auto size = annouceSpr->getContentSize();
    badgeSpr->setScale(0.5f);
    badgeSpr->setPosition({30, 30});
    badgeSpr->setVisible(false);
    annouceBtn->addChild(badgeSpr, 10);
    m_newsBadge = badgeSpr;
  }

  // check server announcement id and set badge visibility
  Ref<RLCreatorLayer> self = this;
  m_announcementTask.spawn(
      web::WebRequest().get("https://gdrate.arcticwoof.xyz/getAnnoucement"),
      [self](web::WebResponse const &res) {
        if (!self)
          return;
        if (!res.ok())
          return;
        auto jsonRes = res.json();
        if (!jsonRes)
          return;
        auto json = jsonRes.unwrap();
        int id = 0;
        if (json.contains("id")) {
          if (auto i = json["id"].as<int>(); i)
            id = i.unwrap();
        }
        int saved = Mod::get()->getSavedValue<int>("annoucementId");
        if (id && id != saved) {
          if (self->m_newsBadge)
            self->m_newsBadge->setVisible(true);
        } else {
          if (self->m_newsBadge)
            self->m_newsBadge->setVisible(false);
        }
      });

  this->addChild(infoMenu);

  // credits button at the bottom right
  auto creditButtonSpr =
      CCSprite::createWithSpriteFrameName("RL_credits01.png"_spr);
  creditButtonSpr->setScale(0.7f);
  auto creditButton = CCMenuItemSpriteExtra::create(
      creditButtonSpr, this, menu_selector(RLCreatorLayer::onCreditsButton));
  creditButton->setPosition({winSize.width - 25, 25});
  infoMenu->addChild(creditButton);

  // supporter button left side of the credits
  auto supportButtonSpr =
      CCSprite::createWithSpriteFrameName("RL_support01.png"_spr);
  supportButtonSpr->setScale(0.7f);
  auto supportButton = CCMenuItemSpriteExtra::create(
      supportButtonSpr, this, menu_selector(RLCreatorLayer::onSupporterButton));
  supportButton->setPosition(
      {creditButton->getPositionX(), creditButton->getPositionY() + 40});
  infoMenu->addChild(supportButton);

  // demonlist
  auto demonListSpr =
      CCSpriteGrayscale::createWithSpriteFrameName("RL_demonList01.png"_spr);
  demonListSpr->setScale(0.7f);
  auto demonListBtn = CCMenuItemSpriteExtra::create(
      demonListSpr, this, menu_selector(RLCreatorLayer::onDemonListButton));
  demonListBtn->setPosition(
      {creditButton->getPositionX(), creditButton->getPositionY() + 80});
  infoMenu->addChild(demonListBtn);

  // shop
  auto shopSpr =
      CCSpriteGrayscale::createWithSpriteFrameName("RL_shop01.png"_spr);
  shopSpr->setScale(0.7f);
  auto shopBtn = CCMenuItemSpriteExtra::create(
      shopSpr, this, menu_selector(RLCreatorLayer::onShopButton));
  shopBtn->setPosition(
      {creditButton->getPositionX(), creditButton->getPositionY() + 120});
  infoMenu->addChild(shopBtn);

  // button bob
  if (Mod::get()->getSavedValue<int>("role") >= 1) {
    auto addDiagloueBtnSpr =
        CCSprite::createWithSpriteFrameName("RL_bobBtn01.png"_spr);
    addDiagloueBtnSpr->setScale(0.7f);
    auto addDialogueBtn = CCMenuItemSpriteExtra::create(
        addDiagloueBtnSpr, this,
        menu_selector(RLCreatorLayer::onSecretDialogueButton));
    addDialogueBtn->setPosition(
        {creditButton->getPositionX(), creditButton->getPositionY() + 160});
    infoMenu->addChild(addDialogueBtn);
  }

  if (!Mod::get()->getSettingValue<bool>("disableModInfo")) {
    // mod info stuff
    auto modInfoBg = CCScale9Sprite::create("square02_small.png");
    modInfoBg->setPosition({winSize.width / 2, 0});
    modInfoBg->setContentSize({160.f, 70.f});
    modInfoBg->setOpacity(100);

    m_modStatusLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_modStatusLabel->setColor({255, 150, 0});
    m_modStatusLabel->setScale(0.3f);
    m_modStatusLabel->setPosition({80.f, 60.f});
    modInfoBg->addChild(m_modStatusLabel);

    std::string modVersionStr = Mod::get()->getVersion().toVString();
    m_modVersionLabel =
        CCLabelBMFont::create(modVersionStr.c_str(), "bigFont.fnt");
    m_modVersionLabel->setColor({255, 150, 0});
    m_modVersionLabel->setScale(0.3f);
    m_modVersionLabel->setPosition({80.f, 45.f});
    modInfoBg->addChild(m_modVersionLabel);

    if (m_modStatusLabel) {
      m_modStatusLabel->setString("Checking...");
      m_modStatusLabel->setColor({255, 150, 0});
    }

    this->addChild(modInfoBg, 10);
  }

  this->scheduleUpdate();
  this->setKeypadEnabled(true);
  return true;
}

void RLCreatorLayer::onSettingsButton(CCObject *sender) {
  openSettingsPopup(getMod());
}

void RLCreatorLayer::onDiscordButton(CCObject *sender) {
  utils::web::openLinkInBrowser("https://discord.gg/jBf2wfBgVT");
  RLAchievements::onReward("misc_discord");
}

void RLCreatorLayer::onBrowserButton(CCObject *sender) {
  createQuickPopup("Rated Layouts Browser",
                   "You will be redirected to the <cl>Rated Layouts Browser "
                   "website</c> in your web browser.\n<cy>Continue?</c>",
                   "No", "Yes", [](auto, bool yes) {
                     if (!yes)
                       return;
                     Notification::create(
                         "Opening Rated Layouts Browser in your web browser",
                         NotificationIcon::Info)
                         ->show();
                     utils::web::openLinkInBrowser(
                         "https://ratedlayouts.arcticwoof.xyz");
                     RLAchievements::onReward("misc_browser");
                   });
}

void RLCreatorLayer::onDemonListButton(CCObject *sender) {
  DialogObject *dialogObj = DialogObject::create(
      "Layout Creator", "The <co>Rated Layouts Demonlist</c> isn't done yet...",
      28, 1.f, false, ccWHITE);
  if (dialogObj) {
    auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
    dialog->addToMainScene();
    dialog->animateInRandomSide();
  }
}

void RLCreatorLayer::onLayoutGauntlets(CCObject *sender) {
  auto gauntletSelect = RLGauntletSelectLayer::create();
  auto scene = CCScene::create();
  scene->addChild(gauntletSelect);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onSupporterButton(CCObject *sender) {
  auto donationPopup = RLDonationPopup::create();
  donationPopup->show();
}

void RLCreatorLayer::onSecretDialogueButton(CCObject *sender) {
  if (Mod::get()->getSavedValue<int>("role") < 1) {
    return;
  }
  auto dialogue = RLAddDialogue::create();
  dialogue->show();
}

void RLCreatorLayer::onShopButton(CCObject *sender) {
  auto obj1 = DialogObject::create("Layout Creator",
                                   "Lamp oil?<d100> Rope?<d100> Bombs?", 28,
                                   1.f, false, ccWHITE);
  // additional follow-up dialog object
  auto obj2 = DialogObject::create("Layout Creator",
                                   "You want it? <d100> It's yours my friend",
                                   28, 1.f, false, ccWHITE);
  auto obj3 = DialogObject::create(
      "Layout Creator",
      fmt::format("As long as you have <cy>{}</c> <cr>Rubies</c>... ",
                  GameToolbox::pointsToString(Mod::get()->getSavedValue<int>("rubies"))),
      28, 1.f, false, ccWHITE);
  auto obj4 = DialogObject::create(
      "Layout Creator", "Wait, that's <cy>exactly how much you have!</c>", 28,
      1.f, false, ccWHITE);
  auto obj5 = DialogObject::create(
      "Layout Creator", "Too bad you <cr>can't buy anything</c> with them...",
      28, 1.f, false, ccWHITE);
  auto obj6 = DialogObject::create("Layout Creator", "For now ;)", 28, 1.f,
                                   false, ccWHITE);

  CCArray *objects = nullptr;
  objects = CCArray::create();
  objects->addObject(obj1);
  objects->addObject(obj2);
  objects->addObject(obj3);
  objects->addObject(obj4);
  objects->addObject(obj5);
  objects->addObject(obj6);
  // show layer
  auto dialog = DialogLayer::createDialogLayer(obj1, objects, 2);
  dialog->addToMainScene();
  dialog->animateInRandomSide();
}

void RLCreatorLayer::onAnnoucementButton(CCObject *sender) {
  // disable the button if provided to avoid spamming
  auto menuItem = static_cast<CCMenuItemSpriteExtra *>(sender);
  if (menuItem)
    menuItem->setEnabled(false);

  Ref<RLCreatorLayer> self = this;
  m_announcementTask.spawn(
      web::WebRequest().get("https://gdrate.arcticwoof.xyz/getAnnoucement"),
      [self, menuItem](web::WebResponse const &res) {
        if (!self)
          return;
        if (!res.ok()) {
          Notification::create("Failed to fetch announcement",
                               NotificationIcon::Error)
              ->show();
          if (menuItem)
            menuItem->setEnabled(true);
          return;
        }

        auto jsonRes = res.json();
        if (!jsonRes) {
          Notification::create("Invalid announcement response",
                               NotificationIcon::Error)
              ->show();
          if (menuItem)
            menuItem->setEnabled(true);
          return;
        }

        auto json = jsonRes.unwrap();
        std::string body = "";
        int id = 0;
        if (json.contains("body")) {
          if (auto s = json["body"].asString(); s)
            body = s.unwrap();
        }
        if (json.contains("id")) {
          if (auto i = json["id"].as<int>(); i)
            id = i.unwrap();
        }

        if (!body.empty()) {
          MDPopup::create("Rated Layouts Annoucement", body.c_str(), "OK")
              ->show();
          RLAchievements::onReward("misc_news");
          if (id) {
            Mod::get()->setSavedValue<int>("annoucementId", id);
            // hide badge since the user just viewed the announcement
            if (self->m_newsBadge)
              self->m_newsBadge->setVisible(false);
          }
        } else {
          Notification::create("No announcement available",
                               NotificationIcon::Warning)
              ->show();
        }

        if (self->m_newsBadge) {
          self->m_newsBadge->setVisible(false);
        }

        if (menuItem)
          menuItem->setEnabled(true);
      });
}

void RLCreatorLayer::onUnknownButton(CCObject *sender) {
  // disable the button first to prevent spamming
  auto menuItem = static_cast<CCMenuItemSpriteExtra *>(sender);
  menuItem->setEnabled(false);
  // fetch dialogue from server and show it in a dialog
  Ref<RLCreatorLayer> self = this;
  m_dialogueTask.spawn(
      web::WebRequest().get("https://gdrate.arcticwoof.xyz/getDialogue"),
      [self, menuItem](web::WebResponse const &res) {
        if (!self)
          return;
        std::string text = "..."; // default text
        int id = 0;
        if (res.ok()) {
          auto jsonRes = res.json();
          if (jsonRes) {
            auto json = jsonRes.unwrap();
            if (json.contains("id")) {
              if (auto i = json["id"].as<int>(); i)
                id = i.unwrap();
            }
            log::info("Fetched dialogue id: {}", id);
            if (auto diag = json["dialogue"].asString(); diag) {
              text = diag.unwrap();
            }
          } else {
            log::error("Failed to parse getDialogue response");
            if (menuItem)
              menuItem->setEnabled(true);
          }
        } else {
          log::error("Failed to fetch dialogue");
          if (menuItem)
            menuItem->setEnabled(true);
        }

        DialogObject *dialogObj = DialogObject::create(
            "Layout Creator", text.c_str(), 28, 1.f, false, ccWHITE);
        if (dialogObj) {
          auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
          dialog->addToMainScene();
          dialog->animateInRandomSide();
          RLAchievements::onReward("misc_creator_1"); // first time dialogue
          Mod::get()->setSavedValue<int>(
              "dialoguesSpoken",
              Mod::get()->getSavedValue<int>("dialoguesSpoken") + 1);

          // secret message
          if (id == 169) {
            RLAchievements::onReward("misc_salt");
          }
          // yap
          if (Mod::get()->getSavedValue<int>("dialoguesSpoken") == 25) {
            RLAchievements::onReward("misc_creator_25");
          }
          if (Mod::get()->getSavedValue<int>("dialoguesSpoken") == 50) {
            RLAchievements::onReward("misc_creator_50");
          }
          if (Mod::get()->getSavedValue<int>("dialoguesSpoken") == 100) {
            RLAchievements::onReward("misc_creator_100");
          }
        }
        menuItem->setEnabled(true);
      });
}

void RLCreatorLayer::onInfoButton(CCObject *sender) {
  MDPopup::create(
      "About Rated Layouts",
      "## <cl>Rated Layouts</cl> is a community-run rating system focusing "
      "on "
      "gameplay in layout levels.\n\n"
      "### Each of the buttons on this screen lets you browse different "
      "categories of rated layouts:\n\n"
      "<cg>**Featured Layouts**</c>: Featured layouts that showcase fun "
      "gameplay and visuals. Each featured levels are ranked based of their "
      "featured score.\n\n"
      "<cg>**Leaderboard**</c>: The top-rated players ranked by blueprint "
      "stars and creator points.\n\n"
      "<cg>**Layout Gauntlets**</c>: Special themed layouts hosted by the "
      "Rated Layouts Team. This holds the <cl>Layout Creator "
      "Contests</c>!\n\n"
      "<cg>**Sent Layouts**</c>: Suggested or sent layouts by the Layout "
      "Moderators. The community can vote on these layouts based of their "
      "Design, Difficulty and Gameplay. <co>(Only enabled if you have at "
      "least "
      "20% in Normal Mode or 80% in Practice Mode)</c>\n\n"
      "<cg>**Search Layouts**</c>: Search for rated layouts by their level "
      "name/ID.\n\n"
      "<cg>**Event Layouts**</c>: Showcases time-limited Daily, Weekly and "
      "Monthly layouts picked by the <cr>Layout Admins</c>.\n\n"
      "### Join the <cb>[Rated Layouts "
      "Discord](https://discord.gg/jBf2wfBgVT)</c> server for more "
      "information "
      "and to submit your layouts for rating.\n\n",
      "OK")
      ->show();
}

void RLCreatorLayer::onAchievementsButton(CCObject *sender) {
  auto achievementLayer = RLAchievementsPopup::create();
  achievementLayer->show();
}

void RLCreatorLayer::onCreditsButton(CCObject *sender) {
  auto creditsPopup = RLCreditsPopup::create();
  creditsPopup->show();
}

void RLCreatorLayer::onDailyLayouts(CCObject *sender) {
  auto dailyPopup = RLEventLayouts::create(RLEventLayouts::EventType::Daily);
  dailyPopup->show();
}

void RLCreatorLayer::onWeeklyLayouts(CCObject *sender) {
  auto weeklyPopup = RLEventLayouts::create(RLEventLayouts::EventType::Weekly);
  weeklyPopup->show();
}

void RLCreatorLayer::onMonthlyLayouts(CCObject *sender) {
  auto monthlyPopup =
      RLEventLayouts::create(RLEventLayouts::EventType::Monthly);
  monthlyPopup->show();
}

void RLCreatorLayer::onBackButton(CCObject *sender) {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}

void RLCreatorLayer::onFeaturedLayouts(CCObject *sender) {
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::Featured, RLLevelBrowserLayer::ParamList(),
      "Featured Layouts");
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onSentLayouts(CCObject *sender) {
  if (Mod::get()->getSavedValue<int>("role") == 2) {
    auto selectPopup = RLSelectSends::create();
    selectPopup->show();
    return;
  }

  RLLevelBrowserLayer::ParamList params;
  params.emplace_back("type", "1");
  auto browserLayer = RLLevelBrowserLayer::create(
      RLLevelBrowserLayer::Mode::Sent, params, "Sent Layouts");
  auto scene = CCScene::create();
  scene->addChild(browserLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::keyBackClicked() { this->onBackButton(nullptr); }

void RLCreatorLayer::update(float dt) {
  // MenuGameLayer handles background animation/scrolling

  // GJGroundLayer handles its own scrolling/animation

  // move background decorations (same speed as ground)
  if (!m_bgDecorations.empty()) {
    float move = m_groundSpeed * dt;
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    for (auto spr : m_bgDecorations) {
      if (!spr)
        continue;
      float w = spr->getContentSize().width;
      float x = spr->getPositionX();
      x -= move;
      if (x <= -w) {
        // wrap by columns so sprite stays in grid alignment
        int cols =
            m_decoCols
                ? m_decoCols
                : static_cast<int>(ceil(winSize.width / m_decoGridX)) + 2;
        x += cols * m_decoGridX;
      }
      spr->setPositionX(x);
    }
  }
}

void RLCreatorLayer::onLeaderboard(CCObject *sender) {
  auto leaderboardLayer = RLLeaderboardLayer::create();
  auto scene = CCScene::create();
  scene->addChild(leaderboardLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onSearchLayouts(CCObject *sender) {
  auto searchLayer = RLSearchLayer::create();
  auto scene = CCScene::create();
  scene->addChild(searchLayer);
  auto transitionFade = CCTransitionFade::create(0.5f, scene);
  CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onEnter() {
  CCLayer::onEnter();

  if (!Mod::get()->getSettingValue<bool>("disableModInfo")) {
    // refresh mod info every time the layer is entered
    if (m_modStatusLabel) {
      m_modStatusLabel->setString("Checking...");
      m_modStatusLabel->setColor({255, 150, 0});
    }
    if (m_modVersionLabel) {
      m_modVersionLabel->setString(
          Mod::get()->getVersion().toVString().c_str());
      m_modVersionLabel->setColor({255, 150, 0});
    }

    Ref<RLCreatorLayer> selfRef = this;
    async::spawn(fetchModInfoAsync(), [selfRef](
                                          std::optional<ModInfo> infoOpt) {
      if (!selfRef)
        return;

      if (!infoOpt) {
        if (selfRef->m_modStatusLabel) {
          selfRef->m_modStatusLabel->setString("Offline");
          selfRef->m_modStatusLabel->setColor({255, 64, 64});
        }
        return;
      }

      auto info = *infoOpt;
      std::string statusText =
          info.status + std::string(" - ") + info.serverVersion;
      if (selfRef->m_modStatusLabel) {
        selfRef->m_modStatusLabel->setString(statusText.c_str());
        if (info.status == "Online") {
          selfRef->m_modStatusLabel->setColor({64, 255, 128});
        } else {
          selfRef->m_modStatusLabel->setColor({255, 150, 0});
        }
      }

      if (selfRef->m_modVersionLabel) {
        selfRef->m_modVersionLabel->setString(
            ("Up-to-date - " + info.modVersion).c_str());
        selfRef->m_modVersionLabel->setColor({64, 255, 128});

        if (info.modVersion != Mod::get()->getVersion().toVString()) {
          selfRef->m_modVersionLabel->setString(
              ("Outdated - " + Mod::get()->getVersion().toVString()).c_str());
          selfRef->m_modVersionLabel->setColor({255, 200, 0});
        }
      }
    });
  }
}

RLCreatorLayer *RLCreatorLayer::create() {
  auto ret = new RLCreatorLayer();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}
