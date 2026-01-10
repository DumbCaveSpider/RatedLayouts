#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

const int USER_COIN = 1329;

// Replace coin visuals when GameObjects are set up
class $modify(GameObject) {
      struct Fields { utils::web::WebTask m_fetchTask; ~Fields() { m_fetchTask.cancel(); } };

      void customSetup() {
            GameObject::customSetup();

            if (this->m_objectID != USER_COIN) return;

            // avoid duplicate
            if (this->getChildByID("rl-blue-coin")) return;

            auto playLayer = PlayLayer::get();
            if (!playLayer || !playLayer->m_level || playLayer->m_level->m_levelID == 0) {
                  return;
            }

            int levelId = playLayer->m_level->m_levelID;

            // fetch level info from server; if isSuggested == true OR request fails, DO NOT apply blue-coin visuals
            auto url = fmt::format("https://gdrate.arcticwoof.xyz/fetch?levelId={}", levelId);
            Ref<GameObject> selfRef = this;
            m_fields->m_fetchTask = web::WebRequest().get(url);
            m_fields->m_fetchTask.listen([selfRef, levelId](web::WebResponse* res) {
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
                  if (isSuggested) {
                        log::debug("skipping replacement", levelId);
                        return;
                  }

                  // TODO: change the coin gameobject sprite to blue coin with animation pls

                  selfRef->m_addToNodeContainer = true;
            });
      }
};
