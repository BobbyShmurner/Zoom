#include <charconv>

#include <geode.custom-keybinds/include/Keybinds.hpp>

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingEvent.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;
using namespace keybinds;

float clamp(float d, float min, float max) {
	const float t = d < min ? min : d;
	return t > max ? max : t;
}

class ZoomManager {
public:
	GLFWwindow* window;
	CCPoint lastMousePos;
	CCPoint deltaMousePos;

	bool isPaused;
	bool middleMouseDown;

	bool autoHideMenu;
	bool autoShowMenu;
	bool altDisablesZoom;

	static ZoomManager* get() {
		static auto inst = new ZoomManager;
		return inst;
	}

	void togglePauseMenu() {
		if (!isPaused) return;

		CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
		if (!pauseLayer) return;

		pauseLayer->setVisible(!pauseLayer->isVisible());
	}

	void setPauseMenuVisible(bool visible) {
		CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
		if (!pauseLayer) return;

		pauseLayer->setVisible(visible);
	}

	void setZoom(float zoom) {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		playLayer->setScale(zoom);
		onScreenResize();
	}

	void zoom(float delta) {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		CCSize contentSize = playLayer->getContentSize();
		float oldScale = playLayer->getScale();
		float newScale;

		if (delta < 0) {
			newScale = oldScale / (1 - delta);
		} else {
			newScale = oldScale * (1 + delta);
		}

		if (newScale < 1.0f) newScale = 1.0f;
		
		CCPoint mouseScreenPos = getMousePosOnScreen();
		CCPoint anchorPoint = CCPoint { mouseScreenPos.x - contentSize.width / 2, -mouseScreenPos.y + contentSize.height / 2};
		CCPoint deltaFromAnchorPrev = playLayer->getPosition() - anchorPoint;

		playLayer->setPosition(anchorPoint);
		playLayer->setScale(newScale);
		playLayer->setPosition(anchorPoint + deltaFromAnchorPrev * newScale / oldScale);
		
		onScreenResize();
	}

	void move(CCPoint delta) {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		CCPoint pos = playLayer->getPosition();
		playLayer->setPosition(pos + delta);

		onScreenMove();
	}

	void setPos(float x, float y) {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		playLayer->setPosition(CCPoint{ x, y });

		onScreenMove();
	}

	float getZoom() {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return 1.0f;

		return playLayer->getScale();
	}

	CCPoint screenToWorld(CCPoint pos) {
		CCSize screenSize = getScreenSize();
		CCSize winSize = CCEGLView::get()->getFrameSize();

		return CCPoint{ pos.x * (screenSize.width / winSize.width), pos.y * (screenSize.height / winSize.height) };
	}

	CCSize getScreenSize() {
		float screenTop = CCDirector::sharedDirector()->getScreenTop();
		float screenBottom = CCDirector::sharedDirector()->getScreenBottom();
		float screenLeft = CCDirector::sharedDirector()->getScreenLeft();
		float screenRight = CCDirector::sharedDirector()->getScreenRight();

		return CCSize{ screenRight - screenLeft, screenTop - screenBottom };
	}

	CCPoint getMousePosOnScreen() {
		return screenToWorld(CCEGLView::get()->getMousePosition());
	}

	CCPoint getMousePosOnNode(CCNode* node) {
		return node->convertToNodeSpace(getMousePosOnScreen());
	}

	void update(float dt) {
		auto pos = CCEGLView::get()->getMousePosition();
		auto lastMousePos = ZoomManager::get()->lastMousePos;

		ZoomManager::get()->deltaMousePos = CCPoint{ pos.x - lastMousePos.x, pos.y - lastMousePos.y };
		ZoomManager::get()->lastMousePos = pos;

		if (!isPaused) return;

		if (middleMouseDown) {
			CCPoint delta = ZoomManager::get()->deltaMousePos;
			move(screenToWorld(CCPoint { delta.x, -delta.y }));
		}
	}

	void onResume() {
		setZoom(1.0f);
		setPos(0.0f, 0.0f);

		isPaused = false;
		ZoomManager::get()->middleMouseDown = false;
	}

	void onPause() {
		isPaused = true;
		ZoomManager::get()->middleMouseDown = false;
	}

	void onScroll(float y, float x) {
		if (!isPaused) return;
		if (autoHideMenu) setPauseMenuVisible(false);

		if (altDisablesZoom) {
			auto kb = CCKeyboardDispatcher::get();
			if (kb->getAltKeyPressed()) {
				return;
			}
		}

		if (y > 0) {
			zoom(-0.1f);
		} else {
			zoom(0.1f);
		}
	}
private:
	void onScreenResize() {
		clampPos();
		if (!isPaused) return;

		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		if (autoShowMenu && playLayer->getScale() == 1.0f) {
			setPauseMenuVisible(true);
		}
	}

	void clampPos() {
		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		CCPoint pos = playLayer->getPosition();
		CCSize screenSize = getScreenSize();
		CCSize contentSize = playLayer->getContentSize();

		float xLimit = (contentSize.width * playLayer->getScale() - screenSize.width) * 0.5f;
		float yLimit = (contentSize.height * playLayer->getScale() - screenSize.height) * 0.5f;

		pos.x = clamp(pos.x, -xLimit, xLimit);
		pos.y = clamp(pos.y, -yLimit, yLimit);

		playLayer->setPosition(pos);
	}

	void onScreenMove() {
		clampPos();
		if (!isPaused) return;

		CCNode* playLayer = CCScene::get()->getChildByID("PlayLayer");
		if (!playLayer) return;

		if (autoHideMenu && playLayer->getScale() != 1.0f) {
			setPauseMenuVisible(false);
		}
	}
};

class $modify(PauseLayer) {
	void customSetup() {
		this->template addEventListener<InvokeBindFilter>([=](InvokeBindEvent* event) {
			if (event->isDown()) {
				ZoomManager::get()->togglePauseMenu();
			}

			return ListenerResult::Propagate;
		}, "toggle_menu"_spr);

		PauseLayer::customSetup();
	}

	void onResume(CCObject* sender) {
		ZoomManager::get()->onResume();
		PauseLayer::onResume(sender);
	}

	void onRestart(CCObject* sender) {
		ZoomManager::get()->onResume();
		PauseLayer::onRestart(sender);
	}

	void onRestartFull(CCObject* sender) {
		ZoomManager::get()->onResume();
		PauseLayer::onRestartFull(sender);
	}

	void onNormalMode(CCObject* sender) {
		ZoomManager::get()->onResume();
		PauseLayer::onNormalMode(sender);
	}

	void onPracticeMode(CCObject* sender) {
		ZoomManager::get()->onResume();
		PauseLayer::onPracticeMode(sender);
	}
};

class $modify(PlayLayer) {
	void pauseGame(bool p0) {
		ZoomManager::get()->onPause();
		PlayLayer::pauseGame(p0);
	}

	void startGame() {
		ZoomManager::get()->onResume();
		PlayLayer::startGame();
	}

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		ZoomManager::get()->onResume();
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}
};

class $modify(CCScheduler) {
	virtual void update(float dt) {
		ZoomManager::get()->update(dt);
		CCScheduler::update(dt);
	}
};

class $modify(CCEGLView) {
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
			if (action == GLFW_PRESS) {
				ZoomManager::get()->middleMouseDown = true;
			}
			else if (action == GLFW_RELEASE) {
				ZoomManager::get()->middleMouseDown = false;
			}
		}

		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
	}
};

class $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		ZoomManager::get()->onScroll(y, x);
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};

$execute {
	ZoomManager::get()->autoHideMenu = Mod::get()->getSettingValue<bool>("auto-hide-menu");
	listenForSettingChanges("auto-hide-menu", +[](bool enable) {
		ZoomManager::get()->autoHideMenu = enable;
	});

	ZoomManager::get()->autoShowMenu = Mod::get()->getSettingValue<bool>("auto-show-menu");
	listenForSettingChanges("auto-show-menu", +[](bool enable) {
		ZoomManager::get()->autoShowMenu = enable;
	});

	ZoomManager::get()->autoShowMenu = Mod::get()->getSettingValue<bool>("alt-disables-zoom");
	listenForSettingChanges("alt-disables-zoom", +[](bool enable) {
		ZoomManager::get()->altDisablesZoom = enable;
	});


    BindManager::get()->registerBindable({
        "toggle_menu"_spr,
        "Toggle Pause Menu",
        "",
        { Keybind::create(KEY_Home, Modifier::None) },
        "Zoom",
		false
    });
}