#include <Geode/Geode.hpp>
#include <Geode/modify/CommentCell.hpp>

using namespace geode::prelude;

class $modify(RLCommentCell, CommentCell)
{
    struct Fields
    {
        int role = 0;
    };

    void loadFromComment(GJComment *comment)
    {
        CommentCell::loadFromComment(comment);

        if (!comment)
        {
            log::warn("Comment is null");
            return;
        }

        fetchUserRole(comment->m_accountID);
    }

    void applyCommentTextColor()
    {
        if (m_fields->role == 0)
        {
            return;
        }

        if (!m_mainLayer)
        {
            log::warn("main layer is null, cannot apply color");
            return;
        }

        ccColor3B color;
        if (m_fields->role == 1)
        {
            color = ccc3(0, 150, 255); // mod comment color
        }
        else if (m_fields->role == 2)
        {
            color = ccc3(150, 0, 0); // admin comment color
        }
        else
        {
            return;
        }

        log::debug("Applying comment text color for role: {}", m_fields->role);

        if (auto commentTextLabel = typeinfo_cast<CCLabelBMFont *>(m_mainLayer->getChildByID("comment-text-label")))
        {
            log::debug("Found comment-text-label, applying color");
            commentTextLabel->setColor(color);
        }
    }

    void fetchUserRole(int accountId)
    {
        log::debug("Fetching role for comment user ID: {}", accountId);
        auto getTask = web::WebRequest()
                           .param("accountId", accountId)
                           .get("https://gdrate.arcticwoof.xyz/commentProfile");

        Ref<RLCommentCell> cellRef = this; // commentcell ref

        getTask.listen([cellRef](web::WebResponse *response)
                       {
            log::debug("Received role response from server for comment");

            // did this so it doesnt crash if the cell is deleted before response
            // yea took me a while
            if (!cellRef)
            {
                log::warn("CommentCell has been destroyed, skipping role update");
                return;
            }

            if (!response->ok()) {
                log::warn("Server returned non-ok status: {}", response->code());
                return;
            }

            auto jsonRes = response->json();
            if (!jsonRes) {
                log::warn("Failed to parse JSON response");
                return;
            }

            auto json = jsonRes.unwrap();
            int role = json["role"].asInt().unwrapOrDefault();
            cellRef->m_fields->role = role;

            log::debug("User comment role: {}", role);

            cellRef->loadBadgeForComment(); });
    }

    void loadBadgeForComment()
    {
        auto userNameMenu = typeinfo_cast<CCMenu *>(m_mainLayer->getChildByIDRecursive("username-menu"));
        if (!userNameMenu)
        {
            log::warn("username-menu not found in comment cell");
            return;
        }

        if (m_fields->role == 1)
        {
            applyCommentTextColor();

            auto modBadgeSprite = CCSprite::create("rlBadgeMod.png"_spr);
            modBadgeSprite->setScale(0.7f);
            auto modBadgeButton = CCMenuItemSpriteExtra::create(
                modBadgeSprite,
                this,
                menu_selector(RLCommentCell::onModBadge));

            modBadgeButton->setID("rl-comment-mod-badge");
            userNameMenu->addChild(modBadgeButton);
            userNameMenu->updateLayout();
        }
        else if (m_fields->role == 2)
        {
            applyCommentTextColor();

            auto adminBadgeSprite = CCSprite::create("rlBadgeAdmin.png"_spr);
            adminBadgeSprite->setScale(0.7f);
            auto adminBadgeButton = CCMenuItemSpriteExtra::create(
                adminBadgeSprite,
                this,
                menu_selector(RLCommentCell::onAdminBadge));

            adminBadgeButton->setID("rl-comment-admin-badge");
            userNameMenu->addChild(adminBadgeButton);
            userNameMenu->updateLayout();
        }
    }

    void onModBadge(CCObject *sender)
    {
        FLAlertLayer::create(
            "Layout Moderator",
            "This user can suggest layout levels for Rated Layouts to the Layout Admins.",
            "OK")
            ->show();
    }

    void onAdminBadge(CCObject *sender)
    {
        FLAlertLayer::create(
            "Layout Administrator",
            "This user can rate layout levels for Rated Layouts. They can change the featured ranking on the featured layout levels.",
            "OK")
            ->show();
    }
};