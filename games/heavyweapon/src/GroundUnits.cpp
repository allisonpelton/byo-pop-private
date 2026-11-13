// GroundUnits.cpp: the enemy units that drive along the ground of Craft.h: the truck, the enemy tank and the dozer.
// Original: 0x433520 (Dozer), 0x433960 (EnemyTank), 0x435650 (Truck), with the Update bodies at 0x433660, 0x436410 and
// 0x435790. The aircraft are in Aircraft.cpp.

#include "Craft.h"
#include "Board.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"

#include <math.h>
#include <stdlib.h>

using namespace Sexy;

namespace
{
	const double GROUND_UNIT_SPEED = 1.0;			// the sideways speed a ground unit arrives with
	const double GROUND_UNIT_BRAKE = 0.98;			// its speed's factor an update while it is over the field

	// port: a ground unit's arrival, shared by the truck and the enemy tank: it comes in from the side the tank is
	// not on, and holds that side (the board's blocked flags) until it leaves or dies
	void ArriveFromTankFarSide(Enemy* theEnemy, double theY)
	{
		theEnemy->mY = theY;
		Board* aBoard = theEnemy->mBoard;
		if (aBoard->mTankX < 0.0)
		{
			theEnemy->mMirrored = true;
			theEnemy->mVelX = -GROUND_UNIT_SPEED;
			theEnemy->mX = (double)(theEnemy->GetCelWidth() / 2 + CRAFT_EDGE_X);
			aBoard->mBlockedRight = true;
		}
		else
		{
			theEnemy->mMirrored = false;
			theEnemy->mVelX = GROUND_UNIT_SPEED;
			theEnemy->mX = (double)(-CRAFT_EDGE_X - theEnemy->GetCelWidth() / 2);
			aBoard->mBlockedLeft = true;
		}
	}

	// port: the truck's and the enemy tank's speed: over the field they slow by 2% an update; with the tank dead or
	// the level ending they drive on out (up to a speed of 1)
	void DriveGroundUnit(Enemy* theEnemy)
	{
		Board* aBoard = theEnemy->mBoard;
		if (aBoard->mTankDeadTime == 0 &&
			(aBoard->mLevelTime < aBoard->mLevelLength - CRAFT_LEAVING_TIME || aBoard->mIsSurvival))
		{
			if (-(double)CRAFT_EDGE_X < theEnemy->mX && theEnemy->mX < (double)CRAFT_EDGE_X)
				theEnemy->mVelX = theEnemy->mVelX * GROUND_UNIT_BRAKE;
		}
		else
		{
			theEnemy->mVelX = (theEnemy->mMirrored ? 0.02 : -0.02) + theEnemy->mVelX;
			if (theEnemy->mVelX < -1.0)
				theEnemy->mVelX = -1.0;
			if (1.0 < theEnemy->mVelX)
				theEnemy->mVelX = 1.0;
		}
	}

	// port: the wheels' frame, 0..9 (10 cels), which follows the speed
	void TurnWheels(Enemy* theEnemy)
	{
		theEnemy->mFrame = theEnemy->mVelX * (1.0 / 3.0) + theEnemy->mFrame + 0.3;
		if (10.0 <= theEnemy->mFrame)
			theEnemy->mFrame = theEnemy->mFrame - 10.0;
		if (theEnemy->mFrame < 0.0)
			theEnemy->mFrame = theEnemy->mFrame + 9.0;
	}

	// port: it has left the field past an edge
	bool HasLeftField(Enemy* theEnemy)
	{
		return (theEnemy->mX < (double)(-CRAFT_EDGE_X - theEnemy->GetCelWidth() / 2)) ||
			((double)(theEnemy->GetCelWidth() / 2 + CRAFT_EDGE_X) < theEnemy->mX);
	}
}

//
// Truck
//

// 0x435650: it drives along the road at y 445 and throws rockets at the tank
Truck::Truck(Board* theBoard) : Enemy(theBoard)
{
	mRocketDelay = 0;
	mHealth = mApp->mCraft[CRAFT_TRUCK].mArmor;
	mSprite = mApp->mSprites[IMAGE_TRUCK];
	mPoints = mApp->mCraft[CRAFT_TRUCK].mPoints;
	ArriveFromTankFarSide(this, 445.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_TRUCK].mIndex;
}

// 0x435760
Truck::~Truck()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_TRUCK].mIndex;
}

// 0x435790: a rocket from above its front now and then, with a random frame speed of 0.05 to 0.149
void Truck::Update()
{
	Enemy::Update();
	DriveGroundUnit(this);
	if (mRocketDelay > 0)
		mRocketDelay--;
	TurnWheels(this);

	if (mRocketDelay == 0 && mBoard->mTankDeadTime == 0 && -(double)CRAFT_EDGE_X < mX && mX < (double)CRAFT_EDGE_X &&
		Rand() % ROCKET_ODDS == 0)
	{
		mRocketDelay = ROCKET_DELAY;
		new Rpg(mBoard, mX, mY - 25.0, mMirrored, (double)(Rand() % 100 + 50) * 0.001);
	}

	if (HasLeftField(this))
	{
		mBoard->mBlockedLeft = false;
		mBoard->mBlockedRight = false;
		Remove();
	}
}

// 0x4376f0
void Truck::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x4359e0
void Truck::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_ENEMYTANKSHADOW]->Draw(g, (int)mX, SHADOW_Y, true, 0, 0, false);
	Enemy::Draw(g);
}

// 0x435a20
void Truck::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_TRUCK].mPoints, (int)mX, (int)mY);
	mBoard->mBlockedLeft = false;
	mBoard->mBlockedRight = false;
}

//
// EnemyTank
//

// 0x433960: it drives along at y 440, its gun at cel 7
EnemyTank::EnemyTank(Board* theBoard) : Enemy(theBoard)
{
	mShellDelay = SHELL_DELAY;
	mHealth = mApp->mCraft[CRAFT_ENEMYTANK].mArmor;
	mSprite = mApp->mSprites[IMAGE_ENEMYTANK];
	mPoints = mApp->mCraft[CRAFT_ENEMYTANK].mPoints;
	mGunAngle = 7.0;
	mGunStep = 0.03;
	ArriveFromTankFarSide(this, 440.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_ENEMYTANK].mIndex;
}

// 0x433aa0
EnemyTank::~EnemyTank()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_ENEMYTANK].mIndex;
}

// 0x433ad0
void EnemyTank::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_ENEMYTANKSHADOW]->Draw(g, (int)mX, SHADOW_Y, true, 0, 0, false);
	mApp->mSprites[IMAGE_ENEMYGUN]->Draw(g, (int)mX, GUN_Y, true, (int)mGunAngle, mMirrored, false);
	Enemy::Draw(g);
}

// 0x436410: the gun sweeps up and down its 8 cels (up at 0.03 then 0.04 a cel an update, down at 0.04); it fires a shell
// from the gun's muzzle along its angle (0.1571 radians a cel) at a speed that grows with the angle
void EnemyTank::Update()
{
	Enemy::Update();
	DriveGroundUnit(this);
	if (mShellDelay > 0)
		mShellDelay--;
	TurnWheels(this);

	mGunAngle = mGunAngle + mGunStep;
	if (8.0 <= mGunAngle)
	{
		mGunStep = -0.04;
		mGunAngle = 7.0;
	}
	if (mGunAngle < 0.0)
	{
		mGunStep = 0.04;
		mGunAngle = 0.0;
	}

	if (mShellDelay == 0 && mBoard->mTankDeadTime == 0 && -(double)CRAFT_EDGE_X < mX && mX < (double)CRAFT_EDGE_X)
	{
		double anAngle = mGunAngle * 0.1571;
		if (mMirrored)
			anAngle = 3.1416 - anAngle;

		Bullet* aBullet = new Bullet();
		mBoard->mBullets.Append(aBullet);
		double aCos = cos(anAngle);
		aBullet->mX = aCos * 24.0 + (double)(mMirrored ? -18 : 18) + mX;
		double aSin = sin(anAngle);
		aBullet->mY = (mY - 5.0) - aSin * 24.0;
		aBullet->mVelX = (mGunAngle * 0.5 + 2.0) * aCos;
		aBullet->mGravity = 0.05;
		aBullet->mIsEnemy = true;
		aBullet->mVelY = -((mGunAngle * 0.5 + 2.0) * aSin);
		aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
		aBullet->mColumn = 0;
		aBullet->mRow = 0;
		mShellDelay = SHELL_DELAY;
		mApp->PlaySample(SOUND_ENEMYTANKGUN, (int)(mX * 3.0));
		mBoard->AddMuzzleFlash(aBullet->mX, aBullet->mY, mVelX, 0.0);
	}

	if (HasLeftField(this))
	{
		mBoard->mBlockedLeft = false;
		mBoard->mBlockedRight = false;
		Remove();
	}
}

// 0x4376f0
void EnemyTank::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x433b40
void EnemyTank::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_ENEMYTANK].mPoints, (int)mX, (int)mY);
	mBoard->mBlockedLeft = false;
	mBoard->mBlockedRight = false;
}

//
// Dozer
//

// 0x433520: it drives along at y 425 and holds the board's dozer flag until it is gone
Dozer::Dozer(Board* theBoard) : Enemy(theBoard)
{
	mHealth = mApp->mCraft[CRAFT_DOZER].mArmor;
	mSprite = mApp->mSprites[IMAGE_DOZER];
	mY = 425.0;
	mPoints = mApp->mCraft[CRAFT_DOZER].mPoints;
	if (mBoard->mTankX < 0.0)
	{
		mMirrored = true;
		mX = (double)(GetCelWidth() / 2 + CRAFT_EDGE_X);
		mVelX = -GROUND_UNIT_SPEED;
	}
	else
	{
		mMirrored = false;
		mX = (double)(-CRAFT_EDGE_X - GetCelWidth() / 2);
		mVelX = GROUND_UNIT_SPEED;
	}
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_DOZER].mIndex;
	mBoard->mDozerOut = true;
}

// 0x433630
Dozer::~Dozer()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_DOZER].mIndex;
	mBoard->mDozerOut = false;
}

// 0x433660: touching the tank kills it; it drives at the tank's side, speeding up (to 1.5) while it is not hit, and
// slowing to 0.5 against its way once hit and over the field; with the tank dead or the level ending it leaves at up
// to 5
void Dozer::Update()
{
	Enemy::Update();
	if (mBoard->mTankDeadTime == 0)
	{
		if (mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, mApp->mSprites[IMAGE_TANK], (int)mBoard->mTankX,
			mBoard->mTankY, (int)mBoard->mTreadFrame, 0, false, true))
			mBoard->KillTank();
	}

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 &&
		(aBoard->mLevelTime < aBoard->mLevelLength - CRAFT_LEAVING_TIME || aBoard->mIsSurvival))
	{
		if (mHitFlash == 0)
		{
			mVelX = mVelX - (double)((int)mMirrored * 2 - 1) * 0.02;
			if (mVelX < -1.5)
				mVelX = -1.5;
			if (mVelX > 1.5)
				mVelX = 1.5;
		}
		else if (GetCelWidth() / 2 + abs((int)mX) <= CRAFT_EDGE_X - 1)
		{
			mVelX = (double)((int)mMirrored * 2 - 1) * 0.02 + mVelX;
			if (mVelX < -0.5 && !mMirrored)
				mVelX = -0.5;
			if (mVelX > 0.5 && mMirrored)
				mVelX = 0.5;
		}
	}
	else
	{
		mVelX = (mMirrored ? 0.03 : -0.03) + mVelX;
		if (mVelX < -5.0)
			mVelX = -5.0;
		if (mVelX > 5.0)
			mVelX = 5.0;
	}

	mFrame = mFrame - (mVelX * (1.0 / 3.0) + 0.3);
	if (10.0 <= mFrame)
		mFrame = mFrame - 10.0;
	if (mFrame < 0.0)
		mFrame = mFrame + 9.0;

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void Dozer::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x4338e0
void Dozer::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_DOZERSHADOW]->Draw(g, (int)mX, CRAFT_GROUND_Y, true, 0, 0, false);
	Enemy::Draw(g);
}

// 0x433920
void Dozer::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_DOZER].mPoints, (int)mX, (int)mY);
}
