#include <Geode/Geode.hpp>
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"

using namespace geode::prelude;

class RLSecretLayer1 : public cocos2d::CCLayer {
private:
    bool init() override;
    void keyBackClicked() override;
    void onEnterTransitionDidFinish() override;
    void onExitTransitionDidStart() override;

    TextInput* m_rewardInput = nullptr;
    LoadingSpinner* m_spinner = nullptr;
    bool m_isRedeeming = false;
    std::string m_redeemCode;

    CCParticleSystemQuad* m_oracleParticle = nullptr;
    CCParticleSystemQuad* m_crystalParticle = nullptr;

    void onRedeem(CCObject* sender);
    void startRedeemRequest();
    void finishRedeem();

    void hideRewardInput();
    void cleanupSpinner();

    CCSprite* m_oracleSpr = nullptr;
    CCMenuItemSpriteExtra* m_oracleBtn = nullptr;
    CCLabelBMFont* m_textLabel = nullptr;

public:
    static RLSecretLayer1* create();
};
