#include "BadgesRegistry.hpp"
#include <Geode/Geode.hpp>

std::unordered_map<int, std::vector<Badge>> &BadgesRegistry::pending()
{
      static std::unordered_map<int, std::vector<Badge>> g_pendingBadges;
      return g_pendingBadges;
}

using namespace geode::prelude;

$execute
{
      log::info("executed badges");
      BadgesAPI::registerBadge(
          "rl-mod-badge",
          "Rated Layouts Moderator",
          "This user can <cj>suggest layout levels</c> for <cl>Rated "
          "Layouts</c> to the <cr>Layout Admins</c>. They have the ability to <co>moderate the leaderboard</c>.",
          []
          {
                return CCSprite::create("RL_badgeMod01.png"_spr);
          },
          [](const Badge &badge, const UserInfo &user)
          {
                BadgesRegistry::pending()[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-admin-badge",
          "Rated Layouts Admin",
          "This user can <cj>rate layout levels</c> for <cl>Rated "
          "Layouts</c>. They have the same power as <cg>Moderators</c> but including the ability to change the <cy>featured ranking on the "
          "featured layout levels</c> and <cg>set event layouts</c>.",
          []
          {
                return CCSprite::create("RL_badgeAdmin01.png"_spr);
          },
          [](const Badge &badge, const UserInfo &user)
          {
                BadgesRegistry::pending()[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-owner-badge",
          "Rated Layouts Owner",
          "<cf>ArcticWoof</c> is the <ca>Owner and Developer</c> of <cl>Rated Layouts</c> Geode Mod.\nHe controls and manages everything within <cl>Rated Layouts</c>, including updates and adding new features as well as the ability to <cg>promote users to Layout Moderators or Administrators</c>.",
          []
          {
                return CCSprite::create("RL_badgeOwner.png"_spr);
          },
          [](const Badge &badge, const UserInfo &user)
          {
                BadgesRegistry::pending()[user.accountID].push_back(badge);
          });
      BadgesAPI::registerBadge(
          "rl-supporter-badge",
          "Rated Layouts Supporter",
          "This user is a <cp>Layout Supporter</c>! They have supported the development of <cl>Rated Layouts</c> through membership donations.\n\nYou can become a <cp>Layout Supporter</c> by donating via <cp>Ko-Fi</c>",
          []
          {
                return CCSprite::create("RL_badgeSupporter.png"_spr);
          },
          [](const Badge &badge, const UserInfo &user)
          {
                BadgesRegistry::pending()[user.accountID].push_back(badge);
          });
}