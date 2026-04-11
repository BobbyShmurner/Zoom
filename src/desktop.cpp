#ifdef GEODE_IS_DESKTOP

#include "utils.hpp"
#include "desktop.hpp"
#include "settings.hpp"

#include <Geode/Geode.hpp>

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>
#else
#include <objc/message.h>
#endif // GEODE_IS_WINDOWS
#include <Geode/modify/CCScheduler.hpp>

#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

WindowsZoomManager* WindowsZoomManager::get() {
	static auto inst = new WindowsZoomManager;
	return inst;
}

void WindowsZoomManager::togglePauseMenu() {
	if (!isPaused) return;

	CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
	if (!pauseLayer) return;

	pauseLayer->setVisible(!pauseLayer->isVisible());
}

void WindowsZoomManager::setPauseMenuVisible(bool visible) {
	CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
	if (!pauseLayer) return;

	pauseLayer->setVisible(visible);
}

void WindowsZoomManager::setZoom(float zoom) {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	playLayer->setScale(zoom);
	onScreenModified();
}

void WindowsZoomManager::zoom(float delta) {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	// CCPoint mouseScreenPos = getMousePosOnScreen();
	CCPoint mousePos = getMousePos();

	zoomPlayLayer(playLayer, delta, mousePos);
	onScreenModified();
}

void WindowsZoomManager::move(CCPoint delta) {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	CCPoint pos = playLayer->getPosition();
	playLayer->setPosition(pos + delta);

	onScreenModified();
}

void WindowsZoomManager::setPos(float x, float y) {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	playLayer->setPosition(CCPoint{ x, y });

	onScreenModified();
}

float WindowsZoomManager::getZoom() {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return 1.0f;

	return playLayer->getScale();
}

CCPoint WindowsZoomManager::screenToWorld(CCPoint pos) {
	CCSize screenSize = getScreenSize();
	CCSize winSize = CCEGLView::get()->getFrameSize();

	return CCPoint{ pos.x * (screenSize.width / winSize.width), pos.y * (screenSize.height / winSize.height) };
}

// CCPoint WindowsZoomManager::getMousePosOnScreen() {
// 	return screenToWorld(getMousePos());
// }

CCPoint WindowsZoomManager::getMousePosOnNode(CCNode* node) {
	return node->convertToNodeSpace(getMousePos());
}

void WindowsZoomManager::update(float dt) {
	if (!isPaused) return;

	if (!CCScene::get()->getChildByID("PauseLayer")) {
		this->onResume();
		return;
	}

	auto mousePos = getMousePos();
	auto lastMousePos = WindowsZoomManager::get()->lastMousePos;

	WindowsZoomManager::get()->deltaMousePos = CCPoint{ mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
	WindowsZoomManager::get()->lastMousePos = mousePos;

#ifdef GEODE_IS_WINDOWS
	// GetAsyncKeyState stores the current pressed state in the high-order bit.
	isPanning = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
#endif

	if (isPanning) {
		CCPoint delta = WindowsZoomManager::get()->deltaMousePos;
		move(delta);
	}
}

void WindowsZoomManager::onResume() {
	setZoom(1.0f);
	setPos(0.0f, 0.0f);

	isPaused = false;
	WindowsZoomManager::get()->isPanning = false;
}

void WindowsZoomManager::onPause() {
	isPaused = true;
	WindowsZoomManager::get()->isPanning = false;
}

void WindowsZoomManager::onScroll(float y, float x) {
	if (!isPaused) return;

	if (!CCScene::get()->getChildByID("PauseLayer")) {
		this->onResume();
		return;
	}

	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	if (SettingsManager::get()->altDisablesZoom) {
		auto kb = CCKeyboardDispatcher::get();
		if (kb->getAltKeyPressed()) {
			return;
		}
	}
	
	float zoomDelta = SettingsManager::get()->zoomSensitivity * 0.1f;
	
	if (Loader::get()->isModLoaded("prevter.smooth-scroll")) {
		zoom(-y * zoomDelta * 0.1f);
	} else if (y > 0) {
		zoom(-zoomDelta);
	} else {
		zoom(zoomDelta);
	}

	if (y > 0) {
		if (SettingsManager::get()->autoShowMenu && playLayer->getScale() <= 1.01f) {
			setPauseMenuVisible(true);
		}
	} else {
		if (SettingsManager::get()->autoHideMenu) setPauseMenuVisible(false);
	}
}

void WindowsZoomManager::onScreenModified() {
	CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
	if (!playLayer) return;

	clampPlayLayerPos(playLayer);
	if (!isPaused) return;
}

class $modify(PauseLayer) {
	void customSetup() {
		this->addEventListener(
            KeybindSettingPressedEventV3(Mod::get(), "toggle-menu"),
            [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
                if (down) {
					WindowsZoomManager::get()->togglePauseMenu();
				}

				return ListenerResult::Propagate;
            }
        );

		PauseLayer::customSetup();
	}

	void onResume(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onResume(sender);
	}

	void onRestart(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onRestart(sender);
	}

	void onRestartFull(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onRestartFull(sender);
	}

	void onNormalMode(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onNormalMode(sender);
	}

	void onPracticeMode(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onPracticeMode(sender);
	}
};

class $modify(PlayLayer) {
	void pauseGame(bool p0) {
		WindowsZoomManager::get()->onPause();
		PlayLayer::pauseGame(p0);
	}

	void startGame() {
		WindowsZoomManager::get()->onResume();
		PlayLayer::startGame();
	}

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		WindowsZoomManager::get()->onResume();
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}
};

class $modify(CCScheduler) {
	virtual void update(float dt) {
		WindowsZoomManager::get()->update(dt);
		CCScheduler::update(dt);
	}
};

#ifndef GEODE_IS_WINDOWS
void otherMouseDownHook(void* self, SEL sel, void* event) {
	WindowsZoomManager::get()->isPanning = true;
	reinterpret_cast<void(*)(void*, SEL, void*)>(objc_msgSend)(self, sel, event);
}

void otherMouseUpHook(void* self, SEL sel, void* event) {
	WindowsZoomManager::get()->isPanning = false;
	reinterpret_cast<void(*)(void*, SEL, void*)>(objc_msgSend)(self, sel, event);
}

$execute {
	if (auto hook = ObjcHook::create("EAGLView", "otherMouseDown:", &otherMouseDownHook)) {
		(void) Mod::get()->claimHook(hook.unwrap());
	}
	
	if (auto hook = ObjcHook::create("EAGLView", "otherMouseUp:", &otherMouseUpHook)) {
		(void) Mod::get()->claimHook(hook.unwrap());
	}
}
#endif // GEODE_IS_WINDOWS

class $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		WindowsZoomManager::get()->onScroll(y, x);
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};
#endif // GEODE_IS_DESKTOP
