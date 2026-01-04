#include "RLGauntletSelectLayer.hpp"

#include <Geode/Geode.hpp>

#include "RLAnnoucementPopup.hpp"
#include "RLGauntletLevelsLayer.hpp"

using namespace geode::prelude;

RLGauntletSelectLayer* RLGauntletSelectLayer::create() {
      auto ret = new RLGauntletSelectLayer();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      delete ret;
      return nullptr;
}

bool RLGauntletSelectLayer::init() {
      if (!CCLayer::init())
            return false;

      auto bg = createLayerBG();
      addChild(bg, -1);
      bg->setColor({50, 50, 50});

      auto winSize = CCDirector::sharedDirector()->getWinSize();

      // title
      auto titleSprite = CCSprite::create("RL_titleGauntlet.png"_spr);
      titleSprite->setPosition({winSize.width / 2, winSize.height - 30});
      titleSprite->setScale(.7f);
      this->addChild(titleSprite, 10);

      // crap
      addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

      auto backMenu = CCMenu::create();
      backMenu->setPosition({0, 0});

      auto backButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
      CCMenuItemSpriteExtra* backButton = CCMenuItemSpriteExtra::create(
          backButtonSpr, this, menu_selector(RLGauntletSelectLayer::onBackButton));
      backButton->setPosition({25, winSize.height - 25});
      backMenu->addChild(backButton);
      this->addChild(backMenu);

      this->setKeypadEnabled(true);

      m_loadingCircle = LoadingSpinner::create({100.f});
      m_loadingCircle->setPosition(winSize / 2);
      this->addChild(m_loadingCircle);

      // @geode-ignore(unknown-resource)
      auto annouceSpr = AccountButtonSprite::createWithSpriteFrameName("geode.loader/news.png", 1.f, AccountBaseColor::Purple, AccountBaseSize::Normal);
      annouceSpr->setScale(0.7f);
      auto annouceBtn = CCMenuItemSpriteExtra::create(annouceSpr, this, menu_selector(RLGauntletSelectLayer::onInfoButton));
      annouceBtn->setPosition({25, 25});
      backMenu->addChild(annouceBtn);

      fetchGauntlets();
      return true;
}

void RLGauntletSelectLayer::onInfoButton(CCObject* sender) {
      auto announcement = RLAnnoucementPopup::create();
      announcement->show();
}

void RLGauntletSelectLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(
          0.5f, PopTransition::kPopTransitionFade);
}
void RLGauntletSelectLayer::keyBackClicked() { this->onBackButton(nullptr); }

void RLGauntletSelectLayer::onGauntletButtonClick(CCObject* sender) {
      auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
      if (!item) return;
      int index = item->getTag();

      if (index >= 0 && index < static_cast<int>(m_gauntletsArray.size())) {
            auto gauntlet = m_gauntletsArray[index];
            auto layer = RLGauntletLevelsLayer::create(gauntlet);
            auto scene = CCScene::create();
            scene->addChild(layer);
            CCDirector::sharedDirector()->pushScene(
                CCTransitionFade::create(0.5f, scene));
      }
}

void RLGauntletSelectLayer::fetchGauntlets() {
      web::WebRequest request;
      m_gauntletsListener = [this](web::WebResponse* response) {
            if (response->ok()) {
                  auto jsonRes = response->json();
                  if (jsonRes.isOk()) {
                        this->onGauntletsFetched(jsonRes.unwrap());
                  } else {
                        log::error("Failed to parse JSON: {}", jsonRes.unwrapErr());
                        Notification::create("Failed to parse gauntlets data", NotificationIcon::Error)->show();
                  }
            } else {
                  log::error("Failed to fetch gauntlets: {}", response->string().unwrapOr("Unknown error"));
                  Notification::create("Failed to fetch gauntlets", NotificationIcon::Error)->show();
            }
      };
      request.get("https://gdrate.arcticwoof.xyz/getGauntlets").listen(m_gauntletsListener);
}

void RLGauntletSelectLayer::onGauntletsFetched(matjson::Value const& json) {
      m_loadingCircle->setVisible(false);
      createGauntletButtons(json);
}

void RLGauntletSelectLayer::createGauntletButtons(matjson::Value const& gauntlets) {
      auto winSize = CCDirector::sharedDirector()->getWinSize();

      m_gauntletsMenu = CCMenu::create();
      m_gauntletsMenu->setPosition({0, 0});
      this->addChild(m_gauntletsMenu);

      if (!gauntlets.isArray()) {
            log::error("Expected gauntlets to be an array");
            return;
      }

      auto gauntletsArray = gauntlets.asArray().unwrap();

      // Store gauntlets for later reference
      m_gauntletsArray.clear();
      m_gauntletButtons.clear();
      for (auto& g : gauntletsArray) {
            m_gauntletsArray.push_back(g);
      }

      const int maxButtonsPerRow = 3;  // kept for consistency
      const float buttonWidth = 110.0f;
      const float buttonHeight = 240.0f;
      const float spacingX = 30.0f;
      const float spacingY = 80.0f;

      float centerY = winSize.height / 2 - 20;

      for (size_t i = 0; i < gauntletsArray.size(); i++) {
            auto gauntlet = gauntletsArray[i];

            if (!gauntlet.isObject()) continue;

            int gauntletId = gauntlet["id"].asInt().unwrapOr(0);
            std::string gauntletName = gauntlet["name"].asString().unwrapOr("Unknown");

            auto gauntletBg = CCScale9Sprite::create("GJ_squareB_01.png");
            gauntletBg->setContentSize({110, 240});

            // name label with line breaks at spaces
            std::string formattedName = gauntletName;
            std::replace(formattedName.begin(), formattedName.end(), ' ', '\n');
            auto nameLabel = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
            auto nameLabelShadow = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
            nameLabel->setAlignment(kCCTextAlignmentCenter);
            nameLabel->setPosition({gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height - 15});
            nameLabel->setAnchorPoint({0.5f, 1.0f});
            nameLabel->setScale(0.5f);

            nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
            nameLabelShadow->setPosition({nameLabel->getPositionX() + 2, nameLabel->getPositionY() - 2});
            nameLabelShadow->setAnchorPoint({0.5f, 1.0f});
            nameLabelShadow->setColor({0, 0, 0});
            nameLabelShadow->setOpacity(60);
            nameLabelShadow->setScale(0.5f);

            gauntletBg->addChild(nameLabel, 3);
            gauntletBg->addChild(nameLabelShadow, 2);

            // difficulty range label (min-max) with star icon to the right
            int minDiff = gauntlet["min_difficulty"].asInt().unwrapOr(0);
            int maxDiff = gauntlet["max_difficulty"].asInt().unwrapOr(0);
            if (minDiff > 0 || maxDiff > 0) {
                  std::string diffText = fmt::format("{}-{}", minDiff, maxDiff);

                  auto diffLabel = CCLabelBMFont::create(diffText.c_str(), "bigFont.fnt");
                  auto diffLabelShadow = CCLabelBMFont::create(diffText.c_str(), "bigFont.fnt");
                  diffLabelShadow->setColor({0, 0, 0});
                  diffLabelShadow->setOpacity(60);
                  if (diffLabel) {
                        diffLabel->setAlignment(kCCTextAlignmentCenter);
                        diffLabel->setScale(0.45f);
                        diffLabel->setPosition({gauntletBg->getContentSize().width / 2 + 10, 50});
                        diffLabel->setAnchorPoint({1.f, 0.5f});
                        // shadow
                        diffLabelShadow->setAlignment(kCCTextAlignmentCenter);
                        diffLabelShadow->setAnchorPoint({1.f, 0.5f});
                        diffLabelShadow->setScale(0.45f);
                        diffLabelShadow->setPosition({diffLabel->getPositionX() + 2, diffLabel->getPositionY() - 2});

                        gauntletBg->addChild(diffLabel, 3);
                        gauntletBg->addChild(diffLabelShadow, 2);

                        // star icon to the right of the difficulty label
                        auto diffStar = CCSprite::create("RL_starSmall.png"_spr);
                        auto diffStarShadow = CCSprite::create("RL_starSmall.png"_spr);
                        if (diffStar && diffStarShadow) {
                              diffStar->setAnchorPoint({0.f, 0.5f});
                              diffStar->setPosition({gauntletBg->getContentSize().width / 2 + 15, diffLabel->getPositionY()});

                              diffStarShadow->setAnchorPoint({0.f, 0.5f});
                              diffStarShadow->setPosition({diffStar->getPositionX() + 2, diffStar->getPositionY() - 2});
                              diffStarShadow->setColor({0, 0, 0});
                              diffStarShadow->setOpacity(60);
                              gauntletBg->addChild(diffStarShadow, 2);
                              gauntletBg->addChild(diffStar, 3);
                        }
                  }
            }

            std::string spriteName = fmt::format("RL_gauntlet-{}.png"_spr, gauntletId);
            auto gauntletSprite = CCSprite::create(spriteName.c_str());
            auto gauntletSpriteShadow = CCSprite::create(spriteName.c_str());
            gauntletSpriteShadow->setColor({0, 0, 0});
            gauntletSpriteShadow->setOpacity(50);
            gauntletSpriteShadow->setScaleY(1.2f);

            gauntletBg->addChild(gauntletSprite, 3);
            gauntletBg->addChild(gauntletSpriteShadow, 2);
            gauntletSprite->setPosition(gauntletBg->getContentSize() / 2);
            gauntletSpriteShadow->setPosition({gauntletSprite->getPositionX(), gauntletSprite->getPositionY() - 6});

            auto button = CCMenuItemSpriteExtra::create(
                gauntletBg,
                this,
                menu_selector(RLGauntletSelectLayer::onGauntletButtonClick));

            // tag button with the gauntlet index for reliable lookup
            button->setTag(static_cast<int>(i));

            // add to menu but default hide
            m_gauntletButtons.push_back(button);
            m_gauntletsMenu->addChild(button);
            button->setVisible(false);
      }

      // setup pagination if needed
      int total = static_cast<int>(m_gauntletButtons.size());
      int totalPages = (total + m_pageSize - 1) / m_pageSize;
      // cleanup any existing page controls
      if (m_prevPageBtn) {
            m_prevPageBtn->removeFromParent();
            m_prevPageBtn = nullptr;
      }
      if (m_nextPageBtn) {
            m_nextPageBtn->removeFromParent();
            m_nextPageBtn = nullptr;
      }
      if (m_pageLabel) {
            m_pageLabel->removeFromParent();
            m_pageLabel = nullptr;
      }

      if (totalPages > 1) {
            // create prev/next buttons and page label
            auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            if (prevSpr) prevSpr->setFlipX(true);

            m_prevPageBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(RLGauntletSelectLayer::onPrevPage));
            m_nextPageBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(RLGauntletSelectLayer::onNextPage));

            // position below buttons
            m_prevPageBtn->setPosition({25, winSize.height / 2});
            m_nextPageBtn->setPosition({winSize.width - 25, winSize.height / 2});

            auto navMenu = CCMenu::create();
            navMenu->setPosition({0, 0});
            navMenu->addChild(m_prevPageBtn);
            navMenu->addChild(m_nextPageBtn);
            this->addChild(navMenu);

            m_pageLabel = CCLabelBMFont::create(fmt::format("{}/{}", m_currentPage + 1, totalPages).c_str(), "bigFont.fnt");
            m_pageLabel->setPosition({winSize.width / 2, 12});
            m_pageLabel->setScale(0.5f);
            this->addChild(m_pageLabel, 5);
      }

      // show first page
      m_currentPage = 0;
      updatePage();
}

void RLGauntletSelectLayer::onPrevPage(CCObject* sender) {
      int total = static_cast<int>(m_gauntletButtons.size());
      int totalPages = (total + m_pageSize - 1) / m_pageSize;
      if (totalPages <= 1) return;
      if (m_currentPage > 0) {
            m_currentPage--;
            updatePage();
      }
}

void RLGauntletSelectLayer::onNextPage(CCObject* sender) {
      int total = static_cast<int>(m_gauntletButtons.size());
      int totalPages = (total + m_pageSize - 1) / m_pageSize;
      if (totalPages <= 1) return;
      if (m_currentPage < totalPages - 1) {
            m_currentPage++;
            updatePage();
      }
}

void RLGauntletSelectLayer::updatePage() {
      auto winSize = CCDirector::sharedDirector()->getWinSize();
      const float buttonWidth = 110.0f;
      const float spacingX = 30.0f;
      float centerY = winSize.height / 2 - 20;

      int total = static_cast<int>(m_gauntletButtons.size());
      int totalPages = (total + m_pageSize - 1) / m_pageSize;

      // hide all by default
      for (size_t i = 0; i < m_gauntletButtons.size(); ++i) {
            if (m_gauntletButtons[i]) m_gauntletButtons[i]->setVisible(false);
      }

      if (total == 0) return;

      int start = m_currentPage * m_pageSize;
      int count = std::min(m_pageSize, total - start);

      float totalButtonsWidth = count * (buttonWidth + spacingX);
      float startX = (winSize.width - totalButtonsWidth) / 2 + buttonWidth / 2 + spacingX / 2;

      for (int p = 0; p < count; ++p) {
            int idx = start + p;
            if (idx < 0 || idx >= total) continue;
            auto btn = m_gauntletButtons[idx];
            if (!btn) continue;

            int col = p % m_pageSize;
            float buttonX = startX + col * (buttonWidth + spacingX);
            float buttonY = centerY;
            btn->setPosition({buttonX, buttonY});
            btn->setVisible(true);
      }

      // update nav controls
      if (m_pageLabel) {
            m_pageLabel->setString(fmt::format("{}/{}", m_currentPage + 1, std::max(1, totalPages)).c_str());
      }
      if (m_prevPageBtn) {
            m_prevPageBtn->setEnabled(m_currentPage > 0);
            m_prevPageBtn->setOpacity(m_currentPage > 0 ? 255 : 120);
      }
      if (m_nextPageBtn) {
            m_nextPageBtn->setEnabled(m_currentPage < totalPages - 1);
            m_nextPageBtn->setOpacity(m_currentPage < totalPages - 1 ? 255 : 120);
      }
}