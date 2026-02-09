#include "RLLevelBrowserLayer.hpp"

#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <algorithm>
#include <charconv>

#include <cue/RepeatingBackground.hpp>

using namespace geode::prelude;

static constexpr CCSize LIST_SIZE{356.f, 220.f};
static constexpr int PER_PAGE = 10;

RLLevelBrowserLayer *RLLevelBrowserLayer::create(Mode mode,
                                                 ParamList const &params,
                                                 std::string const &title) {
  auto ret = new RLLevelBrowserLayer();
  ret->m_mode = mode;
  ret->m_modeParams = params;
  ret->m_title = title;
  if (ret && ret->init(nullptr)) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

bool RLLevelBrowserLayer::init(GJSearchObject *object) {
  if (!CCLayer::init())
    return false;

  m_searchObject = object;

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

  auto winSize = CCDirector::sharedDirector()->getWinSize();

  const char *title = m_title.empty() ? "Rated Layouts" : m_title.c_str();

  auto uiMenu = CCMenu::create();
  uiMenu->setPosition({0, 0});

  auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
  auto backButton = CCMenuItemSpriteExtra::create(
      backSpr, this, menu_selector(RLLevelBrowserLayer::onBackButton));
  backButton->setPosition({25, winSize.height - 25});
  uiMenu->addChild(backButton);

  auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
  infoSpr->setScale(0.7f);
  auto infoButton = CCMenuItemSpriteExtra::create(
      infoSpr, this, menu_selector(RLLevelBrowserLayer::onInfoButton));
  infoButton->setPosition({25, 25});
  uiMenu->addChild(infoButton);

  m_listLayer = GJListLayer::create(nullptr, title, {191, 114, 62, 255},
                                    LIST_SIZE.width, LIST_SIZE.height, 0);
  m_listLayer->setPosition(
      {winSize / 2 - m_listLayer->getScaledContentSize() / 2 - 5});

  auto scrollLayer =
      ScrollLayer::create({m_listLayer->getContentSize().width,
                           m_listLayer->getContentSize().height});
  scrollLayer->setPosition({0, 0});
  m_listLayer->addChild(scrollLayer);
  m_scrollLayer = scrollLayer;

  auto contentLayer = scrollLayer->m_contentLayer;
  if (contentLayer) {
    auto layout = ColumnLayout::create();
    contentLayer->setLayout(layout);
    layout->setGap(0.f);
    layout->setAutoGrowAxis(220.f);
    layout->setAxisReverse(true);
    layout->setAxisAlignment(AxisAlignment::End);
    auto spinner = LoadingSpinner::create(64.f);
    if (spinner) {
      spinner->setID("rl-spinner");
      auto win = CCDirector::sharedDirector()->getWinSize();
      spinner->setPosition(win / 2);
      this->addChild(spinner, 1000);
      m_circle = spinner;
    }
  }

  this->addChild(m_listLayer);
  this->addChild(uiMenu, 10);

  m_levelsLabel = CCLabelBMFont::create("", "goldFont.fnt");
  m_levelsLabel->setPosition({winSize.width - 5, winSize.height - 5});
  m_levelsLabel->setAnchorPoint({1.f, 1.f});
  m_levelsLabel->setScale(0.45f);
  this->addChild(m_levelsLabel, 10);

  auto pageBtnSpr = CCSprite::create("GJ_button_02.png");
  pageBtnSpr->setScale(0.7f);
  if (pageBtnSpr) {
    m_pageButton = CCMenuItemSpriteExtra::create(
        pageBtnSpr, this, menu_selector(RLLevelBrowserLayer::onPageButton));
    if (m_pageButton) {
      auto pageMenu = CCMenu::create();
      pageMenu->setID("rl-page-menu");
      pageMenu->setContentSize({28, 110});
      pageMenu->setPosition({winSize.width - 7, winSize.height - 80});
      pageMenu->setAnchorPoint({1.f, 0.5f});
      pageMenu->setLayout(ColumnLayout::create()
                              ->setGap(5.f)
                              ->setAutoScale(true)
                              ->setGrowCrossAxis(true)
                              ->setCrossAxisOverflow(true)
                              ->setAxisReverse(true)
                              ->setAxisAlignment(AxisAlignment::End));
      pageMenu->addChild(m_pageButton);
      this->addChild(pageMenu, 10);
      pageMenu->updateLayout();

      m_pageButtonLabel =
          CCLabelBMFont::create(numToString(m_page + 1).c_str(), "bigFont.fnt");
      if (m_pageButtonLabel) {
        auto size = m_pageButton->getContentSize();
        m_pageButtonLabel->setPosition({size.width / 2.f, size.height / 2.f});
        m_pageButtonLabel->setAnchorPoint({0.5f, 0.5f});
        m_pageButtonLabel->setID("rl-page-label");
        m_pageButtonLabel->setScale(0.6f);
        m_pageButton->addChild(m_pageButtonLabel, 1);
      }
      this->updatePageButton();
    }
  }

  auto refreshSpr = CCSprite::createWithSpriteFrameName("RL_refresh01.png"_spr);
  m_refreshBtn = CCMenuItemSpriteExtra::create(
      refreshSpr, this, menu_selector(RLLevelBrowserLayer::onRefresh));
  m_refreshBtn->setPosition({winSize.width - 35, 35});
  uiMenu->addChild(m_refreshBtn);

  // page buttons
  auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
  m_prevButton = CCMenuItemSpriteExtra::create(
      prevSpr, this, menu_selector(RLLevelBrowserLayer::onPrevPage));
  m_prevButton->setPosition({20, winSize.height / 2});
  uiMenu->addChild(m_prevButton);
  if (m_prevButton)
    m_prevButton->setVisible(false);

  auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
  nextSpr->setFlipX(true);
  m_nextButton = CCMenuItemSpriteExtra::create(
      nextSpr, this, menu_selector(RLLevelBrowserLayer::onNextPage));
  m_nextButton->setPosition({winSize.width - 20, winSize.height / 2});
  uiMenu->addChild(m_nextButton);
  if (m_nextButton)
    m_nextButton->setVisible(false);

  m_circle = nullptr;

  auto glm = GameLevelManager::get();
  if (glm) {
    glm->m_levelManagerDelegate = this;
    this->refreshLevels(false);
  }

  this->scheduleUpdate();
  this->setKeypadEnabled(true);

  // perform auto-fetch depending on mode
  if (m_mode == Mode::Featured) {
    int type = 2;
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc()) {
        type = parsed;
      } else {
        type = 2;
      }
    }
    this->fetchLevelsForType(type);
  } else if (m_mode == Mode::Sent || m_mode == Mode::AdminSent ||
             m_mode == Mode::LegendarySends) {
    int type = (m_mode == Mode::AdminSent)
                   ? 4
                   : (m_mode == Mode::LegendarySends ? 5 : 1);
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        type = parsed;
    }
    this->fetchLevelsForType(type);
  } else if (m_mode == Mode::Search || m_mode == Mode::EventSafe) {
    if (!m_modeParams.empty())
      this->performSearchQuery(m_modeParams);
  } else if (m_mode == Mode::Account) {
    int accountId = GJAccountManager::get()->m_accountID;
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        accountId = parsed;
    }
    this->fetchAccountLevels(accountId);
  }

  return true;
}

void RLLevelBrowserLayer::keyBackClicked() { onBackButton(nullptr); }

void RLLevelBrowserLayer::onBackButton(CCObject *sender) {
  CCDirector::sharedDirector()->popSceneWithTransition(
      0.5f, PopTransition::kPopTransitionFade);
}

void RLLevelBrowserLayer::onPrevPage(CCObject *sender) {
  if (m_loading)
    return;
  if (m_page > 0) {
    m_page--;
    m_levelCache.clear();
    this->updatePageButton();
    if (m_mode == Mode::Featured || m_mode == Mode::Sent ||
        m_mode == Mode::AdminSent || m_mode == Mode::LegendarySends) {
      int type = 0;
      if (m_mode == Mode::Featured)
        type = 2;
      else if (m_mode == Mode::AdminSent)
        type = 4;
      else if (m_mode == Mode::LegendarySends)
        type = 5;
      else
        type = 1; // Sent
      if (!m_modeParams.empty()) {
        auto &val = m_modeParams.front().second;
        int parsed = 0;
        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
        if (res.ec == std::errc())
          type = parsed;
      }
      this->fetchLevelsForType(type);
    } else if (m_mode == Mode::Account) {
      int accountId = GJAccountManager::get()->m_accountID;
      if (!m_modeParams.empty()) {
        auto &val = m_modeParams.front().second;
        int parsed = 0;
        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
        if (res.ec == std::errc())
          accountId = parsed;
      }
      this->fetchAccountLevels(accountId);
    } else if (m_mode == Mode::Search || m_mode == Mode::EventSafe) {
      this->performSearchQuery(m_modeParams);
    }
  }
}

void RLLevelBrowserLayer::onNextPage(CCObject *sender) {
  if (m_loading)
    return;
  if (m_page + 1 < m_totalPages) {
    m_page++;
    m_levelCache.clear();
    this->updatePageButton();
    if (m_mode == Mode::Featured || m_mode == Mode::Sent ||
        m_mode == Mode::AdminSent || m_mode == Mode::LegendarySends) {
      int type = 0;
      if (m_mode == Mode::Featured)
        type = 2;
      else if (m_mode == Mode::AdminSent)
        type = 4;
      else if (m_mode == Mode::LegendarySends)
        type = 5;
      else
        type = 1; // Sent
      if (!m_modeParams.empty()) {
        auto &val = m_modeParams.front().second;
        int parsed = 0;
        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
        if (res.ec == std::errc())
          type = parsed;
      }
      this->fetchLevelsForType(type);
    } else if (m_mode == Mode::Account) {
      int accountId = GJAccountManager::get()->m_accountID;
      if (!m_modeParams.empty()) {
        auto &val = m_modeParams.front().second;
        int parsed = 0;
        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
        if (res.ec == std::errc())
          accountId = parsed;
      }
      this->fetchAccountLevels(accountId);
    } else if (m_mode == Mode::Search || m_mode == Mode::EventSafe) {
      this->performSearchQuery(m_modeParams);
    }
  }
}

void RLLevelBrowserLayer::onRefresh(CCObject *sender) {
  this->refreshLevels(false);
}

void RLLevelBrowserLayer::onPageButton(CCObject *sender) {
  int current = std::clamp(m_page + 1, 1, std::max(1, m_totalPages));
  int begin = 1;
  int end = std::max(1, m_totalPages);
  auto popup = SetIDPopup::create(current, begin, end, "Go to Page", "Go",
                                  false, current, 60.f, true, true);
  if (popup) {
    popup->m_delegate = this;
    popup->show();
  }
}

void RLLevelBrowserLayer::setIDPopupClosed(SetIDPopup *popup, int value) {
  if (!this->getParent() || !this->isRunning())
    return;
  if (!popup || popup->m_cancelled)
    return;
  if (value < 1)
    value = 1;
  if (m_totalPages > 0 && value > m_totalPages)
    value = m_totalPages;
  m_page = value - 1;
  m_levelCache.clear();
  this->updatePageButton();
  if (m_mode == Mode::Featured || m_mode == Mode::Sent ||
      m_mode == Mode::AdminSent || m_mode == Mode::LegendarySends) {
    int type = 0;
    if (m_mode == Mode::Featured)
      type = 2;
    else if (m_mode == Mode::AdminSent)
      type = 4;
    else if (m_mode == Mode::LegendarySends)
      type = 5;
    else
      type = 1;
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        type = parsed;
    }
    this->fetchLevelsForType(type);
  } else if (m_mode == Mode::Account) {
    int accountId = GJAccountManager::get()->m_accountID;
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        accountId = parsed;
    }
    this->fetchAccountLevels(accountId);
  } else if (m_mode == Mode::Search || m_mode == Mode::EventSafe) {
    this->performSearchQuery(m_modeParams);
  }
}

void RLLevelBrowserLayer::updatePageButton() {
  if (m_pageButtonLabel) {
    m_pageButtonLabel->setString(numToString(m_page + 1).c_str());
  }
  // only show the page picker when we have more than one page and the levels
  // label has been populated
  bool labelHasText = m_levelsLabel && m_levelsLabel->getString() &&
                      m_levelsLabel->getString()[0] != '\0';
  if (m_pageButton) {
    m_pageButton->setVisible(m_totalPages > 1 && labelHasText);
  }
  if (m_prevButton) {
    m_prevButton->setVisible(m_page > 0);
  }
  if (m_nextButton) {
    m_nextButton->setVisible(m_page + 1 < m_totalPages);
  }
}

void RLLevelBrowserLayer::refreshLevels(bool force) {
  startLoading();
  // set delegate and request
  auto glm = GameLevelManager::get();
  if (!glm)
    return;
  glm->m_levelManagerDelegate = this;

  // re-run the getLevels request with the current page
  if (m_mode == Mode::Featured || m_mode == Mode::Sent ||
      m_mode == Mode::AdminSent || m_mode == Mode::LegendarySends) {
    if (force)
      m_page = 0;
    int type = 0;
    if (m_mode == Mode::Featured)
      type = 2;
    else if (m_mode == Mode::AdminSent)
      type = 4;
    else if (m_mode == Mode::LegendarySends)
      type = 5;
    else
      type = 1; // Sent
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        type = parsed;
    }
    this->fetchLevelsForType(type);
    return;
  }

  if (m_mode == Mode::Account) {
    if (force)
      m_page = 0;
    int accountId = GJAccountManager::get()->m_accountID;
    if (!m_modeParams.empty()) {
      auto &val = m_modeParams.front().second;
      int parsed = 0;
      auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
      if (res.ec == std::errc())
        accountId = parsed;
    }
    this->fetchAccountLevels(accountId);
    return;
  }

  if (m_mode == Mode::Search || m_mode == Mode::EventSafe) {
    if (!m_modeParams.empty()) {
      this->performSearchQuery(m_modeParams);
    } else if (m_searchObject) {
      glm->getOnlineLevels(m_searchObject);
    } else {
      stopLoading();
    }
  } else {
    if (m_searchObject) {
      glm->getOnlineLevels(m_searchObject);
    } else {
      stopLoading();
    }
  }
}

void RLLevelBrowserLayer::fetchLevelsForType(int type) {
  m_searchTask.cancel();
  m_levelCache.clear();

  startLoading();
  Ref<RLLevelBrowserLayer> self = this;
  auto req = web::WebRequest();
  req.param("type", numToString(type))
      .param("amount", numToString(PER_PAGE))
      .param("page", numToString(self->m_page + 1));
  self->m_searchTask.spawn(
      req.get("https://gdrate.arcticwoof.xyz/getLevels"),
      [self](web::WebResponse res) {
        if (!self)
          return;
        if (!self->getParent() || !self->isRunning())
          return;
        if (!res.ok()) {
          Notification::create("Failed to fetch levels",
                               NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto jsonRes = res.json();
        if (!jsonRes) {
          Notification::create("Invalid response", NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("page")) {
          if (auto p = json["page"].as<int>(); p) {
            int srvPage = p.unwrap();
            self->m_page = std::max(0, srvPage - 1);
          }
        }

        if (json.contains("total")) {
          if (auto tp = json["total"].as<int>(); tp) {
            self->m_totalLevels = tp.unwrap();
            self->m_totalPages =
                (self->m_totalLevels + PER_PAGE - 1) / PER_PAGE;
          }
        }

        if (self->m_totalPages < 1)
          self->m_totalPages = 1;

        if (self->m_page < 0)
          self->m_page = 0;
        if (self->m_page >= self->m_totalPages) {
          self->m_page =
              (self->m_totalPages > 0) ? (self->m_totalPages - 1) : 0;
        }
        self->updatePageButton();

        std::string levelIDs;
        bool first = true;
        if (json.contains("levelIds")) {
          auto arr = json["levelIds"];
          for (auto v : arr) {
            auto id = v.as<int>();
            if (!id)
              continue;
            if (!first)
              levelIDs += ",";
            levelIDs += numToString(id.unwrap());
            first = false;
          }
        }
        if (!levelIDs.empty()) {
          self->m_searchObject =
              GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
          auto glm = GameLevelManager::get();
          if (glm) {
            glm->m_levelManagerDelegate = self;
            glm->getOnlineLevels(self->m_searchObject);
          } else {
            self->stopLoading();
          }
        } else {
          Notification::create("No levels found", NotificationIcon::Warning)
              ->show();
          self->stopLoading();
        }
      });
}

// fetch layout ids for a specific account
void RLLevelBrowserLayer::fetchAccountLevels(int accountId) {
  m_searchTask.cancel();
  m_levelCache.clear();

  startLoading();
  Ref<RLLevelBrowserLayer> self = this;
  auto req = web::WebRequest();
  req.param("accountId", numToString(accountId))
      .param("amount", numToString(PER_PAGE))
      .param("page", numToString(self->m_page + 1));
  self->m_searchTask.spawn(
      req.get("https://gdrate.arcticwoof.xyz/getAccountLevels"),
      [self](web::WebResponse const &res) {
        if (!self)
          return;
        if (!self->getParent() || !self->isRunning())
          return;
        if (!res.ok()) {
          Notification::create("Failed to fetch levels",
                               NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto jsonRes = res.json();
        if (!jsonRes) {
          Notification::create("Invalid response", NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("page")) {
          if (auto p = json["page"].as<int>(); p) {
            int srvPage = p.unwrap();
            self->m_page = std::max(0, srvPage - 1);
          }
        }

        if (json.contains("total")) {
          if (auto tp = json["total"].as<int>(); tp) {
            self->m_totalLevels = tp.unwrap();
            self->m_totalPages =
                (self->m_totalLevels + PER_PAGE - 1) / PER_PAGE;
          }
        }

        if (self->m_totalPages < 1)
          self->m_totalPages = 1;

        if (self->m_page < 0)
          self->m_page = 0;
        if (self->m_page >= self->m_totalPages) {
          self->m_page =
              (self->m_totalPages > 0) ? (self->m_totalPages - 1) : 0;
        }
        self->updatePageButton();

        std::string levelIDs;
        bool first = true;
        if (json.contains("levels")) {
          auto arr = json["levels"];
          for (auto v : arr) {
            auto id = v.as<int>();
            if (!id)
              continue;
            if (!first)
              levelIDs += ",";
            levelIDs += numToString(id.unwrap());
            first = false;
          }
        } else if (json.contains("levelIds")) {
          auto arr = json["levelIds"];
          for (auto v : arr) {
            auto id = v.as<int>();
            if (!id)
              continue;
            if (!first)
              levelIDs += ",";
            levelIDs += numToString(id.unwrap());
            first = false;
          }
        }

        if (!levelIDs.empty()) {
          self->m_searchObject =
              GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
          auto glm = GameLevelManager::get();
          if (glm) {
            glm->m_levelManagerDelegate = self;
            glm->getOnlineLevels(self->m_searchObject);
          } else {
            self->stopLoading();
          }
        } else {
          Notification::create("No levels found", NotificationIcon::Warning)
              ->show();
          self->stopLoading();
        }
      });
}

void RLLevelBrowserLayer::performSearchQuery(ParamList const &params) {
  m_searchTask.cancel();
  m_levelCache.clear();
  startLoading();
  Ref<RLLevelBrowserLayer> self = this;

  // call the event endpoint directly and use returned level ids
  for (auto const &p : params) {
    if (p.first == "safe") {
      std::string url =
          std::string("https://gdrate.arcticwoof.xyz/getEvent?safe=") +
          p.second + std::string("&amount=") + numToString(PER_PAGE) +
          std::string("&page=") + numToString(self->m_page + 1);
      self->m_searchTask.spawn(
          web::WebRequest().get(url), [self](web::WebResponse const &res) {
            if (!self)
              return;
            if (!self->getParent() || !self->isRunning())
              return;
            if (!res.ok()) {
              Notification::create("Failed to fetch event safe list",
                                   NotificationIcon::Error)
                  ->show();
              self->stopLoading();
              return;
            }
            auto jsonRes = res.json();
            if (!jsonRes) {
              Notification::create("Invalid event response",
                                   NotificationIcon::Warning)
                  ->show();
              self->stopLoading();
              return;
            }
            auto json = jsonRes.unwrap();

            // handle page / total if present
            if (json.contains("page")) {
              if (auto p = json["page"].as<int>(); p) {
                int srvPage = p.unwrap();
                self->m_page = std::max(0, srvPage - 1);
              }
            }
            if (json.contains("total")) {
              if (auto t = json["total"].as<int>(); t) {
                self->m_totalLevels = t.unwrap();
                self->m_totalPages =
                    (self->m_totalLevels + PER_PAGE - 1) / PER_PAGE;
              }
            }
            if (self->m_totalPages < 1)
              self->m_totalPages = 1;
            self->updatePageButton();

            std::string levelIDs;
            bool first = true;
            if (json.contains("levelIds")) {
              auto arr = json["levelIds"];
              for (auto v : arr) {
                auto id = v.as<int>();
                if (!id)
                  continue;
                if (!first)
                  levelIDs += ",";
                levelIDs += numToString(id.unwrap());
                first = false;
              }
            } else if (json.type() == matjson::Type::Array) {
              for (auto v : json) {
                auto id = v.as<int>();
                if (!id)
                  continue;
                if (!first)
                  levelIDs += ",";
                levelIDs += numToString(id.unwrap());
                first = false;
              }
            }

            if (!levelIDs.empty()) {
              self->m_searchObject =
                  GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
              auto glm = GameLevelManager::get();
              if (glm) {
                glm->m_levelManagerDelegate = self;
                glm->getOnlineLevels(self->m_searchObject);
              } else {
                self->stopLoading();
              }
            } else {
              Notification::create("No levels returned",
                                   NotificationIcon::Warning)
                  ->show();
              self->stopLoading();
            }
          });
      return;
    }
  }

  // proxy params to the search endpoint
  auto req = web::WebRequest();
  for (auto &p : params)
    req.param(p.first.c_str(), p.second);
  // ensure account id is present and include paging parameters
  req.param("accountId", numToString(GJAccountManager::get()->m_accountID))
      .param("amount", numToString(PER_PAGE))
      .param("page", numToString(self->m_page + 1));
  self->m_searchTask.spawn(
      req.get("https://gdrate.arcticwoof.xyz/search"),
      [self](web::WebResponse const &res) {
        if (!self)
          return;
        if (!self->getParent() || !self->isRunning())
          return;
        if (!res.ok()) {
          Notification::create("Search request failed", NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto jsonRes = res.json();
        if (!jsonRes) {
          Notification::create("Failed to parse search response",
                               NotificationIcon::Error)
              ->show();
          self->stopLoading();
          return;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("page")) {
          if (auto p = json["page"].as<int>(); p) {
            int srvPage = p.unwrap();
            self->m_page = std::max(0, srvPage - 1);
          }
        }

        if (json.contains("total")) {
          if (auto tp = json["total"].as<int>(); tp) {
            self->m_totalLevels = tp.unwrap();
            self->m_totalPages =
                (self->m_totalLevels + PER_PAGE - 1) / PER_PAGE;
          }
        }

        if (self->m_totalPages < 1)
          self->m_totalPages = 1;

        if (self->m_page < 0)
          self->m_page = 0;
        if (self->m_page >= self->m_totalPages) {
          self->m_page =
              (self->m_totalPages > 0) ? (self->m_totalPages - 1) : 0;
        }
        self->updatePageButton();

        std::string levelIDs;
        bool first = true;
        if (json.contains("levelIds")) {
          auto arr = json["levelIds"];
          for (auto v : arr) {
            auto id = v.as<int>();
            if (!id)
              continue;
            if (!first)
              levelIDs += ",";
            levelIDs += numToString(id.unwrap());
            first = false;
          }
        }
        if (!levelIDs.empty()) {
          self->m_searchObject =
              GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
          auto glm = GameLevelManager::get();
          if (glm) {
            glm->m_levelManagerDelegate = self;
            glm->getOnlineLevels(self->m_searchObject);
          } else {
            self->stopLoading();
          }
        } else {
          Notification::create("No results returned", NotificationIcon::Warning)
              ->show();
          self->stopLoading();
        }
      });
}

void RLLevelBrowserLayer::startLoading() {
  m_loading = true;
  if (m_circle) {
    m_circle->removeFromParent();
    m_circle = nullptr;
  }
  // remove any existing spinner nodes safely
  if (auto spinner = this->getChildByID("rl-spinner")) {
    spinner->removeFromParent();
  }
  if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
    if (auto childSpinner =
            m_scrollLayer->m_contentLayer->getChildByID("rl-spinner")) {
      childSpinner->removeFromParent();
    }
    m_scrollLayer->m_contentLayer->removeAllChildrenWithCleanup(true);
    auto spinner = LoadingSpinner::create(64.f);
    if (spinner) {
      spinner->setID("rl-spinner");
      auto win = CCDirector::sharedDirector()->getWinSize();
      spinner->setPosition(win / 2);
      this->addChild(spinner, 1000);
      m_circle = spinner;
    }
  } else {
    // fallback: show spinner on the layer
    auto spinner = LoadingSpinner::create(64.f);
    if (spinner) {
      spinner->setID("rl-spinner");
      auto win = CCDirector::sharedDirector()->getWinSize();
      spinner->setPosition(win / 2);
      this->addChild(spinner, 100);
      m_circle = spinner;
    }
  }
}

void RLLevelBrowserLayer::stopLoading() {
  m_loading = false;
  if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
    if (auto childSpinner =
            m_scrollLayer->m_contentLayer->getChildByID("rl-spinner")) {
      childSpinner->removeFromParent();
    }
  }
  if (auto spinner = this->getChildByID("rl-spinner")) {
    spinner->removeFromParent();
  }
  m_circle = nullptr;
}

void RLLevelBrowserLayer::loadLevelsFinished(CCArray *levels, char const *key,
                                             int p2) {
  if (!this->getParent() || !this->isRunning()) {
    return;
  }

  // cache and populate
  if (!levels) {
    stopLoading();
    return;
  }

  // store into cache map
  for (auto lvlObj : CCArrayExt<GJGameLevel *>(levels)) {
    if (!lvlObj)
      continue;
    m_levelCache[lvlObj->m_levelID] = lvlObj;
  }

  populateFromArray(levels);
  stopLoading();
}

void RLLevelBrowserLayer::loadLevelsFailed(char const *key, int p1) {
  Notification::create("Failed to load levels", NotificationIcon::Error)
      ->show();
  stopLoading();
}

void RLLevelBrowserLayer::populateFromArray(CCArray *levels) {
  if (!m_scrollLayer || !m_scrollLayer->m_contentLayer || !levels)
    return;
  auto contentLayer = m_scrollLayer->m_contentLayer;
  contentLayer->removeAllChildrenWithCleanup(true);

  const float cellH = 90.f;
  for (GJGameLevel *level : CCArrayExt<GJGameLevel *>(levels)) {
    if (!level)
      continue;
    auto cell = new LevelCell("", 356.f, cellH);
    cell->autorelease();
    cell->loadFromLevel(level);
    cell->setContentSize({356.f, cellH});
    cell->setAnchorPoint({0.0f, 1.0f});
    contentLayer->addChild(cell);
  }

  int returned = static_cast<int>(levels->count());
  int first = m_page * PER_PAGE + 1;
  int last = m_page * PER_PAGE + returned;
  if (returned == 0) {
    first = 0;
    last = 0;
  }
  int total = (m_totalLevels > 0) ? m_totalLevels : returned;
  m_levelsLabel->setString(
      fmt::format("{} to {} of {}", first, last, total).c_str());

  m_needsLayout = true;
  this->updatePageButton();
}

void RLLevelBrowserLayer::onEnter() {
  CCLayer::onEnter();
  this->setTouchEnabled(true);
  this->scheduleUpdate();
}

void RLLevelBrowserLayer::onExit() {
  // clear delegate to avoid dangling callbacks when this layer is removed
  auto glm = GameLevelManager::get();
  if (glm && glm->m_levelManagerDelegate == this) {
    log::debug("clearing GameLevelManager delegate on exit");
    glm->m_levelManagerDelegate = nullptr;
  }
  CCLayer::onExit();
}

void RLLevelBrowserLayer::onInfoButton(CCObject *sender) {
  FLAlertLayer::create(
      "Layouts",
      "<cl>Layout</c> listed here rewards <cl>Spark</c> or <co>Planets</c> if "
      "is rated by <cf>ArcticWoof</c> or <cr>Layout Admins</c>. For sent "
      "layouts, each of the layouts here has at least <cg>one sent by Layout "
      "Mods</c> and can only be rated when it has at least <cb>3+ sends</c>.",
      "OK")
      ->show();
}

void RLLevelBrowserLayer::update(float dt) {
  // Deferred layout if requested
  if (m_needsLayout) {
    if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
      auto contentLayer = m_scrollLayer->m_contentLayer;
      if (contentLayer->getChildren() &&
          contentLayer->getChildren()->count() > 0) {
        contentLayer->updateLayout();
        if (m_scrollLayer)
          m_scrollLayer->scrollToTop();
      }
    }
    m_needsLayout = false;
  }

  // background tiles
  if (!m_bgTiles.empty()) {
    float move = m_bgSpeed * dt;
    int num = static_cast<int>(m_bgTiles.size());
    for (auto spr : m_bgTiles) {
      if (!spr)
        continue;
      float tileW = spr->getContentSize().width;
      float x = spr->getPositionX();
      x -= move;
      if (x <= -tileW) {
        x += tileW * num;
      }
      spr->setPositionX(x);
    }
  }

  // ground tiles
  if (!m_groundTiles.empty()) {
    float move = m_groundSpeed * dt;
    int num = static_cast<int>(m_groundTiles.size());
    for (auto spr : m_groundTiles) {
      if (!spr)
        continue;
      float tileW = spr->getContentSize().width;
      float x = spr->getPositionX();
      x -= move;
      if (x <= -tileW) {
        x += tileW * num;
      }
      spr->setPositionX(x);
    }
  }

  if (m_pageButtonLabel) {
    m_pageButtonLabel->setString(numToString(m_page + 1).c_str());
  }
  if (m_pageButton) {
    m_pageButton->setVisible(m_totalPages > 1);
  }
}
