#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
using namespace geode::prelude;

class RLModRatePopup : public geode::Popup {
    friend class RLModRatePayloadBuilder;
public:
    enum class PopupRole {
        Mod,
        Admin,
        Dev,
    };

    static RLModRatePopup* create(PopupRole role,
        std::string title = "Rate Layout",
        GJGameLevel* level = nullptr);
    ~RLModRatePopup() {
        m_getModLevelTask.cancel();
        m_setRateTask.cancel();
        m_setUnrateTask.cancel();
        m_setEventTask.cancel();
        m_deleteSendsTask.cancel();
        m_unsendTask.cancel();
    }

private:
    PopupRole m_role = PopupRole::Mod;
    std::string m_title;
    GJGameLevel* m_level;
    GJDifficultySprite* m_difficultySprite;
    CCMenuItemSpriteExtra* m_difficultyButton;
    int m_coinCycleState;
    bool m_isDemonMode;
    bool m_isFeatured;
    bool m_isEpicFeatured;
    bool m_isLegendary;
    CCMenuItemToggler* m_verifiedToggleItem;
    CCMenuItemToggler* m_silentToggleItem;
    CCMenu* m_normalButtonsContainer;
    CCMenu* m_demonButtonsContainer;
    CCNode* m_difficultyContainer;
    geode::TextInput* m_featuredScoreInput;
    geode::TextInput* m_featuredValueInput;
    geode::TextInput* m_difficultyInput;
    geode::TextInput* m_notesInput;
    int m_selectedRating;
    CCMenuItemSpriteExtra* m_submitButtonItem;
    bool m_isRejected;
    int m_levelId;
    std::string m_levelName;
    std::string m_creatorName;
    int m_accountId;
    int m_targetAccountId;
    async::TaskHolder<web::WebResponse> m_getModLevelTask;
    async::TaskHolder<web::WebResponse> m_setRateTask;
    async::TaskHolder<web::WebResponse> m_setUnrateTask;
    async::TaskHolder<web::WebResponse> m_setEventTask;
    async::TaskHolder<web::WebResponse> m_deleteSendsTask;
    async::TaskHolder<web::WebResponse> m_unsendTask;
    async::TaskHolder<web::WebResponse> m_banLevelTask;
    async::TaskHolder<web::WebResponse> m_unbanLevelTask;
    bool init() override;
    void setupBackground();
    void setupRatingButtons();
    void setupRoleToggleAndInfo();
    void setupModActionMenu();
    void setupDevControls();
    void setupDifficultyContainer();
    void setupNotesInput();

    bool prepareRatePayload(matjson::Value& outBody, UploadActionPopup* popup);
    bool prepareSuggestPayload(matjson::Value& outBody, UploadActionPopup* popup);

    void onRateButton(CCObject* sender);
    void onUnrateButton(CCObject* sender);
    void onSuggestButton(CCObject* sender);
    void onRejectButton(CCObject* sender);
    void onToggleFeatured(CCObject* sender);
    void onToggleDemon(CCObject* sender);
    void onRatingButton(CCObject* sender);
    void onInfoButton(CCObject* sender);
    void onDeleteSendsButton(CCObject* sender);
    void onBanLevelButton(CCObject* sender);
    void onUnbanLevelButton(CCObject* sender);
    void onUnsendButton(CCObject* sender);
    void onSetEventButton(CCObject* sender);
    void onToggleLegendary(CCObject* sender);
    void onToggleEpicFeatured(CCObject* sender);
    void onDifficultySpriteClicked(CCObject* sender);
    void updateDifficultySprite(int rating);
    void updateDifficultyCoinPreview();

    // helpers ig
    bool ensureToken(std::string &token, UploadActionPopup* popup, const char* errorMessage = "Token not found!");
    bool validateDifficultyOrRating(UploadActionPopup* popup);
    int determineFeaturedValue() const;
    void applyFeaturedScore(matjson::Value &outBody) const;
    void applyVerifiedFlag(matjson::Value& outBody) const;
    void applyDifficultyField(matjson::Value &outBody);
    void clearRejectState();
    void setSubmitButtonEnabled(bool enabled);
    void synchronizeCoinState();
    void removeDifficultyCoin(const char* id);
    void addDifficultyCoin(const char* spriteName, const char* id);
    void setRejectButtonVisible(bool visible);
};
