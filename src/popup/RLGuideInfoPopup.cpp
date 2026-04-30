#include <Geode/Geode.hpp>
#include "Geode/cocos/cocoa/CCObject.h"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/ui/General.hpp"
#include "Geode/ui/Layout.hpp"
#include "ccTypes.h"
#include "RLGuideInfoPopup.hpp"

using namespace geode::prelude;

RLGuideInfoPopup* RLGuideInfoPopup::create() {
    auto ret = new RLGuideInfoPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RLGuideInfoPopup::init() {
    if (!Popup::init(400.f, 280.f, "GJ_square02.png"))
        return false;

    auto title1 = CCSprite::createWithSpriteFrameName("RL_title.png"_spr);
    title1->setScale(0.8f);
    m_mainLayer->addChildAtPosition(title1, Anchor::Top, {0, -30.f});

    auto title2 = CCLabelBMFont::create("FAQ & Guidebook", "bigFont.fnt");
    title2->setScale(0.6f);
    m_mainLayer->addChildAtPosition(title2, Anchor::Top, {0, -55.f});

    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);

    // FAQ menu
    auto faqMenu = CCMenu::create();
    faqMenu->setPosition(m_mainLayer->getContentSize() / 2.f - CCPoint(0.f, 10.f));
    faqMenu->setContentSize(m_mainLayer->getContentSize() - CCSize(20.f, 60.f));
    faqMenu->setLayout(RowLayout::create()->setGap(10.f)->setCrossAxisOverflow(false)->setGrowCrossAxis(true));
    m_mainLayer->addChild(faqMenu);

    // list of faq buttons
    auto aboutBtn = ButtonSprite::create("About", "goldFont.fnt", "GJ_button_01.png");
    auto aboutItem = CCMenuItemSpriteExtra::create(aboutBtn, this, menu_selector(RLGuideInfoPopup::onAbout));
    faqMenu->addChild(aboutItem);

    auto layoutsBtn = ButtonSprite::create("Layouts", "goldFont.fnt", "GJ_button_01.png");
    auto layoutsItem = CCMenuItemSpriteExtra::create(layoutsBtn, this, menu_selector(RLGuideInfoPopup::onLayouts));
    faqMenu->addChild(layoutsItem);

    auto standardsBtn = ButtonSprite::create("Rating Standards", "goldFont.fnt", "GJ_button_01.png");
    auto standardsItem = CCMenuItemSpriteExtra::create(standardsBtn, this, menu_selector(RLGuideInfoPopup::onStandards));
    faqMenu->addChild(standardsItem);

    auto otherBtn = ButtonSprite::create("Other FAQs", "goldFont.fnt", "GJ_button_01.png");
    auto otherItem = CCMenuItemSpriteExtra::create(otherBtn, this, menu_selector(RLGuideInfoPopup::onOthers));
    faqMenu->addChild(otherItem);

    // notice

    // @geode-ignore(unknown-resource)
    auto noticeBg = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    m_mainLayer->addChildAtPosition(noticeBg, Anchor::Bottom, {0, 30.f});

    auto noticeLabel = CCLabelBMFont::create("Note: This is not a complete guidebook, but it covers the most essential FAQs and information about Rated Layouts.\nMost, if not all, of these FAQs are related to Classic Layouts; Platformer Layouts may differ.\nFor more detailed info and questions, please refer to our Discord server!", "chatFont.fnt");
    noticeLabel->limitLabelWidth(m_mainLayer->getScaledContentSize().width - 40.f, 0.7f, 0.3f);
    noticeLabel->setAlignment(kCCTextAlignmentCenter);
    m_mainLayer->addChildAtPosition(noticeLabel, Anchor::Bottom, {0, 30.f});

    noticeBg->setContentSize({m_mainLayer->getContentSize().width - 20.f, noticeLabel->getContentSize().height});
    noticeBg->setColor({0, 0, 0});

    faqMenu->updateLayout();

    return true;
};

void RLGuideInfoPopup::onAbout(CCObject* sender) {
    MDPopup::create(
        "About Rated Layouts",
        "## <cl>Rated Layouts</cl> is a community-run rating system focusing on gameplay in layout levels.\n\n"
        "### Each of the buttons on this screen lets you browse different categories of rated layouts:\n\n"
        "<cg>**Featured Layouts**</c>: Featured layouts that showcase fun gameplay and visuals. Each featured layout is ranked based on its featured score.\n\n"
        "<cg>**Leaderboard**</c>: The top-rated players ranked by blueprint stars and creator points.\n\n"
        "<cg>**Layout Gauntlets**</c>: Special themed layouts hosted by the Rated Layouts Team. This holds the <cl>Layout Creator Contests</c>!\n\n"
        "<cg>**The Spire**</c>: A tower-themed, platformer-focused, user-created <cl>Rated Layouts</c> level. Explore the Spire and find forsaken lore beyond the <cp>Cosmos</c>.\n\n"
        "<cg>**Sent Layouts**</c>: Suggested or submitted layouts from the Layout Moderators. The community can vote on these layouts based on their <cg>Originality</c>, <cr>Difficulty</c>, and <co>Gameplay</c>.\n\n"
        "To access the <co>Community Vote</c>, you need to meet the following requirements:\n\n"
        "- <cr>For Demon Levels:</c> you have at least <cg>20% in Normal Mode</c> or <cf>80% in Practice Mode</c>\n"
        "- <cl>For Non-Demon Levels:</c> you have at least <cg>50% in Normal Mode</c> or <cf>100% in Practice Mode</c>\n"
        "- <co>For Platformer Levels:</c> you need to have <cy>completed</c> the level\n\n"
        "<cg>**Search Layouts**</c>: Search for rated layouts by their level name/ID.\n\n"
        "<cg>**Event Layouts**</c>: Showcases time-limited Daily, Weekly and Monthly layouts picked by the <cr>Layout Admins</c>.\n\n"
        "### Join the <cb>[Rated Layouts Discord](https://discord.gg/jBf2wfBgVT)</c> server for more information and to submit your layouts for rating.\n\n",
        "OK")
        ->show();
}

void RLGuideInfoPopup::onLayouts(CCObject* sender) {
    MDPopup::create(
        "Layouts",
        "## What is a Layout?\n\n"
        "**Layout** is a level type that puts its main focus on <cg>gameplay</c>. It usually has structuring that goes with the gameplay and strengthens the way it feels, while also guiding the player's path. By default, layouts don't have decoration.\n\n"
        "Layouts can be considered levels. While they can be seen as a **'skeleton'** or **'outline'** of a level, they can also be a full and complete experience. Gameplay can be a form of art and layouts are the medium for it.\n\n"
        "## Can layouts with pure gameplay and without structuring get rated?\n\n"
        "**Yes**, just make sure that there are no secret ways and that there is a clear pattern for players to follow.\n\n"
        "## If a layout has bad sync, can it still get rated?\n\n"
        "Player sync and note representation are not strictly required to obtain a rating.\n\n"
        "Alternatively, creators can focus more on the playability aspect, primarily delivering interesting player movements and engaging gameplay without fully relying on sync and note representation, as long as the **pacing (usage of speed portals) still matches the song perfectly**.\n\n"
        "## When does a layout have too much deco/visuals?\n\n"
        "If the level contains a lot of detailed block design, air decoration, glow, and many visuals/effects, it is generally considered a level and **cannot be rated in <cl>Rated Layouts</c>**, since it is likely to be sent by a GD moderator and rated through RobTop's rating system.\n\n"
        "- Refer to Case 8 of [Non-Rateable Layouts (aka NRL)](https://discord.com/channels/1439944272102162442/1482046209974603877)\n\n"
        "'Layouts' that contain borderline decoration (block design, background, heavy air deco, etc.) enough to be called a level.\n\n"
        "## What is a 'Non-Rateable Layout' (NRL)?\n\n"
        "**'Non-Rateable Layout' (NRL)** is a rating guideline for Rated Layouts. All layouts should abide by these guidelines to be eligible for a rating. These guidelines are important for quality control in our rates.\n\n"
        "There are 14 total reasons why a layout should not be rated in Rated Layouts; they are all available in [Non-Rateable Layouts (aka NRL)](https://discord.com/channels/1439944272102162442/1482046209974603877)\n\n"
        "## Does a layout rating get affected if it's old/outdated?\n\n"
        "**No.** We don't have a bias against old layouts and **do not judge based on how old it is or how dated its style is**, but they may be less likely to be rated or receive a lower rating tier than expected because they are reviewed by the same standards as the newest layouts, which naturally utilize better gameplay mechanics.\n\n"
        "### <co>Important: The ban on 1.0 style is a result of it being easy to replicate and mass produce.</c>\n\n"
        "## Are layouts with NONG allowed to be rated?\n\n"
        "**Yes**, if the song itself is available on the [Song File Hub](https://songfilehub.com/home) or [Jukebox Mod](mod:fleym.nongd). Otherwise, no.\n\n",
        "OK")
        ->show();
}

void RLGuideInfoPopup::onStandards(CCObject* sender) {
    MDPopup::create(
        "Rating Standards",
        "## What are the rating standards for Rated Layouts?\n\n"
        "Rated Layouts has four rating tiers: <cs>Rated</c>, <cg>Featured</c>, <cp>Epic</c>, and <cd>Legendary</c>. Each tier has its own standards and requirements that a layout should meet to be rated in that tier.\n\n"
        "- <cs>**Rated**</c>: A layout must synchronize to the music and is required to represent the song in a proper manner. Base gameplay should show a ground-level understanding of the fundamentals (primary rules and/or principles of gameplay). It must be bug-free and skip-proof.\n"
        "- <cg>**Featured**</c>: A layout must show good sync, song representation, and a decent amount of fundamental knowledge expressed in the gameplay. Structuring is required to have proper composition, while also working well with the base gameplay. The pacing (usage of speed portals/changes) should work well with the music. The gameplay should be adequately playtested and tolerable for the player.\n"
        "- <cp>**Epic**</c>: Layouts are required to showcase detailed note representation in music sync and a large moveset variety. Each movement in the layout is required to work well with the respective note in the song, regardless of whether it is simple or complex. Pacing is also required to be precise with the music. The layout must maintain consistency and quality from start to finish. The gameplay must be engaging and fun for the player, ensuring it is satisfying to pull off and deaths are only the fault of the player.\n"
        "- <cd>**Legendary**</c>: This rating tier is awarded for noteworthy layouts that are flawless and revolutionary, showcase full mastery of advanced fundamentals and aspects of base gameplay, and are able to bring new ideas to the table. These layouts are able to push gameplay standards further, while simultaneously influencing the community in various ways with their own distinctive and unique gameplay style.\n\n"
        "## What criteria do our rating standards actually have?\n\n"
        "### Creativity, Song representation, Fun.\n\n"
        "Each one is always considered. If a layout excels in all three, it will get a higher rate, while excelling in just one aspect will result in a lower rate.\n\n"
        "However, if the layout is completely lacking in any aspect (e.g. it's unplayable, copies another level too much, or doesn't represent the song), it won't be rated at all no matter the quality of other aspects.\n\n"
        "- **Creativity** is how meaningful and intentional the layout is, and how much creative skill it shows. It’s the way all ideas (gp movements, structures) in your level combine together. If they’re original, make sense, and connect smoothly, the creator probably put a lot of creativity into it.\n"
        "- **Song representation** is how the layout expresses the song. To do that, you need to represent each note of the song using just player movements. Usually you would start with sync if you’re a beginner, and then try to match how the song feels.\n"
        "- **Fun (playing experience)** is how you make the player feel both the level and the song. Some important things to know about it are sync, control, and playability (bugs, consistency, etc.). This also ties into song representation, so don’t neglect that. Good song rep can make a level feel tenfold more fun than bad song rep.\n\n"
        "## Will the standards ever get decreased?\n\n"
        "Short answer: **no.**\n\n"
        "Long answer: **Rated Layouts standards** will not be decreased to ensure quality control in our ratings and encourage creators to be more creative.\n\n"
        "## Why are the standards so high?\n\n"
        "Our goal in setting up our standards is to reward creators based on their talent and mastery of layout creation. We don't want to create a system that rewards and provides recognition to anyone who simply creates layouts without putting effort or thought into their work.\n\n"
        "We also want to ensure that spark/planet grinders have a much more pleasant experience playing the levels we rate. This is important for our reputation and image.\n\n"
        "Besides, your goal in creating shouldn't be about being on par with any standards at all. It is heavily advised and incentivized to enjoy the creative process.\n\n"
        "## Why are visuals and effects not evaluated in Rated Layouts?\n\n"
        "Layouts focus entirely on base gameplay and structuring, and gameplay is judged in a different way through idea+execution, song representation, and playability.\n\n"
        "Our rates are meant to explore gameplay on a deeper level. Decoration, visuals, and effects have been explored for a very long time, so it's time for us to expand on what we can do with gameplay with the initiative of this mod.\n\n"
        "While it's cool to want to preserve something that works, we shouldn't be shy of exploring new possibilities, because we aren't removing what worked at all; we're trying to expand upon it.\n\n"
        "Our standards and criteria aren't meant to make it 'harder to get a rate,' but rather encourage people to want to be creative.\n\n"
        "## What are the rating standards for Auto Layouts?\n\n"
        "In contrast to regular layouts, auto layouts do not require any player input and are built specifically for the viewing experience. Because of this, Auto layouts are evaluated slightly differently, prioritizing sync, pacing, and note representation.\n\n"
        "- The layout must be completely bug-free, ensuring no player deaths can ever happen on any attempt.\n"
        "- The player's icon must be visible at all times. Visuals, FX, and shaders still do not apply in the judging system.\n"
        "- The entire layout can be beaten without requiring a single player input.\n\n"
        "Each rating tier still follows the same descriptions as non-auto layouts.\n\n"
        "## What are banned gameplay styles, and why are they banned?\n\n"
        "The following list provides the gameplay styles that are banned in our rating criteria (will be updated in the future based on trends):\n\n"
        "- Robtop (1.0-1.7) / Map Packs / 1.0 Style\n"
        "- Layouts with gameplay that fully replicate the base gameplay of the following list of demon levels, without any expansion/improvement/innovation on the style:\n\n"
        "**(i.e. Slaughterhouse, Grief, KOCMOC, Silent Clubstep, Superhatemeworld, Every End, Sonic Wave Infinity and Nullscapes)**\n\n"
        "- Challenge levels\n"
        "- Troll levels\n"
        "- 'Sh**ty levels' that are nerfed versions of existing levels remade with 1.0 blocks\n"
        "- Layouts that overdo the concept of rotating default blocks **(i.e. KJackpot/SciPred style)**\n\n"
        "These specific styles are banned due to how convenient they are to replicate and mass produce into generic levels. We fully discourage this to ensure our rates are valuable and meaningful.\n\n"
        "The goal is to make Rated Layouts much more tolerable for players, while rewarding creators for their talent and mastery of gameplay skills.\n\n"
        "<co>Note that if you can create a fresh, original take on a banned style, it could be rated anyway. We value how creatively you can use a style.</c>",
        "OK")
        ->show();
}

void RLGuideInfoPopup::onOthers(CCObject* sender) {
    MDPopup::create(
        "Other FAQs",
        "## I believe a certain RL Extreme Demon deserves a higher/lower rating tier. How do I address this?\n\n"
        "Due to the high difficulty making them much less accessible to average players, the playability judgment for such layouts relies heavily on the opinions of their victors. If there are issues regarding whether a layout of this difficulty deserves a lower or higher rating tier, it's up to the victors to report it and discuss it with us. So if you're a victor of any RL extremes and would like to have a word about issues with it, please let us (the mod team) know!\n\n"
        "The other two aspects are still required to decide its suitable rating tier, but only in a descriptive sense.\n\n"
        "## What is a Robtop/mappack/1.0 level? What characteristics define them?\n\n"
        "A **Robtop-styled** level is similar to the old main levels: Stereo Madness, Back on Track, Polargeist, Dry Out, Can't Let Go, etc. A major indicator is abusing square/rectangle structures that only use default checker-like blocks, alongside basic ground-level fundamentals tied together with little or no sync (e.g. regular spike jumps and singular orb clicks).\n\n"
        "**Mappack-styled** levels are about the same, but they sometimes take ideas from later levels (like xStep or Electroman Adventures). They also tend to have weirder or more awkward gameplay. They're similar to 1.0 style.\n\n"
        "And finally, 1.0-styled levels come from the pre-1.6 era (a bit ironic, but yes). They were usually variations of Robtop style with some fresher ideas, but the same general look. A good example is [Demon Park](https://youtu.be/n75dhHnrEYs) or [The Nightmare](https://youtu.be/i0dlZgqA8ds).\n\n"
        "<co>Note that we are NOT banning 1.0 block types.</c>\n\n"
        "## Does Rated Layouts work the same way as a Geometry Dash Private Server (GDPS)?\n\n"
        "**No.** Our system does not work the same way as a Geometry Dash Private Server (GDPS).\n\n"
        "- Since the mod is integrated into RobTop's servers, we need to set a high bar for everyone in our standards and criteria instead of giving out free rates and ignoring quality control. This generally means a lot of effort is required for a layout to be eligible for a rating here.\n\n"
        "- A moderator position is also not easily obtainable here, since you generally need a strong application and background (i.e. being an established gameplay creator, or a player who can reliably beat many extremes with good gameplay analysis skills).\n\n"
        "- We do not have an exclusive way to 'upload' levels. You upload levels the same way you do in the base game. This mod serves as an integrated separate rating system for levels in RobTop's servers, not as a different server host entirely.\n\n"
        "<cl>**TL;DR:** We are not a private server, and we don't do 'free rates' or 'free moderator positions' here. You also upload levels the same way you do in the base game.</c>\n\n"
        "## Where is the layout and its rate stored? Do you need to upload it differently?\n\n"
        "No. You can upload your layout normally, and then it's given a separate rate on RL. The rate is stored on RL; the layout itself is stored on RobTop's servers.\n\n"
        "## Will there be a mythic rating tier?\n\n"
        "᠌**No. Mythic is unnecessary**, since we already have fairly high standards for Legendary. If we were to add Mythic, only a few levels would ever have it, which would be pointless.\n\n",
        "OK")
        ->show();
}