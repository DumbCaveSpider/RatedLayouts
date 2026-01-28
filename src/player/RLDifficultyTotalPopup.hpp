#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLDifficultyTotalPopup : public geode::Popup<> {
     public:
      enum class Mode {
            Stars = 0,
            Planets = 1,
      };

      static RLDifficultyTotalPopup* create(int accountId, Mode mode = Mode::Stars);

     private:
      bool setup() override;
      int m_accountId = 0;
      Mode m_mode = Mode::Stars;
      CCLabelBMFont* m_resultsLabel = nullptr;
      CCLabelBMFont* m_rankLabel = nullptr;
      CCLabelBMFont* m_coinRankLabel = nullptr;
      LoadingSpinner* m_spinner = nullptr;
      CCMenu* m_facesContainer = nullptr;
      RowLayout* m_facesLayout = nullptr;
      std::vector<CCLabelBMFont*> m_countLabels;
      std::vector<GJDifficultySprite*> m_difficultySprites;

      bool m_demonModeActive = false;
      utils::web::WebTask m_difficultyTask;
      void buildDifficultyUI(const std::unordered_map<int, int>& counts);
      void onDemonToggle(CCObject* sender);
      std::unordered_map<int, int> m_counts;
      ~RLDifficultyTotalPopup() {
            m_difficultyTask.cancel();
      }
};