#include "BltFx.h"

#include "Geometry.h"

namespace engine
{

// 0x4d7e80 (feedingfrenzy2): a static's initializer, pi added to itself
static const float TWO_PI = PI + PI;

WaveFx::WaveFx() : mHeight(20), mWidth(5.0f), mPeriod(2.0f), mTime(0.0f), mOffset(0)
{
	mType = TYPE_WAVE;
	buildTable();
}

void WaveFx::update(float theElapsed)
{
	// The sum is compared before it is rounded to the float it is stored as
	double aTime = (double)theElapsed + mTime;
	mTime = (float)aTime;
	if (aTime > mPeriod)
	{
		do
			mTime = mTime - mPeriod;
		while (mPeriod < mTime);
	}
	mOffset = (int)((double)mHeight / mPeriod * mTime);
}

void WaveFx::setShape(float thePeriod, float theWidth, int theHeight)
{
	mPeriod = thePeriod;
	mWidth = theWidth;
	mHeight = theHeight;
	buildTable();
}

int WaveFx::getRowShift(int theRow) const
{
	return mShifts[(unsigned int)(mOffset + theRow) % (unsigned int)mHeight];
}

void WaveFx::buildTable()
{
	mShifts.clear();
	mShifts.resize(mHeight > 0 ? (size_t)mHeight : 0);
	for (int i = 0; i < mHeight; i++)
	{
		float anAngle = (float)((double)i / mHeight * TWO_PI);
		mShifts[i] = (int)(((double)sinOf(anAngle) + 1.0) * mWidth * 256.0);
	}
}

} // namespace engine
