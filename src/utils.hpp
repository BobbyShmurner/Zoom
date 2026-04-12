#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

CCSize getScreenSize();
void clampPlayLayerPos(CCNode* playLayer);
void zoomPlayLayer(CCNode* playLayer, float zoom, CCPoint screenAnchor);
float clamp(float d, float min, float max);
cocos2d::ccColor4F toColor4F(cocos2d::ccColor4B color);
