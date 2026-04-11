#include <charconv>

#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "settings.hpp"

#ifdef GEODE_IS_MOBILE
#include "mobile.hpp"
#endif

using namespace geode::prelude;

$execute {
	geode::log::info("Zoom mod loaded!");
	geode::log::info("Platform: " GEODE_PLATFORM_NAME);

	SettingsManager::get()->init();
}