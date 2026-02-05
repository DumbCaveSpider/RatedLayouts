#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

class RLGauntletSelectLayer : public CCLayer {
     public:
      static RLGauntletSelectLayer* create();

     protected:
      bool init() override;
      void onBackButton(CCObject* sender);
      void keyBackClicked() override;
      void fetchGauntlets();
      void onGauntletsFetched(matjson::Value const& json);
      void createGauntletButtons(matjson::Value const& gauntlets);
      void onGauntletButtonClick(CCObject* sender);
      void onInfoButton(CCObject* sender);

     private:
      // pagination helpers
      void onPrevPage(CCObject* sender);
      void onNextPage(CCObject* sender);
      void updatePage();

      LoadingSpinner* m_loadingCircle = nullptr;
      CCMenu* m_gauntletsMenu = nullptr;
      matjson::Value m_selectedGauntlet = matjson::Value();
      std::vector<matjson::Value> m_gauntletsArray;
      std::vector<CCMenuItemSpriteExtra*> m_gauntletButtons;
      int m_currentPage = 0;
      int m_pageSize = 3;
      CCMenuItemSpriteExtra* m_prevPageBtn = nullptr;
      CCMenuItemSpriteExtra* m_nextPageBtn = nullptr;
      CCLabelBMFont* m_pageLabel = nullptr;
      async::TaskHolder<web::WebResponse> m_gauntletsTask;
      ~RLGauntletSelectLayer() { m_gauntletsTask.cancel(); }
};