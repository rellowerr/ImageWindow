#pragma once

#include <Geode/Geode.hpp>

class ImageOverlay : public cocos2d::CCNode {
private:
    static ImageOverlay* s_instance;

    cocos2d::CCSprite* m_sprite = nullptr;
    cocos2d::extension::CCScale9Sprite* m_frame = nullptr;
    bool m_forceHidden = false;

public:
    static ImageOverlay* create();
    static ImageOverlay* get() { return s_instance; }
    static void updateAll();
    static void toggle();

    bool init() override;
    ~ImageOverlay() override;

    void updateOverlay();
    void setForceHidden(bool hidden);
};
