#include "GnomeBubbleHead.h"

#include "Application.h"
#include "HammerDisplay.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x5919f8: the size the head has at each stage
static const int sStageSizes[5] = { 0, 5, 10, 15, 20 };

// 0x436f10
GnomeBubbleHead::GnomeBubbleHead() :
	Creature(KIND_BUBBLEHEAD),
	mSize(0.0),
	mTargetSize(0.0),
	mMode(0.0),
	mWait(0.0),
	mStage(0)
{
}

// 0x436f60
GnomeBubbleHead::~GnomeBubbleHead()
{
}

// 0x437250
void GnomeBubbleHead::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)(int)mSize;
}

// 0x4370e0
void GnomeBubbleHead::Update(const double thePos[2])
{
	Creature::Update(thePos);

	if (mMode == (double)MODE_INFLATING)
	{
		mSize = mSize + 0.2;
		if (mSize < mTargetSize)
			return;
	}
	else if (mMode == (double)MODE_DEFLATING)
	{
		mSize = mSize - 0.2;
		if (mSize > mTargetSize)
			return;
	}
	else
	{
		// at rest: after a short wait the head may shrink back a stage
		if (mMode == 0.0 && mWait != 0.0)
		{
			double aWait = mWait - 0.01;
			if (aWait < 0.0)
				aWait = 0.0;
			mWait = aWait;
			if (aWait == 0.0 && mStage > 0)
			{
				mMode = (double)MODE_DEFLATING;
				mStage = mStage - 1;
				gApp->PlaySample(SOUND_INFLATING_HEAD_DEFLATE);
				mTargetSize = (double)sStageSizes[mStage];
			}
		}
		return;
	}

	// it has reached its size
	mSize = mTargetSize;
	mMode = 0.0;
	mWait = 0.5;
}

// 0x436f70: with no hammer to answer, the head swells and shrinks again and again: it waits, swells to its full size, waits,
// shrinks away, waits, and so on
void GnomeBubbleHead::UpdateWithoutCubby(const double thePos[2])
{
	if (mMode == (double)MODE_STILL)
	{
		double aWait = mWait - 0.01;
		if (aWait < 0.0)
			aWait = 0.0;
		mWait = aWait;
		if (aWait == 0.0)
		{
			if (mSize == 0.0)
			{
				mMode = (double)MODE_INFLATING;
				gApp->PlaySample(SOUND_INFLATING_HEAD_INFLATE);
			}
			else
			{
				mMode = (double)MODE_DEFLATING;
				gApp->PlaySample(SOUND_INFLATING_HEAD_DEFLATE);
			}
		}
	}
	else if (mMode == (double)MODE_INFLATING)
	{
		double aSize = mSize + 0.25;
		if (aSize > 20.0)
			aSize = 20.0;
		mSize = aSize;
		if (aSize == 20.0)
		{
			mMode = (double)MODE_STILL;
			mWait = 1.0;
		}
	}
	else if (mMode == (double)MODE_DEFLATING)
	{
		double aSize = mSize - 0.15;
		if (aSize < 0.0)
			aSize = 0.0;
		mSize = aSize;
		if (aSize == 0.0)
		{
			mMode = (double)MODE_STILL;
			mWait = 1.0;
		}
	}
}

// 0x437280: the hit adds the hammer's damage to the stage, up to the head popping at 5
bool GnomeBubbleHead::Hit()
{
	gApp->PlaySample(SOUND_HAMMER_HIT_PLASTIC);

	if (GetHammerDamage() + mStage < 6)
		mStage = GetHammerDamage() + mStage;
	else
		mStage = 5;

	if (mStage == 5)
	{
		mHealth = 0;
		gApp->PlaySample(SOUND_INFLATING_HEAD_POP);
		return true;
	}

	mMode = (double)MODE_INFLATING;
	gApp->PlaySample(SOUND_INFLATING_HEAD_INFLATE);
	mTargetSize = (double)sStageSizes[mStage];
	return false;
}

// 0x437330: what is left before it pops
int GnomeBubbleHead::GetHealth()
{
	return 5 - mStage;
}

// 0x437360
std::string GnomeBubbleHead::SaveExtra()
{
	ValueList aList;
	aList.Add(mSize);
	aList.Add(mTargetSize);
	aList.Add(mMode);
	aList.Add(mWait);
	aList.AddInt(mStage);
	return aList.ToString();
}

// 0x437440
void GnomeBubbleHead::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mSize = aList.NextDouble();
	mTargetSize = aList.NextDouble();
	mMode = aList.NextDouble();
	mWait = aList.NextDouble();
	mStage = aList.NextInt();
}
