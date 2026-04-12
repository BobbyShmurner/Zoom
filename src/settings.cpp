#include "settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_DESKTOP
namespace {
	PanMouseButton panMouseButtonFromSetting(std::string_view value) {
		if (value == "Left Click") {
			return PanMouseButton::Left;
		}
		if (value == "Right Click") {
			return PanMouseButton::Right;
		}
		if (value == "Mouse Button 4") {
			return PanMouseButton::Button4;
		}
		if (value == "Mouse Button 5") {
			return PanMouseButton::Button5;
		}
		return PanMouseButton::Middle;
	}
}
#endif

SettingsManager* SettingsManager::get() {
	static auto inst = new SettingsManager;
	return inst;
}

void SettingsManager::init() {
	hidePracticeButtons = Mod::get()->getSettingValue<bool>("hide-practice-buttons");
	listenForSettingChanges<bool>("hide-practice-buttons", [&](bool enable) {
		hidePracticeButtons = enable;
	});

	zoomSensitivity = Mod::get()->getSettingValue<float>("zoom-sensitivity");
	listenForSettingChanges<float>("zoom-sensitivity", [&](float sensitivity) {
		zoomSensitivity = sensitivity;
	});

	showBackButton = Mod::get()->getSettingValue<bool>("show-back-button");
	listenForSettingChanges<bool>("show-back-button", [&](bool enable) {
		showBackButton = enable;
	});

	enableMinimap = Mod::get()->getSettingValue<bool>("enable-minimap");
	listenForSettingChanges<bool>("enable-minimap", [&](bool enable) {
		enableMinimap = enable;
	});

	minimapScale = Mod::get()->getSettingValue<float>("minimap-scale");
	listenForSettingChanges<float>("minimap-scale", [&](float scale) {
		minimapScale = scale;
	});

	minimapOuterFillColor = Mod::get()->getSettingValue<cocos2d::ccColor4B>("minimap-outer-fill-color");
	listenForSettingChanges<cocos2d::ccColor4B>("minimap-outer-fill-color", [&](cocos2d::ccColor4B color) {
		minimapOuterFillColor = color;
	});

	minimapOuterOutlineColor = Mod::get()->getSettingValue<cocos2d::ccColor4B>("minimap-outer-outline-color");
	listenForSettingChanges<cocos2d::ccColor4B>("minimap-outer-outline-color", [&](cocos2d::ccColor4B color) {
		minimapOuterOutlineColor = color;
	});

	minimapInnerFillColor = Mod::get()->getSettingValue<cocos2d::ccColor4B>("minimap-inner-fill-color");
	listenForSettingChanges<cocos2d::ccColor4B>("minimap-inner-fill-color", [&](cocos2d::ccColor4B color) {
		minimapInnerFillColor = color;
	});

	minimapInnerOutlineColor = Mod::get()->getSettingValue<cocos2d::ccColor4B>("minimap-inner-outline-color");
	listenForSettingChanges<cocos2d::ccColor4B>("minimap-inner-outline-color", [&](cocos2d::ccColor4B color) {
		minimapInnerOutlineColor = color;
	});

	#ifdef GEODE_IS_DESKTOP
	autoHideMenu = Mod::get()->getSettingValue<bool>("auto-hide-menu");
	listenForSettingChanges<bool>("auto-hide-menu", [&](bool enable) {
		autoHideMenu = enable;
	});

	autoShowMenu = Mod::get()->getSettingValue<bool>("auto-show-menu");
	listenForSettingChanges<bool>("auto-show-menu", [&](bool enable) {
		autoShowMenu = enable;
	});

	altDisablesZoom = Mod::get()->getSettingValue<bool>("alt-disables-zoom");
	listenForSettingChanges<bool>("alt-disables-zoom", [&](bool enable) {
		altDisablesZoom = enable;
	});

	showZoomMenuButton = Mod::get()->getSettingValue<bool>("show-zoom-menu-button");
	listenForSettingChanges<bool>("show-zoom-menu-button", [&](bool enable) {
		showZoomMenuButton = enable;
	});

	panMouseButton = panMouseButtonFromSetting(Mod::get()->getSettingValue<std::string>("pan-button"));
	listenForSettingChanges<std::string>("pan-button", [&](std::string button) {
		panMouseButton = panMouseButtonFromSetting(button);
	});

	#endif // GEODE_IS_DESKTOP
}
