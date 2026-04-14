#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLGuideInfoPopup : public geode::Popup {
public:
    static RLGuideInfoPopup* create();

private:
    bool init() override;

    void onAbout(CCObject* sender);
    void onLayouts(CCObject* sender);
    void onStandards(CCObject* sender);
    void onOthers(CCObject* sender);
};