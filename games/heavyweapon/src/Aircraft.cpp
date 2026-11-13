// Aircraft.cpp: the enemy aircraft of Craft.h: the bombers and jets, the helicopters, the blimp, the satellite, the
// strafer, the cruise missile and the deflector. The ground units are in GroundUnits.cpp.
// Original: 0x431730..0x435ad0 and the Update, Draw and Killed bodies that follow (alphabetical by class: 0x431730
// BigBomber, 0x431a70 BigCopter, 0x4320c0 Blimp, ...).

#include "Craft.h"
#include "Board.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"

#include <math.h>

using namespace Sexy;

namespace
{
	// port: the sideways speed an aircraft is given on its way in: (Rand() % theSpeedRange) / 100 + theBaseSpeed, and its
	// place just outside the edge it comes in from (mMirrored, chosen by ChooseSide, means it comes from the right)
	void EnterFromSide(Enemy* theEnemy, int theSpeedRange, double theBaseSpeed)
	{
		if (!theEnemy->mMirrored)
		{
			theEnemy->mX = (double)(-CRAFT_EDGE_X - theEnemy->GetCelWidth() / 2);
			theEnemy->mVelX = (double)(Rand() % theSpeedRange) * 0.01 + theBaseSpeed;
		}
		else
		{
			theEnemy->mX = (double)(theEnemy->GetCelWidth() / 2 + CRAFT_EDGE_X);
			theEnemy->mVelX = -((double)(Rand() % theSpeedRange) * 0.01 + theBaseSpeed);
		}
	}

	// port: the same for the helicopters and the satellite, which pick a side at random and do not fly sideways
	void EnterFromRandomSide(Enemy* theEnemy)
	{
		if (Rand() % 2 == 0)
			theEnemy->mX = (double)(-CRAFT_EDGE_X - theEnemy->GetCelWidth() / 2);
		else
			theEnemy->mX = (double)(theEnemy->GetCelWidth() / 2 + CRAFT_EDGE_X);
	}

	// port: it has left the field past an edge (the original writes this out at the end of each Update)
	bool HasLeftField(Enemy* theEnemy)
	{
		return (theEnemy->mX < (double)(-CRAFT_EDGE_X - theEnemy->GetCelWidth() / 2)) ||
			((double)(theEnemy->GetCelWidth() / 2 + CRAFT_EDGE_X) < theEnemy->mX);
	}

	// port: its sprite's colour while it flashes red from a hit (the original writes this out in each Draw of its own)
	void StartHitFlash(Enemy* theEnemy, Graphics* g)
	{
		if (theEnemy->mHitFlash != 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255 - theEnemy->mHitFlash, 255 - theEnemy->mHitFlash, 255));
		}
	}

	// The helicopters' search for the player's bullets (BigCopter, MedCopter and Satellite look at the bullets within
	// a distance of them and at the middle of those)
	const int NUM_BULLET_SEARCH_RANGE_BIG = 200;
	const double HELI_MIN_X = -160.0;			// the helicopters go after bullets only inside this part of the field
	const double HELI_MAX_X = 160.0;
	const double HELI_MIN_Y = 100.0;
	const double HELI_MAX_Y = 200.0;
	const double HELI_REST_X = 100.0;			// without bullets one drifts back inside +-100
	const double HELI_FLEE_SPEED = 0.1;
}

// 0x4376f0
void Sexy::MoveEnemyX(Enemy* theEnemy, float theFrac)
{
	theEnemy->mX = (double)theFrac * theEnemy->mVelX + theEnemy->mX;
}

// 0x435350
void Sexy::MoveEnemyXY(Enemy* theEnemy, float theFrac)
{
	theEnemy->mX = (double)theFrac * theEnemy->mVelX + theEnemy->mX;
	theEnemy->mY = (double)theFrac * theEnemy->mVelY + theEnemy->mY;
}

//
// BigBomber
//

// 0x431730
BigBomber::BigBomber(Board* theBoard) : Enemy(theBoard)
{
	mRunDelay = 0;
	mBombsLeft = 0;
	mBombTimer = 0;
	mHealth = mApp->mCraft[CRAFT_BIGBOMBER].mArmor;
	mSprite = mApp->mSprites[IMAGE_BIGBOMBER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_BIGBOMBER].mPoints;
	EnterFromSide(this, 150, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_BIGBOMBER].mIndex;
}

// 0x431890
BigBomber::~BigBomber()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_BIGBOMBER].mIndex;
}

// 0x4318c0: a run of bombs starts at random over the field (the original reads mVelX, not mX, in the first test, so
// it is always met)
void BigBomber::Update()
{
	Enemy::Update();
	if (mRunDelay > 0)
		mRunDelay--;
	if (mBombTimer > 0)
		mBombTimer--;

	if (-CRAFT_FIRE_RANGE < mVelX && mVelX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 &&
		Rand() % RUN_ODDS == 0 && mRunDelay == 0)
	{
		int aRand = Rand();
		mBombTimer = 0;
		mBombsLeft = aRand % 2 + MIN_RUN_BOMBS;
		mRunDelay = RUN_DELAY;
	}

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && mBombsLeft != 0 &&
		mBombTimer == 0)
	{
		mBoard->AddDumbBomb((int)mX, (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
		mBombTimer = BOMB_SPACING;
		mBombsLeft--;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void BigBomber::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x431a30
void BigBomber::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_BIGBOMBER].mPoints, (int)mX, (int)mY);
}

//
// BigCopter
//

// 0x431a70: it enters at a random side, standing still, and flies by its frame: the frame is its sideways speed
// (mFrame - 4) / 3, 0 to 8, 4 for still
BigCopter::BigCopter(Board* theBoard) : Enemy(theBoard)
{
	mMissileDelay = 0;
	mHealth = mApp->mCraft[CRAFT_BIGCOPTER].mArmor;
	mSprite = mApp->mSprites[IMAGE_BIGCOPTER];
	mFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelX = 0.0;
	mVelY = 0.0;
	mFrame = 4.0;
	mRotorFrame = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	mPoints = mApp->mCraft[CRAFT_BIGCOPTER].mPoints;
	EnterFromRandomSide(this);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_BIGCOPTER].mIndex;
}

// 0x431ba0
BigCopter::~BigCopter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_BIGCOPTER].mIndex;
}

// 0x431bd0
void BigCopter::UpdateF(float theFrac)
{
	mX = (mFrame - 4.0) * (1.0 / 3.0) * (double)theFrac + mX;
	mY = (double)theFrac * mVelY + mY;
}

// 0x431c00
void BigCopter::Draw(Graphics* g)
{
	StartHitFlash(this, g);
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, false);
	mApp->mSprites[IMAGE_ROTORS]->Draw(g, (int)(mX - (double)(GetCelWidth() / 2)), (int)(mY - (double)(GetCelHeight() / 2)),
		false, (int)mRotorFrame, 0, false);
	g->SetColorizeImages(false);
}

// 0x435ad0: the rotor turns; with the tank in play it goes after the middle of the player's bullets within 200 of it
// (changing its frame, and so its speed, and its vertical speed), else it drifts to within 100 of the middle of the
// field (away from it when the tank is dead); a hellfire missile now and then
void BigCopter::Update()
{
	Enemy::Update();
	mRotorFrame += 0.5;
	double aRotorFrames = (double)mApp->mSprites[IMAGE_ROTORS]->mColumns;
	if (aRotorFrames <= mRotorFrame)
		mRotorFrame -= aRotorFrames;
	if (mMissileDelay > 0)
		mMissileDelay--;

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 && (aBoard->mLevelTime < aBoard->mLevelLength - CRAFT_LEAVING_TIME || aBoard->mIsSurvival))
	{
		double aSumX = 0.0;
		double aSumY = 0.0;
		int aCount = 0;
		for (aBoard->mBullets.mCurrent = aBoard->mBullets.mHead; aBoard->mBullets.mCurrent->mNext != NULL; )
		{
			aBoard->mBullets.mCurrent = aBoard->mBullets.mCurrent->mNext;
			Bullet* aBullet = aBoard->mBullets.Current();
			if (!aBullet->mIsEnemy)
			{
				if (sqrt(pow(aBullet->mX - mX, 2.0) + pow(aBullet->mY - mY, 2.0)) < NUM_BULLET_SEARCH_RANGE_BIG)
				{
					aSumX = aSumX + aBullet->mX;
					aCount++;
					aSumY = aSumY + aBullet->mY;
				}
			}
		}

		if (aCount > 0 && HELI_MIN_X < mX && mX < HELI_MAX_X && HELI_MIN_Y < mY && mY < HELI_MAX_Y)
		{
			aSumX = aSumX * (1.0 / (double)aCount);
			aSumY = (1.0 / (double)aCount) * aSumY;
			if (mX < aSumX - 10.0)
				mFrame = mFrame - 0.1;
			if (aSumX + 10.0 < mX)
				mFrame = mFrame + 0.1;
			if (mY < aSumY)
				mVelY = mVelY - 0.02;
			if (aSumY < mY)
				mVelY = mVelY + 0.02;
		}
		else
		{
			if (mX < -HELI_REST_X)
				mFrame = mFrame + 0.1;
			if (mX > HELI_REST_X)
				mFrame = mFrame - 0.1;
		}
	}
	else
	{
		if (mX < 0.0)
			mFrame = mFrame - 0.1;
		if (mX >= 0.0)
			mFrame = mFrame + 0.1;
	}

	if (mFrame < 0.0)
		mFrame = 0.0;
	if (mFrame > 8.0)
		mFrame = 8.0;

	if (Rand() % 10 == 0)
		mVelY = ((double)(Rand() % 3) * 0.05 - 0.05) + mVelY;
	if (mY < HELI_MIN_Y)
		mVelY = mVelY + 0.05;
	if (mY > HELI_MAX_Y)
		mVelY = mVelY - 0.05;
	if (mVelY > 0.5)
		mVelY = 0.5;
	if (mVelY < -0.5)
		mVelY = -0.5;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && Rand() % MISSILE_ODDS == 0 && mMissileDelay == 0)
	{
		mBoard->AddHellfire((int)mX, (int)mY, mVelX, mVelY);
		mMissileDelay = MISSILE_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x431ce0
void BigCopter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_BIGCOPTER].mPoints, (int)mX, (int)mY);
}

//
// Blimp
//

// 0x4320c0
Blimp::Blimp(Board* theBoard) : Enemy(theBoard)
{
	mFrontBombDelay = 0;
	mBackBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_BLIMP].mArmor;
	mSprite = mApp->mSprites[IMAGE_BLIMP];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPropFrame = 0.0;
	mPoints = mApp->mCraft[CRAFT_BLIMP].mPoints;
	EnterFromSide(this, 100, 0.5);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_BLIMP].mIndex;
}

// 0x432220
Blimp::~Blimp()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_BLIMP].mIndex;
}

// 0x4323e0: the body, then a propeller at each end (the art's places, for either way it faces)
void Blimp::Draw(Graphics* g)
{
	Enemy::Draw(g);
	Sprite* aPropeller = mApp->mSprites[IMAGE_BLIMPPROP];
	if (!mMirrored)
	{
		aPropeller->Draw(g, (int)(mX + 28.0), (int)(mY + 21.0), true, (int)mPropFrame, 0, false);
		aPropeller->Draw(g, (int)(mX - 88.0), (int)(mY + 20.0), true, (int)mPropFrame, 0, false);
	}
	else
	{
		aPropeller->Draw(g, (int)(mX - 27.0), (int)(mY + 21.0), true, (int)mPropFrame, 0, false);
		aPropeller->Draw(g, (int)(mX + 89.0), (int)(mY + 20.0), true, (int)mPropFrame, 0, false);
	}
}

// 0x432250: a dumb bomb from each end now and then, 60 pixels either side of the centre, each end with its own delay
void Blimp::Update()
{
	Enemy::Update();
	if (mFrontBombDelay > 0)
		mFrontBombDelay--;
	if (mBackBombDelay > 0)
		mBackBombDelay--;
	mPropFrame += 0.4;
	if (mPropFrame >= 4.0)
		mPropFrame -= 4.0;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE)
	{
		if (Rand() % BOMB_ODDS == 0 && mFrontBombDelay == 0)
		{
			mBoard->AddDumbBomb((int)(mX + 60.0), (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
			mFrontBombDelay = BOMB_DELAY;
		}
		if (Rand() % BOMB_ODDS == 0 && mBackBombDelay == 0)
		{
			mBoard->AddDumbBomb((int)(mX - 60.0), (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
			mBackBombDelay = BOMB_DELAY;
		}
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void Blimp::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x435f50: ten pieces of shrapnel fly out of it, at random angles and speeds (1 to 2.4), falling
void Blimp::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_BLIMP].mPoints, (int)mX, (int)mY);

	int aLeft = NUM_SHRAPNEL;
	do
	{
		Bullet* aBullet = new Bullet();
		mBoard->mBullets.Append(aBullet);
		double anAngle = (double)(Rand() % SHRAPNEL_ANGLE_RANGE) * 0.001;
		double aSpeed = (double)(Rand() % 15) * 0.1 + 1.0;
		double aCos = cos(anAngle);
		aBullet->mX = 100.0 * aCos + mX;
		double aSin = sin(anAngle);
		aBullet->mGravity = 0.05;
		aBullet->mIsEnemy = true;
		aBullet->mY = mY - aSin * 30.0;
		aBullet->mVelX = aCos * aSpeed;
		aBullet->mVelY = -(aSin * aSpeed);
		aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
		aBullet->mColumn = 0;
		aBullet->mRow = 0;
		aLeft--;
	}
	while (aLeft != 0);
}

//
// Bomber
//

// 0x4324f0
Bomber::Bomber(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_BOMBER].mArmor;
	mSprite = mApp->mSprites[IMAGE_BOMBER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_BOMBER].mPoints;
	EnterFromSide(this, 200, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_BOMBER].mIndex;
}

// 0x432640
Bomber::~Bomber()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_BOMBER].mIndex;
}

// 0x432670
void Bomber::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && Rand() % BOMB_ODDS == 0 &&
		mBombDelay == 0)
	{
		mBoard->AddDumbBomb((int)mX, (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void Bomber::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x432770
void Bomber::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_BOMBER].mPoints, (int)mX, (int)mY);
}

//
// CruiseMissile
//

// 0x4327b0: it comes in from a random side at 5 pixels an update and does not count as an enemy spawned
CruiseMissile::CruiseMissile(Board* theBoard) : Enemy(theBoard)
{
	mHealth = mApp->mCraft[CRAFT_CRUISEMISSILE].mArmor;
	mSprite = mApp->mSprites[IMAGE_CRUISE];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % ((CRAFT_SKY_HEIGHT - aHeight) / 2) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	mMirrored = Rand() % 2 != 0;
	mPoints = mApp->mCraft[CRAFT_CRUISEMISSILE].mPoints;
	if (mMirrored)
	{
		mVelX = -5.0;
		mX = (double)(GetCelWidth() / 2 + CRAFT_EDGE_X);
	}
	else
	{
		mX = (double)(-CRAFT_EDGE_X - GetCelWidth() / 2);
		mVelX = 5.0;
	}
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_CRUISEMISSILE].mIndex;
}

// 0x432900
CruiseMissile::~CruiseMissile()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_CRUISEMISSILE].mIndex;
}

// 0x432930: mFrame is 0 while it flies level; within 90 pixels of the tank's x it starts to dive (mFrame up 0.2 an
// update to 7, the speed turning from sideways to down); a shield or the tank under it, or the ground, makes it
// explode: a shield loses a level, and the tank dies
void CruiseMissile::Update()
{
	Enemy::Update();
	if (HasLeftField(this))
	{
		Remove();
		return;
	}

	if (0.0 < mFrame)
	{
		mFrame = mFrame + 0.2;
		if (7.0 < mFrame)
			mFrame = 7.0;
		mVelX = (7.0 - mFrame) * 0.7142857142857143 * (double)((int)mMirrored * -2 + 1);
		mVelY = mFrame * 0.7142857142857143;
	}
	if (mFrame == 0.0 && abs((int)(mX - mBoard->mTankX)) < 90)
		mFrame = mFrame + 0.2;

	if (mY <= (double)(470 - GetCelHeight() / 2))
	{
		Board* aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0 && (mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] != 0 || 0.0 < aBoard->mShieldHitGlow))
		{
			if (mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, mApp->mSprites[IMAGE_SHIELD],
				(int)aBoard->mTankX, aBoard->mTankY + 2, (int)aBoard->mShieldFrame, 0, false, true))
			{
				if (mBoard->mShieldHitGlow <= 0.0)
				{
					mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD]--;
					if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] == 0)
						mApp->PlaySample(SOUND_SHIELDDOWN, (int)(mBoard->mTankX * 3.0));
					mBoard->mShieldHitGlow = 9.0;
					mApp->PlaySample(SOUND_SPARKING, (int)(mBoard->mTankX * 3.0));
				}
				Enemy::Explode();
				Remove();
				return;
			}
		}

		aBoard = mBoard;
		if (aBoard->mTankDeadTime != 0)
			return;
		if (!mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, mApp->mSprites[IMAGE_TANK], (int)aBoard->mTankX,
			aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
			return;
		mBoard->KillTank();
	}
	else
	{
		mVelY = 0.0;
		mBoard->AddCrater((int)mX, 0);
	}

	Enemy::Explode();
	Remove();
}

// 0x435350
void CruiseMissile::UpdateF(float theFrac)
{
	MoveEnemyXY(this, theFrac);
}

// 0x432c20: shot down, it was never counted as an enemy
void CruiseMissile::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
	mBoard->AddScore(mApp->mCraft[CRAFT_CRUISEMISSILE].mPoints, (int)mX, (int)mY);
}

//
// Deflector
//

// 0x432c80
Deflector::Deflector(Board* theBoard) : Enemy(theBoard)
{
	mMissileDelay = 0;
	mHealth = mApp->mCraft[CRAFT_DEFLECTOR].mArmor;
	mSprite = mApp->mSprites[IMAGE_DEFLECTOR];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % ((CRAFT_SKY_HEIGHT - aHeight) / 2) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_DEFLECTOR].mPoints;
	mGlow = 0;
	mShieldEnergy = SHIELD_ENERGY;
	EnterFromSide(this, 100, 2.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_DEFLECTOR].mIndex;
}

// 0x432df0
Deflector::~Deflector()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_DEFLECTOR].mIndex;
}

// 0x432e20: while its shield is up, a magenta shield added over it, and a grey flash of it when it turned a bullet
void Deflector::Draw(Graphics* g)
{
	Enemy::Draw(g);
	if (mShieldEnergy > 0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 0, 128));
		mApp->mSprites[IMAGE_DEFLECTSHIELD]->Draw(g, (int)mX, (int)mY, true, (mApp->mUpdateCount / 4) % 4, 0, false);
		if (mGlow != 0)
		{
			g->SetColor(Color(mGlow, mGlow, mGlow));
			mApp->mSprites[IMAGE_DEFLECTSHIELD]->Draw(g, (int)mX, (int)mY, true, 0, 0, false);
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x436110: the shield turns the tank's bullets within 60 pixels of it back at the tank (they become the enemies'),
// each costing it its damage in shield energy; a hellfire missile now and then
void Deflector::Update()
{
	Enemy::Update();
	if (mShieldEnergy > 0)
	{
		Board* aBoard = mBoard;
		aBoard->mBullets.mCurrent = aBoard->mBullets.mHead;
		while (aBoard->mBullets.mCurrent->mNext != NULL)
		{
			aBoard->mBullets.mCurrent = aBoard->mBullets.mCurrent->mNext;
			Bullet* aBullet = aBoard->mBullets.Current();
			if (!aBullet->mIsEnemy)
			{
				double aDeltaY = aBullet->mY - mY;
				double aDeltaX = aBullet->mX - mX;
				if (sqrt(pow(aDeltaX, 2.0) + pow(aDeltaY, 2.0)) <= SHIELD_RADIUS)
				{
					double aBearing = atan2(aDeltaX, aDeltaY);
					aBullet->mIsEnemy = true;
					// the bullet's velocity loses 1.5 times its part along the direction away from the shield
					double aCos = cos(-aBearing - 1.57);
					double aSin = sin(-aBearing - 1.57);
					double aAlong = aSin * aBullet->mVelY + aCos * aBullet->mVelX;
					aBullet->mVelX = aBullet->mVelX - aAlong * aCos * 1.5;
					aBullet->mVelY = aBullet->mVelY - aAlong * aSin * 1.5;
					aBullet->mColumn = 0;
					aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
					aBullet->mRow = 0;
					mGlow = GLOW_MAX;
					mShieldEnergy = (int)((double)mShieldEnergy - aBullet->mDamage);
					mApp->PlaySample(SOUND_DEFLECT, (int)(aBullet->mX * 3.0));
					mBoard->AddMuzzleFlash(aBullet->mX, aBullet->mY, 0.0, 0.0);
				}
			}
		}
	}

	if (mMissileDelay > 0)
		mMissileDelay--;
	if (mGlow != 0)
		mGlow -= GLOW_FADE;
	if (mGlow < 0)
		mGlow = 0;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && Rand() % MISSILE_ODDS == 0 &&
		mMissileDelay == 0)
	{
		mBoard->AddHellfire((int)mX, (int)mY, mVelX, 0.0);
		mMissileDelay = MISSILE_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void Deflector::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x432f10
void Deflector::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_DEFLECTOR].mPoints, (int)mX, (int)mY);
}

//
// DeltaBomber
//

// 0x432f50
DeltaBomber::DeltaBomber(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_DELTABOMBER].mArmor;
	mSprite = mApp->mSprites[IMAGE_DELTABOMBER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_DELTABOMBER].mPoints;
	EnterFromSide(this, 200, 2.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_DELTABOMBER].mIndex;
}

// 0x4330a0
DeltaBomber::~DeltaBomber()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_DELTABOMBER].mIndex;
}

// 0x4330d0
void DeltaBomber::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && Rand() % BOMB_ODDS == 0 &&
		mBombDelay == 0)
	{
		new FragBomb(mBoard, mX, (double)(GetCelHeight() / 2) + mY, mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void DeltaBomber::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x433220
void DeltaBomber::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_DELTABOMBER].mPoints, (int)mX, (int)mY);
}

//
// DeltaJet
//

// 0x433260
DeltaJet::DeltaJet(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_DELTAJET].mArmor;
	mSprite = mApp->mSprites[IMAGE_DELTAJET];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_DELTAJET].mPoints;
	EnterFromSide(this, 200, 2.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_DELTAJET].mIndex;
}

// 0x4333b0
DeltaJet::~DeltaJet()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_DELTAJET].mIndex;
}

// 0x4333e0
void DeltaJet::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && Rand() % BOMB_ODDS == 0 &&
		mBombDelay == 0)
	{
		mBoard->AddIronBomb((int)mX, (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void DeltaJet::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x4334e0
void DeltaJet::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_DELTAJET].mPoints, (int)mX, (int)mY);
}

//
// FatBomber
//

// 0x433b90
FatBomber::FatBomber(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_FATBOMBER].mArmor;
	mSprite = mApp->mSprites[IMAGE_FATBOMBER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % ((CRAFT_SKY_HEIGHT - aHeight) / 2) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_FATBOMBER].mPoints;
	EnterFromSide(this, 100, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_FATBOMBER].mIndex;
}

// 0x433cf0
FatBomber::~FatBomber()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_FATBOMBER].mIndex;
}

// 0x433d20: a fat boy every 150 updates over the field, dropped even with the tank dead
void FatBomber::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBombDelay == 0)
	{
		new FatBoy(mBoard, mX, (double)(GetCelHeight() / 2) + mY, mVelX, 0.0, mMirrored, false);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void FatBomber::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x433e50
void FatBomber::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_FATBOMBER].mPoints, (int)mX, (int)mY);
}

//
// JetFighter
//

// 0x433e90
JetFighter::JetFighter(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_JETFIGHTER].mArmor;
	mSprite = mApp->mSprites[IMAGE_JETFIGHTER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_JETFIGHTER].mPoints;
	EnterFromSide(this, 200, 2.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_JETFIGHTER].mIndex;
}

// 0x433fe0
JetFighter::~JetFighter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_JETFIGHTER].mIndex;
}

// 0x434010
void JetFighter::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBoard->mTankDeadTime == 0 && Rand() % BOMB_ODDS == 0 &&
		mBombDelay == 0)
	{
		new LGB(mBoard, mX, (double)(GetCelHeight() / 2) + mY, mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void JetFighter::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x434160
void JetFighter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_JETFIGHTER].mPoints, (int)mX, (int)mY);
}

//
// MedCopter
//

// 0x4341a0: like the big copter, it enters at a random side; its frame (0 to 6, 3 for still) is its speed
MedCopter::MedCopter(Board* theBoard) : Enemy(theBoard)
{
	mMissileDelay = 0;
	mHealth = mApp->mCraft[CRAFT_MEDCOPTER].mArmor;
	mSprite = mApp->mSprites[IMAGE_MEDCOPTER];
	mFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelX = 0.0;
	mVelY = 0.0;
	mFrame = 3.0;
	mRotorFrame = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	mPoints = mApp->mCraft[CRAFT_MEDCOPTER].mPoints;
	EnterFromRandomSide(this);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_MEDCOPTER].mIndex;
}

// 0x4342d0
MedCopter::~MedCopter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_MEDCOPTER].mIndex;
}

// 0x434300
void MedCopter::UpdateF(float theFrac)
{
	mX = (mFrame - 3.0) * 0.5 * (double)theFrac + mX;
	mY = (double)theFrac * mVelY + mY;
}

// 0x434330: the body, and its rotor 22 above its centre
void MedCopter::Draw(Graphics* g)
{
	StartHitFlash(this, g);
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, false);
	mApp->mSprites[IMAGE_MEDROTOR]->Draw(g, (int)mX, (int)(mY - 22.0), true, (int)mRotorFrame, 0, false);
	g->SetColorizeImages(false);
}

// 0x4367d0: as the big copter's, with the bullets searched for within 200 and other speeds
void MedCopter::Update()
{
	Enemy::Update();
	mRotorFrame += 0.5;
	double aRotorFrames = (double)mApp->mSprites[IMAGE_MEDROTOR]->mColumns;
	if (aRotorFrames <= mRotorFrame)
		mRotorFrame -= aRotorFrames;
	if (mMissileDelay > 0)
		mMissileDelay--;

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 && (aBoard->mLevelTime < aBoard->mLevelLength - CRAFT_LEAVING_TIME || aBoard->mIsSurvival))
	{
		double aSumX = 0.0;
		double aSumY = 0.0;
		int aCount = 0;
		for (aBoard->mBullets.mCurrent = aBoard->mBullets.mHead; aBoard->mBullets.mCurrent->mNext != NULL; )
		{
			aBoard->mBullets.mCurrent = aBoard->mBullets.mCurrent->mNext;
			Bullet* aBullet = aBoard->mBullets.Current();
			if (!aBullet->mIsEnemy)
			{
				if (sqrt(pow(aBullet->mX - mX, 2.0) + pow(aBullet->mY - mY, 2.0)) < NUM_BULLET_SEARCH_RANGE_BIG)
				{
					aSumX = aSumX + aBullet->mX;
					aCount++;
					aSumY = aSumY + aBullet->mY;
				}
			}
		}

		if (aCount > 0 && HELI_MIN_X < mX && mX < HELI_MAX_X && HELI_MIN_Y < mY && mY < HELI_MAX_Y)
		{
			aSumX = aSumX * (1.0 / (double)aCount);
			aSumY = (1.0 / (double)aCount) * aSumY;
			if (mX < aSumX - 10.0)
				mFrame = mFrame - 0.15;
			if (aSumX + 10.0 < mX)
				mFrame = mFrame + 0.15;
			if (mY < aSumY)
				mVelY = mVelY - 0.02;
			if (aSumY < mY)
				mVelY = mVelY + 0.02;
		}
		else
		{
			if (mX < -HELI_REST_X)
				mFrame = mFrame + 0.15;
			if (mX > HELI_REST_X)
				mFrame = mFrame - 0.15;
		}
	}
	else
	{
		if (mX < 0.0)
			mFrame = mFrame - 0.15;
		if (mX >= 0.0)
			mFrame = mFrame + 0.15;
	}

	if (mFrame < 0.0)
		mFrame = 0.0;
	if (mFrame > 6.0)
		mFrame = 6.0;

	if (Rand() % 10 == 0)
		mVelY = ((double)(Rand() % 3) * 0.05 - 0.05) + mVelY;
	if (mY < HELI_MIN_Y)
		mVelY = mVelY + 0.02;
	if (mY > HELI_MAX_Y)
		mVelY = mVelY - 0.02;
	if (mVelY > 0.5)
		mVelY = 0.5;
	if (mVelY < -0.5)
		mVelY = -0.5;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && Rand() % MISSILE_ODDS == 0 && mMissileDelay == 0)
	{
		mBoard->AddHellfire((int)mX, (int)mY, mVelX, mVelY);
		mMissileDelay = MISSILE_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4343f0
void MedCopter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_MEDCOPTER].mPoints, (int)mX, (int)mY);
}

//
// PropFighter
//

// 0x434430
PropFighter::PropFighter(Board* theBoard) : Enemy(theBoard)
{
	mHealth = mApp->mCraft[CRAFT_PROPFIGHTER].mArmor;
	mSprite = mApp->mSprites[IMAGE_PROPFIGHTER];
	mFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_PROPFIGHTER].mPoints;
	EnterFromSide(this, 200, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_PROPFIGHTER].mIndex;
}

// 0x434590
PropFighter::~PropFighter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_PROPFIGHTER].mIndex;
}

// 0x4345c0: its propeller turns (4 cels)
void PropFighter::Update()
{
	Enemy::Update();
	mFrame += 0.4;
	if (mFrame >= 4.0)
		mFrame -= 4.0;

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void PropFighter::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x434650
void PropFighter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_PROPFIGHTER].mPoints, (int)mX, (int)mY);
}

//
// SmallJet
//

// 0x434690
SmallJet::SmallJet(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_SMALLJET].mArmor;
	mSprite = mApp->mSprites[IMAGE_SMALLJET];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_SMALLJET].mPoints;
	EnterFromSide(this, 200, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_SMALLJET].mIndex;
}

// 0x4347e0
SmallJet::~SmallJet()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_SMALLJET].mIndex;
}

// 0x434810: no bomb while the help screen is up
void SmallJet::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && Rand() % BOMB_ODDS == 0 && mBombDelay == 0 &&
		mApp->mHelpScreen == NULL)
	{
		mBoard->AddDumbBomb((int)mX, (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void SmallJet::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x434910
void SmallJet::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_SMALLJET].mPoints, (int)mX, (int)mY);
}

//
// Satellite
//

// 0x434950: it enters at a random side with no speed; it drifts by its own steering
Satellite::Satellite(Board* theBoard) : Enemy(theBoard)
{
	mBeamTime = 0;
	mBeamDelay = 200;
	mHealth = mApp->mCraft[CRAFT_SATELLITE].mArmor;
	mSprite = mApp->mSprites[IMAGE_SATELLITE];
	mBeamFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelX = 0.0;
	mVelY = 0.0;
	mFrame = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	mSmokeCounter = 0;
	mPoints = mApp->mCraft[CRAFT_SATELLITE].mPoints;
	mKeyDelay = 0;
	mChatterDelay = 0;
	EnterFromRandomSide(this);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_SATELLITE].mIndex;
}

// 0x434a90
Satellite::~Satellite()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_SATELLITE].mIndex;
}

// 0x434ac0: a beam from it down to the ground: 4 lines added, each pair a pixel farther apart and a shade greener,
// then the burn where it reaches the ground and the beam's source over the satellite
void Satellite::Draw(Graphics* g)
{
	Enemy::Draw(g);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	if (0 < mBeamTime && mBeamTime < BEAM_BURN_START)
	{
		int aRed = 255;
		int aGreen = 255;
		for (int i = 0; aGreen >= 0; i++)
		{
			g->SetColor(Color(aRed, aGreen, aRed));
			g->DrawLine((int)((double)i + mX), (int)(mY + 48.0), (int)((double)i + mX), CRAFT_GROUND_Y + 12);
			g->DrawLine((int)(mX - (double)i), (int)(mY + 48.0), (int)(mX - (double)i), CRAFT_GROUND_Y + 12);
			aGreen -= 0x40;
			aRed -= 0x20;
		}
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		mApp->mSprites[IMAGE_LASERBURN]->Draw(g, (int)mX, CRAFT_GROUND_Y + 12, true, (mApp->mUpdateCount % 40) / 4, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mBeamTime != 0)
		mApp->mSprites[IMAGE_SATLASER]->Draw(g, (int)mX, (int)(mY + 47.0), true, (int)mBeamFrame, 0, false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x436c80: it steers toward the middle of the player's bullets within 150 of it, else toward the tank (away from the
// middle when the tank is dead), and keeps in the band y 100..200 at up to 2 pixels an update sideways and 1 up and
// down; a beam starts at random (a rising sound for 100 updates, then it fires for 99: with it on, a shield within
// 50 pixels of the tank goes and the tank within 32 dies); its radio sounds, and its sparks
void Satellite::Update()
{
	Enemy::Update();
	if (mBeamDelay > 0)
		mBeamDelay--;
	if (mBeamTime > 0)
		mBeamTime--;
	if (mKeyDelay != 0)
		mKeyDelay--;
	if (mChatterDelay != 0)
		mChatterDelay--;
	mFrame += 0.2;
	if (mFrame >= 10.0)
		mFrame -= 10.0;
	mBeamFrame += 0.5;
	if (mBeamFrame >= 10.0)
		mBeamFrame -= 10.0;

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 && (aBoard->mLevelTime < aBoard->mLevelLength - CRAFT_LEAVING_TIME || aBoard->mIsSurvival))
	{
		double aSumX = 0.0;
		double aSumY = 0.0;
		int aCount = 0;
		for (aBoard->mBullets.mCurrent = aBoard->mBullets.mHead; aBoard->mBullets.mCurrent->mNext != NULL; )
		{
			aBoard->mBullets.mCurrent = aBoard->mBullets.mCurrent->mNext;
			Bullet* aBullet = aBoard->mBullets.Current();
			if (!aBullet->mIsEnemy)
			{
				if (sqrt(pow(aBullet->mX - mX, 2.0) + pow(aBullet->mY - mY, 2.0)) < 150.0)
				{
					aSumX = aSumX + aBullet->mX;
					aCount++;
					aSumY = aSumY + aBullet->mY;
				}
			}
		}

		if (aCount > 0 && HELI_MIN_X < mX && mX < HELI_MAX_X && HELI_MIN_Y < mY && mY < HELI_MAX_Y)
		{
			aSumX = aSumX * (1.0 / (double)aCount);
			aSumY = (1.0 / (double)aCount) * aSumY;
			if (mX < aSumX)
				mVelX = mVelX - HELI_FLEE_SPEED;
			if (aSumX < mX)
				mVelX = mVelX + HELI_FLEE_SPEED;
			if (mY < aSumY)
				mVelY = mVelY - 0.03;
			if (aSumY < mY)
				mVelY = mVelY + 0.03;
		}
		else
		{
			if (mX < mBoard->mTankX)
				mVelX = mVelX + 0.02;
			if (mX > mBoard->mTankX)
				mVelX = mVelX - 0.02;
		}
	}
	else
	{
		if (mX < 0.0)
			mVelX = mVelX - 0.03;
		if (mX >= 0.0)
			mVelX = mVelX + 0.03;
	}

	if (Rand() % 80 == 0)
		mVelY = ((double)(Rand() % 4) * 0.1 - 0.8) + mVelY;
	if (mY < HELI_MIN_Y)
		mVelY = mVelY + 0.1;
	if (mY > HELI_MAX_Y)
		mVelY = mVelY - 0.1;
	if (mVelX > 2.0)
		mVelX = 2.0;
	if (mVelX < -2.0)
		mVelX = -2.0;
	if (mVelY > 1.0)
		mVelY = 1.0;
	if (mVelY < -1.0)
		mVelY = -1.0;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && Rand() % BEAM_ODDS == 0 && mBeamDelay == 0 &&
		mBoard->mTankDeadTime == 0)
	{
		mBeamDelay = BEAM_DELAY;
		mBeamTime = BEAM_TIME;
		mApp->PlaySample(SOUND_LASERPOWERUP, (int)(mX * 3.0));
	}
	if (mBeamTime == BEAM_BURN_START)
		mApp->PlaySample(SOUND_SATLASER, (int)(mX * 3.0));

	if (0 < mBeamTime && mBeamTime < BEAM_BURN_START && mBoard->mTankDeadTime == 0)
	{
		if (mBoard->mTankX - 50.0 < mX && mX < mBoard->mTankX + 50.0 && mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] != 0)
		{
			mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] = 0;
			mBoard->mShieldHitGlow = 9.0;
			mApp->PlaySample(SOUND_SPARKING, (int)(mBoard->mTankX * 3.0));
		}
		if (mBoard->mTankX - TANK_KILL_RANGE < mX && mX < mBoard->mTankX + TANK_KILL_RANGE)
			mBoard->KillTank();
	}

	if (Rand() % 100 == 0 && mChatterDelay == 0)
	{
		int aPan = (int)(mX * 3.0);
		mApp->PlaySample(Rand() % 7 + SOUND_SAT1, aPan);
		mChatterDelay = 100;
	}
	else
	{
		if (Rand() % 4 == 0 && mKeyDelay == 0)
		{
			int aLongKey = Rand() % 2;
			mApp->PlaySample(aLongKey + SOUND_SHORTKEY, (int)(mX * 3.0));
			mKeyDelay = (aLongKey != 0 ? 10 : 0) + 12;
		}
	}

	mSmokeCounter++;
	if (mSmokeCounter > 3)
	{
		mSmokeCounter = 0;
		mBoard->mParticles.Emit(mApp->mSprites[IMAGE_SPARK], mX + 22.0, mY + 28.0, mVelX + 2.0, mVelY + 1.0, 1, 1.0, 0.0,
			0.0, 30, true, -1);
		mBoard->mParticles.Emit(mApp->mSprites[IMAGE_SPARK], mX - 22.0, mY + 28.0, mVelX - 2.0, mVelY + 1.0, 1, 1.0, 0.0,
			0.0, 30, true, -1);
	}

	if (HasLeftField(this))
		Remove();
}

// 0x435350
void Satellite::UpdateF(float theFrac)
{
	MoveEnemyXY(this, theFrac);
}

// 0x434c40
void Satellite::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_SATELLITE].mPoints, (int)mX, (int)mY);
}

//
// SmallCopter
//

// 0x434c80: it flies lower than the others (a sky of 180)
SmallCopter::SmallCopter(Board* theBoard) : Enemy(theBoard)
{
	mReloadDelay = 0;
	mHealth = mApp->mCraft[CRAFT_SMALLCOPTER].mArmor;
	mSprite = mApp->mSprites[IMAGE_SMALLCOPTER];
	mFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (180 - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mShotDelay = 0;
	mPoints = mApp->mCraft[CRAFT_SMALLCOPTER].mPoints;
	EnterFromSide(this, 150, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_SMALLCOPTER].mIndex;
}

// 0x434de0
SmallCopter::~SmallCopter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_SMALLCOPTER].mIndex;
}

// 0x434f70: while it fires, the muzzle flash (cel 4 down to 0) added at its lower edge
void SmallCopter::Draw(Graphics* g)
{
	Enemy::Draw(g);
	if (mShotDelay != 0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		mApp->mSprites[IMAGE_MUZZLEFLASH]->Draw(g, (int)mX, (int)((double)(GetCelHeight() / 2) + mY), true, mShotDelay - 1, 0,
			false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x434e10: it fires a bullet at the tank (not while the help screen is up): the angle is measured from the vertical
void SmallCopter::Update()
{
	Enemy::Update();
	if (mReloadDelay > 0)
		mReloadDelay--;
	mFrame += 0.4;
	if (mFrame >= (double)FRAMES)
		mFrame -= (double)FRAMES;
	if (mShotDelay != 0)
		mShotDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && Rand() % SHOT_ODDS == 0 && mReloadDelay == 0 &&
		mApp->mHelpScreen == NULL)
	{
		double anAngle = atan2(mBoard->mTankX - mX, (double)mBoard->mTankY - mY) + 1.5705;
		mBoard->AddEnemyBullet((int)mX, (int)((double)(GetCelHeight() / 2) + mY), anAngle, 0.0);
		mReloadDelay = SHOT_DELAY;
		mShotDelay = MUZZLE_FLASH_TIME;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void SmallCopter::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x434fe0
void SmallCopter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_SMALLCOPTER].mPoints, (int)mX, (int)mY);
}

//
// Strafer
//

// 0x435020: it comes in from just past a side, 460 out, at a random height; the side is its own choice (not
// ChooseSide's); its speed is 2 to 3
Strafer::Strafer(Board* theBoard) : Enemy(theBoard)
{
	mShotDelay = 0;
	mHealth = mApp->mCraft[CRAFT_STRAFER].mArmor;
	mPoints = mApp->mCraft[CRAFT_STRAFER].mPoints;
	mSprite = mApp->mSprites[IMAGE_STRAFER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mY = ((double)(GetCelHeight() / 2) + (double)(aRand % (160 - aHeight))) - 80.0;
	mMirrored = Rand() % 2 != 0;
	mSpeed = (double)(Rand() % 50) * 0.02 + 2.0;
	mX = mMirrored ? 460.0 : -460.0;
	mApp->PlaySample(SOUND_JETDIVE, (int)(mX * 3.0));
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_STRAFER].mIndex;
}

// 0x435160
Strafer::~Strafer()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_STRAFER].mIndex;
}

// 0x435190: mFrame (0..9) is how far round its arc it is, which it steps on only below y 180: the speed's direction
// is 45 degrees less 5 per frame, mirrored for one from the right; it fires at the tank's side of it while in the
// first 6 frames (an enemy bullet out from 40 pixels beyond it at its angle); it goes at 460 from the middle
void Strafer::Update()
{
	Enemy::Update();
	double anAngle = 0.7854 - mFrame * 0.08726612678022899;
	double aCos = cos(anAngle);
	double aVelX = aCos * mSpeed;
	if (mMirrored)
		aVelX = -aVelX;
	mVelX = aVelX;
	double aSin = sin(anAngle);
	mVelY = aSin * mSpeed;
	if ((int)mY > 180)
		mFrame = mFrame + 0.08;
	if (mFrame > 9.0)
		mFrame = 9.0;
	if (mShotDelay != 0)
		mShotDelay--;

	if ((double)(GetCelWidth() / 2 - CRAFT_EDGE_X) < mX && mX < (double)(CRAFT_EDGE_X - GetCelWidth() / 2) &&
		mShotDelay == 0 && mBoard->mTankDeadTime == 0 && mFrame < 6.0)
	{
		// the shot starts 40 pixels ahead of it: to the right of one from the left, to the left of one from the right
		double aShotSpeed = mSpeed + 1.5;
		if (!mMirrored)
			anAngle = 3.142 - anAngle;
		int aShotY = (int)(aSin * 40.0 + mY);
		int aShotX = mMirrored ? (int)(mX - aCos * 40.0) : (int)(aCos * 40.0 + mX);
		mBoard->AddEnemyBullet(aShotX, aShotY, anAngle, aShotSpeed);
		mShotDelay = SHOT_DELAY;
	}

	if (mX < -460.0 || 460.0 < mX)
		Remove();
}

// 0x435350
void Strafer::UpdateF(float theFrac)
{
	MoveEnemyXY(this, theFrac);
}

// 0x435370
void Strafer::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_STRAFER].mPoints, (int)mX, (int)mY);
}

//
// SuperBomber
//

// 0x4353b0
SuperBomber::SuperBomber(Board* theBoard) : Enemy(theBoard)
{
	mBombDelay = 0;
	mHealth = mApp->mCraft[CRAFT_SUPERBOMBER].mArmor;
	mSprite = mApp->mSprites[IMAGE_SUPERBOMBER];
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (CRAFT_SKY_HEIGHT - aHeight) + CRAFT_SKY_TOP + GetCelHeight() / 2);
	ChooseSide();
	mPoints = mApp->mCraft[CRAFT_SUPERBOMBER].mPoints;
	EnterFromSide(this, 150, 1.0);
	mBoard->mLevelEndBlocks += mApp->mCraft[CRAFT_SUPERBOMBER].mIndex;
}

// 0x435500
SuperBomber::~SuperBomber()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[CRAFT_SUPERBOMBER].mIndex;
}

// 0x435530: an iron bomb every 50 updates over the field
void SuperBomber::Update()
{
	Enemy::Update();
	if (mBombDelay > 0)
		mBombDelay--;

	if (-CRAFT_FIRE_RANGE < mX && mX < CRAFT_FIRE_RANGE && mBombDelay == 0)
	{
		mBoard->AddIronBomb((int)mX, (int)((double)(GetCelHeight() / 2) + mY), mVelX, mVelY, mMirrored);
		mBombDelay = BOMB_DELAY;
	}

	if (HasLeftField(this))
		Remove();
}

// 0x4376f0
void SuperBomber::UpdateF(float theFrac)
{
	MoveEnemyX(this, theFrac);
}

// 0x435610
void SuperBomber::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mBoard->AddScore(mApp->mCraft[CRAFT_SUPERBOMBER].mPoints, (int)mX, (int)mY);
}
