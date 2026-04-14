#include <Geode/Geode.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include "Geode/utils/async.hpp"

using namespace geode::prelude;

class RLDonationPopup : public geode::Popup {
public:
    static RLDonationPopup* create();

private:
    bool init() override;
    void onClick(CCObject* sender);
    void onGetBadge(CCObject* sender);
    void onAccessBadge(CCObject* sender);

protected:
    UploadActionPopup* m_uploadPopup = nullptr;
    async::TaskHolder<Result<std::string>> m_authTask;
    async::TaskHolder<web::WebResponse> m_getAccessTask;

    ~RLDonationPopup() {
        m_authTask.cancel();
        m_getAccessTask.cancel();
    }
};
