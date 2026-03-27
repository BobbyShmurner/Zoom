#include "settings.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_DESKTOP
#include "desktop.hpp"
#endif

SettingsManager* SettingsManager::get() {
	static auto inst = new SettingsManager;
	return inst;
}

void SettingsManager::init() {
	#ifdef GEODE_IS_DESKTOP
	autoHideMenu = Mod::get()->getSettingValue<bool>("auto-hide-menu");
	listenForSettingChanges<bool>("auto-hide-menu", [this](bool enable) {
		autoHideMenu = enable;
	});

	autoShowMenu = Mod::get()->getSettingValue<bool>("auto-show-menu");
	listenForSettingChanges<bool>("auto-show-menu", [this](bool enable) {
		autoShowMenu = enable;
	});

	altDisablesZoom = Mod::get()->getSettingValue<bool>("alt-disables-zoom");
	listenForSettingChanges<bool>("alt-disables-zoom", [this](bool enable) {
		altDisablesZoom = enable;
	});

	zoomSensitivity = Mod::get()->getSettingValue<float>("zoom-sensitivity");
	listenForSettingChanges<float>("zoom-sensitivity", [this](float sensitivity) {
		zoomSensitivity = sensitivity;
	});

	listenForKeybindSettingPresses("toggle-menu", [](Keybind const&, bool down, bool repeat, double) {
		if (down && !repeat) {
			WindowsZoomManager::get()->togglePauseMenu();
		}
	});
	#endif // GEODE_IS_DESKTOP
}
