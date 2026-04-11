#ifdef GEODE_IS_MOBILE

#include "utils.hpp"
#include "mobile.hpp"
#include "settings.hpp"

#include <algorithm>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;
AndroidZoomLayer* AndroidZoomLayer::instance = nullptr;

AndroidZoomLayer* AndroidZoomLayer::create(CCNode* sceneLayer) {
	if (instance) {
		geode::log::info("AndroidZoomLayer already exists, closing it first.");
		closeActive(false, false);
	}

	auto layer = new AndroidZoomLayer();
	if (layer && layer->init(sceneLayer)) {
		layer->autorelease();
		instance = layer;
		return layer;
	}

	delete layer;
	return nullptr;
}

void AndroidZoomLayer::closeActive(bool resetView, bool restorePauseLayer) {
	if (instance) {
		instance->close(resetView, restorePauseLayer);
	}
}

bool AndroidZoomLayer::init(CCNode* sceneLayer) {
	if (!CCLayer::init())
		return false;

	if (!sceneLayer) {
		geode::log::error("Scene layer is null!");
		return false;
	}

	if (sceneLayer->getChildByID("AndroidZoomLayer"_spr)) {
		geode::log::error("AndroidZoomLayer already exists in scene!");
		return false;
	}

	sceneLayer->addChild(this);

	if (!this->getPlayLayer()) {
		geode::log::error("PlayLayer is null!");
		return false;
	}

	if (!this->getPauseLayer()) {
		geode::log::error("PauseLayer is null!");
		return false;
	}

	this->getPauseLayer()->setVisible(false);

	// Thanks SillyDoggo for the code snippet :D
	// https://github.com/TheSillyDoggo/GeodeMenu/blob/17b19215b80a263379a560edfaf63c2a3f17e2f8/src/Client/AndroidUI.cpp#L28

	m_backMenu = CCMenu::create();
	m_backMenu->ignoreAnchorPointForPosition(false);
	m_backMenu->setContentSize(ccp(0, 0));
	m_backMenu->setPositionX(0);
	m_backMenu->setPositionY(CCDirector::get()->getWinSize().height);
	m_backMenu->setID("back-menu");
	this->addChild(m_backMenu);

	auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
	backSpr->setOpacity(100);

	auto backBtn = CCMenuItemSpriteExtra::create(backSpr, this, menu_selector(AndroidZoomLayer::onBackButton));
	backBtn->setPosition(ccp(24, -23));
	backBtn->setSizeMult(1.15f);

	m_backMenu->addChild(backBtn);

	this->setID("AndroidZoomLayer"_spr);
    this->setZOrder(11); // One above PauseLayer

	this->setTouchPriority(-250);
	this->setTouchEnabled(true);

	m_backMenu->setTouchPriority(CCDirector::sharedDirector()->getTouchDispatcher()->getTargetPrio());
	CCDirector::sharedDirector()->getTouchDispatcher()->registerForcePrio(m_backMenu, 2);

	geode::log::info("AndroidZoomLayer initialized!");
	return true;
}

CCNode* AndroidZoomLayer::getSceneLayer() {
	return this->getParent();
}

CCNode* AndroidZoomLayer::getPlayLayer() {
	if (auto sceneLayer = this->getSceneLayer()) {
		return sceneLayer->getChildByID("PlayLayer");
	}
	return nullptr;
}

CCNode* AndroidZoomLayer::getPauseLayer() {
	if (auto sceneLayer = this->getSceneLayer()) {
		return sceneLayer->getChildByID("PauseLayer");
	}
	return nullptr;
}

void AndroidZoomLayer::registerWithTouchDispatcher() {
	CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, this->getTouchPriority(), true);
}

void AndroidZoomLayer::onExit() {
	if (m_backMenu) {
		CCTouchDispatcher::get()->unregisterForcePrio(m_backMenu);
		m_backMenu = nullptr;
	}

	m_touches.clear();
	m_isZooming = false;
	m_ZoomAnchor = ccp(0, 0);

	if (AndroidZoomLayer::instance == this) {
		AndroidZoomLayer::instance = nullptr;
	}

	CCLayer::onExit();
}

void AndroidZoomLayer::close(bool resetView, bool restorePauseLayer) {
	if (resetView) {
		if (auto playLayer = this->getPlayLayer()) {
			playLayer->setScale(1.0f);
			playLayer->setPosition(ccp(0, 0));
		}
	}

	if (restorePauseLayer) {
		if (auto pauseLayer = this->getPauseLayer()) {
			pauseLayer->setVisible(true);
		}
	}

	if (this->getParent()) {
		this->removeFromParentAndCleanup(true);
	}
}

void AndroidZoomLayer::onBackButton(CCObject* sender) {
	this->close(true, true);
}

bool AndroidZoomLayer::ccTouchBegan(CCTouch* pTouch, CCEvent* pEvent) {
	m_touches.push_back(pTouch);
	if (m_touches.size() > 1 && !m_isZooming) {
		m_isZooming = true;
		m_ZoomAnchor = getAnchorPoint(m_touches[0], m_touches[1]);
	}

	return true;
}

void AndroidZoomLayer::ccTouchMoved(CCTouch* pTouch, CCEvent* pEvent) {
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	if (m_touches.size() == 1) {
		CCTouch* touch = m_touches[0];
		CCPoint delta = touch->getDelta();
		CCPoint pos = playLayer->getPosition();
		playLayer->setPosition(pos.x + delta.x, pos.y + delta.y);
		clampPlayLayerPos(playLayer);
	} else {
		if (!m_isZooming) return;

		CCTouch* movingTouch = m_touches[0];
		CCTouch* anchoredTouch = m_touches[1];

		// Only process the first two touches
		if (pTouch != movingTouch && pTouch != anchoredTouch) return;
		if (pTouch == anchoredTouch) {
			movingTouch = m_touches[1];
			anchoredTouch = m_touches[0];
		}

		// Center the anchor point between the two touches
		CCPoint newAnchor = getAnchorPoint(movingTouch, anchoredTouch);
		CCPoint deltaAnchor = ccpSub(newAnchor, m_ZoomAnchor);
		m_ZoomAnchor = newAnchor;

		// Move the play layer based on the delta of the anchor point
		CCPoint pos = playLayer->getPosition();
		playLayer->setPosition(pos.x + deltaAnchor.x, pos.y + deltaAnchor.y);

		CCPoint delta = movingTouch->getDelta();
		CCPoint touchDisplacement = ccpSub(movingTouch->getLocation(), anchoredTouch->getLocation());
		float scaleDelta = touchDisplacement.normalize().dot(delta) / 100.0f;

		zoomPlayLayer(playLayer, scaleDelta, m_ZoomAnchor);
		clampPlayLayerPos(playLayer);
	}
}

void AndroidZoomLayer::ccTouchEnded(CCTouch* pTouch, CCEvent* pEvent) {
	removeTouchEvent(pTouch, pEvent);
}

void AndroidZoomLayer::ccTouchCancelled(CCTouch* pTouch, CCEvent* pEvent) {
	removeTouchEvent(pTouch, pEvent);
}

void AndroidZoomLayer::removeTouchEvent(CCTouch* pTouch, CCEvent* pEvent) {
	m_touches.erase(std::remove(m_touches.begin(), m_touches.end(), pTouch), m_touches.end());

	if (m_touches.size() < 2) {
		m_isZooming = false;
		m_ZoomAnchor = ccp(0, 0);
	}
}

CCPoint AndroidZoomLayer::getAnchorPoint(CCTouch* movingTouch, CCTouch* anchoredTouch) {
	auto movingTouchPos = movingTouch->getLocation();
	auto anchoredTouchPos = anchoredTouch->getLocation();

	return ccp(
		(movingTouchPos.x + anchoredTouchPos.x) / 2,
		(movingTouchPos.y + anchoredTouchPos.y) / 2
	);
}

class $modify(AndroidZoomPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		auto rightButtonMenu = getChildByID("right-button-menu");

		auto zoomButtonSprite = CircleButtonSprite::createWithSprite("zoom_button.png"_spr, 1.0f, CircleBaseColor::Green, CircleBaseSize::MediumAlt);
		zoomButtonSprite->getTopNode()->setScale(1.0f);
		zoomButtonSprite->setScale(0.6f);

		auto zoomButton = CCMenuItemSpriteExtra::create(zoomButtonSprite, this, menu_selector(AndroidZoomPauseLayer::onZoomButton));
		zoomButton->setID("zoom-button"_spr);

		rightButtonMenu->addChild(zoomButton);
		rightButtonMenu->updateLayout();
	}

	void onZoomButton(CCObject* sender) {
		AndroidZoomLayer::create(this->getParent());
	}
};

class $modify(AndroidZoomPlayLayer, PlayLayer) {
	void levelComplete() {
		AndroidZoomLayer::closeActive(false, false);
		PlayLayer::levelComplete();
	}

	void onQuit() {
		AndroidZoomLayer::closeActive(false, false);
		PlayLayer::onQuit();
	}

	void onExit() {
		AndroidZoomLayer::closeActive(false, false);
		PlayLayer::onExit();
	}
};

#endif // GEODE_IS_MOBILE
