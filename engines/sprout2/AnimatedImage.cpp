#include "AnimatedImage.h"

#include "DrawContext.h"
#include "Surface.h"

namespace engine
{

AnimatedImage::AnimatedImage()
    : mRun(0), mFrame(0), mMode(ANIMATION_MODE_ONCE), mDirection(0), mRate(1.0f), mTimeLeft(0.0f)
{
	setName("AnimatedImage");
}

void AnimatedImage::paint(DrawContext* theContext)
{
	if (mData && mData->getFrameCount() != 0)
	{
		// The original's vector checks the index and ends the program past the frames (0x4a9ee6 (feedingfrenzy2), the
		// runtime's invalid-parameter end); the port draws nothing there
		Surface* aFrame = (unsigned int)mFrame < (unsigned int)mData->getFrameCount() ? mData->getFrame(mFrame) : nullptr;
		if (isVisible() && aFrame != nullptr)
			theContext->drawImage(aFrame);
	}
	removeState(STATE_VISIBLE);
}

void AnimatedImage::update(UpdateContext& theContext)
{
	if (0.0f <= mTimeLeft)
	{
		mTimeLeft = (float)((double)mTimeLeft - (double)mRate * (double)theContext.mElapsed);
		return;
	}
	while (mTimeLeft < 0.0f)
		advance();
}

void AnimatedImage::computeBounds()
{
	if (!mData || mData->getFrameCount() == 0)
	{
		mBounds.left = 0.0f;
		mBounds.top = 0.0f;
		mBounds.right = 0.0f;
		mBounds.bottom = 0.0f;
	}
	else
	{
		Surface* aFrame = mData->getFrame(0);
		if (aFrame == nullptr)
		{
			mBounds.left = mPosition.x;
			mBounds.top = mPosition.y;
			mBounds.right = mPosition.x;
			mBounds.bottom = mPosition.y;
		}
		else
		{
			const Vector2f& anAnchor = aFrame->getAnchor();
			mBounds.left = mPosition.x - anAnchor.x;
			mBounds.top = mPosition.y - anAnchor.y;
			// The sums with the size are stored as floats before the anchor comes off
			float aRight = (float)((double)aFrame->getWidth() + (double)mPosition.x);
			mBounds.right = aRight - anAnchor.x;
			float aBottom = (float)((double)aFrame->getHeight() + (double)mPosition.y);
			mBounds.bottom = aBottom - anAnchor.y;
		}
	}
	removeState(STATE_BOUNDS_DIRTY);
}

void AnimatedImage::setRun(int theRun, int theOffset)
{
	if (!mData || mData->getFrameCount() == 0 || theRun < 0)
		return;
	mRun = theRun;
	// The original reads the frame time through a null run for a number past the runs (0x46f890 (feedingfrenzy2)
	// returns none); the port leaves the image as it was
	if (theRun >= mData->getAnimationCount())
		return;
	const AnimationRun& aRun = mData->getAnimation(theRun);
	mFrame = (int)aRun.mStart + theOffset;
	mTimeLeft = aRun.mFrameTime;
}

void AnimatedImage::stop()
{
	mDirection = 0;
	mTimeLeft = 0.0f;
	mOwnStates &= ~STATE_ANIMATED;
	removeState(STATE_ANIMATED);
	mFinished.emit((unsigned int)mRun);
}

void AnimatedImage::advance()
{
	if (!mData || mData->getFrameCount() == 0)
	{
		stop();
		return;
	}
	// The original's vector checks the run (0x4f3054 (feedingfrenzy2), the runtime's invalid-parameter end)
	const AnimationRun& aRun = mData->getAnimation(mRun);
	int aFirst = (int)aRun.mStart;
	int aLast = (int)aRun.mEnd - 1;
	if (mDirection == 1 && mFrame == aLast)
	{
		if (mMode == ANIMATION_MODE_LOOP)
			mFrame = aFirst;
		else if (mMode == ANIMATION_MODE_SHUTTLE)
		{
			mDirection = -1;
			// Compared unsigned, as the original does
			mFrame = ((unsigned int)aFirst >= (unsigned int)(aLast - 1)) ? aFirst : aLast - 1;
		}
		else
		{
			stop();
			return;
		}
	}
	else if (mDirection == -1 && mFrame == aFirst)
	{
		if (mMode == ANIMATION_MODE_LOOP)
			mFrame = aLast;
		else if (mMode == ANIMATION_MODE_SHUTTLE)
		{
			mDirection = 1;
			mFrame = ((unsigned int)aLast >= (unsigned int)(aFirst + 1)) ? aFirst + 1 : aLast;
		}
		else
		{
			stop();
			return;
		}
	}
	else
		mFrame += mDirection;
	mTimeLeft = (float)((double)aRun.mFrameTime + (double)mTimeLeft);
}

} // namespace engine
