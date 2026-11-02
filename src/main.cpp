#include <Geode/Geode.hpp>
#include "settings.hpp"

using namespace geode::prelude;

$execute {
	geode::log::info("Zoom mod loaded!");
	geode::log::info("Platform: " GEODE_PLATFORM_NAME);

	SettingsManager::get()->init();
}
