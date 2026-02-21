#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <algorithm>
#include <vector>

using namespace geode::prelude;

const int USER_COIN = 1329;

// callback receives `true` only if request succeeded and coinVerified was true
static void fetchCoinVerified(int levelId, std::function<void(bool)> cb) {
    static async::TaskHolder<web::WebResponse> s_task;
    // cancel any previous request for freshness
    s_task.cancel();
    auto url = fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId);
    s_task.spawn(web::WebRequest().get(url), [cb](web::WebResponse res) {
        if (!res.ok()) {
            cb(false);
            return;
        }
        auto jsonRes = res.json();
        if (!jsonRes) {
            cb(false);
            return;
        }
        bool coinVerified = jsonRes.unwrap()["coinVerified"].asBool().unwrapOr(false);
        cb(coinVerified);
    });
}

// Replace coin visuals when GameObjects are set up
class $modify(EffectGameObject) {
  void customSetup() {
    EffectGameObject::customSetup();

    if (this->m_objectID != USER_COIN)
      return;

    // avoid duplicate from prior runs
    if (this->getChildByID("rl-blue-coin"))
      return;

    auto playLayer = PlayLayer::get();
    if (!playLayer || !playLayer->m_level || playLayer->m_level->m_levelID == 0) {
      return;
    }
    // ensure the custom sprite animations are added to the node container
    this->m_addToNodeContainer = true;

    // prepare animations once (they're static and reused by the async callback)
    static CCAnimation *blueAnim = nullptr;
    static CCSpriteFrame *blueFirst = nullptr;
    static CCAnimation *emptyAnim = nullptr;
    static CCSpriteFrame *emptyFirst = nullptr;
    if (!blueAnim) {
      blueAnim = CCAnimation::create();
      const char *blueNames[4] = {"RL_BlueCoin1.png"_spr,
                                  "RL_BlueCoin2.png"_spr,
                                  "RL_BlueCoin3.png"_spr,
                                  "RL_BlueCoin4.png"_spr};
      for (int fi = 0; fi < 4; ++fi) {
        auto f = CCSpriteFrameCache::sharedSpriteFrameCache()
                     ->spriteFrameByName(blueNames[fi]);
        if (!f) {
          log::warn("GameObjectCoin: missing RL frame {} — skipping",
                    blueNames[fi]);
          continue;
        }
        if (!blueFirst)
          blueFirst = f;
        blueAnim->addSpriteFrame(f);
      }
      blueAnim->setDelayPerUnit(0.10f);
      blueAnim->retain();
    }
    if (!emptyAnim) {
      emptyAnim = CCAnimation::create();
      const char *emptyNames[4] = {"RL_BlueCoinEmpty1.png"_spr,
                                   "RL_BlueCoinEmpty2.png"_spr,
                                   "RL_BlueCoinEmpty3.png"_spr,
                                   "RL_BlueCoinEmpty4.png"_spr};
      for (int fi = 0; fi < 4; ++fi) {
        auto f = CCSpriteFrameCache::sharedSpriteFrameCache()
                     ->spriteFrameByName(emptyNames[fi]);
        if (!f) {
          log::warn("GameObjectCoin: missing RL empty frame {} — skipping",
                    emptyNames[fi]);
          continue;
        }
        if (!emptyFirst)
          emptyFirst = f;
        emptyAnim->addSpriteFrame(f);
      }
      emptyAnim->setDelayPerUnit(0.10f);
      emptyAnim->retain();
    }

    auto applyBlueAnimTo = [](CCSprite *cs) {
      if (!cs)
        return;
      cs->stopAllActions();
      if (blueFirst)
        cs->setDisplayFrame(blueFirst);
      cs->runAction(CCRepeatForever::create(CCAnimate::create(blueAnim)));
      cs->setColor({255, 255, 255});
      cs->setID("rl-blue-coin");
    };
    auto applyEmptyAnimTo = [](CCSprite *cs) {
      if (!cs)
        return;
      cs->stopAllActions();
      if (emptyFirst)
        cs->setDisplayFrame(emptyFirst);
      cs->runAction(CCRepeatForever::create(CCAnimate::create(emptyAnim)));
      cs->setColor({255, 255, 255});
      cs->setID("rl-blue-coin");
    };

    // perform remote check, then apply animation only if verified
    int levelId = playLayer->m_level->m_levelID;
    Ref<EffectGameObject> selfRef = this;
    fetchCoinVerified(levelId, [selfRef, applyBlueAnimTo, applyEmptyAnimTo](bool coinVerified) {
      if (!selfRef || !coinVerified)
        return;

      auto playLayer2 = PlayLayer::get();
      if (!playLayer2 || !playLayer2->m_level)
        return;

      // gather, sort, and style every user coin in the level
      std::vector<EffectGameObject *> userCoins;
      if (playLayer2->m_objects) {
        for (unsigned int i = 0; i < playLayer2->m_objects->count(); ++i) {
          auto obj = static_cast<CCNode *>(
              playLayer2->m_objects->objectAtIndex(i));
          auto eo = typeinfo_cast<EffectGameObject *>(obj);
          if (eo && eo->m_objectID == USER_COIN) {
            userCoins.push_back(eo);
          }
        }
      }
      std::sort(userCoins.begin(), userCoins.end(),
                [](EffectGameObject *a, EffectGameObject *b) {
                  return std::abs(a->getPositionX()) <
                         std::abs(b->getPositionX());
                });

      for (size_t i = 0; i < userCoins.size(); ++i) {
        int index = static_cast<int>(i + 1);
        std::string coinKey = playLayer2->m_level->getCoinKey(index);
        bool collected =
            GameStatsManager::sharedState()->hasPendingUserCoin(coinKey.c_str());
        if (auto cs = typeinfo_cast<CCSprite *>(userCoins[i])) {
          if (collected)
            applyEmptyAnimTo(cs);
          else
            applyBlueAnimTo(cs);
        }
      }
    });

    return;
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
    if (this->m_objectID == USER_COIN) {
      // determine index by sorting all user coins by distance to x=0
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
                  return std::abs(a->getPositionX()) <
                         std::abs(b->getPositionX());
                });
      for (size_t i = 0; i < userCoins.size(); ++i) {
        if (userCoins[i] == this) {
          coinIndexForThis = static_cast<int>(i + 1);
          break;
        }
      }
      if (coinIndexForThis == -1)
        coinIndexForThis = 1;

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
              // build and run the RL animation that matches collected state
              const char *namesBlue[4] = {
                  "RL_BlueCoin1.png"_spr, "RL_BlueCoin2.png"_spr,
                  "RL_BlueCoin3.png"_spr, "RL_BlueCoin4.png"_spr};
              const char *namesEmpty[4] = {
                  "RL_BlueCoinEmpty1.png"_spr, "RL_BlueCoinEmpty2.png"_spr,
                  "RL_BlueCoinEmpty3.png"_spr, "RL_BlueCoinEmpty4.png"_spr};

              CCAnimation *anim = CCAnimation::create();
              CCSpriteFrame *firstFrame = nullptr;
              auto &srcNames = coinCollectedLocal ? namesEmpty : namesBlue;
              for (int fi = 0; fi < 4; ++fi) {
                if (auto f = CCSpriteFrameCache::sharedSpriteFrameCache()
                                 ->spriteFrameByName(srcNames[fi])) {
                  if (!firstFrame)
                    firstFrame = f;
                  anim->addSpriteFrame(f);
                }
              }

              if (anim->getFrames() && anim->getFrames()->count() > 0) {
                anim->setDelayPerUnit(0.05f);
                if (firstFrame)
                  cs->setDisplayFrame(firstFrame);
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