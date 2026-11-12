#include "GnomeMultiplier.h"

#include "Application.h"
#include "Cubby.h"
#include "Floater.h"
#include "Gameplay.h"
#include "Res.h"
#include "Score.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43c130
GnomeMultiplier::GnomeMultiplier() :
	Creature(KIND_MULTIPLIER),
	mFrame(0.0),
	mFrameSpeed(0.1),
	mCenterX(0.0),
	mCenterY(0.0),
	mWaveTimer(0.0),
	mHasRisen(false),
	mStage(STAGE_WAVING)
{
}

// 0x43c1a0
GnomeMultiplier::~GnomeMultiplier()
{
}

// 0x43c350 (0x43c2a0; a stage outside 0 and 1 gives the compiler's garbage, and none is ever reached)
void GnomeMultiplier::GetSheetCell(double theCell[2]) const
{
	if (mStage == STAGE_WAVING)
	{
		theCell[0] = 0.0;
		theCell[1] = (double)((int)mFrame % 9);
		return;
	}
	if (mStage != STAGE_LEAVING)
	{
		theCell[0] = (double)(int)0x80000000;
		theCell[1] = (double)(int)0x80000000;
		return;
	}
	int aCol = (int)mFrame;
	if (4 < aCol)
		aCol = 4;
	theCell[0] = 1.0;
	theCell[1] = (double)aCol;
}

// 0x43c1b0
void GnomeMultiplier::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];
	mFrame = mFrameSpeed + mFrame;

	if (mStage == STAGE_WAVING)
	{
		double aTimer = mWaveTimer - 0.01;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mWaveTimer = aTimer;
		if (aTimer == 0.0 && !IsFrozen())
		{
			gApp->PlaySample(SOUND_FLAG_MOLE_WAVING);
			mWaveTimer = 1.2828;
		}
	}
	if (mStage == STAGE_LEAVING)
	{
		if (4 < (int)mFrame)
			mFinished = true;
	}
	if (mCubby != nullptr && mCubby->mState == Cubby::STATE_UP && !mHasRisen)
		mHasRisen = true;
}

// 0x43c310: he is not taken away at once; unless he is frozen he leaves with his second row of cels first
void GnomeMultiplier::Remove()
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

// 0x43c3b0
bool GnomeMultiplier::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit && !IsFrozen())
	{
		Rect aClip = mCubby->GetEffectRect();
		ScoreDisplayPos aTray = GetBonusTrayPos();
		MakeFloaterShrinking(mCenterX - 40.0, mCenterY - 144.0, aTray.mX, aTray.mY, IMAGE_MOLE_X5, 2, 0, mExposed, aClip.mX,
			aClip.mY, aClip.mWidth, aClip.mHeight);
		GetGameplay()->mFiveTimesDelay = 0.75;
		GetGameplay()->mFiveTimesTimer = 20.0;
		gApp->PlaySample(SOUND_FLAG_MOLE_PICKED_AND_MOVING_TO_UI);
		gApp->PlaySample(SOUND_HAMMER_HIT_HAT);
	}
	return aHit;
}

// 0x43c4d0
std::string GnomeMultiplier::SaveExtra()
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

// 0x43c5c0
void GnomeMultiplier::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mHasRisen = aList.NextBool();
}
