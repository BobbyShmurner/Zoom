#pragma once

#include <Geode/Geode.hpp>

#ifdef GEODE_IS_MOBILE
#include <vector>
#endif

using namespace geode::prelude;

class ZoomLayer : public CCLayer {
public:
	static ZoomLayer* get();
	static ZoomLayer* create(CCNode* sceneLayer);
	static void closeActive(bool resetView = true, bool restorePauseLayer = true);

	bool init(CCNode* sceneLayer);
	void onExit() override;

	void close(bool resetView = true, bool restorePauseLayer = true);
	void resetView();
	void setPauseMenuVisible(bool visible);
	void togglePauseMenu();
	void panBy(CCPoint delta);
	void zoomBy(float delta, CCPoint screenAnchor);
	float getZoom();

	CCNode* getSceneLayer();
	CCNode* getPlayLayer();
	CCNode* getPauseLayer();

#ifdef GEODE_IS_MOBILE
	void registerWithTouchDispatcher() override;
	bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
	void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
	void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
	void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;
#endif

private:
	void onBackButton(CCObject* sender);

#ifdef GEODE_IS_MOBILE
	void removeTouch(CCTouch* touch);
	CCPoint getTouchAnchor(CCTouch* touch1, CCTouch* touch2) const;

	bool m_isZooming = false;
	CCPoint m_zoomAnchor = ccp(0, 0);
	std::vector<CCTouch*> m_touches = {};
#endif

	CCMenu* m_backMenu = nullptr;
};

void addZoomButtonToPauseLayer(CCNode* pauseLayer, CCObject* target, SEL_MenuHandler callback);
