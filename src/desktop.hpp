#ifdef GEODE_IS_DESKTOP
#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class WindowsZoomManager {
public:
	CCPoint lastMousePos = ccp(0, 0);
	CCPoint deltaMousePos = ccp(0, 0);

	bool isPaused = false;
	bool isPanning = false;

	static WindowsZoomManager* get();

	void update(float dt);
	void togglePauseMenu();
	CCPoint screenToWorld(CCPoint pos);

	void setPauseMenuVisible(bool visible);
	void setPos(float x, float y);
	void setZoom(float zoom);

	void zoom(float delta);
	void move(CCPoint delta);

	float getZoom();
	// CCPoint getMousePosOnScreen();
	CCPoint getMousePosOnNode(CCNode* node);
	
	void onResume();
	void onPause();
	void onScroll(float y, float x);
private:
	void onScreenModified();
};
#endif // GEODE_IS_DESKTOP
