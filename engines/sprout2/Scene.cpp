#include "Scene.h"

#include "Engine.h"

namespace engine
{

namespace
{

// The mouse event's button bits
const int MOUSE_LEFT = 1;
const int MOUSE_RIGHT = 2;

} // namespace

Scene::Scene() : mActive(true)
{
	mMouseEvent.mTarget = nullptr;
	mMouseEvent.mButtons = 0;
	mMouseEvent.mScene = this;
	mMouseEvent.mWheelDelta = 0;
}

void Scene::enable()
{
	Engine* anEngine = getEngine();
	anEngine->mUpdateSignal.connect(this, &Scene::updateSlot);
	anEngine->mDrawSignal.connect(this, &Scene::drawSlot);
	anEngine->mActivateSignal.connect(this, &Scene::activateSlot);
	anEngine->mMouseDownSignal.connect(this, &Scene::mouseDownSlot);
	anEngine->mMouseUpSignal.connect(this, &Scene::mouseUpSlot);
	anEngine->mRightDownSignal.connect(this, &Scene::rightDownSlot);
	anEngine->mRightUpSignal.connect(this, &Scene::rightUpSlot);
	anEngine->mMouseMoveSignal.connect(this, &Scene::mouseMoveSlot);
	anEngine->mMouseWheelSignal.connect(this, &Scene::mouseWheelSlot);
}

void Scene::disable()
{
	Container::disable();
	Engine* anEngine = getEngine();
	anEngine->mUpdateSignal.disconnect(this);
	anEngine->mDrawSignal.disconnect(this);
	anEngine->mActivateSignal.disconnect(this);
	anEngine->mMouseDownSignal.disconnect(this);
	anEngine->mMouseUpSignal.disconnect(this);
	anEngine->mRightDownSignal.disconnect(this);
	anEngine->mRightUpSignal.disconnect(this);
	anEngine->mMouseMoveSignal.disconnect(this);
	anEngine->mMouseWheelSignal.disconnect(this);
}

void Scene::update(UpdateContext& theContext)
{
	Container::update(theContext);
	computeBounds();
}

void Scene::activateSlot(bool theActive)
{
	if (theActive)
	{
		if (!mActive)
			findMouseTarget(mMousePosition);
	}
	else if (mActive)
	{
		Component* aCancelled = mMouseCapture != nullptr ? mMouseCapture.get() : mHovered.get();
		if (aCancelled != nullptr)
		{
			mMouseEvent.mTarget = aCancelled;
			aCancelled->mouseCancel(mMouseEvent);
		}
		mHovered = nullptr;
	}
	mActive = theActive;
}

void Scene::mouseDownSlot(const Vector2<int>& thePosition)
{
	mMouseEvent.mButtons |= MOUSE_LEFT;
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget != nullptr)
		aTarget->mouseDown(mMouseEvent);
}

void Scene::mouseUpSlot(const Vector2<int>& thePosition)
{
	mMouseEvent.mButtons &= ~MOUSE_LEFT;
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget != nullptr)
		aTarget->mouseUp(mMouseEvent);
}

void Scene::rightDownSlot(const Vector2<int>& thePosition)
{
	mMouseEvent.mButtons |= MOUSE_RIGHT;
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget != nullptr)
		aTarget->rightDown(mMouseEvent);
}

void Scene::rightUpSlot(const Vector2<int>& thePosition)
{
	mMouseEvent.mButtons &= ~MOUSE_RIGHT;
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget != nullptr)
		aTarget->rightUp(mMouseEvent);
}

void Scene::mouseMoveSlot(const Vector2<int>& thePosition)
{
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget != nullptr)
		aTarget->mouseMove(mMouseEvent);
}

void Scene::mouseWheelSlot(const Vector2<int>& thePosition, int theDelta)
{
	Component* aTarget = findMouseTarget(thePosition);
	if (aTarget == nullptr)
		return;
	mMouseEvent.mWheelDelta = theDelta;
	aTarget->mouseMove(mMouseEvent);
}

Component* Scene::findMouseTarget(const Vector2<int>& thePosition)
{
	if (mHovered != nullptr && mHovered->refCount() == 1)
		mHovered = nullptr;
	mMousePosition = thePosition;
	if (!mActive)
	{
		mMouseEvent.mTarget = nullptr;
		return nullptr;
	}
	mMouseEvent.mPosition.x = (float)thePosition.x;
	mMouseEvent.mPosition.y = (float)thePosition.y;
	// The point is left in the hit component's space
	Component* aHit = hitTest(STATE_INPUT, mMouseEvent.mPosition);
	if (mMouseCapture != nullptr && aHit != mMouseCapture.get())
		aHit = nullptr;
	if (aHit != mHovered.get())
	{
		if (mHovered != nullptr)
		{
			mMouseEvent.mTarget = mHovered.get();
			mHovered->mouseLeave(mMouseEvent);
		}
		mHovered = aHit;
		if (mHovered != nullptr)
		{
			mMouseEvent.mTarget = mHovered.get();
			mHovered->mouseEnter(mMouseEvent);
		}
	}
	if (mMouseCapture == nullptr)
		mMouseEvent.mTarget = aHit;
	else
	{
		mMouseEvent.mTarget = mMouseCapture.get();
		mMouseEvent.mPosition.x = (float)thePosition.x;
		mMouseEvent.mPosition.y = (float)thePosition.y;
		Vector2f anOrigin = mMouseCapture->getAbsolutePosition();
		mMouseEvent.mPosition.x = mMouseEvent.mPosition.x - anOrigin.x;
		mMouseEvent.mPosition.y = mMouseEvent.mPosition.y - anOrigin.y;
	}
	return mMouseEvent.mTarget;
}

} // namespace engine
