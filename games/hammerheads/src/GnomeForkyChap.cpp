#include "GnomeForkyChap.h"

#include "Application.h"
#include "HammerDisplay.h"
#include "Hearts.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43aa90: the speed is drawn first, then the first turn count (one or two turns of the idle loop)
GnomeForkyChap::GnomeForkyChap() :
	Creature(KIND_FORKY_CHAP),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mStage(STAGE_IDLE),
	mRepeats(0),
	mCenterX(0.0),
	mCenterY(0.0)
{
	int aSpeedRand = Rand();
	mFrameSpeed = ((double)aSpeedRand * 4.656612875245797e-10 * 0.06 - 0.03) + 0.1;
	mRepeats = Rand() % 2 + 1;
	gApp->PlaySample(SOUND_VOICE_STABBING_NYA);
}

// 0x43a450
GnomeForkyChap::~GnomeForkyChap()
{
}

// 0x43aa50 (0x43a960 picks the row from the stage and the cel from the frame; a stage outside 0 to 5 gives the garbage the
// compiler left in its switch, and none is ever reached)
void GnomeForkyChap::GetSheetCell(double theCell[2]) const
{
	if (mStage >= STAGE_IDLE && mStage <= STAGE_HURT)
	{
		theCell[0] = (double)mStage;
		theCell[1] = (double)(int)mFrame;
	}
	else
	{
		theCell[0] = (double)(int)0x80000000;
		theCell[1] = (double)(int)0x80000000;
	}
}

// 0x43a4b0
void GnomeForkyChap::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];

	mFrame = mFrameSpeed + mFrame;
	switch (mStage)
	{
	case STAGE_IDLE:
		if (4.0 <= mFrame)
		{
			mRepeats--;
			if (mRepeats < 1)
			{
				mFrame = 0.0;
				mStage = STAGE_RAISE;
				mRepeats = 1;
				return;
			}
			mFrame = mFrame - 4.0;
		}
		break;
	case STAGE_RAISE:
		if (3.0 <= mFrame)
		{
			mRepeats--;
			if (mRepeats > 0)
			{
				mFrame = mFrame - 3.0;
				return;
			}
			mStage = STAGE_STAB;
			mFrame = 0.0;
			mRepeats = Rand() % 7 + 6;
			gApp->PlaySample(SOUND_VOICE_STABBING_HEHE);
		}
		break;
	case STAGE_STAB:
		if (mFrame < 7.0)
			return;
		mRepeats--;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 7.0;
			return;
		}
		mStage = STAGE_LOWER;
		mRepeats = 1;
		mFrame = 0.0;
		break;
	case STAGE_RECOVER:
		if (5.0 <= mFrame)
		{
			mRepeats--;
			if (mRepeats > 0)
			{
				mFrame = mFrame - 5.0;
				return;
			}
			BackToIdle();
		}
		break;
	case STAGE_LOWER:
		if (4.0 <= mFrame)
		{
			mRepeats--;
			if (mRepeats < 1)
			{
				BackToIdle();
				return;
			}
			mFrame = mFrame - 4.0;
		}
		break;
	case STAGE_HURT:
		if (mFrame < 4.0)
			return;
		mRepeats--;
		if (mRepeats > 0)
		{
			mFrame = mFrame - 4.0;
			return;
		}
		mStage = STAGE_RECOVER;
		mRepeats = 1;
		mFrame = 0.0;
		break;
	}
}

// 0x43a460: the cels of the waiting row go round and round
void GnomeForkyChap::UpdateWithoutCubby(const double thePos[2])
{
	Creature::Update(thePos);

	mFrame = mFrameSpeed + mFrame;
	if (mStage == STAGE_IDLE && 4.0 <= mFrame)
		mFrame = mFrame - 4.0;
}

// the end of the recover and lower stages (the same code at 0x43a66e): idle again, one or two turns, with his call
void GnomeForkyChap::BackToIdle()
{
	mStage = STAGE_IDLE;
	mFrame = 0.0;
	mRepeats = Rand() % 2 + 1;
	gApp->PlaySample(SOUND_VOICE_STABBING_NYA);
}

// 0x43a760
bool GnomeForkyChap::Hit()
{
	if (IsFrozen())
		return mHealth == 0;

	// a hit while he is down or on his way back costs the player health, and sends him back to idle (or down)
	if (mStage == STAGE_IDLE || (mStage == STAGE_RAISE && mFrame < 1.0) || (mStage == STAGE_LOWER && 1.0 < mFrame) ||
		mStage == STAGE_HURT || mStage == STAGE_RECOVER)
	{
		Hearts* aHearts = GetHearts();
		if (mVariant != 0 && aHearts != nullptr && aHearts->mActive)
			aHearts->ChangeHealth(HEALTH_ESCAPE, (double)mVariant);

		if (mStage == STAGE_RAISE)
		{
			mFrame = (4.0 - mFrame) - 1.0;
			mStage = STAGE_LOWER;
			mRepeats = 1;
		}
		else if (mStage == STAGE_HURT)
		{
			mFrame = 0.0;
			mStage = STAGE_LOWER;
			mRepeats = 1;
		}
		else if (mStage == STAGE_RECOVER)
		{
			mStage = STAGE_IDLE;
			mFrame = 0.0;
			mRepeats = Rand() % 2 + 1;
			gApp->PlaySample(SOUND_VOICE_STABBING_NYA);
		}
		if (mStage == STAGE_IDLE)
			mRepeats = mRepeats + Rand() % 5 + 4;
	}

	// a hit while the fork is out (the second half of the raise, or the stab) is the one that counts
	if ((mStage == STAGE_RAISE && 1.0 <= mFrame) || mStage == STAGE_STAB)
	{
		int aDamage = GetHammerDamage();
		if (mHealth == aDamage || mHealth - aDamage < 0)
			mHealth = 0;
		else
			mHealth = mHealth - GetHammerDamage();

		mFrame = (mStage == STAGE_RAISE) ? (4.0 - mFrame) - 1.0 : 0.0;
		mStage = STAGE_HURT;
		mRepeats = 1;
		gApp->PlaySample(SOUND_HAMMER_HIT_WOOD);
		gApp->PlaySample(SOUND_VOICE_STABBING_AU);
	}
	return mHealth == 0;
}

// 0x43aba0
std::string GnomeForkyChap::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.AddInt(mRepeats);
	return aList.ToString();
}

// 0x43ac90
void GnomeForkyChap::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mRepeats = aList.NextInt();
}
