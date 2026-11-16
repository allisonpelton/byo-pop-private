#include "FadeContainer.h"

namespace engine
{

FadeContainer::FadeContainer()
	: mFadeTime(0.0f), mFadeDuration(1.0f), mFadingIn(false), mCallback(nullptr), mCallbackData(nullptr),
	  mFadeTarget(1.0f), mDeleteWhenDone(false)
{
	addOwnState(STATE_ANIMATED);
}

void FadeContainer::update(UpdateContext& theContext)
{
	Container::update(theContext);
	if (!(0.0f < mFadeTime) || !(0.0f < mFadeDuration))
		return;
	// The difference is compared before it is rounded to the float it is stored as
	double aTime = (double)mFadeTime - theContext.mElapsed;
	mFadeTime = (float)aTime;
	if (aTime <= 0.0)
	{
		mFadeTime = 0.0f;
		if (mCallback != nullptr)
			mCallback(mCallbackData);
		removeOwnState(STATE_ANIMATED);
		if (!mFadingIn && mFadeTarget == 1.0f)
		{
			setVisible(false);
			if (mDeleteWhenDone)
				addOwnState(STATE_DELETE);
		}
	}
	double aShare = 1.0 - (double)mFadeTime / mFadeDuration;
	if (mFadingIn)
		setOpacity((float)(aShare * mFadeTarget));
	else
		setOpacity((float)(1.0 - mFadeTarget * aShare));
}

void FadeContainer::fade(bool theFadeIn, float theDuration, float theTarget, bool theDeleteWhenDone)
{
	if (theFadeIn == mFadingIn && 0.0f < mFadeTime)
		return;
	float anOpacity = getOpacity();
	if (theFadeIn)
	{
		if (anOpacity == theTarget && isVisible() && mFadeTime == 0.0f)
			return;
		setVisible(true);
		mDeleteWhenDone = false;
	}
	else
	{
		if ((double)anOpacity == 1.0 - (double)theTarget && !isVisible() && mFadeTime == 0.0f)
			return;
		mDeleteWhenDone = theDeleteWhenDone;
	}
	mFadeTarget = theTarget;
	addOwnState(STATE_ANIMATED);
	mFadeDuration = theDuration;
	mFadingIn = theFadeIn;
	mFadeTime = theDuration;
	if (theDuration == 0.0f)
	{
		if (theFadeIn)
			setOpacity(mFadeTarget);
		else
		{
			setVisible(false);
			setOpacity((float)(1.0 - mFadeTarget));
		}
	}
}

void FadeContainer::setFadeCallback(FadeCallback theCallback, void* theData)
{
	mCallback = theCallback;
	mCallbackData = theData;
}

void FadeContainer::clearFadeCallback()
{
	mCallback = nullptr;
	mCallbackData = nullptr;
}

} // namespace engine
