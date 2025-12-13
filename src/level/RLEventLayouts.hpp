#include <Geode/Geode.hpp>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

class RLEventLayouts : public geode::Popup<> {
     public:
      static RLEventLayouts* create();
      ~RLEventLayouts();
      void onDownloadCompleted(int id);
      void onDownloadFailed(int id);
      void restoreUIForLevel(int id);

     private:
      bool setup() override;
      void update(float dt) override;
      void onPlayEvent(CCObject* sender);
      void onCreatorClicked(CCObject* sender);
      void onInfo(CCObject* sender);

      struct EventSection {
            CCLayer* container = nullptr;
            GJDifficultySprite* diff = nullptr;
            CCLabelBMFont* timerLabel = nullptr;
            CCLabelBMFont* levelNameLabel = nullptr;
            CCLabelBMFont* creatorLabel = nullptr;
            CCMenuItem* creatorButton = nullptr;
            CCMenuItem* playButton = nullptr;
            CCLabelBMFont* difficultyValueLabel = nullptr;
            CCSprite* starIcon = nullptr;
            CCSprite* featuredIcon = nullptr;
            int accountId = -1;
            int levelId = -1;
            int featured = 0;
            time_t createdAt = 0;
            double secondsLeft = 0.0;
            LoadingSpinner* spinner = nullptr;
      };
      EventSection m_sections[3];
      CCLayer* m_eventMenu = nullptr;
      bool m_setupFinished = false;
      std::unordered_set<int> m_backgroundDownloads;   // prefetch/background downloads
};