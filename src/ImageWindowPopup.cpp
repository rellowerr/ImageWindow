#include "ImageWindowPopup.hpp"
#include "ImageManager.hpp"
#include "ImageOverlay.hpp"
#include "DragOverlayLayer.hpp"

using namespace geode::prelude;

ImageWindowPopup* ImageWindowPopup::s_instance = nullptr;

void ImageWindowPopup::refreshIfOpen() {
    if (s_instance) {
        s_instance->refreshLibraryList();
    }
}

ImageWindowPopup* ImageWindowPopup::create() {
    auto ret = new ImageWindowPopup();
    if (ret && ret->init(440.f, 290.f)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

ImageWindowPopup::~ImageWindowPopup() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool ImageWindowPopup::init(float width, float height) {
    if (!Popup::init(width, height)) return false;

    s_instance = this;

    this->setTitle("ImageWindow", "goldFont.fnt", 0.75f, 22.f);

    // Setup tab container nodes
    m_libraryNode = CCNode::create();
    m_transformNode = CCNode::create();
    m_settingsNode = CCNode::create();

    m_mainLayer->addChild(m_libraryNode);
    m_mainLayer->addChild(m_transformNode);
    m_mainLayer->addChild(m_settingsNode);

    setupTabs();
    setupLibraryTab();
    setupTransformTab();
    setupSettingsTab();

    selectTab(0);
    return true;
}

void ImageWindowPopup::setupTabs() {
    auto tabMenu = CCMenu::create();
    tabMenu->setPosition(ccp(m_size.width / 2.f, m_size.height - 48.f));
    m_mainLayer->addChild(tabMenu, 5);

    auto sprLib = ButtonSprite::create("Library", "goldFont.fnt", "GJ_button_01.png", 0.65f);
    m_tabBtnLibrary = CCMenuItemSpriteExtra::create(sprLib, this, menu_selector(ImageWindowPopup::onTabClicked));
    m_tabBtnLibrary->setTag(0);
    tabMenu->addChild(m_tabBtnLibrary);

    auto sprTrans = ButtonSprite::create("HUD & Size", "goldFont.fnt", "GJ_button_01.png", 0.65f);
    m_tabBtnTransform = CCMenuItemSpriteExtra::create(sprTrans, this, menu_selector(ImageWindowPopup::onTabClicked));
    m_tabBtnTransform->setTag(1);
    tabMenu->addChild(m_tabBtnTransform);

    auto sprSet = ButtonSprite::create("Settings", "goldFont.fnt", "GJ_button_01.png", 0.65f);
    m_tabBtnSettings = CCMenuItemSpriteExtra::create(sprSet, this, menu_selector(ImageWindowPopup::onTabClicked));
    m_tabBtnSettings->setTag(2);
    tabMenu->addChild(m_tabBtnSettings);

    tabMenu->setLayout(RowLayout::create()->setGap(8.f));
    tabMenu->updateLayout();
}

void ImageWindowPopup::onTabClicked(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    selectTab(node->getTag());
}

void ImageWindowPopup::selectTab(int index) {
    m_currentTab = index;
    m_libraryNode->setVisible(index == 0);
    m_transformNode->setVisible(index == 1);
    m_settingsNode->setVisible(index == 2);

    updateTabButtons();

    if (index == 0) {
        refreshLibraryList();
    } else if (index == 1) {
        updateTransformControls();
    } else if (index == 2) {
        updateSettingsControls();
    }
}

void ImageWindowPopup::updateTabButtons() {
    auto updateBtn = [](CCMenuItemSpriteExtra* btn, bool active, const char* text) {
        if (!btn) return;
        const char* bg = active ? "GJ_button_02.png" : "GJ_button_01.png";
        auto spr = ButtonSprite::create(text, "goldFont.fnt", bg, 0.65f);
        btn->setNormalImage(spr);
    };

    updateBtn(m_tabBtnLibrary, m_currentTab == 0, "Library");
    updateBtn(m_tabBtnTransform, m_currentTab == 1, "HUD & Size");
    updateBtn(m_tabBtnSettings, m_currentTab == 2, "Settings");
}

// ==========================================
// TAB 1: LIBRARY
// ==========================================
void ImageWindowPopup::setupLibraryTab() {
    // Action bar buttons
    auto actionMenu = CCMenu::create();
    actionMenu->setPosition(ccp(m_size.width / 2.f, m_size.height - 80.f));
    m_libraryNode->addChild(actionMenu);

    auto addSpr = ButtonSprite::create("+ Add Image", "goldFont.fnt", "GJ_button_02.png", 0.65f);
    auto addBtn = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(ImageWindowPopup::onAddImage));
    actionMenu->addChild(addBtn);

    auto folderSpr = ButtonSprite::create("Folder", "goldFont.fnt", "GJ_button_04.png", 0.65f);
    auto folderBtn = CCMenuItemSpriteExtra::create(folderSpr, this, menu_selector(ImageWindowPopup::onOpenFolder));
    actionMenu->addChild(folderBtn);

    auto refSpr = ButtonSprite::create("Refresh", "goldFont.fnt", "GJ_button_01.png", 0.65f);
    auto refBtn = CCMenuItemSpriteExtra::create(refSpr, this, menu_selector(ImageWindowPopup::onRefreshLibrary));
    actionMenu->addChild(refBtn);

    actionMenu->setLayout(RowLayout::create()->setGap(10.f));
    actionMenu->updateLayout();

    // List background box
    auto listBg = CCScale9Sprite::create("square02_001.png");
    listBg->setContentSize(CCSize(400.f, 150.f));
    listBg->setPosition(ccp(m_size.width / 2.f, 85.f));
    listBg->setOpacity(130);
    listBg->setColor({ 0, 0, 0 });
    m_libraryNode->addChild(listBg);

    // ScrollLayer for image items
    m_scrollLayer = ScrollLayer::create(CCSize(390.f, 140.f));
    m_scrollLayer->setPosition(ccp(m_size.width / 2.f - 195.f, 15.f));
    m_libraryNode->addChild(m_scrollLayer);
}

void ImageWindowPopup::refreshLibraryList() {
    if (!m_scrollLayer) return;

    auto content = m_scrollLayer->m_contentLayer;
    content->removeAllChildrenWithCleanup(true);

    auto const& images = ImageManager::get()->getImages();
    auto activeName = ImageManager::get()->getActiveImageName();

    if (images.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No images in library.\nClick '+ Add Image' to import\nor put files in mod images folder!", "bigFont.fnt");
        emptyLabel->setAlignment(kCCTextAlignmentCenter);
        emptyLabel->setScale(0.42f);
        emptyLabel->setPosition(ccp(195.f, 70.f));
        emptyLabel->setColor({ 180, 200, 220 });
        content->addChild(emptyLabel);
        content->setContentSize(CCSize(390.f, 140.f));
        return;
    }

    float itemHeight = 44.f;
    float gap = 4.f;
    float totalHeight = std::max(140.f, static_cast<float>(images.size()) * (itemHeight + gap));
    content->setContentSize(CCSize(390.f, totalHeight));

    for (size_t i = 0; i < images.size(); ++i) {
        const auto& item = images[i];
        bool isActive = (item.filename == activeName);

        float itemY = totalHeight - (static_cast<float>(i) + 1.f) * (itemHeight + gap) + 2.f;

        // Row background card
        auto rowBg = CCScale9Sprite::create("square02_001.png");
        rowBg->setContentSize(CCSize(380.f, itemHeight));
        rowBg->setPosition(ccp(195.f, itemY + itemHeight / 2.f));
        if (isActive) {
            rowBg->setColor({ 20, 80, 40 });
            rowBg->setOpacity(190);
        } else {
            rowBg->setColor({ 25, 25, 30 });
            rowBg->setOpacity(140);
        }
        content->addChild(rowBg);

        // Thumbnail
        auto* tex = ImageManager::get()->loadTexture(item.fullPath);
        if (tex) {
            auto thumb = CCSprite::createWithTexture(tex);
            if (thumb) {
                float targetSize = 34.f;
                float sx = targetSize / thumb->getContentSize().width;
                float sy = targetSize / thumb->getContentSize().height;
                thumb->setScale(std::min<float>(sx, sy));
                thumb->setPosition(ccp(26.f, itemY + itemHeight / 2.f));
                content->addChild(thumb);
            }
        }

        // Title and file details
        std::string displayName = item.filename;
        if (displayName.length() > 22) {
            displayName = displayName.substr(0, 19) + "...";
        }
        auto nameLabel = CCLabelBMFont::create(displayName.c_str(), "goldFont.fnt");
        nameLabel->setAnchorPoint(ccp(0.f, 0.5f));
        nameLabel->setScale(0.5f);
        nameLabel->setPosition(ccp(50.f, itemY + itemHeight / 2.f + 7.f));
        content->addChild(nameLabel);

        auto details = fmt::format("{}x{} • {}", 
            static_cast<int>(item.dimensions.width), 
            static_cast<int>(item.dimensions.height), 
            ImageManager::formatFileSize(item.fileSize));
        auto detailsLabel = CCLabelBMFont::create(details.c_str(), "chatFont.fnt");
        detailsLabel->setAnchorPoint(ccp(0.f, 0.5f));
        detailsLabel->setScale(0.5f);
        detailsLabel->setColor({ 180, 180, 180 });
        detailsLabel->setPosition(ccp(50.f, itemY + itemHeight / 2.f - 8.f));
        content->addChild(detailsLabel);

        // Row menu for buttons
        auto rowMenu = CCMenu::create();
        rowMenu->setPosition(ccp(0, 0));
        content->addChild(rowMenu);

        // Select button
        ButtonSprite* selSpr = nullptr;
        if (isActive) {
            selSpr = ButtonSprite::create("Active", "goldFont.fnt", "GJ_button_02.png", 0.6f);
        } else {
            selSpr = ButtonSprite::create("Select", "goldFont.fnt", "GJ_button_01.png", 0.6f);
        }
        auto selBtn = CCMenuItemSpriteExtra::create(selSpr, this, menu_selector(ImageWindowPopup::onSelectImage));
        selBtn->setPosition(ccp(300.f, itemY + itemHeight / 2.f));
        selBtn->setUserObject(CCString::create(item.filename.c_str()));
        rowMenu->addChild(selBtn);

        // Delete button
        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        if (delSpr) {
            delSpr->setScale(0.6f);
            auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(ImageWindowPopup::onDeleteImage));
            delBtn->setPosition(ccp(358.f, itemY + itemHeight / 2.f));
            delBtn->setUserObject(CCString::create(item.filename.c_str()));
            rowMenu->addChild(delBtn);
        }
    }

    m_scrollLayer->scrollToTop();
}

void ImageWindowPopup::onAddImage(CCObject*) {
    ImageManager::get()->pickAndImportImage();
}

void ImageWindowPopup::onOpenFolder(CCObject*) {
    ImageManager::get()->openImagesFolder();
}

void ImageWindowPopup::onRefreshLibrary(CCObject*) {
    ImageManager::get()->refreshImages();
    refreshLibraryList();
    Notification::create("Library refreshed", NotificationIcon::Info)->show();
}

void ImageWindowPopup::onSelectImage(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto nameObj = static_cast<CCString*>(btn->getUserObject());
    if (nameObj) {
        ImageManager::get()->setActiveImage(nameObj->getCString());
        refreshLibraryList();
        Notification::create(fmt::format("Selected: {}", nameObj->getCString()), NotificationIcon::Success)->show();
    }
}

void ImageWindowPopup::onDeleteImage(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto nameObj = static_cast<CCString*>(btn->getUserObject());
    if (!nameObj) return;

    std::string filename = nameObj->getCString();

    geode::createQuickPopup(
        "Delete Image",
        fmt::format("Are you sure you want to delete \"{}\"?", filename),
        "Cancel", "Delete",
        [this, filename](FLAlertLayer*, bool btn2) {
            if (btn2) {
                ImageManager::get()->deleteImage(filename);
                this->refreshLibraryList();
                Notification::create("Image deleted", NotificationIcon::Info)->show();
            }
        }
    );
}

// ==========================================
// TAB 2: TRANSFORM & HUD
// ==========================================
void ImageWindowPopup::setupTransformTab() {
    auto leftMenu = CCMenu::create();
    leftMenu->setPosition(ccp(120.f, m_size.height / 2.f - 10.f));
    m_transformNode->addChild(leftMenu);

    // Scale Slider
    m_scaleLabel = CCLabelBMFont::create("Scale: 100%", "chatFont.fnt");
    m_scaleLabel->setScale(0.7f);
    m_scaleLabel->setPosition(ccp(0.f, 75.f));
    leftMenu->addChild(m_scaleLabel);

    m_scaleSlider = Slider::create(this, menu_selector(ImageWindowPopup::onScaleSlider), 0.65f);
    m_scaleSlider->setPosition(ccp(0.f, 52.f));
    leftMenu->addChild(m_scaleSlider);

    // Opacity Slider
    m_opacityLabel = CCLabelBMFont::create("Opacity: 100%", "chatFont.fnt");
    m_opacityLabel->setScale(0.7f);
    m_opacityLabel->setPosition(ccp(0.f, 25.f));
    leftMenu->addChild(m_opacityLabel);

    m_opacitySlider = Slider::create(this, menu_selector(ImageWindowPopup::onOpacitySlider), 0.65f);
    m_opacitySlider->setPosition(ccp(0.f, 2.f));
    leftMenu->addChild(m_opacitySlider);

    // Rotation Slider
    m_rotationLabel = CCLabelBMFont::create("Rotation: 0 deg", "chatFont.fnt");
    m_rotationLabel->setScale(0.7f);
    m_rotationLabel->setPosition(ccp(0.f, -25.f));
    leftMenu->addChild(m_rotationLabel);

    m_rotationSlider = Slider::create(this, menu_selector(ImageWindowPopup::onRotationSlider), 0.65f);
    m_rotationSlider->setPosition(ccp(0.f, -48.f));
    leftMenu->addChild(m_rotationSlider);

    // Toggles at bottom
    auto togglesMenu = CCMenu::create();
    togglesMenu->setPosition(ccp(m_size.width / 2.f, 30.f));
    m_transformNode->addChild(togglesMenu);

    // Frame toggler
    auto off1 = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto on1 = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    off1->setScale(0.6f); on1->setScale(0.6f);
    m_frameToggler = CCMenuItemToggler::create(off1, on1, this, menu_selector(ImageWindowPopup::onToggleFrame));
    togglesMenu->addChild(m_frameToggler);

    auto frameLabel = CCLabelBMFont::create("HUD Frame", "chatFont.fnt");
    frameLabel->setScale(0.6f);
    togglesMenu->addChild(frameLabel);

    // Flip X toggler
    auto off2 = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto on2 = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    off2->setScale(0.6f); on2->setScale(0.6f);
    m_flipXToggler = CCMenuItemToggler::create(off2, on2, this, menu_selector(ImageWindowPopup::onToggleFlipX));
    togglesMenu->addChild(m_flipXToggler);

    auto flipXLabel = CCLabelBMFont::create("Flip X", "chatFont.fnt");
    flipXLabel->setScale(0.6f);
    togglesMenu->addChild(flipXLabel);

    // Flip Y toggler
    auto off3 = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto on3 = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    off3->setScale(0.6f); on3->setScale(0.6f);
    m_flipYToggler = CCMenuItemToggler::create(off3, on3, this, menu_selector(ImageWindowPopup::onToggleFlipY));
    togglesMenu->addChild(m_flipYToggler);

    auto flipYLabel = CCLabelBMFont::create("Flip Y", "chatFont.fnt");
    flipYLabel->setScale(0.6f);
    togglesMenu->addChild(flipYLabel);

    togglesMenu->setLayout(RowLayout::create()->setGap(10.f));
    togglesMenu->updateLayout();

    // Right side: Presets grid and Drag Mode button
    auto rightMenu = CCMenu::create();
    rightMenu->setPosition(ccp(325.f, m_size.height / 2.f + 10.f));
    m_transformNode->addChild(rightMenu);

    auto presetsTitle = CCLabelBMFont::create("Presets:", "goldFont.fnt");
    presetsTitle->setScale(0.55f);
    presetsTitle->setPosition(ccp(325.f, m_size.height - 82.f));
    m_transformNode->addChild(presetsTitle);

    const char* presetNames[9] = {
        "TL", "TC", "TR",
        "CL", " C", "CR",
        "BL", "BC", "BR"
    };

    auto gridMenu = CCMenu::create();
    gridMenu->setPosition(ccp(325.f, 135.f));
    m_transformNode->addChild(gridMenu);

    for (int i = 0; i < 9; ++i) {
        auto spr = ButtonSprite::create(presetNames[i], "goldFont.fnt", "GJ_button_01.png", 0.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImageWindowPopup::onPresetPosition));
        btn->setTag(i);

        float gx = ((i % 3) - 1) * 36.f;
        float gy = (1 - (i / 3)) * 26.f;
        btn->setPosition(ccp(gx, gy));
        gridMenu->addChild(btn);
    }

    // Drag Mode and Reset buttons
    auto bottomActionMenu = CCMenu::create();
    bottomActionMenu->setPosition(ccp(325.f, 65.f));
    m_transformNode->addChild(bottomActionMenu);

    auto dragSpr = ButtonSprite::create("Drag Mode", "goldFont.fnt", "GJ_button_02.png", 0.65f);
    auto dragBtn = CCMenuItemSpriteExtra::create(dragSpr, this, menu_selector(ImageWindowPopup::onOpenDragMode));
    bottomActionMenu->addChild(dragBtn);

    auto resetSpr = ButtonSprite::create("Reset", "goldFont.fnt", "GJ_button_06.png", 0.65f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(ImageWindowPopup::onResetTransform));
    bottomActionMenu->addChild(resetBtn);

    bottomActionMenu->setLayout(RowLayout::create()->setGap(8.f));
    bottomActionMenu->updateLayout();
}

void ImageWindowPopup::updateTransformControls() {
    float scale = static_cast<float>(Mod::get()->getSettingValue<double>("scale"));
    float opacity = static_cast<float>(Mod::get()->getSettingValue<double>("opacity"));
    float rotation = static_cast<float>(Mod::get()->getSettingValue<double>("rotation"));
    bool showFrame = Mod::get()->getSettingValue<bool>("show-frame");
    bool flipX = Mod::get()->getSettingValue<bool>("flip-x");
    bool flipY = Mod::get()->getSettingValue<bool>("flip-y");

    if (m_scaleSlider) {
        float normScale = (scale - 0.1f) / (3.0f - 0.1f);
        m_scaleSlider->setValue(std::clamp(normScale, 0.0f, 1.0f));
    }
    if (m_scaleLabel) {
        m_scaleLabel->setString(fmt::format("Scale: {:.0f}%", scale * 100.f).c_str());
    }

    if (m_opacitySlider) {
        float normOp = (opacity - 0.05f) / (1.0f - 0.05f);
        m_opacitySlider->setValue(std::clamp(normOp, 0.0f, 1.0f));
    }
    if (m_opacityLabel) {
        m_opacityLabel->setString(fmt::format("Opacity: {:.0f}%", opacity * 100.f).c_str());
    }

    if (m_rotationSlider) {
        float normRot = rotation / 360.f;
        m_rotationSlider->setValue(std::clamp(normRot, 0.0f, 1.0f));
    }
    if (m_rotationLabel) {
        m_rotationLabel->setString(fmt::format("Rotation: {:.0f} deg", rotation).c_str());
    }

    if (m_frameToggler) m_frameToggler->toggle(showFrame);
    if (m_flipXToggler) m_flipXToggler->toggle(flipX);
    if (m_flipYToggler) m_flipYToggler->toggle(flipY);
}

void ImageWindowPopup::onScaleSlider(CCObject*) {
    if (!m_scaleSlider) return;
    float val = m_scaleSlider->getValue();
    float scale = 0.1f + val * (3.0f - 0.1f);

    Mod::get()->setSettingValue("scale", static_cast<double>(scale));
    if (m_scaleLabel) {
        m_scaleLabel->setString(fmt::format("Scale: {:.0f}%", scale * 100.f).c_str());
    }
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onOpacitySlider(CCObject*) {
    if (!m_opacitySlider) return;
    float val = m_opacitySlider->getValue();
    float opacity = 0.05f + val * (1.0f - 0.05f);

    Mod::get()->setSettingValue("opacity", static_cast<double>(opacity));
    if (m_opacityLabel) {
        m_opacityLabel->setString(fmt::format("Opacity: {:.0f}%", opacity * 100.f).c_str());
    }
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onRotationSlider(CCObject*) {
    if (!m_rotationSlider) return;
    float val = m_rotationSlider->getValue();
    float rot = val * 360.f;

    Mod::get()->setSettingValue("rotation", static_cast<double>(rot));
    if (m_rotationLabel) {
        m_rotationLabel->setString(fmt::format("Rotation: {:.0f} deg", rot).c_str());
    }
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onToggleFrame(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("show-frame", !toggler->isToggled());
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onToggleFlipX(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("flip-x", !toggler->isToggled());
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onToggleFlipY(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("flip-y", !toggler->isToggled());
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onPresetPosition(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();

    // 3x3 preset positions
    float px[9] = { 0.12f, 0.50f, 0.88f, 0.12f, 0.50f, 0.88f, 0.12f, 0.50f, 0.88f };
    float py[9] = { 0.88f, 0.88f, 0.88f, 0.50f, 0.50f, 0.50f, 0.12f, 0.12f, 0.12f };

    if (tag >= 0 && tag < 9) {
        Mod::get()->setSettingValue("pos-x", static_cast<double>(px[tag]));
        Mod::get()->setSettingValue("pos-y", static_cast<double>(py[tag]));
        ImageOverlay::updateAll();
        Notification::create("Position updated", NotificationIcon::Success)->show();
    }
}

void ImageWindowPopup::onOpenDragMode(CCObject*) {
    this->setVisible(false);

    auto dragLayer = DragOverlayLayer::create([this]() {
        this->setVisible(true);
        this->updateTransformControls();
    });
    dragLayer->show();
}

void ImageWindowPopup::onResetTransform(CCObject*) {
    Mod::get()->setSettingValue("scale", 1.0);
    Mod::get()->setSettingValue("opacity", 1.0);
    Mod::get()->setSettingValue("rotation", 0.0);
    Mod::get()->setSettingValue("pos-x", 0.85);
    Mod::get()->setSettingValue("pos-y", 0.85);
    Mod::get()->setSettingValue("flip-x", false);
    Mod::get()->setSettingValue("flip-y", false);
    Mod::get()->setSettingValue("show-frame", false);

    updateTransformControls();
    ImageOverlay::updateAll();
    Notification::create("Settings reset", NotificationIcon::Info)->show();
}

// ==========================================
// TAB 3: SETTINGS
// ==========================================
void ImageWindowPopup::setupSettingsTab() {
    auto menu = CCMenu::create();
    menu->setPosition(ccp(130.f, m_size.height / 2.f - 15.f));
    m_settingsNode->addChild(menu);

    auto createToggleRow = [this, menu](const char* labelText, SEL_MenuHandler handler, CCMenuItemToggler*& outToggler, float yOffset) {
        auto off = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        auto on = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        off->setScale(0.65f); on->setScale(0.65f);

        outToggler = CCMenuItemToggler::create(off, on, this, handler);
        outToggler->setPosition(ccp(-90.f, yOffset));
        menu->addChild(outToggler);

        auto lbl = CCLabelBMFont::create(labelText, "chatFont.fnt");
        lbl->setAnchorPoint(ccp(0.f, 0.5f));
        lbl->setScale(0.65f);
        lbl->setPosition(ccp(-70.f, yOffset));
        menu->addChild(lbl);
    };

    createToggleRow("Enable ImageWindow", menu_selector(ImageWindowPopup::onToggleEnabled), m_enabledToggler, 60.f);
    createToggleRow("Hide on Pause", menu_selector(ImageWindowPopup::onToggleHideOnPause), m_hideOnPauseToggler, 30.f);
    createToggleRow("Hide on Death", menu_selector(ImageWindowPopup::onToggleHideOnDeath), m_hideOnDeathToggler, 0.f);
    createToggleRow("Practice Mode Only", menu_selector(ImageWindowPopup::onTogglePracticeOnly), m_practiceOnlyToggler, -30.f);
    createToggleRow("Show in Editor", menu_selector(ImageWindowPopup::onToggleShowInEditor), m_showInEditorToggler, -60.f);

    // Right Column: Z-Order & File Info
    auto rightMenu = CCMenu::create();
    rightMenu->setPosition(ccp(330.f, 140.f));
    m_settingsNode->addChild(rightMenu);

    auto zTitle = CCLabelBMFont::create("Layer (Z-Order):", "goldFont.fnt");
    zTitle->setScale(0.55f);
    zTitle->setPosition(ccp(0.f, 35.f));
    rightMenu->addChild(zTitle);

    m_zOrderBtnSpr = ButtonSprite::create("Over HUD", "goldFont.fnt", "GJ_button_01.png", 0.65f);
    auto zBtn = CCMenuItemSpriteExtra::create(m_zOrderBtnSpr, this, menu_selector(ImageWindowPopup::onCycleZOrder));
    zBtn->setPosition(ccp(0.f, 5.f));
    rightMenu->addChild(zBtn);

    // Info card for active image
    auto infoBg = CCScale9Sprite::create("square02_001.png");
    infoBg->setContentSize(CCSize(160.f, 65.f));
    infoBg->setPosition(ccp(330.f, 55.f));
    infoBg->setOpacity(120);
    infoBg->setColor({ 0, 0, 0 });
    m_settingsNode->addChild(infoBg);

    m_activeFileInfoLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_activeFileInfoLabel->setScale(0.55f);
    m_activeFileInfoLabel->setAlignment(kCCTextAlignmentCenter);
    m_activeFileInfoLabel->setPosition(ccp(330.f, 55.f));
    m_settingsNode->addChild(m_activeFileInfoLabel);
}

void ImageWindowPopup::updateSettingsControls() {
    m_enabledToggler->toggle(Mod::get()->getSettingValue<bool>("enabled"));
    m_hideOnPauseToggler->toggle(Mod::get()->getSettingValue<bool>("hide-on-pause"));
    m_hideOnDeathToggler->toggle(Mod::get()->getSettingValue<bool>("hide-on-death"));
    m_practiceOnlyToggler->toggle(Mod::get()->getSettingValue<bool>("practice-only"));
    m_showInEditorToggler->toggle(Mod::get()->getSettingValue<bool>("show-in-editor"));

    int64_t z = Mod::get()->getSettingValue<int64_t>("z-order");
    const char* zStr = "Over HUD";
    if (z == 0) zStr = "Behind Level";
    else if (z == 1) zStr = "Under UI";
    m_zOrderBtnSpr->setString(zStr);

    auto activeName = ImageManager::get()->getActiveImageName();
    if (activeName.empty()) {
        m_activeFileInfoLabel->setString("Active Image:\n[None Selected]");
    } else {
        if (activeName.length() > 18) {
            activeName = activeName.substr(0, 15) + "...";
        }
        m_activeFileInfoLabel->setString(fmt::format("Active Image:\n{}", activeName).c_str());
    }
}

void ImageWindowPopup::onToggleEnabled(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("enabled", !toggler->isToggled());
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onToggleHideOnPause(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("hide-on-pause", !toggler->isToggled());
}

void ImageWindowPopup::onToggleHideOnDeath(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("hide-on-death", !toggler->isToggled());
}

void ImageWindowPopup::onTogglePracticeOnly(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("practice-only", !toggler->isToggled());
    ImageOverlay::updateAll();
}

void ImageWindowPopup::onToggleShowInEditor(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    Mod::get()->setSettingValue("show-in-editor", !toggler->isToggled());
}

void ImageWindowPopup::onCycleZOrder(CCObject*) {
    int64_t z = Mod::get()->getSettingValue<int64_t>("z-order");
    z = (z + 1) % 3;
    Mod::get()->setSettingValue("z-order", z);

    updateSettingsControls();
    Notification::create("Layer changed (re-enter level)", NotificationIcon::Info)->show();
}
