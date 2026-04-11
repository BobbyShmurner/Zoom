#ifdef GEODE_IS_DESKTOP

#include "desktop.hpp"
#include "settings.hpp"
#include "zoom_layer.hpp"

#include <Geode/Geode.hpp>

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#ifdef GEODE_IS_WINDOWS
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
	if (auto zoomLayer = ZoomLayer::get()) {
		zoomLayer->togglePauseMenu();
	}
}

void WindowsZoomManager::update(float dt) {
	auto mousePos = getMousePos();
	deltaMousePos = ccpSub(mousePos, lastMousePos);
	lastMousePos = mousePos;

#ifdef GEODE_IS_WINDOWS
	// GetAsyncKeyState stores the current pressed state in the high-order bit.
	isPanning = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
#endif

	auto scene = CCScene::get();
	auto zoomLayer = ZoomLayer::get();
	auto pauseLayer = scene ? scene->getChildByID("PauseLayer") : nullptr;
	if (!zoomLayer) {
		if (pauseLayer && isPanning) {
			zoomLayer = ZoomLayer::create(scene);
		} else {
			return;
		}
	}

	if (!zoomLayer->getPlayLayer() || !zoomLayer->getPauseLayer()) {
		ZoomLayer::closeActive(false, false);
		return;
	}

	if (isPanning) {
		zoomLayer->panBy(deltaMousePos);
	}
}

void WindowsZoomManager::onScroll(float y, float x) {
	auto scene = CCScene::get();
	if (!scene || !scene->getChildByID("PauseLayer")) {
		return;
	}

	if (SettingsManager::get()->altDisablesZoom) {
		auto kb = CCKeyboardDispatcher::get();
		if (kb->getAltKeyPressed()) {
			return;
		}
	}

	auto zoomLayer = ZoomLayer::create(scene);
	if (!zoomLayer) {
		return;
	}

	float zoomDelta = SettingsManager::get()->zoomSensitivity * 0.1f;

	if (Loader::get()->isModLoaded("prevter.smooth-scroll")) {
		zoomLayer->zoomBy(-y * zoomDelta * 0.1f, getMousePos());
	} else if (y > 0) {
		zoomLayer->zoomBy(-zoomDelta, getMousePos());
	} else {
		zoomLayer->zoomBy(zoomDelta, getMousePos());
	}

	if (y > 0) {
		if (SettingsManager::get()->autoShowMenu && zoomLayer->getZoom() <= 1.01f) {
			ZoomLayer::closeActive(true, true);
		}
	} else {
		if (SettingsManager::get()->autoHideMenu) {
			if (zoomLayer->getZoom() > 1.01f) {
				zoomLayer->setPauseMenuVisible(false);
			}
		}
	}
}

class $modify(DesktopZoomPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();
		addZoomButtonToPauseLayer(this, this, menu_selector(DesktopZoomPauseLayer::onZoomButton));

		this->addEventListener(
			KeybindSettingPressedEventV3(Mod::get(), "toggle-menu"),
			[this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
				if (down && !repeat) {
					WindowsZoomManager::get()->togglePauseMenu();
				}

				return ListenerResult::Propagate;
			}
		);
	}

	void onZoomButton(CCObject* sender) {
		ZoomLayer::create(this->getParent());
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
