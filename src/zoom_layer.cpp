#include "zoom_layer.hpp"

#include "settings.hpp"
#include "utils.hpp"

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/Popup.hpp>

#ifdef GEODE_IS_DESKTOP
#include "desktop.hpp"
#endif

#include <algorithm>

using namespace geode::prelude;

namespace {
	ZoomLayer* s_activeZoomLayer = nullptr;
	bool s_safeModeBypassed = false;
	bool s_safeModePromptOpen = false;
	constexpr float kMinimapMargin = 12.0f;
	constexpr float kMinimapBaseWidth = 112.0f;
	constexpr float kMinimapBaseHeight = 68.0f;
	constexpr float kMinimapBorderWidth = 1.5f;

	bool shouldShowBackButton() {
		#ifdef GEODE_IS_IOS
		return true;
		#else
		return SettingsManager::get()->showBackButton;
		#endif
	}

	bool isAttemptAlive() {
		auto playLayer = PlayLayer::get();
		if (!playLayer || !playLayer->m_player1 || !playLayer->m_player1->m_isDead) {
			return true;
		}
		return playLayer->m_gameState.m_isDualMode &&
			(!playLayer->m_player2 || !playLayer->m_player2->m_isDead);
	}

	bool isZoomBlockedBySafeMode() {
		return SettingsManager::get()->safeMode && !s_safeModeBypassed && isAttemptAlive();
	}

	void updateZoomButtonAppearance() {
		auto scene = CCScene::get();
		auto pauseLayer = scene ? scene->getChildByID("PauseLayer") : nullptr;
		auto mainLayer = pauseLayer ? pauseLayer->getChildByID("main-layer") : nullptr;
		auto menu = mainLayer ? mainLayer->getChildByID("right-button-menu") : nullptr;
		auto button = menu ? typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("zoom-button"_spr)) : nullptr;
		auto sprite = button ? typeinfo_cast<CCSprite*>(button->getNormalImage()) : nullptr;
		if (sprite) {
			sprite->setOpacity(isZoomBlockedBySafeMode() ? 100 : 255);
		}
	}

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

	void resetSceneView(CCNode* sceneLayer) {
		if (auto playLayer = sceneLayer ? sceneLayer->getChildByID("PlayLayer") : nullptr) {
			playLayer->setScale(1.0f);
			playLayer->setPosition(ccp(0, 0));
		}
	}

	void setScenePauseMenuVisible(CCNode* sceneLayer, bool visible) {
		if (auto pauseLayer = sceneLayer ? sceneLayer->getChildByID("PauseLayer") : nullptr) {
			pauseLayer->setVisible(visible);
		}

		setPracticeButtonsVisible(visible);
	}
}

ZoomLayer* ZoomLayer::get() {
	if (s_activeZoomLayer && !s_activeZoomLayer->getParent()) {
		s_activeZoomLayer = nullptr;
	}

	return s_activeZoomLayer;
}

ZoomLayer* ZoomLayer::request(CCNode* sceneLayer, bool createIfMissing) {
	if (!sceneLayer || !sceneLayer->getChildByID("PauseLayer")) {
		return nullptr;
	}

	if (isAttemptAlive() && !s_safeModeBypassed &&
		!Mod::get()->getSavedValue<bool>("safe-mode-prompt-answered", false)) {
		if (!s_safeModePromptOpen) {
			s_safeModePromptOpen = true;
			createQuickPopup(
				"Enable Zoom Safe Mode?",
				"Ranked Demon List leaderboards, such as <cy>Pointercrate</c> and <cy>AREDL</c>, "
				"now <cr>invalidate runs</c> that use zoom mid-attempt.\n\n"
				"Enable <cg>Safe Mode</c> to prevent zooming while you're alive?\n"
				"<cf>Note: Safe Mode still allows zooming after you die.</c>",
				"Disable", "Enable",
				std::min(460.f, CCDirector::get()->getWinSize().width - 20.f),
				[scene = WeakRef(sceneLayer), createIfMissing](FLAlertLayer*, bool enable) {
					s_safeModePromptOpen = false;
					Mod::get()->setSettingValue<bool>("safe-mode", enable);
					Mod::get()->setSavedValue<bool>("safe-mode-prompt-answered", true);
					if (!enable) {
						Loader::get()->queueInMainThread([scene, createIfMissing] {
							if (auto activeScene = scene.lock(); activeScene && CCScene::get() == activeScene.data()) {
								ZoomLayer::request(activeScene.data(), createIfMissing);
							}
						});
					}
				}
			);
		}
		return nullptr;
	}

	bool activatedBypass = false;
	#ifdef GEODE_IS_DESKTOP
	if (isZoomBlockedBySafeMode() && createIfMissing && PlayLayer::get() && PlayLayer::get()->m_player1 &&
		SettingsManager::get()->safeModeBypass && WindowsZoomManager::get()->isSafeModeBypassHeld()) {
		s_safeModeBypassed = true;
		activatedBypass = true;
	}
	#endif

	if (isZoomBlockedBySafeMode()) {
		static Ref<Notification> notification = [] {
			auto notification = Notification::create(
				"Safe Mode - Zoom disabled mid attempt", NotificationIcon::Error
			);
			for (auto child : CCArrayExt<CCNode*>(notification->getChildren())) {
				if (auto label = child->getChildByType<Label>()) {
					label->setAlignment(Label::Alignment::Center);
					break;
				}
			}
			return notification;
		}();
		std::string message = "Safe Mode - Zoom disabled mid attempt";
		#ifdef GEODE_IS_DESKTOP
		if (!SettingsManager::get()->safeModeBypass) {
			message += "\nEnable bypass in settings";
		} else if (auto key = WindowsZoomManager::get()->safeModeBypassKeyLabel(); !key.empty()) {
			message += "\nHold " + key + " to bypass";
		} else {
			message += "\nSet a bypass key in settings";
		}
		#endif
		notification->setString(message);
		notification->show();
		return nullptr;
	}

	if (auto layer = ZoomLayer::get()) {
		if (layer->getSceneLayer() == sceneLayer && layer->getPlayLayer() && layer->getPauseLayer()) {
			return layer;
		}
		layer->close(false, false);
	}

	auto layer = createIfMissing ? ZoomLayer::create(sceneLayer) : nullptr;
	if (activatedBypass && !layer) {
		s_safeModeBypassed = false;
	}
	return layer;
}

void ZoomLayer::resetSafeModeBypass() {
	s_safeModeBypassed = false;
}

ZoomLayer* ZoomLayer::create(CCNode* sceneLayer) {
	if (isZoomBlockedBySafeMode()) {
		return nullptr;
	}

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

void ZoomLayer::syncActiveView() {
	if (auto layer = ZoomLayer::get()) {
		if (isZoomBlockedBySafeMode()) {
			layer->close(true, true);
			return;
		}
		layer->syncSceneView();
	}
}

void ZoomLayer::enforceSafeMode() {
	if (!isZoomBlockedBySafeMode()) {
		return;
	}
	if (auto scene = CCScene::get(); scene && scene->getChildByID("PauseLayer")) {
		ZoomLayer::closeActive(true, true);
	}
}

void ZoomLayer::closeActive(bool resetView, bool restorePauseLayer) {
	if (auto layer = ZoomLayer::get()) {
		layer->close(resetView, restorePauseLayer);
		return;
	}

	auto sceneLayer = CCScene::get();
	if (!sceneLayer) {
		return;
	}

	// The zoom transform can outlive the UI, so lifecycle exits still need a
	// fallback path that restores the paused view even after the layer is gone.
	if (resetView) {
		resetSceneView(sceneLayer);
	}

	if (restorePauseLayer) {
		setScenePauseMenuVisible(sceneLayer, true);
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
	this->scheduleUpdate();
	m_targetZoom = playLayer->getScale();
	m_targetPosition = playLayer->getPosition();
	m_pauseMenuVisible = false;

	setScenePauseMenuVisible(sceneLayer, false);

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
	m_backMenu->setVisible(shouldShowBackButton());

	m_minimapMenu = CCNode::create();
	m_minimapMenu->ignoreAnchorPointForPosition(false);
	m_minimapMenu->setAnchorPoint(ccp(1.0f, 1.0f));
	m_minimapMenu->setID("minimap-menu"_spr);
	this->addChild(m_minimapMenu);

	if (s_safeModeBypassed) {
		auto label = CCLabelBMFont::create("Safe Mode bypassed for this attempt", "bigFont.fnt");
		label->setScale(0.35f);
		label->setColor(ccc3(255, 180, 50));
		label->setPosition(ccp(CCDirector::get()->getWinSize().width * 0.5f, 14.0f));
		label->setID("safe-mode-bypassed-label"_spr);
		this->addChild(label);
	}

	m_minimapDrawNode = CCDrawNode::create();
	m_minimapDrawNode->ignoreAnchorPointForPosition(false);
	m_minimapDrawNode->setAnchorPoint(ccp(1.0f, 1.0f));
	m_minimapDrawNode->setID("minimap"_spr);
	m_minimapMenu->addChild(m_minimapDrawNode);
	this->updateMinimap();

#ifdef GEODE_IS_MOBILE
	this->setTouchPriority(-250);
	this->setTouchEnabled(true);

	m_backMenu->setTouchPriority(CCDirector::sharedDirector()->getTouchDispatcher()->getTargetPrio());
	CCDirector::sharedDirector()->getTouchDispatcher()->registerForcePrio(m_backMenu, 2, 0);
#endif

	return true;
}

void ZoomLayer::update(float dt) {
	CCLayer::update(dt);
	this->syncSceneView();
	if (m_backMenu) {
		m_backMenu->setVisible(shouldShowBackButton());
	}
	this->updateMinimap();
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
	m_minimapMenu = nullptr;
	m_minimapDrawNode = nullptr;

	if (s_activeZoomLayer == this) {
		s_activeZoomLayer = nullptr;
	}

	CCLayer::onExit();
}

void ZoomLayer::keyBackClicked() {
	this->close(false, true);
}

void ZoomLayer::close(bool resetView, bool restorePauseLayer) {
	this->setKeypadEnabled(false);
	this->setMouseEnabled(false);

	resetView = resetView || isZoomBlockedBySafeMode();
	if (resetView) {
		m_targetZoom = 1.0f;
		m_targetPosition = ccp(0, 0);
		resetSceneView(this->getSceneLayer());
	}

	if (restorePauseLayer) {
		m_pauseMenuVisible = true;
		setScenePauseMenuVisible(this->getSceneLayer(), true);
	}
	else {
		m_pauseMenuVisible = false;
		setPracticeButtonsVisible(true);
	}

	if (this->getParent()) {
		this->removeFromParentAndCleanup(true);
	}
}

void ZoomLayer::resetView() {
	m_targetZoom = 1.0f;
	m_targetPosition = ccp(0, 0);
	resetSceneView(this->getSceneLayer());
}

void ZoomLayer::setPauseMenuVisible(bool visible) {
	m_pauseMenuVisible = visible;
	setScenePauseMenuVisible(this->getSceneLayer(), visible);
}

void ZoomLayer::panBy(CCPoint delta) {
	if (isZoomBlockedBySafeMode()) return;
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	playLayer->setScale(m_targetZoom);
	playLayer->setPosition(m_targetPosition + delta);
	clampPlayLayerPos(playLayer);
	m_targetZoom = playLayer->getScale();
	m_targetPosition = playLayer->getPosition();
}

void ZoomLayer::zoomBy(float delta, CCPoint screenAnchor) {
	if (isZoomBlockedBySafeMode()) return;
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	playLayer->setScale(m_targetZoom);
	playLayer->setPosition(m_targetPosition);
	zoomPlayLayer(playLayer, delta, screenAnchor);
	clampPlayLayerPos(playLayer);
	m_targetZoom = playLayer->getScale();
	m_targetPosition = playLayer->getPosition();
}

float ZoomLayer::getZoom() {
	return m_targetZoom;
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
	this->close(false, true);
}

void ZoomLayer::syncSceneView() {
	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		this->close(false, false);
		return;
	}

	playLayer->setScale(m_targetZoom);
	playLayer->setPosition(m_targetPosition);
	setScenePauseMenuVisible(this->getSceneLayer(), m_pauseMenuVisible);
}

void ZoomLayer::updateMinimap() {
	if (!m_minimapMenu || !m_minimapDrawNode) {
		return;
	}

	m_minimapDrawNode->clear();

	if (!SettingsManager::get()->enableMinimap) {
		m_minimapMenu->setVisible(false);
		return;
	}

	auto playLayer = this->getPlayLayer();
	if (!playLayer) {
		m_minimapMenu->setVisible(false);
		return;
	}
	
	constexpr float MINIMAP_SCALE_MULTIPLIER = 0.5f;

	auto settings = SettingsManager::get();
	auto minimapOuterFillColor = toColor4F(settings->minimapOuterFillColor);
	auto minimapOuterBorderColor = toColor4F(settings->minimapOuterOutlineColor);
	auto minimapInnerFillColor = toColor4F(settings->minimapInnerFillColor);
	auto minimapInnerBorderColor = toColor4F(settings->minimapInnerOutlineColor);
	auto minimapUiScale = settings->minimapScale * MINIMAP_SCALE_MULTIPLIER;
	auto borderWidth = std::max(0.75f, kMinimapBorderWidth * minimapUiScale);
	auto contentSize = playLayer->getContentSize();
	if (contentSize.width <= 0.0f || contentSize.height <= 0.0f) {
		m_minimapMenu->setVisible(false);
		return;
	}

	m_minimapMenu->setVisible(true);
	m_minimapDrawNode->setVisible(true);

	auto minimapMaxWidth = kMinimapBaseWidth * minimapUiScale;
	auto minimapMaxHeight = kMinimapBaseHeight * minimapUiScale;
	auto minimapScale = std::min(minimapMaxWidth / contentSize.width, minimapMaxHeight / contentSize.height);
	auto minimapSize = CCSize(contentSize.width * minimapScale, contentSize.height * minimapScale);
	auto winSize = CCDirector::sharedDirector()->getWinSize();
	auto right = settings->minimapPosition == MinimapPosition::TopRight ||
		settings->minimapPosition == MinimapPosition::BottomRight;
	auto top = settings->minimapPosition == MinimapPosition::TopLeft ||
		settings->minimapPosition == MinimapPosition::TopRight;
	auto minimapMenuPosition = ccp(right ? winSize.width : 0.0f, top ? winSize.height : 0.0f);
	auto minimapPosition = ccp(right ? -kMinimapMargin : kMinimapMargin, top ? -kMinimapMargin : kMinimapMargin);
	m_minimapMenu->setContentSize(CCSizeZero);
	m_minimapMenu->setPosition(minimapMenuPosition);
	m_minimapDrawNode->setAnchorPoint(ccp(right ? 1.0f : 0.0f, top ? 1.0f : 0.0f));
	m_minimapDrawNode->setContentSize(minimapSize);
	m_minimapDrawNode->setPosition(minimapPosition);

	// Seems like the alignment is bugged: center and outside seemed to be swapped
	constexpr cocos2d::BorderAlignment borderAlignment = cocos2d::BorderAlignment::Center;

	auto minimapRect = CCRect(0.0f, 0.0f, minimapSize.width, minimapSize.height);
	m_minimapDrawNode->drawRect(
		minimapRect,
		minimapOuterFillColor,
		borderWidth,
		minimapOuterBorderColor,
		borderAlignment
	);

	auto zoom = std::max(playLayer->getScale(), 1.0f);
	auto screenSize = getScreenSize();
	auto visibleSize = CCSize(
		std::min(contentSize.width, screenSize.width / zoom),
		std::min(contentSize.height, screenSize.height / zoom)
	);

	// Positive layer offsets move the world in the opposite direction, so we
	// convert back into content-space to locate the actual camera window.
	auto viewCenter = ccp(
		contentSize.width * 0.5f - playLayer->getPositionX() / zoom,
		contentSize.height * 0.5f - playLayer->getPositionY() / zoom
	);

	auto visibleOrigin = ccp(
		clamp(viewCenter.x - visibleSize.width * 0.5f, 0.0f, contentSize.width - visibleSize.width),
		clamp(viewCenter.y - visibleSize.height * 0.5f, 0.0f, contentSize.height - visibleSize.height)
	);

	auto viewportRect = CCRect(
		visibleOrigin.x * minimapScale,
		visibleOrigin.y * minimapScale,
		std::max(2.0f, visibleSize.width * minimapScale),
		std::max(2.0f, visibleSize.height * minimapScale)
	);

	m_minimapDrawNode->drawRect(
		viewportRect,
		minimapInnerFillColor,
		borderWidth,
		minimapInnerBorderColor,
		borderAlignment
	);
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
	scaleDelta *= SettingsManager::get()->zoomSensitivity;

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

	#ifdef GEODE_IS_DESKTOP
	if (!SettingsManager::get()->showZoomMenuButton) {
		return;
	}
	#endif

	auto mainLayer = pauseLayer->getChildByID("main-layer");
	auto rightButtonMenu = mainLayer ? mainLayer->getChildByID("right-button-menu") : nullptr;
	if (!rightButtonMenu || rightButtonMenu->getChildByID("zoom-button"_spr)) {
		return;
	}

	auto zoomButtonSprite = CCSprite::createWithSpriteFrameName("sideBtn_search_001.png");
	if (!zoomButtonSprite) return;
	if (auto optionsButton = typeinfo_cast<CCMenuItemSpriteExtra*>(rightButtonMenu->getChildByID("options-button"))) {
		if (auto optionsSprite = optionsButton->getNormalImage()) {
			auto referenceWidth = optionsSprite->getScaledContentSize().width;
			if (referenceWidth > 0.f && zoomButtonSprite->getContentWidth() > 0.f) {
				zoomButtonSprite->setScale(referenceWidth / zoomButtonSprite->getContentWidth());
			}
		}
	}
	zoomButtonSprite->setOpacity(isZoomBlockedBySafeMode() ? 100 : 255);
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

class $modify(ZoomSchedulerCompat, CCScheduler) {
	void update(float dt) {
		CCScheduler::update(dt);

		#ifdef GEODE_IS_DESKTOP
		WindowsZoomManager::get()->update(dt);
		#endif

		ZoomLayer::syncActiveView();
		updateZoomButtonAppearance();
	}
};

class $modify(ZoomPlayLayerLifecycle, PlayLayer) {
	bool init(GJGameLevel* level, bool dontCreateObjects) {
		ZoomLayer::resetSafeModeBypass();
		ZoomLayer::closeActive(true, false);
		return PlayLayer::init(level, dontCreateObjects);
	}

	void startGame() {
		ZoomLayer::closeActive(true, false);
		PlayLayer::startGame();
	}

	void resetLevel() {
		ZoomLayer::resetSafeModeBypass();
		PlayLayer::resetLevel();
	}

	void levelComplete() {
		ZoomLayer::closeActive(false, false);
		PlayLayer::levelComplete();
	}

	void onQuit() {
		ZoomLayer::resetSafeModeBypass();
		ZoomLayer::closeActive(false, false);
		PlayLayer::onQuit();
	}

	void onExit() {
		ZoomLayer::closeActive(false, false);
		PlayLayer::onExit();
	}
};
