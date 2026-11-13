// BossesHead.cpp: the head boss (images\head): a giant head that floats across the top of the field, firing homing
// missiles from two launchers, and now and then charges across the whole field dropping iron bombs.
// Original: 0x43e260 (destructor body), 0x43e2c0 Update, 0x43e6e0 Draw, 0x43e710 Killed, 0x43e7d0 its data,
// 0x43f310 constructor.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Sprite.h"
#include <math.h>

using namespace Sexy;

namespace
{
	const double START_X = -440.0;				// it starts off the left edge
	const double HOVER_Y = 160.0;				// its height while it floats
	const double CHARGE_Y = 220.0;				// and while it charges
	const double DEATH_Y = 200.0;				// it sinks to the middle of the field when it dies
	const double EASE_FACTOR = 0.02;			// the part of the way to its target it moves an update
	const double ARRIVAL_SPEED = 0.1;			// it may start a charge once it has slowed to this
	const double HOVER_TARGET_X = 220.0;		// it floats to this far from the middle, to the way it faces
	const double CHARGE_TARGET_X = 440.0;		// a charge aims this far off, the other way
	const double CHARGE_TARGET_STEP = 3.0;		// and the target moves on this much an update
	const double CHARGE_END_X = 520.0;			// the charge is over this far from the middle
	const int CHARGE_PAUSE = 500;				// updates it floats between charges
	const double FIRE_RANGE_X = 300.0;			// it fires only this near the middle
	const int LAUNCHER_COUNT = 2;
	const double LAUNCHER_Y = -40.0;
	const int LAUNCHER_OFFSET_X[LAUNCHER_COUNT] = { 76, 104 };
	const int LAUNCHER_MIRROR_X[LAUNCHER_COUNT] = { 0x98, 0xd0 };
	const double BOMB_Y = 135.0;
	const int BOMB_SPREAD = 40;
}

// 0x43f310: it loads its armour, points, speed, launchers and bombs from the data, and its sprite
HeadBoss::HeadBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Head", aBlock);						// 0x43e7d0
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mMaxSpeed = aData.Double("", "speed");
	mChargeDelay = aData.Int("Charge", "delay");
	mLauncherFire[0] = aData.Int("Launcher1", "fire");
	mLauncherFire[1] = aData.Int("Launcher2", "fire");
	mBombOn = aData.Int("Bomb", "on");
	mBombOff = aData.Int("Bomb", "off");
	mBombFreq = aData.Int("Bomb", "freq");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\head\\head", 1, 1);
	mLauncherDelay[1] = mLauncherFire[1];
	mMirrored = false;
	mY = HOVER_Y;
	mTargetY = HOVER_Y;
	mLauncherDelay[0] = mLauncherFire[0];
	mX = START_X;
	mVelX = 0.0;
	mVelY = 0.0;
	mFrame = 0.0;
	mTargetX = 0.0;
	mIsCharging = false;
	mChargeWait = CHARGE_PAUSE;
}

// 0x43e260 (deleting 0x43e7a0)
HeadBoss::~HeadBoss()
{
	delete mSprite;
}

// 0x43e2c0: once the level has run out it eases toward its target, floats to a side of the field, and after a pause
// charges across it to the other side, then turns; it fires its launchers while it floats near the middle and drops
// iron bombs while it charges.
void HeadBoss::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	mX = mX + mVelX;
	mY = mY + mVelY;
	mVelX = (mTargetX - mX) * EASE_FACTOR;
	mVelY = (mTargetY - mY) * EASE_FACTOR;
	double aSpeed = sqrt(pow(mVelX, 2.0) + pow(mVelY, 2.0));
	if (aSpeed > mMaxSpeed)
	{
		double aScale = mMaxSpeed / aSpeed;
		mVelX = mVelX * aScale;
		mVelY = mVelY * aScale;
	}

	if (mBoard->mBossDeath != NULL)
	{
		mTargetX = 0.0;
		mTargetY = DEATH_Y;
		return;
	}

	bool aWasMirrored = mMirrored;
	if (!mIsCharging)
	{
		mTargetY = HOVER_Y;
		mTargetX = aWasMirrored ? HOVER_TARGET_X : -HOVER_TARGET_X;
		if (aSpeed < ARRIVAL_SPEED)
		{
			if (mChargeWait == 0)
			{
				mIsCharging = true;
				mTargetY = CHARGE_Y;
				mBombCounter = mBombOff + mBombOn;
				mTargetX = aWasMirrored ? CHARGE_TARGET_X : -CHARGE_TARGET_X;
			}
			else
				mChargeWait--;
		}
	}
	else
	{
		if (!aWasMirrored)
			mTargetX = mTargetX + CHARGE_TARGET_STEP;
		else
			mTargetX = mTargetX - CHARGE_TARGET_STEP;
		if (mX < -CHARGE_END_X || CHARGE_END_X < mX)
		{
			mIsCharging = false;
			mMirrored = !aWasMirrored;
			mChargeWait = CHARGE_PAUSE;
		}
	}

	if (mX > -FIRE_RANGE_X && mX < FIRE_RANGE_X && mBoard->mNukeFlash == 0.0 && !mIsCharging)
	{
		for (int i = 0; i < LAUNCHER_COUNT; i++)
		{
			if (mBoard->mTankDeadTime == 0)
				mLauncherDelay[i]--;
			else
				mLauncherDelay[i] = mLauncherFire[i];
			if (mLauncherDelay[i] < 1)
			{
				double aLaunchX = (double)((mMirrored ? 1 : 0) * -LAUNCHER_MIRROR_X[i]) + mX + (double)LAUNCHER_OFFSET_X[i];
				new HeadMissile(mBoard, aLaunchX, mY + LAUNCHER_Y, mMirrored);
				mLauncherDelay[i] = mLauncherFire[i];
			}
		}
	}

	if (!mIsCharging)
		return;

	// while it charges it drops iron bombs, for the "on" updates of each "on" plus "off"
	if (mBoard->mNukeFlash == 0.0)
	{
		if (mBombCounter == 0)
			mBombCounter = mBombOff + mBombOn;
		else
			mBombCounter--;
		if (mBombCounter < mBombOn && mApp->mUpdateCount % mBombFreq == 0)
			new IronBomb(mBoard, (double)(Rand() % BOMB_SPREAD) + mX - (BOMB_SPREAD / 2), mY + BOMB_Y, 0.0, 0.0, mMirrored);
	}
}

// 0x43e6e0: only once the level has run out
void HeadBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
		Enemy::Draw(g);
}

// 0x43e710: the points, and its death
void HeadBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
	}
}
