#include "FadeContainer.h"

using namespace engine;

// 0x429b70 (wordharmony), 0x466ff0 (pizzafrenzy): the signal build's names it nothing
FadeContainer::FadeContainer()
{
	mCallback = nullptr;
	mCallbackArg = nullptr;
	if (!gSignalBuild)
		setName("FadeContainer");
	addOwnFlags(NODE_LISTENERS);
	mDuration = 1.0f;
	mTarget = 1.0f;
	mFadingIn = false;
	mTimeLeft = 0;
	mRemoveWhenHidden = false;
}

// 0x429a80 (wordharmony), 0x466ef0 (pizzafrenzy): the callback runs before the last step's opacity is set
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
			{
				setVisible(false);
				if (mRemoveWhenHidden)
					addOwnFlags(NODE_REMOVE);
			}
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

// 0x429930 (wordharmony), 0x466d70 (pizzafrenzy): a fade the same way that is still running is left alone, and so is
// one whose end is already reached
void FadeContainer::fade(bool theIn, float theDuration, float theTarget, bool theRemoveWhenHidden)
{
	if (theIn == mFadingIn && mTimeLeft > 0.0f)
		return;
	float anOpacity = getOpacity();
	if (theIn)
	{
		if (theTarget == anOpacity && isVisible() && mTimeLeft == 0.0f)
			return;
		setVisible(true);
		if (gSignalBuild)
		{
			setOpacity(0);
			mRemoveWhenHidden = false;
		}
	}
	else
	{
		if (1.0f - theTarget == anOpacity && !isVisible() && mTimeLeft == 0.0f)
			return;
		if (gSignalBuild)
			mRemoveWhenHidden = theRemoveWhenHidden;
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

// 0x429a60 (wordharmony), 0x466eb0 (pizzafrenzy)
void FadeContainer::setCallback(FadeCallback theCallback, void* theArg)
{
	mCallback = theCallback;
	mCallbackArg = theArg;
}

// 0x409d10 (wordharmony), 0x401140 (pizzafrenzy)
void FadeContainer::clearCallback()
{
	mCallback = nullptr;
	mCallbackArg = nullptr;
}
