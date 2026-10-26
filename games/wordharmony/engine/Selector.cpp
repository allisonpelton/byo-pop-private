#include "Selector.h"
#include "Font.h"
#include "Graphics.h"
#include <cmath>

using namespace engine;

// 0x426f10
Selector::Selector()
{
	mCurrent = -1;
	mNext = -1;
	mTime = 0;
	mDuration = 0;
	mFadeIn = false;
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
	setName("Selector");
}

// 0x426c90: the current child and, during a change, the next one, each in its own state; their visibility is not
// asked (the vector's checked access stays: an index past the end throws)
void Selector::draw(Graphics* g)
{
	if (mVisible)
	{
		if ((int)mChildren.size() <= mCurrent)
			mCurrent = (int)mChildren.size() - 1;
		if (-1 < mCurrent)
		{
			g->pushState();
			Node* aChild = mChildren.at(mCurrent);
			if (aChild != nullptr)
			{
				aChild->applyTransform(g);
				aChild->draw(g);
			}
			g->popState();
		}
		if (-1 < mNext)
		{
			g->pushState();
			Node* aChild = mChildren.at(mNext);
			if (aChild != nullptr)
			{
				aChild->applyTransform(g);
				aChild->draw(g);
			}
			g->popState();
		}
	}
}

// 0x426de0: a fade-in raises the next child's opacity over the current one; a fade-out lowers the current one's, and
// the next one takes its place at the end
void Selector::update(UpdateInfo& theInfo)
{
	Container::update(theInfo);
	if (mDuration != 0.0f)
	{
		float aTime = mTime - theInfo.mSeconds;
		mTime = aTime;
		if (aTime > 0.0f)
		{
			float aFraction = aTime / mDuration;
			if (mFadeIn)
			{
				mChildren.at(mNext)->setOpacity(1.0f - aFraction);
				return;
			}
			mChildren.at(mCurrent)->setOpacity(aFraction);
			return;
		}
		int aNext = mNext;
		mTime = 0;
		mDuration = 0;
		mCurrent = aNext;
		mNext = -1;
		mChildren.at(aNext)->setOpacity(1.0f);
		setFlags(NODE_LAYOUT);
	}
}

// 0x426b70: the selected child's bounds (or, with none, the container's), scaled and moved to the position
void Selector::layout()
{
	mBounds = Rect();
	Node* aSelected = getSelected();
	if (aSelected == nullptr)
	{
		Container::layout();
	}
	else
	{
		if (aSelected->hasFlag(NODE_LAYOUT))
			aSelected->layout();
		mBounds.unite(*aSelected->getBounds());
	}
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(NODE_LAYOUT);
}

// 0x426b00: with a duration the change runs in update; a change that starts during another one finishes the other
void Selector::select(int theIndex, float theDuration, bool theFadeIn)
{
	if (theDuration != 0.0f)
	{
		mDuration = theDuration;
		mFadeIn = theFadeIn;
		mTime = theDuration;
		if (-1 < mNext)
			mCurrent = mNext;
		mNext = theIndex;
		return;
	}
	mCurrent = theIndex;
	setFlags(NODE_LAYOUT);
}

// 0x426d90
Node* Selector::getSelected()
{
	if (-1 < mCurrent && (size_t)mCurrent < mChildren.size())
		return mChildren.at(mCurrent);
	return nullptr;
}
