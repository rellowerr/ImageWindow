#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>

class ImageWindowPopup : public geode::Popup {
private:
    int m_currentTab = 0;

    // Tab buttons
    CCMenuItemSpriteExtra* m_tabBtnLibrary = nullptr;
    CCMenuItemSpriteExtra* m_tabBtnTransform = nullptr;
    CCMenuItemSpriteExtra* m_tabBtnSettings = nullptr;

    // Content container nodes
    cocos2d::CCNode* m_libraryNode = nullptr;
    cocos2d::CCNode* m_transformNode = nullptr;
    cocos2d::CCNode* m_settingsNode = nullptr;

    // Tab 1 (Library) elements
    geode::ScrollLayer* m_scrollLayer = nullptr;

    // Tab 2 (Transform) elements
    Slider* m_scaleSlider = nullptr;
    Slider* m_opacitySlider = nullptr;
    Slider* m_rotationSlider = nullptr;
    cocos2d::CCLabelBMFont* m_scaleLabel = nullptr;
    cocos2d::CCLabelBMFont* m_opacityLabel = nullptr;
    cocos2d::CCLabelBMFont* m_rotationLabel = nullptr;
    CCMenuItemToggler* m_frameToggler = nullptr;
    CCMenuItemToggler* m_flipXToggler = nullptr;
    CCMenuItemToggler* m_flipYToggler = nullptr;

    // Tab 3 (Settings) elements
    CCMenuItemToggler* m_enabledToggler = nullptr;
    CCMenuItemToggler* m_hideOnPauseToggler = nullptr;
    CCMenuItemToggler* m_hideOnDeathToggler = nullptr;
    CCMenuItemToggler* m_practiceOnlyToggler = nullptr;
    CCMenuItemToggler* m_showInEditorToggler = nullptr;
    ButtonSprite* m_zOrderBtnSpr = nullptr;
    cocos2d::CCLabelBMFont* m_activeFileInfoLabel = nullptr;

    bool init(float width, float height);

    void setupTabs();
    void setupLibraryTab();
    void setupTransformTab();
    void setupSettingsTab();

    void selectTab(int index);
    void updateTabButtons();

    // Callbacks
    void onTabClicked(CCObject* sender);
    void onAddImage(CCObject*);
    void onOpenFolder(CCObject*);
    void onRefreshLibrary(CCObject*);
    void onSelectImage(CCObject* sender);
    void onDeleteImage(CCObject* sender);

    void onScaleSlider(CCObject* sender);
    void onOpacitySlider(CCObject* sender);
    void onRotationSlider(CCObject* sender);
    void onToggleFrame(CCObject* sender);
    void onToggleFlipX(CCObject* sender);
    void onToggleFlipY(CCObject* sender);
    void onPresetPosition(CCObject* sender);
    void onOpenDragMode(CCObject*);
    void onResetTransform(CCObject*);

    void onToggleEnabled(CCObject* sender);
    void onToggleHideOnPause(CCObject* sender);
    void onToggleHideOnDeath(CCObject* sender);
    void onTogglePracticeOnly(CCObject* sender);
    void onToggleShowInEditor(CCObject* sender);
    void onCycleZOrder(CCObject* sender);

    void updateTransformControls();
    void updateSettingsControls();
    void refreshLibraryList();

public:
    static ImageWindowPopup* create();
};
