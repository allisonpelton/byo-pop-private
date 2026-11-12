#include "GnomeCupido.h"

#include "Application.h"
#include "Cubby.h"
#include "Floater.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x438fe0
GnomeCupido::GnomeCupido() :
	Creature(KIND_CUPIDO),
	mFrame(0.0),
	mFrameSpeed(0.1),
	mCupidX(0.0),
	mCupidY(0.0),
	mStage(0),
	mTimer(0.0),
	mHasCalled(false)
{
}

// 0x439040
GnomeCupido::~GnomeCupido()
{
}

// 0x439180
void GnomeCupido::GetSheetCell(double theCell[2]) const
{
	theCell[0] = (double)(mStage != 0);
	theCell[1] = (double)((int)mFrame % 11);
}

// 0x439070
void GnomeCupido::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCupidX = thePos[0];
	mCupidY = thePos[1];
	mFrame = mFrameSpeed + mFrame;

	if (mStage == 1)
	{
		double aTimer = mTimer - 0.01;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mTimer = aTimer;
		if (aTimer == 0.0)
		{
			mStage = 2;
			mSpent = true;
		}
	}

	if (mCubby->mState == Cubby::STATE_UP && !mHasCalled)
	{
		mHasCalled = true;
		gApp->PlaySample(SOUND_VOICE_CUPID_LOVE);
	}
}

// 0x439140: he is not taken away at once; unless he is frozen he flies off for a second first
void GnomeCupido::Remove()
{
	bool aFrozen = IsFrozen();
	mUntouchable = true;
	if (aFrozen)
	{
		mFinished = true;
		return;
	}
	mStage = 1;
	mTimer = 1.0;
}

// 0x4393d0
bool GnomeCupido::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit && !IsFrozen())
	{
		ConvertToHearts(this);
		gApp->PlaySample(SOUND_CUPIDO_MAGIC);
		gApp->PlaySample(SOUND_VOICE_CUPID_AU);
		MakeFloater(FLOATER_SCORE_POP, mCupidX, mCupidY, "Love!", 0, "");
	}
	return aHit;
}

// 0x4391f0
std::string GnomeCupido::SaveExtra()
{
	ValueList aList;
	aList.Add(mCupidX);
	aList.Add(mCupidY);
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.Add(mTimer);
	aList.AddBool(mHasCalled);
	return aList.ToString();
}

// 0x4392f0
void GnomeCupido::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCupidX = aList.NextDouble();
	mCupidY = aList.NextDouble();
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mTimer = aList.NextDouble();
	mHasCalled = aList.NextBool();
}
