#ifdef GEODE_IS_DESKTOP
#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class WindowsZoomManager {
public:
	CCPoint lastMousePos = ccp(0, 0);
	CCPoint deltaMousePos = ccp(0, 0);

	bool isPanning = false;

	static WindowsZoomManager* get();

	void update(float dt);
	void togglePauseMenu();
	void onScroll(float y, float x);
};
#endif // GEODE_IS_DESKTOP
