#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/EffectGameObject.hpp>

using namespace geode::prelude;

const int USER_COIN = 1329;
const ccColor3B BRONZE_COLOR = ccColor3B{255, 175, 75};

// Replace coin visuals when GameObjects are set up
class $modify(EffectGameObject) {
      struct Fields {
            utils::web::WebTask m_fetchTask;
            bool m_isSuggested = false;
            ~Fields() { m_fetchTask.cancel(); }
      };

      void customSetup() {
            EffectGameObject::customSetup();

            if (this->m_objectID != USER_COIN) return;

            // avoid duplicate
            if (this->getChildByID("rl-blue-coin")) return;

            auto playLayer = PlayLayer::get();
            if (!playLayer || !playLayer->m_level || playLayer->m_level->m_levelID == 0) {
                  return;
            }

            int levelId = playLayer->m_level->m_levelID;

            auto url = fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId);
            Ref<EffectGameObject> selfRef = this;
            m_fields->m_fetchTask = web::WebRequest().get(url);
            auto fields = m_fields;
            m_fields->m_fetchTask.listen([selfRef, fields, levelId](web::WebResponse* res) {
                  if (!selfRef) return;

                  if (!res || !res->ok()) {
                        log::debug("GameObjectCoin: fetch failed or non-ok for level {}", levelId);
                        return;  // don't apply blue coin if server does not respond OK
                  }

                  auto jsonRes = res->json();
                  if (!jsonRes) {
                        log::debug("GameObjectCoin: invalid JSON response for level {}", levelId);
                        return;
                  }

                  auto json = jsonRes.unwrap();
                  bool isSuggested = json["isSuggested"].asBool().unwrapOrDefault();
                  auto tag = isSuggested ? 69 : 67;  // idk what to name the tags lol

                  if (auto asSprite = typeinfo_cast<CCSprite*>(selfRef.operator->())) {
                        asSprite->setTag(tag);
                        if (!isSuggested) asSprite->setColor({0, 127, 232});
                  }
                  if (auto children = selfRef->getChildren()) {
                        for (auto node : CCArrayExt<CCNode>(children)) {
                              if (auto cs = typeinfo_cast<CCSprite*>(node)) {
                                    cs->setTag(tag);
                                    if (!isSuggested) cs->setColor({0, 127, 232});
                              }
                        }
                  }

                  // btw this is only just makes the coin blue but still uses the default coin sprite.
                  // i still dont know how to replace the actual sprite image lmao

                  selfRef->m_addToNodeContainer = true;
            });
      }
};

class $modify(CCSprite) {
      void setColor(const ccColor3B& color) {
            if (color == BRONZE_COLOR) {
                  int tag = this->getTag();
                  if (tag == 67) {
                        CCSprite::setColor({0, 127, 232});
                        return;
                  }
                  if (tag == 69) {
                        CCSprite::setColor(color);
                        return;
                  }
            }

            CCSprite::setColor(color);
      }
};