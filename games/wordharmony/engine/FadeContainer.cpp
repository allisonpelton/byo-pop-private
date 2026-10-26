#include "FadeContainer.h"

using namespace engine;

// 0x429b70
FadeContainer::FadeContainer()
{
	mCallback = nullptr;
	mCallbackArg = nullptr;
	setName("FadeContainer");
	addOwnFlags(NODE_LISTENERS);
	mDuration = 1.0f;
	mTarget = 1.0f;
	mFadingIn = false;
	mTimeLeft = 0;
}

// 0x429a80: the callback runs before the last step's opacity is set
void FadeContainer::update(UpdateInfo& theInfo)
{
	Container::update(theInfo);
	if (0.0f < mTimeLeft && 0.0f < mDuration)
	{
		float aTimeLeft = mTimeLeft - theInfo.mSeconds;
		mTimeLeft = aTimeLeft;
		if (aTimeLeft < 0.0f)
		{
			mTimeLeft = 0;
			if (mCallback != nullptr)
				mCallback(mCallbackArg);
			removeOwnFlags(NODE_LISTENERS);
			if (!mFadingIn && mTarget == 1.0f)
				setVisible(false);
		}
		float aValue = (1.0f - mTimeLeft / mDuration) * mTarget;
		if (mFadingIn)
		{
			setOpacity(aValue);
			return;
		}
		setOpacity(1.0f - aValue);
	}
}

// 0x429930: a fade the same way that is still running is left alone, and so is one whose end is already reached
void FadeContainer::fade(bool theIn, float theDuration, float theTarget)
{
	if (theIn == mFadingIn && mTimeLeft > 0.0f)
		return;
	float anOpacity = getOpacity();
	if (theIn)
	{
		if (theTarget == anOpacity && isVisible() && mTimeLeft == 0.0f)
			return;
		setVisible(true);
	}
	else
	{
		if (1.0f - theTarget == anOpacity && !isVisible() && mTimeLeft == 0.0f)
			return;
	}
	mTarget = theTarget;
	addOwnFlags(NODE_LISTENERS);
	mTimeLeft = theDuration;
	mFadingIn = theIn;
	mDuration = theDuration;
	if (theDuration == 0.0f)
	{
		if (theIn)
		{
			setOpacity(mTarget);
			return;
		}
		setVisible(false);
		setOpacity(1.0f - mTarget);
	}
}

// 0x429a60
void FadeContainer::setCallback(FadeCallback theCallback, void* theArg)
{
	mCallback = theCallback;
	mCallbackArg = theArg;
}

// 0x409d10
void FadeContainer::clearCallback()
{
	mCallback = nullptr;
	mCallbackArg = nullptr;
}
