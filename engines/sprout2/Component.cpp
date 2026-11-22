#include "Component.h"

#include <algorithm>

#include "Container.h"
#include "DrawContext.h"
#include "Font.h"

namespace engine
{

Component::Component()
	: mParent(nullptr), mPosition(0, 0), mAbsolutePosition(0, 0), mScale(1, 1), mRotation(0), mOpacity(1),
	  mColor(1, 1, 1, 1), mColorMode(0), mFilterMode(0), mOwnStates(0), mTreeStates(0), mOffStates(0)
{
}

Component::~Component()
{
	if (!mName.empty() && getEngine() != nullptr)
		getEngine()->removeNamedComponent(this);
}

Vector2f Component::getAbsolutePosition()
{
	if (mParent == nullptr)
		return mPosition;
	Vector2f aParentPosition = mParent->getAbsolutePosition();
	return Vector2f(aParentPosition.x + mPosition.x, mPosition.y + aParentPosition.y);
}

void Component::setPosition(const Vector2f& thePosition)
{
	mPosition = thePosition;
	addState(STATE_BOUNDS_DIRTY);
}

void Component::setPosition(float x, float y)
{
	mPosition.x = x;
	mPosition.y = y;
	addState(STATE_BOUNDS_DIRTY);
}

void Component::setScale(float theScaleX, float theScaleY)
{
	mScale.x = theScaleX;
	mScale.y = theScaleY;
	addState(STATE_BOUNDS_DIRTY);
}

void Component::setFont(Font* theFont)
{
	mFont = theFont;
	addState(STATE_BOUNDS_DIRTY);
}

void Component::setColor(float theRed, float theGreen, float theBlue)
{
	mColor.r = theRed;
	mColor.g = theGreen;
	mColor.b = theBlue;
}

void Component::setPixelFx(PixelFx* thePixelFx)
{
	mPixelFx = thePixelFx;
	mOwnStates |= STATE_ANIMATED;
	addState(STATE_ANIMATED);
}

void Component::setName(const std::string& theName)
{
	Engine* anEngine = getEngine();
	if (!mName.empty())
		anEngine->removeNamedComponent(this);
	mName = theName;
	anEngine->addNamedComponent(getName(), this);
}

void Component::applyTransform(DrawContext* theContext)
{
	DrawState& aState = theContext->back();
	// The original keeps each product on the x87 stack (53 bits) and rounds once at the store
	aState.mPosition.x = (float)((double)aState.mScale.x * mPosition.x + aState.mPosition.x);
	aState.mPosition.y = (float)((double)mPosition.y * aState.mScale.y + aState.mPosition.y);
	aState.mScale.x = mScale.x * aState.mScale.x;
	aState.mScale.y = mScale.y * aState.mScale.y;
	aState.mRotation = mRotation + aState.mRotation;
	aState.mOpacity = clampOf(mOpacity, 0.0f, 1.0f) * aState.mOpacity;
	if (mFont != nullptr)
		aState.mFont = mFont;
	if (mColorMode != 0)
	{
		aState.mColor = mColor;
		aState.mColorMode = (ColorMode)mColorMode;
	}
	if (mFilterMode != 0)
		aState.mFilterMode = (FilterMode)mFilterMode;
	if (mPixelFx != nullptr)
		aState.mPixelFx = mPixelFx;
}

bool Component::contains(const Vector2f& thePoint)
{
	return mBounds.contains(thePoint.x, thePoint.y);
}

Component* Component::hitTest(int theStates, Vector2f& thePoint)
{
	if ((theStates & ~mOffStates & mTreeStates) == 0)
		return nullptr;
	if (!contains(thePoint))
		return nullptr;
	return this;
}

void Component::toLocal(Vector2f& thePoint)
{
	if (mParent == nullptr)
		return;
	mParent->toLocal(thePoint);
	thePoint.x = thePoint.x - mPosition.x;
	thePoint.y = thePoint.y - mPosition.y;
}

void Component::toParentLocal(Vector2f& thePoint)
{
	if (mParent != nullptr)
		mParent->toLocal(thePoint);
}

void Component::setVisible(bool theVisible)
{
	if (theVisible)
		mOffStates &= ~STATE_VISIBLE;
	else
		mOffStates |= STATE_VISIBLE;
}

bool Component::isVisible()
{
	return ((~mOffStates & mTreeStates) & STATE_VISIBLE) != 0;
}

void Component::setTakesInput(bool theTakesInput)
{
	if (theTakesInput)
		addOwnState(STATE_INPUT);
	else
		removeOwnState(STATE_INPUT);
}

void Component::update(UpdateContext& theContext)
{
	mAbsolutePosition.x = mPosition.x + theContext.mPosition.x;
	mAbsolutePosition.y = mPosition.y + theContext.mPosition.y;
	if (mPixelFx != nullptr)
		mPixelFx->update(theContext.mElapsed);
	applyModifiers(theContext);
	removeState(STATE_ANIMATED | STATE_DELETE | STATE_CHILDREN_CHANGED);
}

void Component::dump(int theIndent)
{
	Engine* anEngine = getEngine();
	std::string aName = mName;
	if (aName.empty())
		aName = "[unknown]";
	for (int i = 0; i < theIndent; i++)
		anEngine->log("\t");
	anEngine->log("<%s name=\"%s\" %s visible=\"%s\" flags=\"%x\" refCount=\"%d\" bounds=\"(%.2f,%.2f),(%.2f,%.2f)\" />\n",
		getClassName().c_str(), aName.c_str(), (mOwnStates & STATE_DELETE) ? "DELETE" : "",
		isVisible() ? "true" : "false", mTreeStates, refCount(), (double)mBounds.left, (double)mBounds.top,
		(double)mBounds.right, (double)mBounds.bottom);
}

void Component::setParent(Container* theParent)
{
	mParent = theParent;
	addRef();
}

void Component::clearParent()
{
	mParent = nullptr;
	release();
}

void Component::addState(int theStates)
{
	if ((theStates & ~mTreeStates) == 0)
		return;
	mTreeStates |= theStates;
	if (mParent != nullptr)
		mParent->addState(theStates);
}

void Component::removeState(int theStates)
{
	if ((theStates & ~mOwnStates & mTreeStates) == 0)
		return;
	mTreeStates = (~theStates & mTreeStates) | mOwnStates;
	if (mParent != nullptr)
		mParent->removeState(theStates);
}

void Component::applyModifiers(UpdateContext& theContext)
{
	for (size_t i = 0; i < mModifiers.size(); i++)
		mModifiers[i]->modify(theContext, this);
}

void Component::addOwnState(int theStates)
{
	mOwnStates |= theStates;
	addState(theStates);
}

void Component::addModifier(Modifier* theModifier)
{
	if (std::find(mModifiers.begin(), mModifiers.end(), theModifier) == mModifiers.end())
		mModifiers.push_back(theModifier);
	addOwnState(STATE_ANIMATED);
}

void Component::removeModifier(Modifier* theModifier)
{
	std::vector<Ref<Modifier>>::iterator anIt = std::find(mModifiers.begin(), mModifiers.end(), theModifier);
	if (anIt != mModifiers.end())
		mModifiers.erase(anIt);
}

void Component::removeOwnState(int theStates)
{
	mOwnStates &= ~theStates;
	removeState(theStates);
}

} // namespace engine
