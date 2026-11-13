#include "GnomeSwitcher.h"

#include "Application.h"
#include "HammerDisplay.h"
#include "Coins.h"
#include "Cubby.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Hearts.h"
#include "Popper.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

static const double RAND_SCALE = 4.656612875245797e-10;		// 1 / 2^31: Rand() as a number from 0 to 1
static const double NO_TIMEOUT = 99999.0;					// the timer while a costume goes on or off: it is not run out

// 0x43eb20
GnomeSwitcher::GnomeSwitcher() :
	Creature(KIND_SWITCHER),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mStage(STAGE_BARE),
	mTimer(0.0),
	mHurtFlash(0.0),
	mFlicker(0),
	mCenterX(0.0),
	mCenterY(0.0),
	mFuseSound(nullptr),
	mFuseTimer(0.0)
{
	mShielded = true;
	mFuseSound = gApp->mSoundManager->GetSoundInstance(SOUND_BOMB_FACE_FUSE);
}

// 0x43ebf0
GnomeSwitcher::~GnomeSwitcher()
{
	if (mFuseSound != nullptr)
		mFuseSound->Release();
}

// 0x43ef90
void GnomeSwitcher::GetSheetCell(double theCell[2]) const
{
	int aSet = (mHurtFlash == 0.0) ? 0 : 2;
	double aRow = 0.0;
	switch (mStage)
	{
	case STAGE_BARE:
		aRow = (double)aSet;
		break;
	case STAGE_COINS_ON:
	{
		int aCol = (int)mFrame;
		if (aCol < 10)
		{
			theCell[0] = (double)aSet;
			theCell[1] = (double)aCol;
			return;
		}
		theCell[0] = (double)(aSet + 1);
		theCell[1] = 0.0;
		return;
	}
	case STAGE_COINS_OFF:
		theCell[0] = (double)(aSet + 1);
		theCell[1] = (double)(int)mFrame;
		return;
	case STAGE_BOMB_ON:
	{
		int aCol = (int)mFrame;
		if (aCol < 10)
		{
			theCell[0] = (double)(aSet + 4);
			theCell[1] = (double)aCol;
			return;
		}
		theCell[0] = 8.0;
		theCell[1] = (double)mFlicker;
		return;
	}
	case STAGE_BOMB_OFF:
		theCell[0] = (double)(aSet + 5);
		theCell[1] = (double)(int)mFrame;
		return;
	}
	theCell[0] = aRow;
	theCell[1] = 0.0;
}

// 0x43ec50 (it draws a random number every step, for the flicker)
void GnomeSwitcher::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1] - mHeight;

	double aTimer = mTimer - 0.01;
	if (aTimer < 0.0)
		aTimer = 0.0;
	mTimer = aTimer;
	double aFlash = mHurtFlash - 0.01;
	if (aFlash < 0.0)
		aFlash = 0.0;
	mHurtFlash = aFlash;

	// the fuse plays again every time its timer is out
	if (0.0 < mFuseTimer)
	{
		double aFuse = mFuseTimer - 0.01;
		if (aFuse < 0.0)
			aFuse = 0.0;
		mFuseTimer = aFuse;
		if (aFuse == 0.0 && !IsFrozen())
		{
			mFuseSound->Play(false, false);
			mFuseTimer = 1.0;
		}
	}

	if (mTimer == 0.0)
	{
		bool aTakeOff = false;
		if (mStage == STAGE_COINS_ON)
		{
			mStage = STAGE_COINS_OFF;
			aTakeOff = true;
		}
		else if (mStage == STAGE_BOMB_ON)
		{
			mStage = STAGE_BOMB_OFF;
			aTakeOff = true;
		}
		else if (mStage == STAGE_BARE)
		{
			mTimer = NO_TIMEOUT;
			int aRand = Rand();
			mFrame = 0.0;
			mFrameSpeed = 0.18;
			mStage = (aRand % 2 != 0) ? STAGE_BOMB_ON : STAGE_COINS_ON;
			gApp->PlaySample(SOUND_BUCKET_GNOME_PUTTING_ON);
			if (mStage == STAGE_BOMB_ON && mFuseSound != nullptr)
			{
				mFuseSound->Play(false, false);
				mFuseTimer = 1.0;
			}
		}
		if (aTakeOff)
		{
			mTimer = NO_TIMEOUT;
			mFrame = 0.0;
			mFrameSpeed = 0.18;
			gApp->PlaySample(SOUND_BUCKET_GNOME_REMOVING);
		}
	}

	int aStage = mStage;
	if (aStage == STAGE_COINS_ON || aStage == STAGE_BOMB_ON)
	{
		double aFrame = mFrame;
		mFrame = aFrame + mFrameSpeed;
		if (10.0 < aFrame + mFrameSpeed)
		{
			mFrameSpeed = 0.0;
			mFrame = 10.0;
			int aRand = Rand();
			if (aStage == STAGE_BOMB_ON)
				mTimer = ((double)aRand * RAND_SCALE - 0.5) + 1.2;
			else
				mTimer = ((double)aRand * RAND_SCALE - 0.5) + 0.75;
		}
	}
	else if (aStage == STAGE_COINS_OFF || aStage == STAGE_BOMB_OFF)
	{
		double aFrame = mFrame;
		mFrame = mFrameSpeed + aFrame;
		if (9.0 <= mFrameSpeed + aFrame)
		{
			mStage = STAGE_BARE;
			int aRand = Rand();
			mTimer = ((double)aRand * RAND_SCALE * 0.25 - 0.125) + 0.25;
			if (mFuseSound != nullptr)
			{
				mFuseSound->Stop();
				mFuseTimer = 0.0;
			}
		}
	}
	mFlicker = Rand() % 4;
}

// 0x43b2b0 (one function for six gnomes): the same as Update
void GnomeSwitcher::UpdateWithoutCubby(const double thePos[2])
{
	Update(thePos);
}

// 0x43f110: a coin leaves him for the counter; the later ones are delayed by a fraction of the flight's time
void GnomeSwitcher::ThrowCoin(double theDelay)
{
	Rect aClip = mCubby->GetEffectRect();
	double aCounter[2];
	GetCoinsPos(aCounter);
	GetStats()->SetInt(STAT_LEVEL_OF_LAST_COIN, gApp->mGameScreen->mLevel);
	GetStats()->AddInt(STAT_COINS_COLLECTED, 1);
	GetCoins()->AddCoins(1);
	FloaterCoin* aCoin = MakeFloaterCoin(mCenterX - 40.0, mCenterY - 150.0, aCounter[0], aCounter[1], mFrame, mExposed,
		aClip.mX, aClip.mY, aClip.mWidth, aClip.mHeight);
	aCoin->mTimeLeft = 0.75 * theDelay;
}

// 0x43f210 (a frozen switcher takes no hit at all)
bool GnomeSwitcher::Hit()
{
	bool aDestroyed = false;
	if (!IsFrozen())
	{
		if (mStage == STAGE_BARE || (mStage == STAGE_BOMB_ON && mFrame < 10.0) || mStage == STAGE_COINS_ON ||
			mStage == STAGE_COINS_OFF || mStage == STAGE_BOMB_OFF)
		{
			int aDamage = GetHammerDamage();
			if (mHealth == aDamage || mHealth - aDamage < 0)
				mHealth = 0;
			else
				mHealth = mHealth - GetHammerDamage();
			aDestroyed = mHealth == 0;
			gApp->PlaySample(SOUND_HAMMER_HIT_WOOD);
			gApp->PlaySample(SOUND_VOICE_SWITCHER_AI);
			mHurtFlash = 1.0;
		}

		// the bomb costume, nearly on: the bomb goes off
		if (mStage == STAGE_BOMB_ON && 9.0 < mFrame)
		{
			GetStats()->AddInt(STAT_BOMBS_HIT, 1);
			GetStats()->SetInt(STAT_LEVEL_OF_LAST_BOMB, gApp->mGameScreen->mLevel);
			Hearts* aHearts = GetHearts();
			if (mVariant != 0 && aHearts != nullptr && aHearts->mActive)
				aHearts->ChangeHealth(HEALTH_ESCAPE, (double)mVariant);

			double aDirection[2] = { 0.0, 1.0 };
			MakePopper(mCenterX, mCenterY - (mAnchorY - mHeadY) * 0.5, 6, aDirection);
			mShatters = true;
			mHealth = 0;
			aDestroyed = true;
		}

		// the coin costume, nearly on: three coins come out of it
		if (mStage == STAGE_COINS_ON && 7.0 < mFrame)
		{
			ThrowCoin(1.0);
			ThrowCoin(0.9);
			ThrowCoin(0.8);
			gApp->PlaySample(SOUND_COIN_THREE_PICKED);
		}
	}
	return aDestroyed;
}

// 0x43f410
std::string GnomeSwitcher::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.Add(mTimer);
	aList.Add(mHurtFlash);
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	return aList.ToString();
}

// 0x43f510
void GnomeSwitcher::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mTimer = aList.NextDouble();
	mHurtFlash = aList.NextDouble();
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
}
