#include "GnomeRabbit.h"

#include "Application.h"
#include "Cubby.h"
#include "Hearts.h"
#include "Popper.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43cb10
GnomeRabbit::GnomeRabbit() :
	Creature(KIND_RABBIT),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mCenterX(0.0),
	mCenterY(0.0),
	mHasRisen(false),
	mStage(STAGE_HOPPING)
{
	int aRand = Rand();
	mFrameSpeed = ((double)aRand * 4.656612875245797e-10 * 0.05 - 0.025) + 0.1;
}

// 0x43cbc0
GnomeRabbit::~GnomeRabbit()
{
}

// 0x43cde0 (0x43cc80; a stage outside 0 and 1 gives the compiler's garbage, and none is ever reached)
void GnomeRabbit::GetSheetCell(double theCell[2]) const
{
	if (mStage == STAGE_HOPPING)
	{
		theCell[0] = 1.0;
		theCell[1] = (double)((int)mFrame % 10);
		return;
	}
	if (mStage != STAGE_LEAVING)
	{
		theCell[0] = (double)(int)0x80000000;
		theCell[1] = (double)(int)0x80000000;
		return;
	}
	int aFrame = (int)mFrame;
	theCell[0] = 0.0;
	theCell[1] = (4 < aFrame) ? 0.0 : (double)(4 - aFrame);
}

// 0x43cc00
void GnomeRabbit::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];
	mFrame = mFrameSpeed + mFrame;

	if (mStage == STAGE_LEAVING && 4 < (int)mFrame)
		mFinished = true;
	if (!mHasRisen && mCubby->mState == Cubby::STATE_UP)
		mHasRisen = true;
}

// 0x43cda0: it is not taken away at once; unless it is frozen it leaves with its cels played backwards first
void GnomeRabbit::Remove()
{
	bool aFrozen = IsFrozen();
	mUntouchable = true;
	if (aFrozen)
	{
		mFinished = true;
		return;
	}
	mStage = STAGE_LEAVING;
	mFrameSpeed = 0.15;
	mFrame = 0.0;
}

// 0x43cd00: every hit makes the hat sound, the one that takes it down gives hearts
bool GnomeRabbit::Hit()
{
	bool aHit = Creature::Hit();
	gApp->PlaySample(SOUND_HAMMER_HIT_HAT);
	if (aHit)
	{
		gApp->PlaySample(SOUND_COIN_SINGLE_PICKED);
		double aDirection[2] = { 0.0, -1.0 };
		MakePopper(mX, mY - 50.0, 5, aDirection);
		GetHearts()->ChangeHealth(HEALTH_GAIN, 5.0);
	}
	return aHit;
}

// 0x43ce40
std::string GnomeRabbit::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.AddBool(mHasRisen);
	return aList.ToString();
}

// 0x43cf30
void GnomeRabbit::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mHasRisen = aList.NextBool();
}
