#include <Geode/Geode.hpp>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

class RLEventLayouts : public geode::Popup<> {
     public:
      enum class EventType {
            Daily = 0,
            Weekly = 1,
            Monthly = 2,
      };

      static RLEventLayouts* create(EventType type);

     private:
      bool setup() override;
      void update(float dt) override;
      void onPlayEvent(CCObject* sender);
      void onCreatorClicked(CCObject* sender);
      void onInfo(CCObject* sender);
      void onSafeButton(CCObject* sender);

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

            // Platformer (planet) container and fields
            CCLayer* platContainer = nullptr;
            GJDifficultySprite* platDiff = nullptr;
            CCLabelBMFont* platLevelNameLabel = nullptr;
            CCLabelBMFont* platCreatorLabel = nullptr;
            CCMenuItem* platCreatorButton = nullptr;
            CCMenuItem* platPlayButton = nullptr;
            CCLabelBMFont* platDifficultyValueLabel = nullptr;
            CCSprite* platStarIcon = nullptr;
            CCSprite* platFeaturedIcon = nullptr;
            int platAccountId = -1;
            int platLevelId = -1;
            int platFeatured = 0;
            double platSecondsLeft = 0.0;

            // Pending request state
            std::string pendingKey;
            int pendingLevelId = -1;
            LoadingSpinner* pendingSpinner = nullptr;
            LoadingSpinner* playSpinner = nullptr;
            LoadingSpinner* platPlaySpinner = nullptr;
            double pendingTimeout = 0.0;  // seconds
      };

      EventSection m_sections[3];
      EventType m_eventType = EventType::Daily;
      CCLayer* m_eventMenu = nullptr;
      bool m_setupFinished = false;
};