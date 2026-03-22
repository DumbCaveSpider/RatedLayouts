#include "RLDialogIcons.hpp"

namespace rl {

    void setDialogObjectIcon(DialogLayer* dialog, int characterFrame) {
        if (!dialog || !dialog->m_mainLayer || !dialog->m_characterSprite) {
            return;
        }

        dialog->m_characterSprite->setVisible(false);

        // when characterFrame = 0 LITERALLY REMOVES THE SPRITE lol
        int iconFrame = characterFrame - 1;
        if (characterFrame <= 0) {
            iconFrame = 0;
        }
        if (iconFrame < 0 || iconFrame >= DialogIconCount) {
            iconFrame = 0;
        }

        for (int frame = 0; frame < DialogIconCount; frame++) {
            int tag = DialogIconTagBase + frame;
            auto icon = typeinfo_cast<CCSprite*>(dialog->m_mainLayer->getChildByTag(tag));
            if (!icon) {
                auto frameName = fmt::format("RL_dialogIcon_{:02}.png"_spr, frame);
                icon = CCSprite::createWithSpriteFrameName(frameName.c_str());
                if (!icon) {
                    continue;
                }
                icon->setPosition(dialog->m_characterSprite->getPosition());
                icon->setTag(tag);
                dialog->m_mainLayer->addChild(icon, 3);
            } else {
                icon->setPosition(dialog->m_characterSprite->getPosition());
            }

            icon->setVisible(frame == iconFrame);
        }
    }

    void setDialogObjectCustomIcon(DialogLayer* dialog, const std::string& frameName) {
        if (!dialog || !dialog->m_mainLayer || !dialog->m_characterSprite) {
            return;
        }

        dialog->m_characterSprite->setVisible(false);

        for (int frame = 0; frame < DialogIconCount; frame++) {
            int tag = DialogIconTagBase + frame;
            if (auto existing = typeinfo_cast<CCSprite*>(dialog->m_mainLayer->getChildByTag(tag))) {
                existing->setVisible(false);
            }
        }

        static constexpr int CustomDialogIconTag = 0xD1A200;
        auto icon = typeinfo_cast<CCSprite*>(dialog->m_mainLayer->getChildByTag(CustomDialogIconTag));
        if (!icon) {
            icon = CCSprite::createWithSpriteFrameName(frameName.c_str());
            if (!icon) {
                return;
            }
            icon->setTag(CustomDialogIconTag);
            dialog->m_mainLayer->addChild(icon, 3);
        }

        icon->setPosition(dialog->m_characterSprite->getPosition());
        icon->setVisible(true);
    }

}  // namespace rl
