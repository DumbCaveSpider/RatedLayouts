#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <algorithm>
#include <limits>
#include <vector>

using namespace geode::prelude;

const int USER_COIN = 1329;
const ccColor3B BRONZE_COLOR = ccColor3B{255, 175, 75};

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
    if (newFrame) {
      const char *engineEmptyNames[4] = {
          "secretCoin_2_b_01_001.png"_spr, "secretCoin_2_b_01_002.png"_spr,
          "secretCoin_2_b_01_003.png"_spr, "secretCoin_2_b_01_004.png"_spr};
      for (int i = 0; i < 4; ++i) {
        auto ef =
            CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                engineEmptyNames[i]);
        if (ef && ef == newFrame) {
          // only substitute in known "coin" contexts (PlayLayer or
          // EffectGameObject)
          for (auto node = static_cast<CCNode *>(this); node;
               node = node->getParent()) {
            if (typeinfo_cast<PlayLayer *>(node) ||
                typeinfo_cast<EffectGameObject *>(node)) {
              // build RL empty animation and apply it to this sprite
              CCAnimation *emptyAnim = CCAnimation::create();
              CCSpriteFrame *firstEmpty = nullptr;
              const char *emptyNames[4] = {
                  "RL_BlueCoinEmpty1.png"_spr, "RL_BlueCoinEmpty2.png"_spr,
                  "RL_BlueCoinEmpty3.png"_spr, "RL_BlueCoinEmpty4.png"_spr};
              for (int ei = 0; ei < 4; ++ei) {
                auto ef2 = CCSpriteFrameCache::sharedSpriteFrameCache()
                               ->spriteFrameByName(emptyNames[ei]);
                if (!ef2)
                  continue;
                if (!firstEmpty)
                  firstEmpty = ef2;
                emptyAnim->addSpriteFrame(ef2);
              }

              if (emptyAnim->getFrames() &&
                  emptyAnim->getFrames()->count() > 0) {
                emptyAnim->setDelayPerUnit(0.10f);
                this->stopAllActions();
                if (firstEmpty)
                  CCSprite::setDisplayFrame(firstEmpty);
                this->runAction(
                    CCRepeatForever::create(CCAnimate::create(emptyAnim)));
                this->setColor({255, 255, 255});
                this->setID("rl-blue-coin");
                return;
              }

              break; // matched context but no RL empty frames available
            }
          }
        }
      }
    }

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