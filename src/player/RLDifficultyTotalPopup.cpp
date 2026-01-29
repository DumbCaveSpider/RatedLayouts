#include "RLDifficultyTotalPopup.hpp"
#include "../custom/RLAchievements.hpp"

#include <unordered_map>
using namespace geode::prelude;

static int rl_mapRatingToLevel_Stars(int rating) {
      switch (rating) {
            case 1:
                  return -1;
            case 2:
                  return 1;
            case 3:
                  return 2;
            case 4:
            case 5:
                  return 3;
            case 6:
            case 7:
                  return 4;
            case 8:
            case 9:
                  return 5;
            case 10:
                  return 7;
            case 15:
                  return 8;
            case 20:
                  return 6;
            case 25:
                  return 9;
            case 30:
                  return 10;
            default:
                  return 0;
      }
}

void RLDifficultyTotalPopup::buildDifficultyUI(const std::unordered_map<int, int>& counts) {
      if (!m_facesContainer) return;
      // clear previous
      m_facesContainer->removeAllChildrenWithCleanup(true);
      for (auto lbl : m_countLabels) {
            if (lbl) lbl->removeFromParent();
      }
      m_countLabels.clear();
      m_difficultySprites.clear();

      // determine groups
      if (!m_demonModeActive) {
            std::vector<std::vector<int>> groups = {{1}, {2}, {3}, {4, 5}, {6, 7}, {8, 9}};
            for (auto& group : groups) {
                  int total = 0;
                  for (int r : group) {
                        auto it = counts.find(r);
                        if (it != counts.end()) total += it->second;
                  }
                  int rep = group[0];
                  int difficultyLevel = rl_mapRatingToLevel_Stars(rep);
                  auto sprite = GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Long);
                  if (!sprite) {
                        sprite = GJDifficultySprite::create(0, GJDifficultyName::Long);
                  }
                  sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);
                  sprite->setColor({255, 255, 255});
                  sprite->setScale(1.0f);
                  m_difficultySprites.push_back(sprite);

                  auto countLabel = CCLabelBMFont::create(numToString(total).c_str(), "goldFont.fnt");
                  countLabel->setScale(0.6f);
                  countLabel->setAnchorPoint({0.5f, 0.5f});
                  m_countLabels.push_back(countLabel);

                  // menu with row layout to hold sprite and count label horizontally
                  auto menu = CCMenu::create();
                  menu->setLayout(RowLayout::create()->setGap(25.f)->setGrowCrossAxis(false));
                  menu->setPosition({0, 0});
                  sprite->setTag(rep);
                  menu->addChild(sprite);
                  // add label as simple node child so row layout arranges it alongside the sprite
                  menu->addChild(countLabel);
                  menu->setContentSize({40.f, 45.f});
                  menu->setAnchorPoint({0.5f, 0.5f});

                  countLabel->setPosition({menu->getContentSize().width / 2.f, -15.f});
                  sprite->setPosition({menu->getContentSize().width / 2.f, menu->getContentSize().height / 2.f});

                  m_facesContainer->addChild(menu);
            }
      } else {
            std::vector<int> demonRatings = {10, 15, 20, 25, 30};
            for (int r : demonRatings) {
                  int total = 0;
                  auto it = counts.find(r);
                  if (it != counts.end()) total = it->second;
                  int difficultyLevel = rl_mapRatingToLevel_Stars(r);
                  auto sprite = GJDifficultySprite::create(difficultyLevel, GJDifficultyName::Long);
                  if (!sprite) {
                        sprite = GJDifficultySprite::create(0, GJDifficultyName::Long);
                  }
                  sprite->updateDifficultyFrame(difficultyLevel, GJDifficultyName::Long);
                  sprite->setColor({255, 255, 255});
                  sprite->setScale(1.0f);
                  m_difficultySprites.push_back(sprite);

                  auto countLabel = CCLabelBMFont::create(numToString(GameToolbox::pointsToString(total)).c_str(), "goldFont.fnt");
                  countLabel->setScale(0.6f);
                  countLabel->setAnchorPoint({0.5f, 0.5f});
                  m_countLabels.push_back(countLabel);

                  // menu with row layout to hold sprite and count label horizontally
                  auto menu = CCMenu::create();
                  menu->setLayout(RowLayout::create()->setGap(25.f)->setGrowCrossAxis(false));
                  menu->setPosition({0, 0});
                  sprite->setTag(r);
                  menu->addChild(sprite);
                  // add label as simple node child so row layout arranges it alongside the sprite
                  menu->addChild(countLabel);
                  menu->setContentSize({40.f, 45.f});
                  menu->setAnchorPoint({0.5f, 0.5f});

                  countLabel->setPosition({menu->getContentSize().width / 2.f, -20.f});
                  sprite->setPosition({menu->getContentSize().width / 2.f, menu->getContentSize().height / 2.f});

                  m_facesContainer->addChild(menu);
            }
      }
      // layout update
      if (m_facesContainer) m_facesContainer->updateLayout();
}

void RLDifficultyTotalPopup::onDemonToggle(CCObject* sender) {
      auto toggler = static_cast<CCMenuItemToggler*>(sender);
      if (!toggler) return;
      m_demonModeActive = !m_demonModeActive;
      buildDifficultyUI(m_counts);
}
RLDifficultyTotalPopup* RLDifficultyTotalPopup::create(int accountId, Mode mode) {
      auto ret = new RLDifficultyTotalPopup();
      ret->m_accountId = accountId;
      ret->m_mode = mode;

      if (ret && ret->initAnchored(380.f, 210.f, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      }

      delete ret;
      return nullptr;
};

bool RLDifficultyTotalPopup::setup() {
      setTitle("Rated Layouts Classic: -");
      auto contentSize = m_mainLayer->getContentSize();
      m_noElasticity = true;
      // spinner
      auto spinner = LoadingSpinner::create(36.f);
      spinner->setPosition({contentSize.width / 2.f, contentSize.height / 2.f});
      m_mainLayer->addChild(spinner);
      m_spinner = spinner;

      // remove close button
      if (auto closeBtn = this->m_closeBtn) {
            closeBtn->removeFromParent();
      }

      // ok (close)
      auto closeBtnSpr = ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png");
      auto closeBtn = CCMenuItemSpriteExtra::create(closeBtnSpr, this, menu_selector(RLDifficultyTotalPopup::onClose));
      closeBtn->setPosition({contentSize.width / 2.f, 25.f});
      m_buttonMenu->addChild(closeBtn);
      // demon toggle
      auto demonOff = CCSpriteGrayscale::createWithSpriteFrameName("GJ_demonIcon_001.png");
      auto demonOn = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
      auto demonToggle = CCMenuItemToggler::create(demonOff, demonOn, this, menu_selector(RLDifficultyTotalPopup::onDemonToggle));
      demonToggle->setScale(1.0f);
      demonToggle->setPosition({contentSize.width - 25.f, contentSize.height - 25.f});
      m_buttonMenu->addChild(demonToggle);

      m_rankLabel = CCLabelBMFont::create("-", "goldFont.fnt");
      m_rankLabel->setScale(0.4f);
      m_rankLabel->setAnchorPoint({1.f, .5f});
      m_rankLabel->setPosition({contentSize.width - 10.f, 15.f});
      m_mainLayer->addChild(m_rankLabel);

      m_coinRankLabel = CCLabelBMFont::create("-", "goldFont.fnt");
      m_coinRankLabel->setScale(0.4f);
      m_coinRankLabel->setAnchorPoint({1.f, .5f});
      m_coinRankLabel->setPosition({contentSize.width - 10.f, 35.f});
      m_coinRankLabel->setVisible(false);
      m_mainLayer->addChild(m_coinRankLabel);

      if (m_mode == Mode::Planets) {
            setTitle("Rated Layouts Platformer: -");
      } else {
            setTitle("Rated Layouts Classic: -");
      }

      m_facesContainer = CCMenu::create();
      auto facesLayout = RowLayout::create();
      facesLayout->setGap(15.f)->setGrowCrossAxis(true)->setCrossAxisOverflow(false)->setAxisAlignment(AxisAlignment::Center);
      m_facesLayout = facesLayout;
      m_facesContainer->setLayout(m_facesLayout);
      m_facesContainer->setContentSize({contentSize.width - 40.f, contentSize.height - 120.f});
      m_facesContainer->setAnchorPoint({0.5f, 0.5f});
      m_facesContainer->setPosition({contentSize.width / 2.f, contentSize.height / 2.f + 10.f});
      m_mainLayer->addChild(m_facesContainer);

      auto req = web::WebRequest();
      req.param("accountid", numToString(m_accountId));
      if (m_mode == RLDifficultyTotalPopup::Mode::Planets) {
            req.param("isPlat", "1");
      }
      Ref<RLDifficultyTotalPopup> self = this;
      m_difficultyTask = req.get("https://gdrate.arcticwoof.xyz/getDifficulty");
      m_difficultyTask.listen([self](web::WebResponse* res) {
            if (!self) return;
            if (!res || !res->ok()) {
                  if (self->m_spinner) {
                        self->m_spinner->removeFromParent();
                        self->m_spinner = nullptr;
                  }
                  Notification::create("Failed to fetch difficulty", NotificationIcon::Error)->show();
                  return;
            }
            auto jsonRes = res->json();
            if (!jsonRes) {
                  Notification::create("Failed to parse difficulty response", NotificationIcon::Error)->show();
                  if (self->m_spinner) {
                        self->m_spinner->removeFromParent();
                        self->m_spinner = nullptr;
                  }
                  return;
            }
            auto json = jsonRes.unwrap();
            if (self->m_spinner) {
                  self->m_spinner->removeFromParent();
                  self->m_spinner = nullptr;
            }
            std::unordered_map<int, int> counts;
            auto difficultyObj = json["difficulty"];
            std::vector<int> keys = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30};
            for (int k : keys) {
                  counts[k] = difficultyObj[numToString(k)].asInt().unwrapOrDefault();
            }
            self->m_counts = counts;
            int totalCount = 0;
            for (auto const& kv : counts) {
                  totalCount += kv.second;
            }
            // player's rank
            int position = json["position"].asInt().unwrapOrDefault();
            int coinRank = json["coinRank"].asInt().unwrapOrDefault();
            std::string titlePrefix = self->m_mode == RLDifficultyTotalPopup::Mode::Planets ? "Rated Layouts Platformer: " : "Rated Layouts Classic: ";
            self->setTitle((titlePrefix + numToString(GameToolbox::pointsToString(totalCount))).c_str());
            if (self->m_rankLabel) {
                  if (position > 0) {
                        self->m_rankLabel->setString((std::string("Global Rank: ") + numToString(position)).c_str());
                        self->m_rankLabel->setVisible(true);
                  } else {
                        self->m_rankLabel->setVisible(false);
                  }
            }
            // coin rank
            if (self->m_coinRankLabel) {
                  if (coinRank > 0) {
                        self->m_coinRankLabel->setString((std::string("Coin Rank: ") + numToString(coinRank)).c_str());
                        self->m_coinRankLabel->setVisible(true);
                  } else {
                        self->m_coinRankLabel->setVisible(false);
                  }
            }

            // icheck for extreme demon achievement
            if (self->m_accountId == GJAccountManager::get()->m_accountID) {
                  if (counts.find(30) != counts.end() && counts[30] > 0) {
                        RLAchievements::onReward("misc_extreme");
                  }
            }
            // build UI
            self->buildDifficultyUI(self->m_counts);
      });
      return true;
}