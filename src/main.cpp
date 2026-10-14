#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "ImageManager.hpp"
#include "ImageOverlay.hpp"
#include "ImageWindowPopup.hpp"

using namespace geode::prelude;

// Hook PlayLayer to display overlay and handle gameplay lifecycle
class $modify(IWPlayLayer, PlayLayer) {
    struct Fields {
        ImageOverlay* m_imageOverlay = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto overlay = ImageOverlay::create();
        overlay->setID("image-window-overlay"_spr);

        int64_t zPref = Mod::get()->getSettingValue<int64_t>("z-order");
        if (zPref == 0) {
            // Behind objects
            this->addChild(overlay, -10);
        } else if (zPref == 1) {
            // Under UI buttons
            if (m_uiLayer) {
                m_uiLayer->addChild(overlay, -1);
            } else {
                this->addChild(overlay, 10);
            }
        } else {
            // Over UI (HUD)
            if (m_uiLayer) {
                m_uiLayer->addChild(overlay, 100);
            } else {
                this->addChild(overlay, 100);
            }
        }

        m_fields->m_imageOverlay = overlay;
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->m_imageOverlay) {
            m_fields->m_imageOverlay->setForceHidden(false);
            m_fields->m_imageOverlay->updateOverlay();
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (Mod::get()->getSettingValue<bool>("hide-on-death")) {
            if (m_fields->m_imageOverlay) {
                m_fields->m_imageOverlay->setForceHidden(true);
            }
        }
    }

    void pauseGame(bool p0) {
        PlayLayer::pauseGame(p0);
        if (Mod::get()->getSettingValue<bool>("hide-on-pause")) {
            if (m_fields->m_imageOverlay) {
                m_fields->m_imageOverlay->setForceHidden(true);
            }
        }
    }

    void resume() {
        PlayLayer::resume();
        if (m_fields->m_imageOverlay) {
            m_fields->m_imageOverlay->setForceHidden(false);
            m_fields->m_imageOverlay->updateOverlay();
        }
    }

    void keyDown(enumKeyCodes key) {
        // F8 hotkey: quick toggle overlay on/off
        if (key == KEY_F8) {
            bool enabled = Mod::get()->getSettingValue<bool>("enabled");
            Mod::get()->setSettingValue("enabled", !enabled);
            ImageOverlay::updateAll();

            auto msg = !enabled ? "ImageWindow: Включено" : "ImageWindow: Выключено";
            Notification::create(msg, NotificationIcon::Info)->show();
            return;
        }

        PlayLayer::keyDown(key);
    }
};

// Hook PauseLayer to provide a convenient in-level button
class $modify(IWPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        CCNode* btnSpr = CCSprite::createWithSpriteFrameName("GJ_paintBtn_001.png");
        if (!btnSpr) {
            btnSpr = ButtonSprite::create("ImgWin", "goldFont.fnt", "GJ_button_01.png", 0.7f);
        } else {
            btnSpr->setScale(0.85f);
        }

        auto btn = CCMenuItemSpriteExtra::create(
            btnSpr, this, menu_selector(IWPauseLayer::onImageWindowBtn)
        );
        btn->setID("image-window-btn"_spr);

        // Find standard menu or create fallback
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) {
            menu = this->getChildByID("bottom-button-menu");
        }
        if (!menu) {
            menu = CCMenu::create();
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            menu->setPosition(ccp(winSize.width - 35.f, 35.f));
            this->addChild(menu, 10);
        }

        menu->addChild(btn);
        menu->updateLayout();
    }

    void onImageWindowBtn(CCObject*) {
        ImageWindowPopup::create()->show();
    }
};

// Hook LevelEditorLayer if enabled in settings
class $modify(IWLevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level) {
        if (!LevelEditorLayer::init(level)) return false;

        if (Mod::get()->getSettingValue<bool>("show-in-editor")) {
            auto overlay = ImageOverlay::create();
            overlay->setID("image-window-overlay"_spr);
            if (m_editorUI) {
                m_editorUI->addChild(overlay, 100);
            } else {
                this->addChild(overlay, 100);
            }
        }

        return true;
    }
};

// Hook EditorPauseLayer to add button in editor pause menu
class $modify(IWEditorPauseLayer, EditorPauseLayer) {
    void customSetup() {
        EditorPauseLayer::customSetup();

        CCNode* btnSpr = CCSprite::createWithSpriteFrameName("GJ_paintBtn_001.png");
        if (!btnSpr) {
            btnSpr = ButtonSprite::create("ImgWin", "goldFont.fnt", "GJ_button_01.png", 0.7f);
        } else {
            btnSpr->setScale(0.85f);
        }

        auto btn = CCMenuItemSpriteExtra::create(
            btnSpr, this, menu_selector(IWEditorPauseLayer::onImageWindowBtn)
        );
        btn->setID("image-window-btn"_spr);

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) {
            menu = this->getChildByID("actions-menu");
        }
        if (!menu) {
            menu = CCMenu::create();
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            menu->setPosition(ccp(winSize.width - 35.f, 35.f));
            this->addChild(menu, 10);
        }

        menu->addChild(btn);
        menu->updateLayout();
    }

    void onImageWindowBtn(CCObject*) {
        ImageWindowPopup::create()->show();
    }
};

// Hook MenuLayer to add access from main menu
class $modify(IWMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto bottomMenu = this->getChildByID("bottom-menu");
        if (bottomMenu) {
            CCNode* btnSpr = CCSprite::createWithSpriteFrameName("GJ_paintBtn_001.png");
            if (!btnSpr) {
                btnSpr = ButtonSprite::create("ImgWin", "goldFont.fnt", "GJ_button_01.png", 0.7f);
            } else {
                btnSpr->setScale(0.85f);
            }

            auto btn = CCMenuItemSpriteExtra::create(
                btnSpr, this, menu_selector(IWMenuLayer::onImageWindowBtn)
            );
            btn->setID("image-window-btn"_spr);
            bottomMenu->addChild(btn);
            bottomMenu->updateLayout();
        }

        return true;
    }

    void onImageWindowBtn(CCObject*) {
        ImageWindowPopup::create()->show();
    }
};

// Early initialization for ImageManager
$on_mod(Loaded) {
    ImageManager::get()->init();
}
