#ifdef GEODE_IS_MOBILE
#pragma once

#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

class AndroidZoomLayer : public CCLayer {
public:
	static AndroidZoomLayer* instance;
	static AndroidZoomLayer* create(CCNode* sceneLayer);

	bool init(CCNode* sceneLayer);
	void registerWithTouchDispatcher() override;
	void onExit() override;
	bool ccTouchBegan(CCTouch* pTouch, CCEvent* pEvent) override;
	void ccTouchMoved(CCTouch* pTouch, CCEvent* pEvent) override;
	void ccTouchEnded(CCTouch* pTouch, CCEvent* pEvent) override;
	void ccTouchCancelled(CCTouch* pTouch, CCEvent* pEvent) override;
	void onBackButton(CCObject* sender);
	void onBackButton23(CCObject* sender);
	void removeTouchEvent(CCTouch* pTouch, CCEvent* pEvent);
private:
	CCPoint getAnchorPoint(CCTouch* touch1, CCTouch* touch2);

	bool m_isZooming = false;
	CCPoint m_ZoomAnchor = ccp(0, 0);

	CCNode* m_sceneLayer;
	CCNode* m_playLayer;
	CCNode* m_pauseLayer;
	CCMenu* m_backMenu = nullptr;
	std::vector<CCTouch*> m_touches = {};
};

#endif // GEODE_IS_MOBILE
