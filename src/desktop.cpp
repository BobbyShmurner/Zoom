#ifdef GEODE_IS_DESKTOP

#include "desktop.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "zoom_layer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

namespace {
	constexpr float kClosedZoomThreshold = 1.001f;

	CCPoint getScreenCenter() {
		auto screenSize = getScreenSize();
		return ccp(screenSize.width * 0.5f, screenSize.height * 0.5f);
	}

	bool hasVisibleBlockingPopup(CCNode* node) {
		if (!node || !node->isVisible()) {
			return false;
		}

		if (typeinfo_cast<FLAlertLayer*>(node)) {
			return true;
		}

		for (auto child : CCArrayExt<CCNode*>(node->getChildren())) {
			if (hasVisibleBlockingPopup(child)) {
				return true;
			}
		}

		return false;
	}

	bool hasBlockingPausePopup() {
		auto scene = CCScene::get();
		if (!scene) {
			return false;
		}

		for (auto child : CCArrayExt<CCNode*>(scene->getChildren())) {
			if (hasVisibleBlockingPopup(child)) {
				return true;
			}
		}

		return false;
	}

	ZoomLayer* getPausedZoomLayer(bool createIfMissing) {
		auto scene = CCScene::get();
		if (!scene || !scene->getChildByID("PauseLayer")) {
			return nullptr;
		}

		if (auto zoomLayer = ZoomLayer::get()) {
			if (zoomLayer->getPlayLayer() && zoomLayer->getPauseLayer()) {
				return zoomLayer;
			}

			ZoomLayer::closeActive(false, false);
		}

		return createIfMissing ? ZoomLayer::create(scene) : nullptr;
	}

	void updateZoomLayerMenuState(ZoomLayer* zoomLayer) {
		if (!zoomLayer) {
			return;
		}

		if (zoomLayer->getZoom() <= kClosedZoomThreshold) {
			if (SettingsManager::get()->autoShowMenu) {
				ZoomLayer::closeActive(true, true);
			}
		}
		else if (SettingsManager::get()->autoHideMenu) {
			zoomLayer->setPauseMenuVisible(false);
		}
	}
}

WindowsZoomManager* WindowsZoomManager::get() {
	static auto inst = new WindowsZoomManager;
	return inst;
}

void WindowsZoomManager::toggleZoomUI() {
	if (hasBlockingPausePopup()) {
		return;
	}

	if (auto zoomLayer = ZoomLayer::get()) {
		zoomLayer->close(false, true);
		return;
	}

	auto scene = CCScene::get();
	if (!scene || !scene->getChildByID("PauseLayer")) {
		return;
	}

	ZoomLayer::create(scene);
}

void WindowsZoomManager::onMouseInput(MouseInputData const& input) {
	auto pressed = input.action == MouseInputData::Action::Press;

	switch (input.button) {
		case MouseInputData::Button::Left:
			leftMouseDown = pressed;
			break;

		case MouseInputData::Button::Right:
			rightMouseDown = pressed;
			break;

		case MouseInputData::Button::Middle:
			middleMouseDown = pressed;
			break;

		case MouseInputData::Button::Button4:
			mouse4Down = pressed;
			break;

		case MouseInputData::Button::Button5:
			mouse5Down = pressed;
			break;
	}
}

void WindowsZoomManager::setPanKeybindState(bool down) {
	panKeybindDown = down;
}

void WindowsZoomManager::resetTransientInputs() {
	panKeybindDown = false;
}

bool WindowsZoomManager::isPanInputHeld() const {
	return panKeybindDown || this->isPanButtonHeld();
}

bool WindowsZoomManager::isPanButtonHeld() const {
	switch (SettingsManager::get()->panMouseButton) {
		case PanMouseButton::Left:
			return leftMouseDown;

		case PanMouseButton::Right:
			return rightMouseDown;

		case PanMouseButton::Middle:
			return middleMouseDown;

		case PanMouseButton::Button4:
			return mouse4Down;

		case PanMouseButton::Button5:
			return mouse5Down;
	}

	return false;
}

void WindowsZoomManager::update(float dt) {
	auto mousePos = getMousePos();
	deltaMousePos = ccpSub(mousePos, lastMousePos);
	lastMousePos = mousePos;

	auto scene = CCScene::get();
	if (!scene || !scene->getChildByID("PauseLayer")) {
		this->resetTransientInputs();
		return;
	}

	if (hasBlockingPausePopup()) {
		return;
	}

	auto zoomLayer = getPausedZoomLayer(false);
	if (!zoomLayer || zoomLayer->getZoom() <= kClosedZoomThreshold) {
		return;
	}

	if (this->isPanInputHeld()) {
		zoomLayer->panBy(deltaMousePos);
	}
}

void WindowsZoomManager::onScroll(float y, float x) {
	if (SettingsManager::get()->altDisablesZoom) {
		auto kb = CCKeyboardDispatcher::get();
		if (kb->getAltKeyPressed()) {
			return;
		}
	}

	if (hasBlockingPausePopup()) {
		return;
	}

	auto zoomLayer = getPausedZoomLayer(true);
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

	updateZoomLayerMenuState(zoomLayer);
}

void WindowsZoomManager::onZoomKey(bool zoomIn) {
	if (SettingsManager::get()->altDisablesZoom) {
		auto kb = CCKeyboardDispatcher::get();
		if (kb->getAltKeyPressed()) {
			return;
		}
	}

	if (hasBlockingPausePopup()) {
		return;
	}

	auto zoomLayer = getPausedZoomLayer(zoomIn);
	if (!zoomLayer) {
		return;
	}

	auto zoomDelta = SettingsManager::get()->zoomSensitivity * 0.1f;
	zoomLayer->zoomBy(zoomIn ? zoomDelta : -zoomDelta, getMousePos());
	updateZoomLayerMenuState(zoomLayer);
}

class $modify(DesktopZoomPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();
		addZoomButtonToPauseLayer(this, this, menu_selector(DesktopZoomPauseLayer::onZoomButton));

		this->addEventListener(
			KeybindSettingPressedEventV3(Mod::get(), "toggle-menu"),
			[this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
				if (down && !repeat) {
					WindowsZoomManager::get()->toggleZoomUI();
				}

					return ListenerResult::Propagate;
				}
			);

		this->addEventListener(
			KeybindSettingPressedEventV3(Mod::get(), "zoom-in"),
			[this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
				if (down) {
					WindowsZoomManager::get()->onZoomKey(true);
				}

				return ListenerResult::Propagate;
			}
		);

		this->addEventListener(
			KeybindSettingPressedEventV3(Mod::get(), "zoom-out"),
			[this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
				if (down) {
					WindowsZoomManager::get()->onZoomKey(false);
				}

				return ListenerResult::Propagate;
			}
		);

		this->addEventListener(
			KeybindSettingPressedEventV3(Mod::get(), "pan"),
			[this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
				WindowsZoomManager::get()->setPanKeybindState(down);
				return ListenerResult::Propagate;
			}
		);
	}

	void onZoomButton(CCObject* sender) {
		ZoomLayer::create(this->getParent());
	}

	void onResume(CCObject* sender) {
		WindowsZoomManager::get()->resetTransientInputs();
		PauseLayer::onResume(sender);
	}

	void onRestart(CCObject* sender) {
		WindowsZoomManager::get()->resetTransientInputs();
		PauseLayer::onRestart(sender);
	}

	void onRestartFull(CCObject* sender) {
		WindowsZoomManager::get()->resetTransientInputs();
		PauseLayer::onRestartFull(sender);
	}

	void onQuit(CCObject* sender) {
		WindowsZoomManager::get()->resetTransientInputs();
		PauseLayer::onQuit(sender);
	}
};

class $modify(CCScheduler) {
	virtual void update(float dt) {
		WindowsZoomManager::get()->update(dt);
		CCScheduler::update(dt);
	}
};

$execute {
	MouseInputEvent().listen([](MouseInputData& input) -> bool {
		WindowsZoomManager::get()->onMouseInput(input);
		return ListenerResult::Propagate;
	}).leak();
}

class $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		WindowsZoomManager::get()->onScroll(y, x);
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};
#endif // GEODE_IS_DESKTOP
