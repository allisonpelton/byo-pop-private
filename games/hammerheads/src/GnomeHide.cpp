#include "GnomeHide.h"

#include "Application.h"
#include "Hearts.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

static const double RAND_SCALE = 4.656612875245797e-10;		// 1 / 2^31: Rand() as a number from 0 to 1

// 0x43b200
GnomeHide::GnomeHide() :
	Creature(KIND_HIDE),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mStage(STAGE_BARE),
	mTimer(0.0),
	mHurtFlash(0.0)
{
	int aRand = Rand();
	mTimer = (((double)aRand * RAND_SCALE * 0.6 - 0.3) + 2.2) * 0.8;
}

// 0x43b2a0
GnomeHide::~GnomeHide()
{
}

// 0x43b5d0: three rows of cels a set, the second set after a hit that counted
void GnomeHide::GetSheetCell(double theCell[2]) const
{
	int aSet = (mHurtFlash == 0.0) ? 0 : 3;
	if (mStage == STAGE_PUTTING_ON)
	{
		theCell[0] = (double)(aSet + 1);
		theCell[1] = (double)(int)mFrame;
		return;
	}
	if (mStage == STAGE_TAKING_OFF)
	{
		theCell[0] = (double)(aSet + 2);
		theCell[1] = (double)(int)mFrame;
		return;
	}
	theCell[0] = (double)aSet;
	theCell[1] = 0.0;
}

// 0x43b2c0
void GnomeHide::Update(const double thePos[2])
{
	Creature::Update(thePos);

	double aTimer = mTimer - 0.01;
	if (aTimer < 0.0)
		aTimer = 0.0;
	mTimer = aTimer;
	double aFlash = mHurtFlash - 0.01;
	if (aFlash < 0.0)
		aFlash = 0.0;
	mHurtFlash = aFlash;

	if (aTimer == 0.0)
	{
		if (mStage == STAGE_PUTTING_ON)
		{
			int aRand = Rand();
			mStage = STAGE_TAKING_OFF;
			mTimer = ((double)aRand * RAND_SCALE * 0.6 - 0.3) + 2.2;
			mFrame = 0.0;
			mFrameSpeed = 0.18;
			gApp->PlaySample(SOUND_BUCKET_GNOME_REMOVING);
		}
		else if (mStage == STAGE_BARE)
		{
			int aRand = Rand();
			mStage = STAGE_PUTTING_ON;
			mTimer = ((double)aRand * RAND_SCALE * 0.3 - 0.15) + 0.8;
			mFrame = 0.0;
			mFrameSpeed = 0.18;
			gApp->PlaySample(SOUND_BUCKET_GNOME_PUTTING_ON);
		}
	}

	if (mStage == STAGE_PUTTING_ON)
	{
		double aFrame = mFrame;
		mFrame = aFrame + mFrameSpeed;
		if (10.0 <= aFrame + mFrameSpeed)
		{
			mFrameSpeed = 0.0;
			mFrame = 10.0;
		}
	}
	else if (mStage == STAGE_TAKING_OFF)
	{
		double aFrame = mFrame;
		mFrame = mFrameSpeed + aFrame;
		if (11.0 <= mFrameSpeed + aFrame)
		{
			mStage = STAGE_BARE;
			int aRand = Rand();
			mTimer = ((double)aRand * RAND_SCALE * 0.6 - 0.3) + 2.2;
		}
	}
}

// 0x43b4b0
bool GnomeHide::Hit()
{
	if (!IsFrozen())
	{
		// the bucket is off, or not yet far enough down, or already coming up: the hit counts
		if (mStage == STAGE_BARE || (mStage == STAGE_PUTTING_ON && mFrame < 7.0) || (mStage == STAGE_TAKING_OFF && 3.0 < mFrame))
		{
			int aDamage = GetHammerDamage();
			if (mHealth == aDamage || mHealth - aDamage < 0)
				mHealth = 0;
			else
				mHealth = mHealth - GetHammerDamage();

			if (mHealth != 0)
			{
				gApp->PlaySample(SOUND_HAMMER_HIT_WOOD);
				gApp->PlaySample(SOUND_VOICE_BUCKET_AU);
			}
			mHurtFlash = 1.0;
		}

		// the bucket is on: the hit bounces off it and costs the player health
		if (((mStage == STAGE_PUTTING_ON && 7.0 <= mFrame) || (mStage == STAGE_TAKING_OFF && mFrame <= 3.0)) && mVariant != 0)
		{
			Hearts* aHearts = GetHearts();
			if (aHearts != nullptr && aHearts->mActive)
				aHearts->ChangeHealth(HEALTH_ESCAPE, (double)mVariant);
		}
	}
	return mHealth == 0;
}

// 0x43b6a0
std::string GnomeHide::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.Add(mTimer);
	aList.Add(mHurtFlash);
	return aList.ToString();
}

// 0x43b780
void GnomeHide::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mTimer = aList.NextDouble();
	mHurtFlash = aList.NextDouble();
}
