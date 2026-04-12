#pragma once

#include <string>
#include <Geode/Geode.hpp>

#ifdef GEODE_IS_DESKTOP
enum class PanMouseButton {
	Left,
	Right,
	Middle,
	Button4,
	Button5,
};
#endif

class SettingsManager {
public:
	static SettingsManager* get();
	void init();

	bool hidePracticeButtons = true;
	bool enableMinimap = true;
	float minimapScale = 1.0f;
	cocos2d::ccColor4B minimapOuterFillColor = { 235, 235, 235, 96 };
	cocos2d::ccColor4B minimapOuterOutlineColor = { 235, 235, 235, 96 };
	cocos2d::ccColor4B minimapInnerFillColor = { 255, 26, 26, 96 };
	cocos2d::ccColor4B minimapInnerOutlineColor = { 255, 26, 26, 96 };

	#ifdef GEODE_IS_DESKTOP
	bool autoHideMenu;
	bool autoShowMenu;
	bool altDisablesZoom;
	float zoomSensitivity;
	PanMouseButton panMouseButton = PanMouseButton::Middle;
	#endif
};
