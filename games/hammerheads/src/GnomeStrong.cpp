#include "GnomeStrong.h"

#include "Application.h"
#include "Coins.h"
#include "Cubby.h"
#include "Floater.h"
#include "HammerDisplay.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43e410
GnomeStrong::GnomeStrong() :
	Creature(KIND_STRONG),
	mFrame(0.0),
	mFrameSpeed(0.18),
	mStage(STAGE_IDLE),
	mRepeats(0),
	mCenterX(0.0),
	mCenterY(0.0),
	mCoinsToTake(0),
	mCoinTimer(0)
{
	Rand();		// a number is drawn here and thrown away; the repeat count is always 45 to start with
	mRepeats = 0x2d;
}

// 0x43e2d0
GnomeStrong::~GnomeStrong()
{
}

// 0x43e330
void GnomeStrong::GetSheetCell(double theCell[2]) const
{
	switch (mStage)
	{
	default:
		theCell[0] = 0.0;
		theCell[1] = 0.0;
		return;
	case STAGE_FLEX:
		theCell[0] = 1.0;
		theCell[1] = (double)(int)mFrame;
		return;
	case STAGE_REST:
		theCell[0] = 2.0;
		theCell[1] = (double)(int)mFrame;
		return;
	case STAGE_FLEX_AGAIN:
		theCell[0] = 3.0;
		theCell[1] = (double)(int)mFrame;
		return;
	case STAGE_HIT:
		theCell[0] = 3.0;
		theCell[1] = (double)(int)mFrame;
		return;
	}
}

// 0x43e4d0
void GnomeStrong::Update(const double thePos[2])
{
	Creature::Update(thePos);

	mCenterX = thePos[0];
	mCenterY = thePos[1] - mHeight;
	mFrame = mFrameSpeed + mFrame;

	// the coins it has started to take leave the counter one after the other, five steps apart
	if (mCoinsToTake != 0)
	{
		int aTimer = mCoinTimer - 1;
		if (aTimer < 0)
			aTimer = 0;
		mCoinTimer = aTimer;
		if (aTimer == 0)
		{
			mCoinTimer = 5;
			if (GetStats()->GetInt(STAT_COIN_BALANCE) < 1)
				mCoinsToTake = 0;
			else
			{
				mCoinsToTake = mCoinsToTake - 1;
				GetStats()->AddInt(STAT_COIN_BALANCE, -1);

				double aCounter[2];
				GetCoinsPos(aCounter);
				Rect aClip = mCubby->GetEffectRect();
				FloaterCoin* aCoin = MakeFloaterCoin(aCounter[0], aCounter[1], (mCenterX - 50.0) - 18.0,
					(mCenterY - 155.0) + 25.0, mFrame, mExposed, aClip.mX, aClip.mY, aClip.mWidth, aClip.mHeight);
				aCoin->SetDuration(0.35);
				aCoin->SetKeepSize(true);
			}
		}
	}

	switch (mStage)
	{
	case STAGE_IDLE:
		if (mFrame < 1.0)
			return;
		mRepeats = mRepeats - 1;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 1.0;
			return;
		}
		if (GetStats()->GetInt(STAT_COIN_BALANCE) < 1)
			return;
		if (mCubby->mState != Cubby::STATE_UP)
			return;
		mFrame = 0.0;
		mStage = STAGE_FLEX;
		mCoinsToTake = GetStats()->GetInt(STAT_COIN_BALANCE) < 5 ? GetStats()->GetInt(STAT_COIN_BALANCE) : 5;
		mCoinTimer = 0;
		gApp->PlaySample(SOUND_COIN_THREE_PICKED);
		return;
	case STAGE_FLEX:
		if (mFrame < 14.0)
			return;
		mStage = STAGE_REST;
		mFrame = 0.0;
		mRepeats = 5;
		gApp->PlaySample(SOUND_VOICE_STRONG_HEHEHEHE);
		return;
	case STAGE_REST:
		if (mFrame < 2.0)
			return;
		mRepeats = mRepeats - 1;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 2.0;
			return;
		}
		break;
	case STAGE_FLEX_AGAIN:
		if (mFrame < 15.0)
			return;
		mRepeats = mRepeats - 1;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 15.0;
			return;
		}
		break;
	case STAGE_HIT:
		if (mFrame < 7.0)
			return;
		mRepeats = mRepeats - 1;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 7.0;
			return;
		}
		break;
	default:
		return;
	}

	// back to waiting, for a few more turns of the frame count
	mStage = STAGE_IDLE;
	mFrame = 0.0;
	int aRandom = Rand();
	mRepeats = aRandom % 11 + 5;
}

// 0x43e8a0: it stands up to the hit and shows it (the hammer's damage comes off its health as for any gnome)
bool GnomeStrong::Hit()
{
	mFrame = 0.0;
	mStage = STAGE_HIT;
	mRepeats = 3;
	gApp->PlaySample(SOUND_HAMMER_HIT_WOOD2);

	int aDamage = GetHammerDamage();
	if (mHealth == aDamage || mHealth - aDamage < 0)
		mHealth = 0;
	else
		mHealth = mHealth - GetHammerDamage();

	if (mHealth != 0)
		gApp->PlaySample(SOUND_VOICE_STRONG_AU);
	return mHealth == 0;
}

// 0x43e920
std::string GnomeStrong::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.AddInt(mRepeats);
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.AddInt(mCoinsToTake);
	aList.AddInt(mCoinTimer);
	return aList.ToString();
}

// 0x43ea30
void GnomeStrong::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mRepeats = aList.NextInt();
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mCoinsToTake = aList.NextInt();
	mCoinTimer = aList.NextInt();
}
