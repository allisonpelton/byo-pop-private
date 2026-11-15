#include "Modifiers.h"
#include <cmath>

using namespace engine;

namespace
{
const double PI = 3.1415927410125732;

// port: from theMin at the sine's bottom to theMax at its top: (sine + 1) over two, between the two
float swing(double theSine, float theMin, float theMax)
{
	return (float)((theSine + 1.0f) * (double)(theMax - theMin) * 0.5f + (double)theMin);
}
}

// 0x483d10 (pizzafrenzy)
ScaleModifier::ScaleModifier(float thePeriodX, float theMinX, float theMaxX, float thePeriodY, float theMinY,
                             float theMaxY, float theTime) :
	mPeriodX(thePeriodX),
	mPeriodY(thePeriodY),
	mMinX(theMinX),
	mMaxX(theMaxX),
	mMinY(theMinY),
	mMaxY(theMaxY),
	mTime(theTime)
{
}

// 0x483de0 (pizzafrenzy): the scale across and down, each from its own sine
void ScaleModifier::update(UpdateInfo& theInfo, Node* theNode)
{
	float aTime = (float)((double)theInfo.mSeconds + (double)mTime);
	mTime = aTime;
	float aScaleY = swing(sin((double)aTime / (double)mPeriodY * PI * 2.0), mMinY, mMaxY);
	float aScaleX = swing(sin((double)aTime / (double)mPeriodX * PI * 2.0), mMinX, mMaxX);
	theNode->setScale(aScaleX, aScaleY);
}

// 0x481a90 (pizzafrenzy)
OpacityModifier::OpacityModifier(float thePeriod, float theMin, float theMax, float theTime) :
	mPeriod(thePeriod),
	mMin(theMin),
	mMax(theMax),
	mTime(theTime)
{
}

// 0x481b50 (pizzafrenzy)
void OpacityModifier::update(UpdateInfo& theInfo, Node* theNode)
{
	float aTime = (float)((double)theInfo.mSeconds + (double)mTime);
	mTime = aTime;
	theNode->setOpacity(swing(sin((double)aTime / (double)mPeriod * PI * 2.0), mMin, mMax));
}

// 0x483ac0 (pizzafrenzy)
ColorFlashModifier::ColorFlashModifier(float thePeriod, const Color4& theColor, float theTime, int theColorMode) :
	mPeriod(thePeriod),
	mColor(theColor),
	mColorMode(theColorMode),
	mTime(theTime)
{
}

// 0x483bc0 (pizzafrenzy): the share of the colour (the sine over its range, from nothing to all) is the node's colour;
// the original gives the green its share of the stored blue and the blue its share of the stored green
void ColorFlashModifier::update(UpdateInfo& theInfo, Node* theNode)
{
	float aTime = (float)((double)theInfo.mSeconds + (double)mTime);
	mTime = aTime;
	float aPhase = (float)((double)aTime / (double)mPeriod * PI * 2.0);
	float aShare = (float)((sin((double)aPhase) + 1.0f) * 0.5f);
	Color4 aColor;
	aColor.r = aShare * mColor.r;
	aColor.g = aShare * mColor.b;
	aColor.b = aShare * mColor.g;
	theNode->setColor(aColor);
	theNode->setColorMode(mColorMode);
}
