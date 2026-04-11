#ifdef GEODE_IS_DESKTOP
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>

using namespace geode::prelude;

class WindowsZoomManager {
public:
	CCPoint lastMousePos = ccp(0, 0);
	CCPoint deltaMousePos = ccp(0, 0);

	bool leftMouseDown = false;
	bool rightMouseDown = false;
	bool middleMouseDown = false;
	bool mouse4Down = false;
	bool mouse5Down = false;

	static WindowsZoomManager* get();

	void update(float dt);
	void toggleZoomUI();
	void onMouseInput(MouseInputData const& input);
	void onScroll(float y, float x);
	void onZoomKey(bool zoomIn);

private:
	bool isPanButtonHeld() const;
};
#endif // GEODE_IS_DESKTOP
