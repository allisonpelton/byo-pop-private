#include "GrowMeter.h"
#include "Font.h"
#include "Graphics.h"

using namespace engine;

namespace feedingfrenzy
{

// The rectangle a new bar fills, and its colour
const float START_RIGHT = 239.0f;
const float START_BOTTOM = 8.0f;
const float START_RED = 0.9f;
const float START_GREEN = 0.9f;
const float START_BLUE = 0.1f;

// 0x433ea0
GrowMeter::GrowMeter() : mValue(0.0f), mRed(START_RED), mGreen(START_GREEN), mBlue(START_BLUE)
{
	mRect.set(0.0f, 0.0f, START_RIGHT, START_BOTTOM);
	mFillHeight = mRect.bottom - mRect.top;
	mFillWidth = (mRect.right - mRect.left) * mValue;
	setFlags(NODE_DRAWABLE);
}

// 0x433e10
void GrowMeter::draw(Graphics* g)
{
	g->setColor(mRed, mGreen, mBlue);
	g->fillRect(Rect(0.0f, 0.0f, mFillWidth, mFillHeight));
	g->setColor(1.0f, 1.0f, 1.0f);
}

// 0x433dd0
void GrowMeter::layout()
{
	clearFlags(NODE_LAYOUT);
}

// 0x433ce0
void GrowMeter::setRect(const Rect& theRect)
{
	mRect = theRect;
	mFillWidth = 0.0f;
	mFillHeight = mRect.bottom - mRect.top;
}

// 0x433d20
void GrowMeter::setValue(float theValue)
{
	mValue = theValue;
	mFillWidth = (mRect.right - mRect.left) * theValue;
}

// 0x433de0
void GrowMeter::setBarColor(float theRed, float theGreen, float theBlue)
{
	mRed = theRed;
	mGreen = theGreen;
	mBlue = theBlue;
}

} // namespace feedingfrenzy
