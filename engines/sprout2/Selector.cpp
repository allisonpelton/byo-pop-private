#include "Selector.h"

#include "DrawContext.h"

#include <cmath>

namespace engine
{

Selector::Selector() : mCurrent(-1), mNext(-1), mFadeTimer(0.0f), mFadeTime(0.0f), mFadeIn(false)
{
	addOwnState(STATE_VISIBLE | STATE_ANIMATED);
}

void Selector::paint(DrawContext* theContext)
{
	if (!isVisible())
		return;
	if (getChildCount() <= mCurrent)
		mCurrent = getChildCount() - 1;
	if (mCurrent >= 0)
	{
		theContext->pushState();
		Component* aChild = mChildren[mCurrent];
		if (aChild != nullptr)
		{
			aChild->applyTransform(theContext);
			aChild->paint(theContext);
		}
		// The original has popState's work written out here
		theContext->popState();
	}
	if (mNext >= 0)
	{
		theContext->pushState();
		Component* aChild = mChildren[mNext];
		if (aChild != nullptr)
		{
			aChild->applyTransform(theContext);
			aChild->paint(theContext);
		}
		theContext->popState();
	}
}

void Selector::update(UpdateContext& theContext)
{
	Container::update(theContext);
	if (mFadeTime == 0.0f)
		return;
	double aTimer = (double)mFadeTimer - theContext.mElapsed;
	mFadeTimer = (float)aTimer;
	if (aTimer > 0.0)
	{
		float aShare = (float)(aTimer / mFadeTime);
		if (mFadeIn)
			mChildren[mNext]->setOpacity(1.0f - aShare);
		else
			mChildren[mCurrent]->setOpacity(aShare);
		return;
	}
	mFadeTimer = 0.0f;
	mFadeTime = 0.0f;
	mChildren[mCurrent]->setOpacity(1.0f);
	mCurrent = mNext;
	mNext = -1;
	mChildren[mCurrent]->setOpacity(1.0f);
	addState(STATE_BOUNDS_DIRTY);
}

void Selector::computeBounds()
{
	mBounds = RectF(0.0f, 0.0f, 0.0f, 0.0f);
	Component* aSelected = getSelected();
	if (aSelected == nullptr)
		Container::computeBounds();
	else
	{
		if ((~aSelected->getOffStates() & aSelected->getTreeStates() & STATE_BOUNDS_DIRTY) != 0)
			aSelected->computeBounds();
		const RectF& aChildBounds = aSelected->getBounds();
		mBounds.left = minOf(mBounds.left, aChildBounds.left);
		mBounds.right = maxOf(mBounds.right, aChildBounds.right);
		mBounds.top = minOf(mBounds.top, aChildBounds.top);
		mBounds.bottom = maxOf(mBounds.bottom, aChildBounds.bottom);
	}
	float aScaleX = std::fabs(mScale.x);
	float aScaleY = std::fabs(mScale.y);
	mBounds.left = mBounds.left * aScaleX;
	mBounds.right = mBounds.right * aScaleX;
	mBounds.top = mBounds.top * aScaleY;
	mBounds.bottom = mBounds.bottom * aScaleY;
	mBounds.left = mBounds.left + mPosition.x;
	mBounds.right = mBounds.right + mPosition.x;
	mBounds.top = mBounds.top + mPosition.y;
	mBounds.bottom = mBounds.bottom + mPosition.y;
	removeState(STATE_BOUNDS_DIRTY);
}

void Selector::select(int theIndex, float theFadeTime, bool theFadeIn)
{
	if (theFadeTime == 0.0f)
	{
		if (mNext >= 0)
		{
			mChildren[mNext]->setOpacity(1.0f);
			mNext = -1;
		}
		mFadeTime = 0.0f;
		mFadeTimer = 0.0f;
		mCurrent = theIndex;
		if (theIndex >= 0)
			mChildren[theIndex]->setOpacity(1.0f);
		addState(STATE_BOUNDS_DIRTY);
		return;
	}
	// A fade under way is cut short: the child it faded to becomes the one faded from
	mFadeTime = theFadeTime;
	mFadeIn = theFadeIn;
	mFadeTimer = theFadeTime;
	if (mNext >= 0)
		mCurrent = mNext;
	mNext = theIndex;
}

Component* Selector::getSelected()
{
	if (mCurrent >= 0 && mCurrent < getChildCount())
		return mChildren[mCurrent];
	return nullptr;
}

} // namespace engine
