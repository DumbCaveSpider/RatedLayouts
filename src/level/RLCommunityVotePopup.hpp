#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLCommunityVotePopup : public geode::Popup {
     public:
      static RLCommunityVotePopup* create();
      static RLCommunityVotePopup* create(int levelId);

     private:
      bool init() override;

      // handlers
      void onSubmit(CCObject*);
      void onInfo(CCObject*);

      int m_levelId = 0;
      int m_originalityVote = 0;
      int m_difficultyVote = 0;
      int m_gameplayVote = 0;

      CCLabelBMFont* m_originalityScoreLabel = nullptr;
      CCLabelBMFont* m_difficultyScoreLabel = nullptr;
      CCLabelBMFont* m_gameplayScoreLabel = nullptr;
      CCLabelBMFont* m_modDifficultyLabel = nullptr;

      geode::TextInput* m_originalityInput = nullptr;
      geode::TextInput* m_difficultyInput = nullptr;
      geode::TextInput* m_gameplayInput = nullptr;

      CCMenuItemSpriteExtra* m_submitBtn = nullptr;
      CCMenuItemSpriteExtra* m_toggleAllBtn = nullptr;
      bool m_forceShowScores = false;
      CCLabelBMFont* m_totalVotesLabel = nullptr;

      void onToggleAll(CCObject* sender);
      void refreshFromServer();

      async::TaskHolder<web::WebResponse> m_getVoteTask;
      async::TaskHolder<web::WebResponse> m_submitVoteTask;
      ~RLCommunityVotePopup() {
            m_getVoteTask.cancel();
            m_submitVoteTask.cancel();
      }
};