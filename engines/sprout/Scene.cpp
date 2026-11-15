#include "Scene.h"
#include "App.h"
#include "Host.h"

using namespace engine;

// 0x471a40 (pizzafrenzy)
Scene::Scene() : mCapture(nullptr)
{
	mEvent.mTarget = nullptr;
	mEvent.mPosition = Point(0, 0);
	mEvent.mButtons = 0;
}

// 0x471760 (pizzafrenzy)
Scene::~Scene()
{
	disconnect();
}

// 0x471b50 (pizzafrenzy)
void Scene::connect()
{
	App* anApp = App::instance();
	anApp->mUpdateSignal.connect(this, &Scene::onAppUpdate);
	anApp->mDrawSignal.connect(this, &Scene::onAppDraw);
	anApp->mMouseDownSignal.connect(this, &Scene::onAppMouseDown);
	anApp->mMouseUpSignal.connect(this, &Scene::onAppMouseUp);
	anApp->mRightDownSignal.connect(this, &Scene::onAppRightDown);
	anApp->mRightUpSignal.connect(this, &Scene::onAppRightUp);
	anApp->mMouseMoveSignal.connect(this, &Scene::onAppMouseMove);
}

// 0x471690 (pizzafrenzy): the layout's flags cleared first (0x4671e0 (pizzafrenzy))
void Scene::disconnect()
{
	App* anApp = App::instance();
	if (anApp == nullptr)
		return;
	anApp->mUpdateSignal.disconnect(this);
	anApp->mDrawSignal.disconnect(this);
	anApp->mMouseDownSignal.disconnect(this);
	anApp->mMouseUpSignal.disconnect(this);
	anApp->mRightDownSignal.disconnect(this);
	anApp->mRightUpSignal.disconnect(this);
	anApp->mMouseMoveSignal.disconnect(this);
}

// 0x4710f0 (pizzafrenzy)
void Scene::update(UpdateInfo& theInfo)
{
	Container::update(theInfo);
	Container::layout();
}

// 0x471130 (pizzafrenzy)
void Scene::onAppUpdate(UpdateInfo& theInfo)
{
	update(theInfo);
}

// 0x471120 (pizzafrenzy)
void Scene::onAppDraw(Graphics* g)
{
	draw(g);
}

// 0x4712a0 (pizzafrenzy): a hovered node the scene alone still holds (it left the scene) is let go first
Node* Scene::findTarget(const Point& thePoint)
{
	if (mHover != nullptr && mHover->refCount() == 1)
		mHover = nullptr;
	mEvent.mPosition = thePoint;
	Node* aTarget = hitTest(NODE_MOUSE, mEvent.mPosition);
	if (mCapture != nullptr && aTarget != mCapture)
		aTarget = nullptr;
	if (aTarget != mHover.get())
	{
		if (mHover != nullptr)
		{
			mEvent.mTarget = mHover;
			mHover->mouseExitEvent(mEvent);
		}
		mHover = aTarget;
		if (aTarget != nullptr)
		{
			mEvent.mTarget = aTarget;
			aTarget->mouseEnterEvent(mEvent);
		}
	}
	if (mCapture != nullptr)
	{
		mEvent.mTarget = mCapture;
		Point* anOrigin = mCapture->getAbsPosition();
		mEvent.mPosition = Point(thePoint.x - anOrigin->x, thePoint.y - anOrigin->y);
		return mEvent.mTarget;
	}
	mEvent.mTarget = aTarget;
	return aTarget;
}

// 0x4714c0 (pizzafrenzy)
void Scene::onAppMouseDown(Point& thePoint)
{
	mEvent.mButtons |= 1;
	Node* aTarget = findTarget(thePoint);
	if (aTarget != nullptr)
		aTarget->mouseDownEvent(mEvent);
}

// 0x4714f0 (pizzafrenzy)
void Scene::onAppMouseUp(Point& thePoint)
{
	mEvent.mButtons &= ~1u;
	Node* aTarget = findTarget(thePoint);
	if (aTarget != nullptr)
		aTarget->mouseUpEvent(mEvent);
}

// 0x471520 (pizzafrenzy)
void Scene::onAppRightDown(Point& thePoint)
{
	mEvent.mButtons |= 2;
	Node* aTarget = findTarget(thePoint);
	if (aTarget != nullptr)
		aTarget->rightDownEvent(mEvent);
}

// 0x471550 (pizzafrenzy)
void Scene::onAppRightUp(Point& thePoint)
{
	mEvent.mButtons &= ~2u;
	Node* aTarget = findTarget(thePoint);
	if (aTarget != nullptr)
		aTarget->rightUpEvent(mEvent);
}

// 0x471580 (pizzafrenzy)
void Scene::onAppMouseMove(Point& thePoint)
{
	Node* aTarget = findTarget(thePoint);
	if (aTarget != nullptr)
		aTarget->mouseMoveEvent(mEvent);
}

// 0x4715b0 (pizzafrenzy): the application's mouse position (its slot 0xc8 (pizzafrenzy))
void Scene::refreshMouse()
{
	Point aPoint = App::instance()->getMousePosition();
	Node* aTarget = findTarget(aPoint);
	if (aTarget != nullptr)
		aTarget->mouseMoveEvent(mEvent);
}
