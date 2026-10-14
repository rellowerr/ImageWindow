#include "ImageOverlay.hpp"
#include "ImageManager.hpp"
#include "ImageWindowPopup.hpp"

using namespace geode::prelude;

ImageOverlay* ImageOverlay::s_instance = nullptr;

ImageOverlay* ImageOverlay::create() {
    auto ret = new ImageOverlay();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ImageOverlay::init() {
    if (!CCNode::init()) return false;

    s_instance = this;
    this->setAnchorPoint(ccp(0.5f, 0.5f));

    // Optional frame background
    m_frame = CCScale9Sprite::create("square02_001.png");
    if (m_frame) {
        m_frame->setAnchorPoint(ccp(0.5f, 0.5f));
        m_frame->setOpacity(170);
        m_frame->setColor({ 20, 20, 25 });
        this->addChild(m_frame, 0);
    }

    // Main image sprite
    m_sprite = CCSprite::create();
    if (m_sprite) {
        m_sprite->setAnchorPoint(ccp(0.5f, 0.5f));
        this->addChild(m_sprite, 1);
    }

    updateOverlay();
    return true;
}

ImageOverlay::~ImageOverlay() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void ImageOverlay::updateAll() {
    if (s_instance) {
        s_instance->updateOverlay();
    }
}

void ImageOverlay::toggle() {
    bool current = Mod::get()->getSettingValue<bool>("enabled");
    bool next = !current;
    Mod::get()->setSettingValue("enabled", next);
    ImageOverlay::updateAll();
    ImageWindowPopup::refreshIfOpen();

    if (next && !ImageManager::get()->getActiveTexture()) {
        Notification::create("Image: Enabled (No image selected)", NotificationIcon::Warning, 1.0f)->show();
    } else {
        Notification::create(next ? "Image: Shown" : "Image: Hidden", NotificationIcon::Info, 0.8f)->show();
    }
}

void ImageOverlay::setForceHidden(bool hidden) {
    m_forceHidden = hidden;
    if (hidden) {
        this->setVisible(false);
    } else {
        updateOverlay();
    }
}

void ImageOverlay::updateOverlay() {
    if (m_forceHidden) {
        this->setVisible(false);
        return;
    }

    bool enabled = Mod::get()->getSettingValue<bool>("enabled");
    if (!enabled) {
        this->setVisible(false);
        return;
    }

    // Check practice-only setting if in PlayLayer
    bool practiceOnly = Mod::get()->getSettingValue<bool>("practice-only");
    if (practiceOnly) {
        auto playLayer = PlayLayer::get();
        if (playLayer && !playLayer->m_isPracticeMode) {
            this->setVisible(false);
            return;
        }
    }

    auto* texture = ImageManager::get()->getActiveTexture();
    if (!texture || !m_sprite) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);

    // Apply texture
    m_sprite->setTexture(texture);
    m_sprite->setTextureRect(CCRect(0.f, 0.f, texture->getContentSize().width, texture->getContentSize().height));

    // Read transform settings
    float scale = static_cast<float>(Mod::get()->getSettingValue<double>("scale"));
    float opacity = static_cast<float>(Mod::get()->getSettingValue<double>("opacity"));
    float rotation = static_cast<float>(Mod::get()->getSettingValue<double>("rotation"));
    float posX = static_cast<float>(Mod::get()->getSettingValue<double>("pos-x"));
    float posY = static_cast<float>(Mod::get()->getSettingValue<double>("pos-y"));
    bool flipX = Mod::get()->getSettingValue<bool>("flip-x");
    bool flipY = Mod::get()->getSettingValue<bool>("flip-y");
    bool showFrame = Mod::get()->getSettingValue<bool>("show-frame");

    // Apply transforms
    m_sprite->setScale(scale);
    m_sprite->setOpacity(static_cast<GLubyte>(std::clamp(opacity, 0.0f, 1.0f) * 255.f));
    m_sprite->setRotation(rotation);
    m_sprite->setFlipX(flipX);
    m_sprite->setFlipY(flipY);

    // Screen positioning
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setPosition(ccp(posX * winSize.width, posY * winSize.height));

    // Update HUD window frame
    if (m_frame) {
        if (showFrame) {
            m_frame->setVisible(true);
            auto sprSize = m_sprite->getScaledContentSize();
            float padding = 12.f;
            m_frame->setContentSize(CCSize(sprSize.width + padding * 2.f, sprSize.height + padding * 2.f));
            m_frame->setPosition(m_sprite->getPosition());
            m_frame->setRotation(rotation);
        } else {
            m_frame->setVisible(false);
        }
    }
}
