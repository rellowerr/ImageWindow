#include "DragOverlayLayer.hpp"
#include "ImageManager.hpp"
#include "ImageOverlay.hpp"

using namespace geode::prelude;

DragOverlayLayer* DragOverlayLayer::create(std::function<void()> onCloseCallback) {
    auto ret = new DragOverlayLayer();
    if (ret && ret->init(onCloseCallback)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DragOverlayLayer::init(std::function<void()> onCloseCallback) {
    if (!FLAlertLayer::init(140)) return false;

    m_onCloseCallback = onCloseCallback;
    m_winSize = CCDirector::sharedDirector()->getWinSize();

    // Read current positions
    m_normX = static_cast<float>(Mod::get()->getSettingValue<double>("pos-x"));
    m_normY = static_cast<float>(Mod::get()->getSettingValue<double>("pos-y"));
    m_currentPos = ccp(m_normX * m_winSize.width, m_normY * m_winSize.height);

    // Subtle dark backdrop to highlight draggable element
    auto bg = CCScale9Sprite::create("square02_001.png");
    if (bg) {
        bg->setContentSize(m_winSize);
        bg->setPosition(m_winSize / 2);
        bg->setOpacity(100);
        bg->setColor({ 0, 0, 0 });
        this->addChild(bg, -1);
    }

    // Top instruction label
    auto titleLabel = CCLabelBMFont::create("Перетаскивание изображения", "bigFont.fnt");
    titleLabel->setScale(0.6f);
    titleLabel->setPosition(ccp(m_winSize.width / 2, m_winSize.height - 28.f));
    this->addChild(titleLabel, 10);

    auto hintLabel = CCLabelBMFont::create("Зажмите мышью и перемещайте в любую точку экрана", "chatFont.fnt");
    hintLabel->setScale(0.75f);
    hintLabel->setColor({ 200, 220, 255 });
    hintLabel->setPosition(ccp(m_winSize.width / 2, m_winSize.height - 52.f));
    this->addChild(hintLabel, 10);

    // Bottom coordinates label
    m_coordsLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_coordsLabel->setScale(0.65f);
    m_coordsLabel->setPosition(ccp(m_winSize.width / 2, 28.f));
    this->addChild(m_coordsLabel, 10);

    // Active image preview sprite
    auto* texture = ImageManager::get()->getActiveTexture();
    m_previewSprite = CCSprite::create();
    if (texture) {
        m_previewSprite->setTexture(texture);
        m_previewSprite->setTextureRect(CCRect(0.f, 0.f, texture->getContentSize().width, texture->getContentSize().height));
    }
    m_previewSprite->setAnchorPoint(ccp(0.5f, 0.5f));

    // Transform properties
    float scale = static_cast<float>(Mod::get()->getSettingValue<double>("scale"));
    float opacity = static_cast<float>(Mod::get()->getSettingValue<double>("opacity"));
    float rotation = static_cast<float>(Mod::get()->getSettingValue<double>("rotation"));
    bool flipX = Mod::get()->getSettingValue<bool>("flip-x");
    bool flipY = Mod::get()->getSettingValue<bool>("flip-y");

    m_previewSprite->setScale(scale);
    m_previewSprite->setOpacity(static_cast<GLubyte>(std::clamp(opacity, 0.0f, 1.0f) * 255.f));
    m_previewSprite->setRotation(rotation);
    m_previewSprite->setFlipX(flipX);
    m_previewSprite->setFlipY(flipY);
    m_previewSprite->setPosition(m_currentPos);
    this->addChild(m_previewSprite, 5);

    // Border highlight
    m_borderNode = CCScale9Sprite::create("square02_001.png");
    if (m_borderNode) {
        m_borderNode->setColor({ 0, 255, 200 });
        m_borderNode->setOpacity(180);
        this->addChild(m_borderNode, 4);
    }

    // Top control buttons menu
    auto menu = CCMenu::create();
    menu->setPosition(CCPointZero);
    this->addChild(menu, 15);

    // Save button (top right)
    auto saveSpr = ButtonSprite::create("Сохранить", "goldFont.fnt", "GJ_button_02.png", 0.75f);
    auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(DragOverlayLayer::onSave));
    saveBtn->setPosition(ccp(m_winSize.width - 60.f, m_winSize.height - 30.f));
    menu->addChild(saveBtn);

    // Cancel button (top left)
    auto cancelSpr = ButtonSprite::create("Отмена", "goldFont.fnt", "GJ_button_06.png", 0.75f);
    auto cancelBtn = CCMenuItemSpriteExtra::create(cancelSpr, this, menu_selector(DragOverlayLayer::onCancel));
    cancelBtn->setPosition(ccp(55.f, m_winSize.height - 30.f));
    menu->addChild(cancelBtn);

    updateVisuals();
    return true;
}

void DragOverlayLayer::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

void DragOverlayLayer::updateVisuals() {
    if (m_previewSprite) {
        m_previewSprite->setPosition(m_currentPos);

        if (m_borderNode) {
            auto sprSize = m_previewSprite->getScaledContentSize();
            float padding = 8.f;
            m_borderNode->setContentSize(CCSize(sprSize.width + padding * 2.f, sprSize.height + padding * 2.f));
            m_borderNode->setPosition(m_currentPos);
            m_borderNode->setRotation(m_previewSprite->getRotation());
        }
    }

    if (m_coordsLabel) {
        int pctX = static_cast<int>(std::round(m_normX * 100.f));
        int pctY = static_cast<int>(std::round(m_normY * 100.f));
        m_coordsLabel->setString(fmt::format("X: {}%  |  Y: {}%", pctX, pctY).c_str());
    }
}

bool DragOverlayLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    auto loc = touch->getLocation();
    m_touchOffset = loc - m_currentPos;
    m_isDragging = true;
    return true;
}

void DragOverlayLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_isDragging) return;

    auto loc = touch->getLocation();
    m_currentPos = loc - m_touchOffset;

    // Clamp inside screen bounds
    m_currentPos.x = std::clamp(m_currentPos.x, 0.f, m_winSize.width);
    m_currentPos.y = std::clamp(m_currentPos.y, 0.f, m_winSize.height);

    m_normX = m_currentPos.x / m_winSize.width;
    m_normY = m_currentPos.y / m_winSize.height;

    updateVisuals();
}

void DragOverlayLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    m_isDragging = false;
}

void DragOverlayLayer::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    m_isDragging = false;
}

void DragOverlayLayer::onSave(CCObject*) {
    Mod::get()->setSettingValue("pos-x", static_cast<double>(m_normX));
    Mod::get()->setSettingValue("pos-y", static_cast<double>(m_normY));

    ImageOverlay::updateAll();

    this->keyBackClicked();
}

void DragOverlayLayer::onCancel(CCObject*) {
    this->keyBackClicked();
}

void DragOverlayLayer::keyBackClicked() {
    this->setTouchEnabled(false);
    this->removeFromParentAndCleanup(true);

    if (m_onCloseCallback) {
        m_onCloseCallback();
    }
}
