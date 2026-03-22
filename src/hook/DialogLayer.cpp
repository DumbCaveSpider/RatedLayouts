#include <Geode/Geode.hpp>
#include <Geode/modify/DialogLayer.hpp>
#include "../include/RLDialogIcons.hpp"

using namespace geode::prelude;
using namespace rl;

class $modify(RLDialogLayerHook, DialogLayer) {
    void displayDialogObject(DialogObject* dialogObject) {
        DialogLayer::displayDialogObject(dialogObject);
        if (!dialogObject || !this->m_mainLayer) {
            return;
        }

        // only apply to The Oracle dialogs
        if (dialogObject->m_character == "The Oracle") {
            if (this->m_characterSprite) {
                this->m_characterSprite->setVisible(false);
            } else {
                return;
            }

            if (dialogObject->m_characterFrame >= 0 &&
                dialogObject->m_characterFrame < DialogIconCount) {
                setDialogObjectIcon(this, dialogObject->m_characterFrame);
                return;
            }

            if (dialogObject->getTag() == 10100) {
                setDialogObjectIcon(this, 0);
                return;
            }

            setDialogObjectIcon(this, 0);
            return;
        }

        // restore default character sprite and hide custom icons.
        if (this->m_characterSprite) {
            this->m_characterSprite->setVisible(true);
        }
        for (int frame = 0; frame < DialogIconCount; frame++) {
            auto icon = typeinfo_cast<CCSprite*>(
                this->m_mainLayer->getChildByTag(rl::DialogIconTagBase + frame));
            if (icon) {
                icon->setVisible(false);
            }
        }

        auto customIcon = typeinfo_cast<CCSprite*>(
            this->m_mainLayer->getChildByTag(0xD1A200));
        if (customIcon) {
            customIcon->setVisible(false);
        }
    }
};
