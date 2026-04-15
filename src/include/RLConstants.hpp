#pragma once
#include <Geode/Geode.hpp>
#include <Geode/binding/GJAccountManager.hpp>

using namespace geode::prelude;

namespace rl {
    // Account ID of ArcticWoof and bonneville1
    constexpr int DEV_ACCOUNT_ID = 7689052;
    constexpr int NOVA_ACCOUNT_ID = 4882817;
    

    // Account ID of the backend developer.
    constexpr int BACKEND_DEV_ACCOUNT_ID = 4882817;

    // Base URL for all Rated Layouts API endpoints.
    constexpr std::string_view BASE_API_URL = "https://gdrate.arcticwoof.xyz";

    inline bool isUserHasPerms() {
        // check if user has any roles by checking saved values
        return Mod::get()->getSavedValue<bool>("isClassicMod") ||
               Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
               Mod::get()->getSavedValue<bool>("isLeaderboardMod") ||
               Mod::get()->getSavedValue<bool>("isLeaderboardAdmin") ||
               Mod::get()->getSavedValue<bool>("isPlatMod") ||
               Mod::get()->getSavedValue<bool>("isPlatAdmin");
    }

    // global admin/mod
    inline bool isUserAdmin() {
        return Mod::get()->getSavedValue<bool>("isClassicAdmin") ||
               Mod::get()->getSavedValue<bool>("isLeaderboardAdmin") ||
               Mod::get()->getSavedValue<bool>("isPlatAdmin");
    }

    inline bool isUserMod() {
        return Mod::get()->getSavedValue<bool>("isClassicMod") ||
               Mod::get()->getSavedValue<bool>("isLeaderboardMod") ||
               Mod::get()->getSavedValue<bool>("isPlatMod");
    }

    // specific admin/mod
    inline bool isUserClassicRole() {
        return Mod::get()->getSavedValue<bool>("isClassicAdmin") || Mod::get()->getSavedValue<bool>("isClassicMod");
    }

    inline bool isUserPlatformerRole() {
        return Mod::get()->getSavedValue<bool>("isPlatAdmin") || Mod::get()->getSavedValue<bool>("isPlatMod");
    }

    inline bool isUserLeaderboardRole() {
        return Mod::get()->getSavedValue<bool>("isLeaderboardAdmin") || Mod::get()->getSavedValue<bool>("isLeaderboardMod");
    }

    // supporter roles
    inline bool isUserSupporter() {
        return Mod::get()->getSavedValue<bool>("isSupporter") ||
               Mod::get()->getSavedValue<bool>("isBooster");
    }

    // check individual roles
    inline bool isUserOwner() {
        return GJAccountManager::sharedState()->m_accountID == DEV_ACCOUNT_ID ||
               GJAccountManager::sharedState()->m_accountID == NOVA_ACCOUNT_ID;
    }

    inline bool isUserClassicAdmin() {
        return Mod::get()->getSavedValue<bool>("isClassicAdmin");
    }

    inline bool isUserClassicMod() {
        return Mod::get()->getSavedValue<bool>("isClassicMod");
    }

    inline bool isUserPlatformerAdmin() {
        return Mod::get()->getSavedValue<bool>("isPlatAdmin");
    }

    inline bool isUserPlatformerMod() {
        return Mod::get()->getSavedValue<bool>("isPlatMod");
    }

    inline bool isUserLeaderboardAdmin() {
        return Mod::get()->getSavedValue<bool>("isLeaderboardAdmin");
    }

    inline bool isUserLeaderboardMod() {
        return Mod::get()->getSavedValue<bool>("isLeaderboardMod");
    }

    // FLAlertLayer of the role information
    inline void showDevInfo() {
        FLAlertLayer::create(
            "Rated Layouts Developer",
            "<cf>This user</c> is a <cl>developer</c> of <cl>Rated Layouts</c>, responsible for maintaining the <co>backend</c>, <cp>Discord bots</c>, and other infrastructure outside of the mod client.",
            "OK")
            ->show();
    }

    inline void showOwnerInfo() {
        FLAlertLayer::create(
            "Rated Layouts Owner",
            "<cf>ArcticWoof</c> is the creator and owner of <cl>Rated Layouts</c>. He is the main developer and maintainer of this <cp>Geode Mod</c> and has the ability to <cg>promote admins</c>.",
            "OK")
            ->show();
    }

    inline void showClassicAdminInfo() {
        FLAlertLayer::create(
            "Classic Layout Admin",
            "<cr>Classic Layout Admin</c> has the ability to <cy>rate classic levels</c>, <cb>suggest classic levels</c>, <cc>manage Featured Layouts</c> and <cg>promote users into Classic Layout Mod</c> for <cl>Rated Layouts</c>.",
            "OK")
            ->show();
    }

    inline void showClassicModInfo() {
        FLAlertLayer::create(
            "Classic Layout Mod",
            "<cb>Classic Layout Moderator</c> can suggest levels for classic layouts to <cr>Classic Layout Admins</c>.",
            "OK")
            ->show();
    }

    inline void showLeaderboardAdminInfo() {
        FLAlertLayer::create(
            "LB Layout Admin",
            "<cr>Leaderboard Layout Admin</c> has the same abilities as <cc>Leaderboard Layout Moderator</c>, but can also <cf>whitelist</c> users and <cg>promote users into Leaderboard Layout Mod</c>.",
            "OK")
            ->show();
    }

    inline void showLeaderboardModInfo() {
        FLAlertLayer::create(
            "LB Layout Mod",
            "<cb>Leaderboard Layout Moderator</c> is responsible for <co>managing and moderating the leaderboard</c> section of <cl>Rated Layouts</c>.",
            "OK")
            ->show();
    }

    inline void showPlatAdminInfo() {
        FLAlertLayer::create(
            "Platformer Layout Admin",
            "<cr>Platformer Layout Admin</c> has the ability to <cy>rate platformer levels</c>, <cb>suggest platformer levels</c>, <cc>manage Featured Layouts</c> and <cg>promote users into Platformer Layout Mod</c> for <cl>Rated Layouts</c>.",
            "OK")
            ->show();
    }

    inline void showPlatModInfo() {
        FLAlertLayer::create(
            "Platformer Layout Mod",
            "<cb>Platformer Layout Moderator</c> can suggest levels for platformer layouts to <cr>Platformer Layout Admins</c>.",
            "OK")
            ->show();
    }

    inline void showSupporterInfo() {
        FLAlertLayer::create(
            "Layout Supporter",
            "<cp>Layout Supporter</c> are those who donated to support the development of <cl>Rated Layouts</c>.",
            "OK")
            ->show();
    }

    inline void showBoosterInfo() {
        FLAlertLayer::create(
            "Layout Booster",
            "<ca>Layout Booster</c> are those who boosted the <cl>Rated "
            "Layouts Discord server</c>, they also have the same "
            "benefits as <cp>Layout Supporter</c>.",
            "OK")
            ->show();
    }
}  // namespace rl
