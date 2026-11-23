#include "Modifiers.h"

namespace engine
{

WaveModifier::WaveModifier(float theXPeriod, float theYPeriod, float theXAmplitude, float theYAmplitude,
	float thePhase)
	: mXPeriod(theXPeriod), mYPeriod(theYPeriod), mXAmplitude(theXAmplitude), mYAmplitude(theYAmplitude),
	  mTime(thePhase), mXOffset(0.0f), mYOffset(0.0f)
{
	if (mXPeriod == 0.0f)
		mXPeriod = 1.0f;
	if (mYPeriod == 0.0f)
		mYPeriod = 1.0f;
}

void WaveModifier::modify(UpdateContext& theContext, Component* theComponent)
{
	// The x87 keeps the sum unrounded for the across wave; the up-and-down one reads it back as the float stored
	double aTime = (double)theContext.mElapsed + mTime;
	mTime = (float)aTime;
	float anXOffset = (float)((double)sinOf((float)(aTime / mXPeriod * PI * 2.0)) * mXAmplitude);
	float aYOffset = (float)((double)sinOf((float)((double)mTime / mYPeriod * PI * 2.0)) * mYAmplitude);
	Vector2f aPosition = theComponent->getPosition();
	aPosition.x = (float)(((double)anXOffset - mXOffset) + aPosition.x);
	aPosition.y = (float)(((double)aYOffset - mYOffset) + aPosition.y);
	mXOffset = anXOffset;
	mYOffset = aYOffset;
	theComponent->setPosition(aPosition);
}

void OpacityModifier::modify(UpdateContext& theContext, Component* theComponent)
{
	// The x87 keeps the sum unrounded for the angle
	double aTime = (double)theContext.mElapsed + mTime;
	mTime = (float)aTime;
	float aSine = sinOf((float)(aTime / mPeriod * PI * 2.0));
	theComponent->setOpacity((float)(((double)aSine + 1.0f) * ((double)mHigh - mLow) * 0.5f + mLow));
}

} // namespace engine
