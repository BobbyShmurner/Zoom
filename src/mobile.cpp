#ifdef GEODE_IS_MOBILE

#include "mobile.hpp"
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(AndroidZoomPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();
		addZoomButtonToPauseLayer(this, this, menu_selector(AndroidZoomPauseLayer::onZoomButton));
	}

	void onZoomButton(CCObject* sender) {
		ZoomLayer::create(this->getParent());
	}
};

#endif // GEODE_IS_MOBILE
