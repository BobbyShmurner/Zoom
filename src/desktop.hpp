#ifdef GEODE_IS_DESKTOP
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <unordered_set>

using namespace geode::prelude;

class ZoomLayer;

class WindowsZoomManager {
public:
	CCPoint lastMousePos = ccp(0, 0);
	CCPoint deltaMousePos = ccp(0, 0);

	bool leftMouseDown = false;
	bool rightMouseDown = false;
	bool middleMouseDown = false;
	bool mouse4Down = false;
	bool mouse5Down = false;
	bool panKeybindDown = false;

	static WindowsZoomManager* get();

	void update(float dt);
	void toggleZoomUI();
	void onMouseInput(MouseInputData const& input);
	void onKeyboardInput(KeyboardInputData const& input);
	bool isSafeModeBypassHeld() const;
	std::string safeModeBypassKeyLabel() const;
	void onScroll(float y);
	void onZoomKey(bool zoomIn);
	void setPanKeybindState(bool down);
	void resetTransientInputs();

private:
	std::unordered_set<cocos2d::enumKeyCodes> m_heldKeys;
	WeakRef<CCNode> m_zoomLayer;
	CCPoint m_zoomAnchor = ccp(0, 0);
	float m_pendingZoom = 0.0f;
	void applyZoomStep(ZoomLayer* zoomLayer, bool zoomIn);
	bool isPanInputHeld() const;
	bool isPanButtonHeld() const;
};
#endif // GEODE_IS_DESKTOP
