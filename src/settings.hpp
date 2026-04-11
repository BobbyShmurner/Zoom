#pragma once

#include <string>

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

	#ifdef GEODE_IS_DESKTOP
	bool autoHideMenu;
	bool autoShowMenu;
	bool altDisablesZoom;
	float zoomSensitivity;
	PanMouseButton panMouseButton = PanMouseButton::Middle;
	#endif
};
