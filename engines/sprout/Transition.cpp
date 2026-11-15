#include "Transition.h"

using namespace engine;

// 0x469000 (pizzafrenzy): a new transition counts as finished
Transition::Transition(float theDuration)
{
	mNode = nullptr;
	mDuration = theDuration;
	mDirection = IN;
	mProgress = 0;
	mTimeLeft = 0;
	mDone = true;
}

// 0x468ee0 (pizzafrenzy)
void Transition::update(UpdateInfo& theInfo, Node* theNode)
{
	if (!mDone)
	{
		float aTimeLeft = mTimeLeft - theInfo.mSeconds;
		mTimeLeft = aTimeLeft;
		if (aTimeLeft < 0.0f)
			aTimeLeft = 0.0f;
		mTimeLeft = aTimeLeft;
		if (mDirection == IN)
			mProgress = 1.0f - aTimeLeft / mDuration;
		else if (mDirection == OUT)
			mProgress = aTimeLeft / mDuration;
		apply();
		if ((mDirection == IN && mProgress == 1.0f) || (mDirection == OUT && mProgress == 0.0f))
			mDone = true;
	}
}

// 0x468f70 (pizzafrenzy): the time left is the progress's share of the seconds, whichever way it runs
void Transition::setProgress(float theProgress)
{
	mProgress = theProgress;
	if (1.0f < theProgress)
		theProgress = 1.0f;
	mProgress = theProgress;
	if (theProgress < 0.0f)
		theProgress = 0.0f;
	mProgress = theProgress;
	mTimeLeft = theProgress * mDuration;
	apply();
	if (!mDone && ((mDirection == IN && mProgress == 1.0f) || (mDirection == OUT && mProgress == 0.0f)))
		mDone = true;
}

// 0x468e90 (pizzafrenzy)
void Transition::start(int theDirection)
{
	if (mDone)
	{
		mDirection = theDirection;
		mTimeLeft = mDuration;
		mDone = false;
		mProgress = 0;
		return;
	}
	if (theDirection != mDirection)
	{
		mDirection = theDirection;
		mTimeLeft = mDuration * mProgress;
	}
}

// 0x468e50 (pizzafrenzy): the mask's bit 1, which hides a node (0x467d70 (pizzafrenzy) and 0x467d80 (pizzafrenzy)), is
// the node's visible field here
void FadeTransition::apply()
{
	mNode->setOpacity(mProgress);
	if (mProgress == 0.0f)
	{
		mNode->mVisible = false;
		return;
	}
	mNode->mVisible = true;
}

// 0x475160 (pizzafrenzy)
SlideTransition::SlideTransition(float theDuration) : Transition(theDuration), mFrom(0, 0), mTo(0, 0)
{
}

// 0x475040 (pizzafrenzy)
void SlideTransition::apply()
{
	mNode->setOpacity(1.0f);
	mInterpolator.modify(mProgress, mNode);
}

// 0x475070 (pizzafrenzy): the first key leaves at once at speed 1; the second is reached slowing to a stop over the
// last 0.3 of the progress
void SlideTransition::setPoints(float theFromX, float theFromY, float theToX, float theToY)
{
	mInterpolator.clear();
	mTo.x = theToX;
	mTo.y = theToY;
	mFrom.x = theFromX;
	mFrom.y = theFromY;
	PositionKey aKey;
	aKey.mValue = mFrom;
	aKey.mTime = 0.0f;
	aKey.mInTime = 0.0f;
	aKey.mInSpeed = 1.0f;
	aKey.mOutTime = 0.0f;
	aKey.mOutSpeed = 1.0f;
	mInterpolator.addKey(aKey);
	aKey.mValue = mTo;
	aKey.mTime = 1.0f;
	aKey.mInTime = 0.3f;
	aKey.mInSpeed = 0.0f;
	aKey.mOutTime = 0.3f;
	aKey.mOutSpeed = 1.0f;
	mInterpolator.addKey(aKey);
}
