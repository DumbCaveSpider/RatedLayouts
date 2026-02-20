#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <algorithm>
#include <vector>

using namespace geode::prelude;

const int USER_COIN = 1329;
const ccColor3B BRONZE_COLOR = ccColor3B{255, 175, 75};
bool g_isRatedLayout = false;

// Replace coin visuals when GameObjects are set up
class $modify(EffectGameObject) {
  struct Fields {
    async::TaskHolder<web::WebResponse> m_fetchTask;
    bool m_isSuggested = false;
    bool m_coinVerified = false;
    ~Fields() { m_fetchTask.cancel(); }
  };

  void customSetup() {
    EffectGameObject::customSetup();

    if (this->m_objectID != USER_COIN)
      return;

    // avoid duplicate
    if (this->getChildByID("rl-blue-coin"))
      return;

    auto playLayer = PlayLayer::get();
    if (!playLayer || !playLayer->m_level ||
        playLayer->m_level->m_levelID == 0) {
      return;
    }

    int levelId = playLayer->m_level->m_levelID;
    this->m_addToNodeContainer = true;

    auto url =
        fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId);
    Ref<EffectGameObject> selfRef = this;
    auto fields = m_fields;
    m_fields->m_fetchTask.spawn(
        web::WebRequest().get(url),
        [this, selfRef, fields, levelId](web::WebResponse res) {
          if (!selfRef)
            return;

          if (!res.ok()) {
            log::debug("GameObjectCoin: fetch failed or non-ok for level {}",
                       levelId);
            return; // don't apply blue coin if server does not respond OK
          }

          auto jsonRes = res.json();
          if (!jsonRes) {
            log::debug("GameObjectCoin: invalid JSON response for level {}",
                       levelId);
            return;
          }

          auto json = jsonRes.unwrap();
          bool coinVerified = json["coinVerified"].asBool().unwrapOrDefault();

          // persist server-side flag for later checks and only proceed if true
          m_fields->m_coinVerified = coinVerified;
          if (!coinVerified) {
            return; // do not apply custom blue/empty animations
          }

          g_isRatedLayout = true;

          // coinVerified == true -> use RL_BlueCoin frames only (no fallback)
          CCAnimation *blueAnim = CCAnimation::create();
          CCSpriteFrame *firstFrame = nullptr;
          const char *blueNames[4] = {
              "RL_BlueCoin1.png"_spr, "RL_BlueCoin2.png"_spr,
              "RL_BlueCoin3.png"_spr, "RL_BlueCoin4.png"_spr};

          for (int fi = 0; fi < 4; ++fi) {
            // ONLY load RL frames directly; do not fallback to secret names or
            // raw addImage here
            CCSpriteFrame *f =
                CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                    blueNames[fi]);
            if (!f) {
              log::warn("GameObjectCoin: missing RL frame {} — skipping",
                        blueNames[fi]);
              continue;
            }

            if (!firstFrame)
              firstFrame = f;
            blueAnim->addSpriteFrame(f);
          }

          bool haveAnim =
              (blueAnim->getFrames() && blueAnim->getFrames()->count() > 0);
          if (!haveAnim) {
            log::warn("GameObjectCoin: no RL_BlueCoin frames available — "
                      "leaving default coin");
            return;
          }

          blueAnim->setDelayPerUnit(0.10f);

          // build empty animation (used when engine shows the "empty" secret
          // frame — e.g. coin already collected)
          CCAnimation *emptyAnim = CCAnimation::create();
          CCSpriteFrame *firstEmpty = nullptr;
          const char *emptyNames[4] = {
              "RL_BlueCoinEmpty1.png"_spr, "RL_BlueCoinEmpty2.png"_spr,
              "RL_BlueCoinEmpty3.png"_spr, "RL_BlueCoinEmpty4.png"_spr};
          for (int fi = 0; fi < 4; ++fi) {
            CCSpriteFrame *f =
                CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                    emptyNames[fi]);
            if (!f) {
              log::warn("GameObjectCoin: missing RL empty frame {} — skipping",
                        emptyNames[fi]);
              continue;
            }
            if (!firstEmpty)
              firstEmpty = f;
            emptyAnim->addSpriteFrame(f);
          }
          emptyAnim->setDelayPerUnit(0.10f);

          // helper: detect engine "empty" secret frames by pointer equality
          auto isEngineEmptyFrame = [](CCSprite *cs) -> bool {
            if (!cs)
              return false;
            auto curTex = cs->getTexture();
            auto curRect = cs->getTextureRect();
            if (!curTex)
              return false;

            const char *engineEmptyNames[4] = {
                "secretCoin_2_b_01_001.png", "secretCoin_2_b_01_002.png",
                "secretCoin_2_b_01_003.png", "secretCoin_2_b_01_004.png"};
            for (int i = 0; i < 4; ++i) {
              auto ef = CCSpriteFrameCache::sharedSpriteFrameCache()
                            ->spriteFrameByName(engineEmptyNames[i]);
              if (!ef)
                continue;
              if (ef->getTexture() == curTex) {
                // compare rects too for exact match
                if (ef->getRect().equals(curRect))
                  return true;
              }
            }
            return false;
          };

          auto applyBlueAnimTo = [blueAnim, firstFrame](CCSprite *cs) {
            if (!cs)
              return;
            cs->stopAllActions();

            cs->setDisplayFrame(firstFrame);
            auto animate = CCAnimate::create(blueAnim);
            cs->runAction(CCRepeatForever::create(animate));

            cs->setColor({255, 255, 255});
            cs->setID("rl-blue-coin");
          };

          auto applyEmptyAnimTo = [emptyAnim, firstEmpty](CCSprite *cs) {
            if (!cs)
              return;
            cs->stopAllActions();

            if (firstEmpty)
              cs->setDisplayFrame(firstEmpty);
            auto animate = CCAnimate::create(emptyAnim);
            cs->runAction(CCRepeatForever::create(animate));

            cs->setColor({255, 255, 255});
            cs->setID("rl-blue-coin");
          };

          // determine which of the three level coins this EffectGameObject
          // represents
          int coinIndexForThis = -1;

          std::vector<EffectGameObject *> userCoins;
          if (auto playLayer = PlayLayer::get()) {
            if (playLayer->m_objects) {
              for (unsigned int i = 0; i < playLayer->m_objects->count(); ++i) {
                auto obj = static_cast<CCNode *>(
                    playLayer->m_objects->objectAtIndex(i));
                auto eo = typeinfo_cast<EffectGameObject *>(obj);
                if (eo && eo->m_objectID == USER_COIN) {
                  userCoins.push_back(eo);
                }
              }
            }
          }

          // sort coins by their X position
          std::sort(userCoins.begin(), userCoins.end(),
                    [](EffectGameObject *a, EffectGameObject *b) {
                      return a->getPositionX() < b->getPositionX();
                    });

          for (size_t i = 0; i < userCoins.size(); ++i) {
            if (userCoins[i] == selfRef.operator->()) {
              coinIndexForThis = i + 1; // 1-indexed
              log::debug("GameObjectCoin: resolved coin index={} by position",
                         coinIndexForThis);
              break;
            }
          }

          if (coinIndexForThis == -1) {
            log::warn("GameObjectCoin: could not resolve coin index for this "
                      "instance; defaulting to 1");
            coinIndexForThis = 1;
          }

          // query collected state for the resolved coin index and apply exactly
          // once
          std::string coinKey =
              PlayLayer::get()->m_level->getCoinKey(coinIndexForThis);
          bool coinCollectedLocal =
              GameStatsManager::sharedState()->hasPendingUserCoin(
                  coinKey.c_str());

          if (auto asSprite = typeinfo_cast<CCSprite *>(selfRef.operator->())) {
            if (coinCollectedLocal) {
              log::debug("GameObjectCoin: applying EMPTY animation to "
                         "coinIndex={} (collectedLocal={})",
                         coinIndexForThis, coinCollectedLocal);
              applyEmptyAnimTo(asSprite);
            } else {
              log::debug("GameObjectCoin: applying BLUE animation to "
                         "coinIndex={} (collectedLocal={})",
                         coinIndexForThis, coinCollectedLocal);
              applyBlueAnimTo(asSprite);
            }
          }
        });
  }
};

class $modify(CCSprite) {
  void setDisplayFrame(CCSpriteFrame *newFrame) {
    if (this->getID() == "rl-blue-coin") {
      if (!newFrame) {
        auto frame =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                "RL_BlueCoin1.png"_spr);
        if (frame) {
          CCSprite::setDisplayFrame(frame);
        }
        return;
      }

      // allow the frame only if it matches one of the RL_BlueCoin or
      const char *rlNames[4] = {"RL_BlueCoin1.png"_spr, "RL_BlueCoin2.png"_spr,
                                "RL_BlueCoin3.png"_spr, "RL_BlueCoin4.png"_spr};
      const char *rlEmptyNames[4] = {
          "RL_BlueCoinEmpty1.png"_spr, "RL_BlueCoinEmpty2.png"_spr,
          "RL_BlueCoinEmpty3.png"_spr, "RL_BlueCoinEmpty4.png"_spr};
      for (int i = 0; i < 4; ++i) {
        auto rf =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                rlNames[i]);
        if (rf && rf->getTexture() == newFrame->getTexture()) {
          CCSprite::setDisplayFrame(newFrame);
          return;
        }
        auto ef =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                rlEmptyNames[i]);
        if (ef && ef->getTexture() == newFrame->getTexture()) {
          CCSprite::setDisplayFrame(newFrame);
          return;
        }
      }
      return;
    }
    CCSprite::setDisplayFrame(newFrame);
  }
};

class $modify(GameObject) {
  void playDestroyObjectAnim(GJBaseGameLayer *b) {
    if (g_isRatedLayout && this->m_objectID == USER_COIN) {
      log::debug("GameObjectCoin: playDestroyObjectAnim (rated layout) for {}",
                 this);

      // resolve which coin index this GameObject represents by sorting all
      // USER_COIN objects by their X position (closest to 0 == index 1)
      int coinIndexForThis = -1;
      std::vector<GameObject *> userCoins;
      if (auto playLayer = PlayLayer::get()) {
        if (playLayer->m_objects) {
          for (unsigned int i = 0; i < playLayer->m_objects->count(); ++i) {
            auto obj =
                static_cast<CCNode *>(playLayer->m_objects->objectAtIndex(i));
            if (auto go = typeinfo_cast<GameObject *>(obj)) {
              if (go->m_objectID == USER_COIN)
                userCoins.push_back(go);
            }
          }
        }
      }
      std::sort(userCoins.begin(), userCoins.end(),
                [](GameObject *a, GameObject *b) {
                  return a->getPositionX() < b->getPositionX();
                });
      for (size_t i = 0; i < userCoins.size(); ++i) {
        if (userCoins[i] == this) {
          coinIndexForThis = (int)i + 1;
          break;
        }
      }
      if (coinIndexForThis == -1)
        coinIndexForThis = 1;

      // determine local collected state for this coin (true -> EMPTY)
      bool coinCollectedLocal = false;
      if (auto pl = PlayLayer::get()) {
        std::string coinKey = pl->m_level->getCoinKey(coinIndexForThis);
        coinCollectedLocal =
            GameStatsManager::sharedState()->hasPendingUserCoin(
                coinKey.c_str());
      }

      log::debug("GameObjectCoin: coinIndexForThis={} collectedLocal={}",
                 coinIndexForThis, coinCollectedLocal);

      // snapshot direct children of the parent
      CCNode *parent = this->getParent();
      std::vector<CCNode *> beforeChildren;
      if (parent && parent->getChildren()) {
        auto ch = parent->getChildren();
        for (unsigned int i = 0; i < ch->count(); ++i)
          beforeChildren.push_back(static_cast<CCNode *>(ch->objectAtIndex(i)));
      }

      GameObject::playDestroyObjectAnim(b);

      if (parent && typeinfo_cast<CCNodeContainer *>(parent) &&
          parent->getChildren()) {
        auto ch = parent->getChildren();
        for (unsigned int i = 0; i < ch->count(); ++i) {
          auto node = static_cast<CCNode *>(ch->objectAtIndex(i));

          // skip nodes that existed before the call
          bool existed = false;
          for (auto &n : beforeChildren) {
            if (n == node) {
              existed = true;
              break;
            }
          }
          if (existed)
            continue;

          std::vector<CCNode *> stack{node};
          while (!stack.empty()) {
            auto cur = stack.back();
            stack.pop_back();
            if (!cur)
              continue;
            if (auto cs = typeinfo_cast<CCSprite *>(cur)) {
              log::debug("GameObjectCoin: tagged destroy-effect CCSprite {}",
                         static_cast<void *>(cs));

              // build and run the RL animation that matches collected state
              const char *namesBlue[4] = {"RL_BlueCoin1.png"_spr,
                                          "RL_BlueCoin2.png"_spr,
                                          "RL_BlueCoin3.png"_spr,
                                          "RL_BlueCoin4.png"_spr};
              const char *namesEmpty[4] = {"RL_BlueCoinEmpty1.png"_spr,
                                           "RL_BlueCoinEmpty2.png"_spr,
                                           "RL_BlueCoinEmpty3.png"_spr,
                                           "RL_BlueCoinEmpty4.png"_spr};

              CCAnimation *anim = CCAnimation::create();
              CCSpriteFrame *firstFrame = nullptr;
              auto &srcNames = coinCollectedLocal ? namesEmpty : namesBlue;
              for (int fi = 0; fi < 4; ++fi) {
                if (auto f = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(srcNames[fi])) {
                  if (!firstFrame) firstFrame = f;
                  anim->addSpriteFrame(f);
                }
              }

              if (anim->getFrames() && anim->getFrames()->count() > 0) {
                anim->setDelayPerUnit(0.10f);
                if (firstFrame) cs->setDisplayFrame(firstFrame);
                cs->runAction(CCRepeatForever::create(CCAnimate::create(anim)));
                cs->setColor({255, 255, 255});
                cs->setID("rl-blue-coin");
              }
            }
            if (auto cch = cur->getChildren()) {
              for (unsigned int j = 0; j < cch->count(); ++j)
                stack.push_back(static_cast<CCNode *>(cch->objectAtIndex(j)));
            }
          }
        }
      }

      return;
    }

    GameObject::playDestroyObjectAnim(b);
  }
};