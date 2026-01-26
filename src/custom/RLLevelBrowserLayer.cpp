#include "RLLevelBrowserLayer.hpp"

#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <algorithm>
#include <charconv>

#include "RLSearchLayer.hpp"

using namespace geode::prelude;

static constexpr CCSize LIST_SIZE{358.f, 220.f};
static constexpr int PAGE_SIZE = 10;


RLLevelBrowserLayer* RLLevelBrowserLayer::create(Mode mode, ParamList const& params, std::string const& title) {
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

bool RLLevelBrowserLayer::init(GJSearchObject* object) {
      if (!CCLayer::init()) return false;

      m_searchObject = object;

      if (Mod::get()->getSettingValue<bool>("disableBackground") == true) {
            auto bg = createLayerBG();
            addChild(bg, -1);
      }

      auto winSize = CCDirector::sharedDirector()->getWinSize();

      m_bgContainer = CCNode::create();
      m_bgContainer->setContentSize(winSize);
      this->addChild(m_bgContainer, -7);

      if (Mod::get()->getSettingValue<bool>("disableBackground") == false) {
            auto value = Mod::get()->getSettingValue<int>("backgroundType");
            std::string bgIndex = (value >= 1 && value <= 9) ? ("0" + numToString(value)) : numToString(value);
            std::string bgName = "game_bg_" + bgIndex + "_001.png";
            auto testBg = CCSprite::create(bgName.c_str());
            if (!testBg) {
                  testBg = CCSprite::create("game_bg_01_001.png");
            }
            if (testBg) {
                  float tileW = testBg->getContentSize().width;
                  int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
                  for (int i = 0; i < tiles; ++i) {
                        auto bgSpr = CCSprite::create(bgName.c_str());
                        if (!bgSpr) bgSpr = CCSprite::create("game_bg_01_001.png");
                        if (!bgSpr) continue;
                        bgSpr->setAnchorPoint({0.f, 0.f});
                        bgSpr->setPosition({i * tileW, 0.f});
                        bgSpr->setColor(Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground"));
                        m_bgContainer->addChild(bgSpr);
                        m_bgTiles.push_back(bgSpr);
                  }
            }

            m_groundContainer = CCNode::create();
            m_groundContainer->setContentSize(winSize);
            this->addChild(m_groundContainer, -5);

            auto floorValue = Mod::get()->getSettingValue<int>("floorType");
            std::string floorIndex = (floorValue >= 1 && floorValue <= 9) ? ("0" + numToString(floorValue)) : numToString(floorValue);
            std::string groundName = "groundSquare_" + floorIndex + "_001.png";
            auto testGround = CCSprite::create(groundName.c_str());
            if (!testGround) testGround = CCSprite::create("groundSquare_01_001.png");
            if (testGround) {
                  float tileW = testGround->getContentSize().width;
                  int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
                  for (int i = 0; i < tiles; ++i) {
                        auto gSpr = CCSprite::create(groundName.c_str());
                        if (!gSpr) gSpr = CCSprite::create("groundSquare_01_001.png");
                        if (!gSpr) continue;
                        gSpr->setAnchorPoint({0.f, 0.f});
                        gSpr->setPosition({i * tileW, -70.f});
                        gSpr->setColor(Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbFloor"));
                        m_groundContainer->addChild(gSpr);
                        m_groundTiles.push_back(gSpr);
                  }
            }

            auto floorLineSpr = CCSprite::createWithSpriteFrameName("floorLine_01_001.png");
            floorLineSpr->setPosition({winSize.width / 2, 58});
            m_groundContainer->addChild(floorLineSpr, -4);
      }

      const char* title = m_title.empty() ? "Rated Layouts" : m_title.c_str();

      // back
      auto backMenu = CCMenu::create();
      backMenu->setPosition({0, 0});
      auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
      auto backButton = CCMenuItemSpriteExtra::create(backSpr, this, menu_selector(RLLevelBrowserLayer::onBackButton));
      backButton->setPosition({25, winSize.height - 25});
      backMenu->addChild(backButton);
      this->addChild(backMenu);

      m_listLayer = GJListLayer::create(nullptr, title, {191, 114, 62, 255}, LIST_SIZE.width, LIST_SIZE.height, 0);
      m_listLayer->setPosition({winSize / 2 - m_listLayer->getScaledContentSize() / 2 - 5});

      auto scrollLayer = ScrollLayer::create({m_listLayer->getContentSize().width, m_listLayer->getContentSize().height});
      scrollLayer->setPosition({0, 0});
      m_listLayer->addChild(scrollLayer);
      m_scrollLayer = scrollLayer;

      auto contentLayer = scrollLayer->m_contentLayer;
      if (contentLayer) {
            auto layout = ColumnLayout::create();
            contentLayer->setLayout(layout);
            layout->setGap(0.f);
            layout->setAutoGrowAxis(0.f);
            layout->setAxisReverse(true);
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

      // level count label
      m_levelsLabel = CCLabelBMFont::create("", "goldFont.fnt");
      m_levelsLabel->setPosition({winSize.width - 5, winSize.height - 5});
      m_levelsLabel->setAnchorPoint({1.f, 1.f});
      m_levelsLabel->setScale(0.45f);
      this->addChild(m_levelsLabel, 10);

      // refresh button
      auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
      m_refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(RLLevelBrowserLayer::onRefresh));
      m_refreshBtn->setPosition({winSize.width - 35, 35});
      auto refreshMenu = CCMenu::create();
      refreshMenu->setPosition({0, 0});
      refreshMenu->addChild(m_refreshBtn);
      this->addChild(refreshMenu);

      // page buttons
      auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
      m_prevButton = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(RLLevelBrowserLayer::onPrevPage));
      m_prevButton->setPosition({20, winSize.height / 2});
      auto prevMenu = CCMenu::create();
      prevMenu->setPosition({0, 0});
      prevMenu->addChild(m_prevButton);
      this->addChild(prevMenu);

      auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
      nextSpr->setFlipX(true);
      m_nextButton = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(RLLevelBrowserLayer::onNextPage));
      m_nextButton->setPosition({winSize.width - 20, winSize.height / 2});
      auto nextMenu = CCMenu::create();
      nextMenu->setPosition({0, 0});
      nextMenu->addChild(m_nextButton);
      this->addChild(nextMenu);

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
                  auto& val = m_modeParams.front().second;
                  int parsed = 0;
                  auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
                  if (res.ec == std::errc()) {
                        type = parsed;
                  } else {
                        type = 2;
                  }
            }
            this->fetchLevelsForType(type);
      } else if (m_mode == Mode::Sent || m_mode == Mode::AdminSent) {
            int type = (m_mode == Mode::AdminSent) ? 4 : 1;
            if (!m_modeParams.empty()) {
                  auto& val = m_modeParams.front().second;
                  int parsed = 0;
                  auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
                  if (res.ec == std::errc()) type = parsed;
            }
            this->fetchLevelsForType(type);
      } else if (m_mode == Mode::Search) {
            if (!m_modeParams.empty()) this->performSearchQuery(m_modeParams);
      }

      return true;
}

void RLLevelBrowserLayer::keyBackClicked() { onBackButton(nullptr); }

void RLLevelBrowserLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void RLLevelBrowserLayer::onPrevPage(CCObject* sender) {
      if (m_loading) return;
      if (m_page > 0) {
            m_page--;
            m_levelCache.clear();
            if (m_mode == Mode::Featured || m_mode == Mode::Sent || m_mode == Mode::AdminSent) {
                  int type = 0;
                  if (m_mode == Mode::Featured) type = 2;
                  else if (m_mode == Mode::AdminSent) type = 4;
                  else type = 1; // Sent
                  if (!m_modeParams.empty()) {
                        auto& val = m_modeParams.front().second;
                        int parsed = 0;
                        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
                        if (res.ec == std::errc()) type = parsed;
                  }
                  this->fetchLevelsForType(type);
            } else if (m_mode == Mode::Search) {
                  this->performSearchQuery(m_modeParams);
            }
      }
}

void RLLevelBrowserLayer::onNextPage(CCObject* sender) {
      if (m_loading) return;
      if (m_page + 1 < m_totalPages) {
            m_page++;
            m_levelCache.clear();
            if (m_mode == Mode::Featured || m_mode == Mode::Sent || m_mode == Mode::AdminSent) {
                  int type = 0;
                  if (m_mode == Mode::Featured) type = 2;
                  else if (m_mode == Mode::AdminSent) type = 4;
                  else type = 1; // Sent
                  if (!m_modeParams.empty()) {
                        auto& val = m_modeParams.front().second;
                        int parsed = 0;
                        auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
                        if (res.ec == std::errc()) type = parsed;
                  }
                  this->fetchLevelsForType(type);
            } else if (m_mode == Mode::Search) {
                  this->performSearchQuery(m_modeParams);
            }
      }
}

void RLLevelBrowserLayer::onRefresh(CCObject* sender) {
      this->refreshLevels(true);
}

void RLLevelBrowserLayer::refreshLevels(bool force) {
      startLoading();
      // set delegate and request
      auto glm = GameLevelManager::get();
      if (!glm) return;
      glm->m_levelManagerDelegate = this;

      // re-run the getLevels request with the current page
      if (m_mode == Mode::Featured || m_mode == Mode::Sent || m_mode == Mode::AdminSent) {
            if (force) m_page = 0;
            int type = 0;
            if (m_mode == Mode::Featured) type = 2;
            else if (m_mode == Mode::AdminSent) type = 4;
            else type = 1;  // Sent
            if (!m_modeParams.empty()) {
                  auto& val = m_modeParams.front().second;
                  int parsed = 0;
                  auto res = std::from_chars(val.data(), val.data() + val.size(), parsed);
                  if (res.ec == std::errc()) type = parsed;
            }
            this->fetchLevelsForType(type);
            return;
      }

      if (m_searchObject) {
            glm->getOnlineLevels(m_searchObject);
      } else {
            // fallback: request nothing
            stopLoading();
      }
}

void RLLevelBrowserLayer::fetchLevelsForType(int type) {
      m_searchTask.cancel();
      m_levelCache.clear();

      startLoading();
      Ref<RLLevelBrowserLayer> self = this;
      auto req = web::WebRequest();
      req.param("type", numToString(type))
          .param("amount", "10")
          .param("page", numToString(self->m_page + 1));
      self->m_searchTask = req.get("https://gdrate.arcticwoof.xyz/getLevels");
      self->m_searchTask.listen([self](web::WebResponse* res) {
            if (!self) return;
            if (!res || !res->ok()) {
                  Notification::create("Failed to fetch levels", NotificationIcon::Error)->show();
                  self->stopLoading();
                  return;
            }
            auto jsonRes = res->json();
            if (!jsonRes) {
                  Notification::create("Invalid response", NotificationIcon::Error)->show();
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
                        self->m_totalPages = (self->m_totalLevels + PAGE_SIZE - 1) / PAGE_SIZE;
                  }
            }

            if (self->m_totalPages < 1) self->m_totalPages = 1;

            if (self->m_page < 0) self->m_page = 0;
            if (self->m_page >= self->m_totalPages) {
                  self->m_page = (self->m_totalPages > 0) ? (self->m_totalPages - 1) : 0;
            }

            std::string levelIDs;
            bool first = true;
            if (json.contains("levelIds")) {
                  auto arr = json["levelIds"];
                  for (auto v : arr) {
                        auto id = v.as<int>();
                        if (!id) continue;
                        if (!first) levelIDs += ",";
                        levelIDs += numToString(id.unwrap());
                        first = false;
                  }
            }
            if (!levelIDs.empty()) {
                  self->m_searchObject = GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
                  auto glm = GameLevelManager::get();
                  if (glm) {
                        glm->m_levelManagerDelegate = self;
                        glm->getOnlineLevels(self->m_searchObject);
                  } else {
                        self->stopLoading();
                  }
            } else {
                  Notification::create("No levels found", NotificationIcon::Warning)->show();
                  self->stopLoading();
            }
      });
}

void RLLevelBrowserLayer::performSearchQuery(ParamList const& params) {
      startLoading();
      Ref<RLLevelBrowserLayer> self = this;
      auto req = web::WebRequest();
      for (auto& p : params) req.param(p.first.c_str(), p.second);
      // ensure account id is present and include paging parameters
      req.param("accountId", numToString(GJAccountManager::get()->m_accountID))
          .param("amount", numToString(PAGE_SIZE))
          .param("page", numToString(self->m_page + 1));
      self->m_searchTask = req.get("https://gdrate.arcticwoof.xyz/search");
      self->m_searchTask.listen([self](web::WebResponse* res) {
            if (!self) return;
            if (!res || !res->ok()) {
                  Notification::create("Search request failed", NotificationIcon::Error)->show();
                  self->stopLoading();
                  return;
            }
            auto jsonRes = res->json();
            if (!jsonRes) {
                  Notification::create("Failed to parse search response", NotificationIcon::Error)->show();
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
                        self->m_totalPages = (self->m_totalLevels + PAGE_SIZE - 1) / PAGE_SIZE;
                  }
            }

            if (self->m_totalPages < 1) self->m_totalPages = 1;

            if (self->m_page < 0) self->m_page = 0;
            if (self->m_page >= self->m_totalPages) {
                  self->m_page = (self->m_totalPages > 0) ? (self->m_totalPages - 1) : 0;
            }

            std::string levelIDs;
            bool first = true;
            if (json.contains("levelIds")) {
                  auto arr = json["levelIds"];
                  for (auto v : arr) {
                        auto id = v.as<int>();
                        if (!id) continue;
                        if (!first) levelIDs += ",";
                        levelIDs += numToString(id.unwrap());
                        first = false;
                  }
            }
            if (!levelIDs.empty()) {
                  self->m_searchObject = GJSearchObject::create(SearchType::Type19, levelIDs.c_str());
                  self->refreshLevels(false);
            } else {
                  Notification::create("No results returned", NotificationIcon::Warning)->show();
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
            if (auto childSpinner = m_scrollLayer->m_contentLayer->getChildByID("rl-spinner")) {
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
            if (auto childSpinner = m_scrollLayer->m_contentLayer->getChildByID("rl-spinner")) {
                  childSpinner->removeFromParent();
            }
      }
      if (auto spinner = this->getChildByID("rl-spinner")) {
            spinner->removeFromParent();
      }
      m_circle = nullptr;
}

void RLLevelBrowserLayer::loadLevelsFinished(CCArray* levels, char const* key, int p2) {
      if (!this->getParent() || !this->isRunning()) {
            return;
      }

      // cache and populate
      if (!levels) {
            stopLoading();
            return;
      }

      // store into cache map
      for (auto lvlObj : CCArrayExt<GJGameLevel*>(levels)) {
            if (!lvlObj) continue;
            m_levelCache[lvlObj->m_levelID] = lvlObj;
      }

      populateFromArray(levels);
      stopLoading();
}

void RLLevelBrowserLayer::loadLevelsFailed(char const* key, int p1) {
      Notification::create("Failed to load levels", NotificationIcon::Error)->show();
      stopLoading();
}

void RLLevelBrowserLayer::populateFromArray(CCArray* levels) {
      if (!m_scrollLayer || !m_scrollLayer->m_contentLayer || !levels) return;
      auto contentLayer = m_scrollLayer->m_contentLayer;
      contentLayer->removeAllChildrenWithCleanup(true);

      const float cellH = 90.f;
      for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(levels)) {
            if (!level) continue;
            auto cell = new LevelCell("", 356.f, cellH);
            cell->autorelease();
            cell->loadFromLevel(level);
            cell->setContentSize({356.f, cellH});
            cell->setAnchorPoint({0.0f, 1.0f});
            contentLayer->addChild(cell);
      }

      int returned = static_cast<int>(levels->count());
      int first = m_page * PAGE_SIZE + 1;
      int last = m_page * PAGE_SIZE + returned;
      if (returned == 0) {
            first = 0;
            last = 0;
      }
      int total = (m_totalLevels > 0) ? m_totalLevels : returned;
      m_levelsLabel->setString(fmt::format("{} to {} of {}", first, last, total).c_str());

      if (contentLayer) contentLayer->updateLayout();
      if (m_scrollLayer) m_scrollLayer->scrollToTop();
}

void RLLevelBrowserLayer::onEnter() {
      CCLayer::onEnter();
      this->setTouchEnabled(true);
      this->scheduleUpdate();
}

void RLLevelBrowserLayer::update(float dt) {
      // background tiles
      if (!m_bgTiles.empty()) {
            float move = m_bgSpeed * dt;
            int num = static_cast<int>(m_bgTiles.size());
            for (auto spr : m_bgTiles) {
                  if (!spr) continue;
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
                  if (!spr) continue;
                  float tileW = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -tileW) {
                        x += tileW * num;
                  }
                  spr->setPositionX(x);
            }
      }
}
