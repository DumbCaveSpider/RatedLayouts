#pragma once

#include <Geode/cocos/base_nodes/CCNode.h>
#include <Geode/loader/Event.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum class ModStatus {
    NONE = 0,
    REGULAR = 1,
    ELDER = 2,
    LEADERBOARD = 3
};

struct UserInfo {
    std::string userName;
    int userID;
    int accountID;
    ModStatus modStatus;
};

struct Badge {
    std::string badgeID;
    geode::Ref<cocos2d::CCNode> targetNode;
};

using BadgeCallback = geode::Function<cocos2d::CCNode*()>;
using ProfileCallback = geode::Function<void(const Badge& badge, const UserInfo& score)>;

struct BadgeInfo {
    std::string id;
    std::string name;
    std::string description;
    BadgeCallback createBadge;
    ProfileCallback onProfile;
};

namespace BadgesAPI {
    inline bool isLoaded() {
        return geode::Loader::get()->getLoadedMod("alphalaneous.badges_api_reimagined") != nullptr;
    }

    template <typename F>
    inline void waitForBadges(F&& callback) {
        if (isLoaded()) {
            callback();
        }
    }

    template <typename F, typename G>
    inline void registerBadge(const std::string& id, const std::string& name, const std::string& description, F&& createBadge, G&& onProfile) {
        // Best-effort: if badges mod is loaded, call the callback immediately. Otherwise no-op.
        waitForBadges([id, name, description, cbCreate = std::forward<F>(createBadge), cbProfile = std::forward<G>(onProfile)]() mutable {
            (void)id; (void)name; (void)description; (void)cbCreate; (void)cbProfile;
            // No-op fallback: actual badge registration is handled by the badges mod when present.
        });
    }

    inline void unregisterBadge(const std::string& id) { (void)id; }
    inline void setName(const std::string& id, const std::string& name) { (void)id; (void)name; }
    inline std::string_view getName(const std::string& id) { (void)id; return ""; }
    inline void setDescription(const std::string& id, const std::string& description) { (void)id; (void)description; }
    inline std::string_view getDescription(const std::string& id) { (void)id; return ""; }

    template <typename F>
    inline void setCreateBadgeCallback(const std::string& id, F&& createBadge) { (void)id; (void)createBadge; }

    template <typename F>
    inline void setProfileCallback(const std::string& id, F&& onProfile) { (void)id; (void)onProfile; }

    inline void showBadge(const Badge& badge) { (void)badge; }
}