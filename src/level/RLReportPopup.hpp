#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLReportPopup : public geode::Popup<> {
     public:
      static RLReportPopup* create(int levelId);

     private:
      bool setup() override;
      void onSubmit(CCObject* sender);
      void onInfo(CCObject* sender);
      int m_levelId = 0;

      // toggle buttons for report reasons
      CCMenuItemToggler* m_plagiarismToggle = nullptr;
      CCMenuItemToggler* m_secretWayToggle = nullptr;
      CCMenuItemToggler* m_lowEffortToggle = nullptr;
      CCMenuItemToggler* m_unverifiedToggle = nullptr;
      CCMenuItemToggler* m_nsfwContentToggle = nullptr;
      CCMenuItemToggler* m_misrateToggle = nullptr;
      CCMenuItemToggler* m_decoratedToggle = nullptr;

      CCMenu* m_toggleMenu = nullptr;
      geode::TextInput* m_reasonInput = nullptr; 
};