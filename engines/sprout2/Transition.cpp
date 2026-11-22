#include "Transition.h"

#include "Engine.h"
#include "Geometry.h"

namespace engine
{

bool Transition::isAtEnd() const
{
	return (mDirection == TRANSITION_IN && mProgress == 1.0f) || (mDirection == TRANSITION_OUT && mProgress == 0.0f);
}

void Transition::modify(UpdateContext& theContext, Component* theComponent)
{
	if (mDone)
		return;
	mRemaining = maxOf((float)((double)mRemaining - theContext.mElapsed), 0.0f);
	if (mDirection == TRANSITION_IN)
		mProgress = (float)(1.0 - (double)mRemaining / mDuration);
	else if (mDirection == TRANSITION_OUT)
		mProgress = (float)((double)mRemaining / mDuration);
	apply();
	if (isAtEnd())
		mDone = true;
}

void Transition::setProgress(float theProgress)
{
	mProgress = maxOf(minOf(theProgress, 1.0f), 0.0f);
	mRemaining = (float)((double)mProgress * mDuration);
	apply();
	if (!mDone && isAtEnd())
		mDone = true;
}

void Transition::start(int theDirection)
{
	if (mDone)
	{
		mDone = false;
		mRemaining = mDuration;
		mProgress = 0.0f;
	}
	else
	{
		if (theDirection == mDirection)
			return;
		mRemaining = (float)((double)mDuration * mProgress);
	}
	mDirection = theDirection;
}

void FadeTransition::apply()
{
	mTarget->setOpacity(mProgress);
	if (mProgress != 0.0f)
		mTarget->removeOffState(STATE_VISIBLE);
	else
		mTarget->addOffState(STATE_VISIBLE);
}

} // namespace engine
