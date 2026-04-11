#include "zoom_layer.hpp"

#include "settings.hpp"
#include "utils.hpp"

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#ifdef GEODE_IS_MOBILE
#include <algorithm>
#endif

using namespace geode::prelude;

namespace {
	ZoomLayer* s_activeZoomLayer = nullptr;

	void setPracticeButtonsVisible(bool visible) {
		if (!SettingsManager::get()->hidePracticeButtons) {
			return;
		}

		if (auto uiLayer = UILayer::get()) {
			auto playLayer = PlayLayer::get();
			if (!playLayer || !playLayer->m_isPracticeMode) {
				uiLayer->toggleCheckpointsMenu(false);
				return;
			}

			uiLayer->toggleCheckpointsMenu(visible);
		}
	}
}

ZoomLayer* ZoomLayer::get() {
	if (s_activeZoomLayer && !s_activeZoomLayer->getParent()) {
		s_activeZoomLayer = nullptr;
	}

	return s_activeZoomLayer;
}

ZoomLayer* ZoomLayer::create(CCNode* sceneLayer) {
	if (auto layer = ZoomLayer::get()) {
		if (layer->getSceneLayer() == sceneLayer) {
			return layer;
		}

		layer->close(false, false);
	}

	auto layer = new ZoomLayer();
	if (layer && layer->init(sceneLayer)) {
		layer->autorelease();
		s_activeZoomLayer = layer;
		return layer;
	}

	delete layer;
	return nullptr;
}

void ZoomLayer::closeActive(bool resetView, bool restorePauseLayer) {
	if (auto layer = ZoomLayer::get()) {
		layer->close(resetView, restorePauseLayer);
	}
}

bool ZoomLayer::init(CCNode* sceneLayer) {
	if (!CCLayer::init()) {
		return false;
	}

	if (!sceneLayer) {
		geode::log::error("ZoomLayer scene layer is null.");
		return false;
	}

	if (sceneLayer->getChildByID("zoom-layer"_spr)) {
		geode::log::error("ZoomLayer already exists in scene.");
		return false;
	}

	auto playLayer = sceneLayer->getChildByID("PlayLayer");
	auto pauseLayer = sceneLayer->getChildByID("PauseLayer");
	if (!playLayer || !pauseLayer) {
		geode::log::error("ZoomLayer could not find PlayLayer or PauseLayer.");
		return false;
	}

	this->setID("zoom-layer"_spr);
	this->setZOrder(11);
	sceneLayer->addChild(this);
	this->setKeypadEnabled(true);
	this->setMouseEnabled(true);

	pauseLayer->setVisible(false);
	setPracticeButtonsVisible(false);

	m_backMenu = CCMenu::create();
	m_backMenu->ignoreAnchorPointForPosition(false);
	m_backMenu->setContentSize(ccp(0, 0));
	m_backMenu->setPositionX(0);
	m_backMenu->setPositionY(CCDirector::get()->getWinSize().height);
	m_backMenu->setID("back-menu"_spr);
	this->addChild(m_backMenu);

	auto backSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
	backSprite->setOpacity(100);

	auto backButton = CCMenuItemSpriteExtra::create(backSprite, this, menu_selector(ZoomLayer::onBackButton));
	backButton->setPosition(ccp(24, -23));
	backButton->setSizeMult(1.15f);
	backButton->setID("back-button"_spr);
	m_backMenu->addChild(backButton);

#ifdef GEODE_IS_MOBILE
	this->setTouchPriority(-250);
	this->setTouchEnabled(true);

	m_backMenu->setTouchPriority(CCDirector::sharedDirector()->getTouchDispatcher()->getTargetPrio());
	CCDirector::sharedDirector()->getTouchDispatcher()->registerForcePrio(m_backMenu, 2);
#endif

	return true;
}

void ZoomLayer::onExit() {
#ifdef GEODE_IS_MOBILE
	if (m_backMenu) {
		CCTouchDispatcher::get()->unregisterForcePrio(m_backMenu);
	}

	m_touches.clear();
	m_isZooming = false;
	m_zoomAnchor = ccp(0, 0);
#endif

	m_backMenu = nullptr;

	if (s_activeZoomLayer == this) {
		s_activeZoomLayer = nullptr;
	}

	CCLayer::onExit();
}

void ZoomLayer::keyBackClicked() {
	this->close(true, true);
}

void ZoomLayer::close(bool resetView, bool restorePauseLayer) {
	this->setKeypadEnabled(false);
	this->setMouseEnabled(false);

	if (resetView) {
		this->resetView();
	}

	if (restorePauseLayer) {
		this->setPauseMenuVisible(true);
	}
	else {
		setPracticeButtonsVisible(true);
	}

	if (this->getParent()) {
		this->removeFromParentAndCleanup(true);
	}
}

void ZoomLayer::resetView() {
	if (auto playLayer = this->getPlayLayer()) {
		playLayer->setScale(1.0f);
		playLayer->setPosition(ccp(0, 0));
	}
}

void ZoomLayer::setPauseMenuVisible(bool visible) {
	if (auto pauseLayer = this->getPauseLayer()) {
		pauseLayer->setVisible(visible);
	}

	setPracticeButtonsVisible(visible);
}

void ZoomLayer::togglePauseMenu() {
	if (auto pauseLayer = this->getPauseLayer()) {
		this->setPauseMenuVisible(!pauseLayer->isVisible());
	}
}

void ZoomLayer::panBy(CCPoint delta) {
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	playLayer->setPosition(playLayer->getPosition() + delta);
	clampPlayLayerPos(playLayer);
}

void ZoomLayer::zoomBy(float delta, CCPoint screenAnchor) {
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	zoomPlayLayer(playLayer, delta, screenAnchor);
	clampPlayLayerPos(playLayer);
}

float ZoomLayer::getZoom() {
	if (auto playLayer = this->getPlayLayer()) {
		return playLayer->getScale();
	}

	return 1.0f;
}

CCNode* ZoomLayer::getSceneLayer() {
	return this->getParent();
}

CCNode* ZoomLayer::getPlayLayer() {
	if (auto sceneLayer = this->getSceneLayer()) {
		return sceneLayer->getChildByID("PlayLayer");
	}

	return nullptr;
}

CCNode* ZoomLayer::getPauseLayer() {
	if (auto sceneLayer = this->getSceneLayer()) {
		return sceneLayer->getChildByID("PauseLayer");
	}

	return nullptr;
}

void ZoomLayer::onBackButton(CCObject* sender) {
	this->close(true, true);
}

#ifdef GEODE_IS_MOBILE
void ZoomLayer::registerWithTouchDispatcher() {
	CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, this->getTouchPriority(), true);
}

bool ZoomLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
	m_touches.push_back(touch);
	if (m_touches.size() > 1 && !m_isZooming) {
		m_isZooming = true;
		m_zoomAnchor = this->getTouchAnchor(m_touches[0], m_touches[1]);
	}

	return true;
}

void ZoomLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	if (m_touches.size() == 1) {
		this->panBy(m_touches[0]->getDelta());
		return;
	}

	if (!m_isZooming) {
		return;
	}

	auto movingTouch = m_touches[0];
	auto anchoredTouch = m_touches[1];
	if (touch != movingTouch && touch != anchoredTouch) {
		return;
	}

	if (touch == anchoredTouch) {
		movingTouch = m_touches[1];
		anchoredTouch = m_touches[0];
	}

	auto newAnchor = this->getTouchAnchor(movingTouch, anchoredTouch);
	auto deltaAnchor = ccpSub(newAnchor, m_zoomAnchor);
	m_zoomAnchor = newAnchor;

	this->panBy(deltaAnchor);

	auto delta = movingTouch->getDelta();
	auto touchDisplacement = ccpSub(movingTouch->getLocation(), anchoredTouch->getLocation());
	auto scaleDelta = touchDisplacement.normalize().dot(delta) / 100.0f;

	this->zoomBy(scaleDelta, m_zoomAnchor);
}

void ZoomLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
	this->removeTouch(touch);
}

void ZoomLayer::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
	this->removeTouch(touch);
}

void ZoomLayer::removeTouch(CCTouch* touch) {
	m_touches.erase(std::remove(m_touches.begin(), m_touches.end(), touch), m_touches.end());

	if (m_touches.size() < 2) {
		m_isZooming = false;
		m_zoomAnchor = ccp(0, 0);
	}
}

CCPoint ZoomLayer::getTouchAnchor(CCTouch* touch1, CCTouch* touch2) const {
	auto touch1Pos = touch1->getLocation();
	auto touch2Pos = touch2->getLocation();

	return ccp(
		(touch1Pos.x + touch2Pos.x) / 2,
		(touch1Pos.y + touch2Pos.y) / 2
	);
}
#endif

void addZoomButtonToPauseLayer(CCNode* pauseLayer, CCObject* target, SEL_MenuHandler callback) {
	if (!pauseLayer) {
		return;
	}

	auto rightButtonMenu = pauseLayer->getChildByID("right-button-menu");
	if (!rightButtonMenu || rightButtonMenu->getChildByID("zoom-button"_spr)) {
		return;
	}

	auto zoomButtonSprite = CircleButtonSprite::createWithSprite(
		"zoom_button.png"_spr,
		1.0f,
		CircleBaseColor::Green,
		CircleBaseSize::MediumAlt
	);
	zoomButtonSprite->getTopNode()->setScale(1.0f);
	zoomButtonSprite->setScale(0.6f);

	auto zoomButton = CCMenuItemSpriteExtra::create(zoomButtonSprite, target, callback);
	zoomButton->setID("zoom-button"_spr);
	rightButtonMenu->addChild(zoomButton);
	rightButtonMenu->updateLayout();
}

class $modify(ZoomPauseLayerLifecycle, PauseLayer) {
	void onResume(CCObject* sender) {
		ZoomLayer::closeActive(true, false);
		PauseLayer::onResume(sender);
	}

	void onRestart(CCObject* sender) {
		ZoomLayer::closeActive(true, false);
		PauseLayer::onRestart(sender);
	}

	void onRestartFull(CCObject* sender) {
		ZoomLayer::closeActive(true, false);
		PauseLayer::onRestartFull(sender);
	}

	void onNormalMode(CCObject* sender) {
		ZoomLayer::closeActive(true, false);
		PauseLayer::onNormalMode(sender);
	}

	void onPracticeMode(CCObject* sender) {
		ZoomLayer::closeActive(true, false);
		PauseLayer::onPracticeMode(sender);
	}
};

class $modify(ZoomPlayLayerLifecycle, PlayLayer) {
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		ZoomLayer::closeActive(true, false);
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}

	void startGame() {
		ZoomLayer::closeActive(true, false);
		PlayLayer::startGame();
	}

	void levelComplete() {
		ZoomLayer::closeActive(false, false);
		PlayLayer::levelComplete();
	}

	void onQuit() {
		ZoomLayer::closeActive(false, false);
		PlayLayer::onQuit();
	}

	void onExit() {
		ZoomLayer::closeActive(false, false);
		PlayLayer::onExit();
	}
};
