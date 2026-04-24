# v1.0.9

- Fixed the issue with the Gauntlet Level Selection background not moving correctly when dragging.
- Fixed the issue where the third coin is showing the bronze coin texture instead of the blue coin texture.
- Added **Update Account Info** button in the mod settings to update your account information within Rated Layouts.
- Added **Disable Rated Layouts Mod Requests** in the mod settings to disable the RL mod requests and use the REQ button normally for Vanilla GD Mods.
- Fixed the issue where the news announcement badge doesn't disappear after viewing the announcement.
- Fixed the issue where it shows a non-existent achievements.
- Added popup when clicking the Discord Server button to confirm before joining
- Added Vanilla Report Button in the RL Report Popup.
- Clearing the search will clear the selected filters

## v1.0.8

- Fixed bug where the announcement didn't show the alert at all.
- Removed hardcoded Owner role
- Added Developer Badge and Role
- Added New Gauntlet Sprites
- Added **Disable Garage Stats** in the Mod Settings
- Fixed Typos

## v1.0.7

- Remove Top Creator toggler.
- Fixed the cache clearing not working correctly when the server returns non-ok status.
- Fixed the Rubies position offset incorrectly when fetching a level that is cached.
- Tweaked the **Event Layouts** animation to be similar to Vanilla Daily Level animation.
- Improved the **News Annoucements** to include past announcements and a better layout.

## v1.0.6

- Added Caching on Levels and Comments, this reduces the amount of server requests and improves performance.
- Changed the Community Votes requirements to the following:
  - <cr>Demon Layouts</c>: 20% Normal Mode and 80% Practice Mode
  - <cg>Non-Demon Layouts</cg>: 50% Normal Mode and 100% Practice Mode
  - <co>Platformer Layouts</co>: Must have completed the level (regardless of the completion percentage)
- Fixed Typos... again
- Fixed crash when entering The Spire after completing a level
- Added new Gauntlet Sprite
- The Blue Spark Button on the Vanilla Search Layer will now go directly to the Rated Layouts Search Layer
- Updated Mod About Page
- Removed **Vigilant Citizen** achievement.
- Added back **Get Gem To Win Experience** achievement.

## v1.0.5

- Removed **Legacy Rate** feature entirely.
- Added GDPS check, it will give a warning when trying to access Rated Layouts features on GDPS.
- Added a toggle for "Top Creators" in the leaderboard layer to show creators who interacted with the mod
- Fixed more typos
- Fixed a small bug with the leaderboard tab not deselecting properly
- _A mod developer was annoyed with the amount of warn logs filling up the console and got really annoyed at me for it, so yea I just commented out the warn logs for non-ok status on the web requests._

## v1.0.4

- Updated the about mod page
- Fixed issue where **Laying Out!** achievement isn't unlocking after completing a rated layout.
- Fixed issue where user with **Disabled Reward Animation** enabled still getting the reward notification even when it's already completed.
- Fixed the Author's name in the Shop Item list not showing. _(Due to the backend changes)_
- Fixed issue where Spire levels not updating after completing a Spire level.
- Fixed typos in the information texts
- Notification are off on levels by default. _(You can enable it in the mod settings)_
- Added **Rated Layouts Menu Button** in the Main Menu. _(Can be disabled in the mod settings)_
- Added **Guidebook** as a replacement to the info button in the main menu. _(It will ask you when opening the layer first)_
- Added **Key** button, mainly used for internal uses.
- Moved the **Votes Leaderboard** to the main Leaderboard Tabs.
- Nav buttons on the Ruby Shop now wraps around when reaching the end of the list.

## v1.0.3

- Reverted April Fools related changes
  - Removed **Get Gem to Win** achievement
- Removed **Layout Report System** button on non-rated layouts.
- Fixed and Tweaked the Achievement Icons scaled incorrectly in the Achievements Popup.
- Fixed issue where Layout Gauntlets levels showing as incompleted even when its been completed already.

## v1.0.2-goog

- Fixed a crash when deleting a comment with a nameplate crashes your game.
- Added Back button for mods/admins when clicking the Sent Tab
- Rubies redeemed from the **Observatory** are now localy stored
- Reset Rubies will also reset the redeemed codes
- Added **goog** cat dialogue and achievement
- Added **Get Gem To Win** achievement

## v1.0.0

### The Spire Update!

- Added **The Spire**. Platformer-focus user created Rated Layouts levels. Explore the Spire and find forsaken lore beyond the <cp>Cosmos</c>
- Added **New Secret Character**. _Discover this character and your access to the Spire will be granted_
- Added **Disable Notification in Level** in the mod settings
- Added **Comment Text Background** for better visibility of comments on bright nameplates
- Added **Leaderboard Whitelist** for Leaderboard Moderators
- Added **Nameplate Test** to test your nameplate images in-game before submitting them.
- Tweaked the **Comment Cell Colors** for users with a bright nameplate
- Tweaked the **Leaderboard** to show the nameplate more visible
- Tweaked the **Rubies Labels** on the Level Info Layer and the End Level Layer
- Replaced the use of the old scrolling system to use Cue's list node
- Fixed issue where the text label in the Achievements overflows
- Fixed issue where the Shopkeeper and the Plushie sprite are floating on screens with different aspect ratios
- Fixed issue where the RL Stats Menu is overlapping with the [Better Progression](mod:itzkiba.better_progression)
- Fixed issue where on smaller screens, the Button Menu is overlapping with the socials menu
- Fixed issue where the comment text color isn't applied for users with [Comment Emojis Reloaded](mod:prevter.comment_emojis) mod enabled
- Fixed issue where Difficulty Score voted 10 or higher doesn't apply correctly when voted
- Fixed issue with the nameplate item hitbox area has been set incorrectly
- Stats Info Button at the Profile Page will now show their Rated Layouts stats when the stats menu is toggled to Rated Layouts stats
- Updated the **Community Votes** information
- Improved the UI especially for the User Level Panel and the Moderator/Admin Panel
- Replaced the Buttons into a Dropdown Menu in the Shop Layer
- Moderator Note are now clickable and will show a Popup with the full note text
- Layout Admins can now promote/demote Layout Moderators
- Internal Code Refactor and optimizations

## v0.4.3-beta.1

- Added **Community Vote Leaderboard** to show the top voters in the community vote. (Found in the Community Vote popup)
- Added **Disable Stats Menu Animation** option in the mod settings
- Added **Moderators Notes** section in the level info layer to show any notes from moderators about the level
- Added **Vote Count** in the Profile Page.
- Replaced the **Ruby Shopkeeper** with a new character hehehe
- Added **Coin Unverified** filter in the Custom Search Layer to show levels that has unverified/no coins.
- Added **Report Level** on sent layouts levels.
- Added **Disable Comment Glow on Nameplate** option in the mod settings. _(Enabled by Default)_
- Added **Appeal Button** for users who got leaderboard banned. (Found in the Difficulty Stats popup)
- Improved the **Credits Popup**
- Improved **Moderator Panel** with additional tools and information for moderators and admins.
- Tweaked the **User Moderator Panel** permissions and access.
- Nameplates are now **Server-sided** to reduce the mod size and allow more nameplates to be added without needing to update the mod.
- More nameplates!

## v0.4.2-beta.1

- Refactored the Moderation system and introducing **Role Specific Badges**! Moderators and Admins will now have different badges to distinguish them in the comments and the leaderboard.
  - **Platformer Layout Moderator** badge for moderators who can suggest platformer rated layouts.
  - **Classic Layout Moderator** badge for moderators who can suggest classic rated layouts.
  - **Platformer Layout Admin** badge for admins who can suggest/rate platformer rated layouts.
  - **Classic Layout Admin** badge for admins who can suggest/rate classic rated layouts.
  - **Leaderboard Layout Moderator** badge for moderators who can moderate the leaderboard.
- Added **Badge API** back as a dependency
- Added **Most Sents and Least Sents** for Admins and Mods in the Sent Layouts section.
- Added **Classic & Platformer** filters on the Sent Layouts section.
- Added More **Nameplates**
- Added seperate timer for the **Event Layouts**
- Added Additional tools for **Leaderboard Layout Mods**
- Fixed issue where rubies isn't rewarded correctly at the End Level Layer.
- Fixed issue where the **Disable Reward Animation** option isn't applying on rubies collection.
- Fixed issue with the Menu Buttons in the **Profile Page** got cut off.

## v0.4.1-beta.1

- Bump to Geode v5.0.0 stable
- Added **Community Vote Leaderboard** to show the top voters in the community vote. (Found in the Community Vote popup)
- Fixed the crash relating to entering the level info screen.
- Improved the Moderator Popup UI

## v0.4.0-beta.1

- Ported to Geode v5.0.0
- Improved **Level Browser**! Now you can search more than **100 levels**!
- New **Gauntlet Sprites**!
- Added **Legendary Rated Layouts**. Layouts that are rated Legendary will have a special title animation and particle effects.
- Added **Epic Layouts Particles** making epic layouts more standout.
- Added **Customizable Background** in the Rated Layouts Creator Layer and Leaderboard Layer. _(Can be changed in the mod settings)_
- Added **Rated Layout Browser** button!
- Added **Coins Verified** status for levels that has verified Blue Coins.
- Added **Completed**, **Uncompleted** and **Coins Verified** filters in the Custom Search Layer.
- Added **Coin Rank** in the Difficulty Stats Popup.
- Added **Custom Achievements**! Collect achievements by completing certain milestones and special interactions.
- Added **Random Level** button in the Custom Search Layer to play a random rated layout level!
- Added **Total Demon Count** in the Difficulty Stats Popup.
- Added **Layout Booster Badge** for users who has boosted the Rated Layouts Discord server.
- Added **Newly Rated/Event Notifications**
- Added **Featured Score** label in the level cells for featured rated layouts.
- Added **Ruby Collectables** for playing Rated Layouts. (Thanks to [Darkore](user:3595559) for the Ruby texture design!)
- Fixed the Leaderboard and added refresh button to manually refresh the leaderboard data.
- Tweak the **Event Layouts** with the new sprite title and improved the UI.
- Added working animated **Blue Coins Sprites** in the levels
- Added Indicators for Top 3 players in the Leaderboard Layer.
- Layout that is Rated by RobTop will automatically gets unrated.
- Change the sprites to be in a spritesheet.
- Clean-up the Mod Rate Popup code/UI.
- Proper error message handling
- Removed Page API dependency
- Removed Level Rating and User Caching
- Removed Badge API dependency
- Secret features... shh...

## v0.3.6-beta.1

- **Added Blue Coins!** Thanks to [Darkore](user:3595559) for the Blue Coin texture design! You can now collect Blue Coins by collecting them in the Rated Layouts levels.
- **NEW Featured and Epic Texture** Replaced the old texture to a new and clearer design. _Thanks to myself for the new design!!!!!_

_(Be sure to re-enter your previously beaten levels to collect Blue Coins if you have played Rated Layouts before this update!)_

- Fixed issue where liking/refreshing level in level info will set the difficulty sprite incorrectly especially on any levels that isn't rated layouts.
- Fully patched/fixed the web request crashes.

## v0.3.5-beta.1

- **Profile Stats Changed!** The Rated layouts stats are now on a separate section in the profile page. Click the new Planet Button in the profile page to view your Rated Layouts stats!
- **Added Gauntlet Select Levels!** You can now select and play Layout Gauntlet levels from the Gauntlet Select Layer.
- **Added Report Rated Layouts!** You can now report Rated Layouts that violates the Rated Layouts guidelines directly from the Level Info Layer.
- **New Mod Title!** Thanks to [KrazyGFX](https://x.com/Krazy_GFX/status/2006175258361905195?s=20) for the design
- Changed "Design" to "Originality" in the Community Vote popup.
- Added Confirmation popup when submitting a vote in the Community Vote popup.
- Added Layout Gauntlet Button on the Vanilla Gauntlet Select Layer to easily access the Rated Layout Gauntlets.
- Added "Suggest" button for Layout Admins
- Improved the Layout Moderators and Admins UI.
- Comment Glow will now apply when an user has planets in their stats.
- Layout Admins can now view 3+ sends in the Sent Layouts section.
- Layout Moderators and Admins can now reject suggested layouts.
- Layout Mods can now see the total sends in the Mod Rate Popup.
- Community Vote button is now shown at the right side of the menu in the Level Info Layer.
- Community Vote will now always show on Rated layouts instead of just suggested layouts.
- Event Layouts are now more responsive when pressing the play button
- Fixed issue where submitting a vote with no score defaulted to 1. It should not be included in the vote if empty.
- Fixed issue where the Leaderboard tabs are misaligned on smaller screens.
- Fixed issue where the Leaderboard tabs are transparent when selected.
- Fixed issue where the game crashes when the web requests finishes but the layer is already destroyed.
- Fixed issue when refreshing or liking a rated layout level, the difficulty sprites in the profile page isn't updated correctly.
- Fixed issue where the time completion for platformer rated layouts isn't showing the correct time.

_Thanks to [delivel-tech](https://github.com/delivel-tech) & [iAndyHD3](https://github.com/iAndyHD3) for this update and fixes!_

## v0.3.4-beta.2

- Very minor fixes relating to ko-fi links being invaild.

## v0.3.4-beta.1

- Added Disable Creator Points in mod settings. This is for specific users who see it as a negative aspect of the mod.
- Minor fixes and improvements.

## v0.3.3-beta.1

- **Added Layout Supporter Badge!** Support Rated Layouts development by donating via Ko-fi and get a special badge shown to all players and a colored comment!
- You can view the total votes count for each suggested levels in the Sent Layouts levels.
- Scores in the Community Vote popup are now more precises.
- Layout Moderators and Admins can now toggle the scores in the Community Vote popup even if they haven't voted yet.
- Added **Layout Supporters** users in the credits list.
- Minor fixes and improvements.

## v0.3.2-beta.1

- Removed [More Difficulties](mod:uproxide.more_difficulties) from the incompatible mods, but the more difficulties sprite won't be shown for layouts that is rated.
- Added "Clear Cached Data" button in the mod settings to clear all cached rating and profile data.
- Fixed issue where the sprites on the reward animation is showing the incorrect texture.
- Fixed issue where the stars/planets reward animation crashes upon completing a level.
- Fixed issue where Comment Cell isn't fetching the user profile data correctly.
- Tweaked the rating difficulty sprite positions to be more consistent.
- Announcement button sprite changed to a more button like.

_Thanks to [hiimjasmine00](https://github.com/hiimjasmine00) for the fixes!_

## v0.3.1-beta.1

- **Added "Classic" Filter** in the Custom Search to filter Classic Rated Layout Levels.
- **Added Announcement Button** in the Rated Layouts Creator Layer to view the latest news and updates about Rated Layouts.
- Added Planets Collectable count in the Garage Layer.
- Epic, Featured and Awarded filters are no longer mutually exclusive, you can now select multiple of them at once.

## v0.3.0-beta.1

### The Planets Platformer Update!

- **Added Planets Collectables!** Collect Planets by completing Platformer Rated Layout Levels. Thanks to [Darkore](user:3595559) for the Planet texture design!
- **Added Platformer Rated Layout Levels!** You can now play Platformer Rated Layout Levels and earn Planets Collectables!
- **Added Platformer Rated Layouts Filter!** You can now filter levels by Platformer Rated Layouts in the Custom Search.
- **Added Platformer Difficulty Stats!** You can now view your Platformer Rated Layout Levels stats in your profile page, including the number of levels you've beaten for each platformer difficulty. (You can view them by clicking the Planets icon in the profile page.)
- **Added Username Filter!** You can now filter layout levels by creator's username in the Custom Search.
- **Added Platformer Layouts into the Event Layouts!** You can now find Platformer Rated Layout Levels in the Event Layouts section.
- Made Blueprint Points clickable in the Profile Page to view the account's rated layout levels.
- Layout Moderators and Admins can now suggest/rate platformer layout levels.
- Internal Moderator and Admin UI tweaks.
- Added additional checks on the "anti-cheat" for beating levels.
- Fixed issue with the labels in the Event Layouts layer being misaligned.
- Fixed issue where spamming the buttons makes the popup appear multiple times.

## v0.2.9-beta.1

- Tweaked the **Community Vote** UI. If you are a Layout Moderator or Admin, it's unlocked by default. Others has to meet the percentage requirements to access it.
- Community vote scores are now hidden until the user has voted, this ensure that users are not biased by the current votes.
- Added **Discord Invite Button** in the Rated Layouts Main Layer to easily join the Rated Layouts Discord server.
- Added **Global Rank** on the Difficulty Stats popup to show your global rank based on the number of stars you've collected.
- **Secret Dialogue messages are now dynamic**. You might get different messages each time you click it!
- **Layout Moderators and Admins** can now submit custom dialogue messages for users to see when they click the secret dialogue button.

## v0.2.8-beta.1

- **Added Layout Gauntlets Annoucement!** Be sure to check out the Layout Gauntlets Creator Contest!!
- Fixed issue where [Badge API](mod:jouca.badgesapi) was set to the wrong recommended version
- Removed [Demons In Between](mod:hiimjustin000.demons_in_between) from the incompatible mods as it doesn't conflict at all.

### _Note: You can still have [More Difficulties](mod:uproxide.more_difficulties) enabled, it just shows a warning that it conflicts but you can still use both mods._

## v0.2.7-beta.1

- **New Texture Update!** Huge thanks to [Dasshu](user:1975253) for creating new textures and logo for Rated Layouts mod and [Darkore](user:3595559) for the new stars icon mockup!
- Removed "Newly Rated" button. _You can still find newly rated layouts in the Custom Search layer by simply clicking the search button without any filters applied._
- Seperated the Event Layouts into Daily, Weekly, and Monthly Layouts.
- **Added Layout Gauntlets!** A special themed layout gauntlets curated by Rated Layouts Admins and Moderators. Used to host special layout creator contests.
- **Added Community Vote!** Rated Layouts community can now vote on suggested layouts in the Sent Layouts section. This helps better curate layouts for the Layout Admins.
- **Added Safe Event Layouts!** You can view past event layouts by clicking the Safe button in the Event Layouts layer.
- Added the following mod incompatibilities to prevent conflicts with the difficulty sprite:
  - [More Difficulties](mod:uproxide.more_difficulties)
  - [Demons In Between](mod:hiimjustin000.demons_in_between)
- Added Epic Layout Suggest for Layout Moderators.
- Improved "anti-cheat" for beating levels.
- Fixed issue where users cache isn't always up to date when showing stats in the comments.
- Fixed issue when liking/unliking levels, the stars count in the level info layer isn't updated correctly.
- Tweaked the glow effect and the Layout Moderators and Admins' comment colors to be less intense.

## v0.2.6-beta.1

- Fixed bug where Layout Admins can't submit featured score for Epic Layouts.
- Fixed bug where comments text color on non-compact mode was not applied correctly.
- Fixed issue where Blueprint Stars isn't showing the correct amount at the Garage Layer.
- Fixed issue where users who has [Comment Emojis Reloaded](mod:prevter.comment_emojis) mod installed can't see Mods/Admins colored comments.
- Fixed issue where the Blueprint Stars label is overflowing outside the container in the Profile Page. (Looks nice for those who has 1000+ stars now!)
- Added Stars value formatting. _(Now it shows 1,234 instead of 1234)_
- Added Rated Layout Button at the Level Search Layer.

## v0.2.5-beta.1

- Minor fixes and code cleanup on Event Layouts.

## v0.2.4-beta.1

- Fixed issue with the event layouts loading the level incorrectly.
- Fixed issue with the Search Background Menu not displaying correctly.

## v0.2.3-beta.1

- **Added Difficulty Stats!** You can view your difficulty stats in your profile page, including the number of levels you've beaten for each difficulty. (You can view them by clicking the Blueprint Stars icon in the profile page.)
- **Added User Comment Glow!** You can easily identify comments from users using Rated Layouts mod.
- **Added Event Layouts!** Play daily, weekly, and monthly rated layouts curated by the Rated Layouts team and Admins.
- **Added Epic Rating!** Levels can now be rated as Epic by Admins, giving them a special status and recognition.
- **Added Epic Search Filter!** You can now filter levels by Epic rating in the Custom Search.
- Tweaks for Search Layer.

## v0.2.2-beta.1

- **Added Custom Search!** You can now search for Rated Layout Levels using various filters such as <cr>difficulty</c>, <co>level type</co> and more!
- Removed the Level Search Quick Menu from the Search Tab in favor of the custom search layer in the Rated Layouts Creator Layer.
- Fixed the sprite not showing the correct star icon in the end level layer when claiming stars.

## v0.2.1-beta.1

- Added fetched comments caching to reduce server requests and included in the disabled fetch requests option.
- Minor fixes.

## v0.2.0-beta.1

- Added an option to disable all fetch requests from the server to get the level rating data.
- Fixed an issue where it can't search for levels that has more than 100 level IDs.

## v0.1.9-beta.1

- Fixed the Star Reward animation on the end level layer.
- Added Notification and sound effect when claiming stars from the level info layer if the reward animation is disabled.
- Minor fixes.

## v0.1.8-beta.1

- Added User logs and control panel (for moderators and admins)
- Added credits or layout moderators and admins list
- Added an settings to disable moving background
- Added Disable Reward Animation (useful if you on mobile and it kept crashing)
- Removed the background decoration because it kept lagging the game
- Minor fixes

## v0.1.7-beta.1

- Added Info Button at the Rated Layouts Creator Layer that explains about Rated Layouts and its features.
- My sanity is in a dialog form now...
- Minor fixes and improvements.

## v0.1.6-beta.1

- Added a custom layer for all Rated Layouts related features. (found at the bottom left of the creator layer)
- Sprite changes to make sure its consistent with the theme.

## v0.1.5-beta.1

- Added Player's Icons at the leaderboard. Might not update based on your profile but it's usually stored for statistics purposes.
- Role request is similar process as getting a moderator role in the actual game. Good luck

## v0.1.4-beta.1

- Added Leaderboard! You can now view the top players based on the number of stars and creator points they have collected from rated layouts.

## v0.1.3-beta.1

- Levels ratings are cached locally to reduce server requests and improve performance. (and removed when deleting the level)
- Stars are now claimed when you enter the level from the level info layer if already beaten before.
- Fixed issue where the stars icons in the level info layer are misaligned when refreshing the level.
- Added Stars reward animation at the end level layer when beaten the level legitimately.

## v0.1.2-beta.1

- Fixed a crash that could occur when loading profile pages or comments if the user navigated away before the server responded.
- Added a notification to inform users if they attempt to claim stars for a level they have already claimed stars for.
- Blueprint Stars can only be claimed once per level completion to prevent exploitation.

## v0.1.1-beta.1

- Minor fixes, mostly just tweaking the position of the star icon on level cells.
- Fixes some unexpected crashes.

## v0.1.0-beta.1

- "i love robtop's rating system, its so fair and totally not rigged, i swear i have 12 sends in adrifto and still not rated. totally fair"
