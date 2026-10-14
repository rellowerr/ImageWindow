#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <functional>

class DragOverlayLayer : public FLAlertLayer {
private:
    std::function<void()> m_onCloseCallback;
    cocos2d::CCSize m_winSize;
    cocos2d::CCPoint m_currentPos;
    cocos2d::CCPoint m_touchOffset;
    bool m_isDragging = false;

    float m_normX = 0.5f;
    float m_normY = 0.5f;

    cocos2d::CCSprite* m_previewSprite = nullptr;
    cocos2d::extension::CCScale9Sprite* m_borderNode = nullptr;
    cocos2d::CCLabelBMFont* m_coordsLabel = nullptr;

    bool init(std::function<void()> onCloseCallback);

    void onSave(CCObject*);
    void onCancel(CCObject*);
    void updateVisuals();

public:
    static DragOverlayLayer* create(std::function<void()> onCloseCallback = nullptr);

    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void keyBackClicked() override;
};
