#include "utils.hpp"
#include <algorithm>
using namespace geode::prelude;

void zoomPlayLayer(CCNode* playLayer, float delta, CCPoint screenAnchor) {
	if (!playLayer) return;

	CCSize contentSize = playLayer->getContentSize();
	CCPoint anchorPoint = ccp(
		screenAnchor.x - contentSize.width / 2,
		screenAnchor.y - contentSize.height / 2
	);

	float oldScale = playLayer->getScale();
	float newScale;

	if (delta < 0) {
		newScale = oldScale / (1 - delta);
	} else {
		newScale = oldScale * (1 + delta);
	}

	if (newScale < 1.0f) newScale = 1.0f;
	
	CCPoint deltaFromAnchorPrev = playLayer->getPosition() - anchorPoint;

	playLayer->setPosition(anchorPoint);
	playLayer->setScale(newScale);
	playLayer->setPosition(anchorPoint + deltaFromAnchorPrev * newScale / oldScale);
}

CCSize getScreenSize() {
	float screenTop = CCDirector::sharedDirector()->getScreenTop();
	float screenBottom = CCDirector::sharedDirector()->getScreenBottom();
	float screenLeft = CCDirector::sharedDirector()->getScreenLeft();
	float screenRight = CCDirector::sharedDirector()->getScreenRight();

	return CCSize{ screenRight - screenLeft, screenTop - screenBottom };
}

void clampPlayLayerPos(CCNode* playLayer) {
	if (!playLayer) return;

	CCPoint pos = playLayer->getPosition();
	CCSize screenSize = getScreenSize();
	CCSize contentSize = playLayer->getContentSize();

	float xLimit = (contentSize.width * playLayer->getScale() - screenSize.width) * 0.5f;
	float yLimit = (contentSize.height * playLayer->getScale() - screenSize.height) * 0.5f;

	pos.x = std::clamp(pos.x, -xLimit, xLimit);
	pos.y = std::clamp(pos.y, -yLimit, yLimit);

	playLayer->setPosition(pos);
}