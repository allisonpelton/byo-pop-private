#include "TankShots.h"
#include "Board.h"
#include "DDImage.h"
#include "Enemy.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "Particles.h"
#include "Res.h"
#include "Sprite.h"

#include <math.h>
#include <stdlib.h>

using namespace Sexy;

namespace
{
	const int FIELD_EDGE_X = 320;
	const double SEARCH_START = 100000.0;		// the distance an enemy must beat to be the target
	const double HEADING_PER_FRAME = 0.3141690229343387;	// the homing missile's frame is its heading: this many radians
	const double HEADING_OFFSET = 1.5705;
	const double ENEMY_BOTTOM = 480.0;			// the static strike ignores enemies below this

	// port: the exhaust at a rocket's or missile's start
	void EmitLaunchSmoke(Board* theBoard, double x, double y, double theVelX, double theVelY)
	{
		theBoard->mParticles.Emit(theBoard->mApp->mSprites[IMAGE_SMOKE], x, y, theVelX, theVelY, 4, 1.0, -0.05, -0.5, 40, true,
			-1);
	}

	// port: the sparks behind a missile
	void EmitTrail(Board* theBoard, double x, double y, double theVelX, double theVelY)
	{
		theBoard->mParticles.Emit(theBoard->mApp->mSprites[IMAGE_SPARK], x, y, theVelX, theVelY, 1, 1.0, 0.0, 0.0, 30, true,
			-1);
	}

	// port: does the shot's sprite touch the enemy (a missile's and a rocket's test, written out in each)
	bool TouchesEnemy(Projectile* theShot, Enemy* theEnemy)
	{
		return theShot->mSprite->Collides((int)theShot->mX, (int)theShot->mY, (int)theShot->mFrame, 0, false,
			theEnemy->GetSprite(), (int)theEnemy->mX, (int)theEnemy->mY, (int)theEnemy->mFrame, 0, theEnemy->mMirrored,
			true);
	}

	// port: the end of a missile's or rocket's Update: it goes once it is off the sides or the top
	void FinishShot(Projectile* theShot)
	{
		if ((double)(-FIELD_EDGE_X - theShot->GetCelWidth() / 2) <= theShot->mX &&
			theShot->mX <= (double)(theShot->GetCelWidth() / 2 + FIELD_EDGE_X) && 0.0 <= theShot->mY)
			return;
		theShot->Remove();
	}

	// port: a missile or rocket has hit theEnemy, on which mEnemies' walk stands: it flashes and loses theDamage health;
	// if it lives, sparks fly; if it dies it is killed (a boss stays on the list); the shot goes either way
	void HitEnemy(Projectile* theShot, Enemy* theEnemy, double theDamage)
	{
		Board* aBoard = theShot->mBoard;
		theEnemy->mHitFlash = 255;
		theEnemy->mHealth -= theDamage;
		if (theEnemy->mHealth >= 0.0)
		{
			aBoard->mApp->PlaySample(SOUND_SMALLEXPLODE, (int)(theShot->mX * 3.0));
			aBoard->mParticles.Emit(aBoard->mApp->mSprites[IMAGE_SPARK], theShot->mX, theShot->mY, theEnemy->mVelX,
				theEnemy->mVelY, (int)(aBoard->mApp->mParticles * 10.0), 2.0, 0.05, 3.0, 30, true, -1);
		}
		else
		{
			theEnemy->Killed();
			if (theEnemy != aBoard->mBoss)
				aBoard->mEnemies.RemoveCurrent();
		}
		theShot->Remove();
	}
}

//
// StaticStrike
//

// 0x44aa10
StaticStrike::StaticStrike(Board* theBoard) : Projectile(theBoard)
{
	for (int i = 0; i < TRAIL_LENGTH; i++)
	{
		mTrailX[i] = mBoard->mTankX;
		mTrailY[i] = (double)(mBoard->mTankY - 4);
	}
	mIsDestructible = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_BOLT];
	mHitsLeft = mBoard->mApp->mWeapons[HeavyWeaponApp::WEAPON_STATIC] * 2;
	mBoard->mApp->PlaySample(SOUND_STATICSHOT, (int)(mBoard->mTankX * 3.0));
	mLastTarget = NULL;
	mTarget = NULL;
	mTarget = FindTarget();
}

// 0x447cf0
StaticStrike::~StaticStrike()
{
}

// 0x449e60: the nearest living enemy to the head of the path, not the player's and not the one hit last; it is then the
// one hit last
Enemy* StaticStrike::FindTarget()
{
	double aBest = SEARCH_START;
	Enemy* aResult = NULL;
	ObjectList<Enemy>& aList = mBoard->mEnemies;
	ObjectList<Enemy>::Node* aSavedCurrent = aList.mCurrent;
	for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
	{
		aList.mCurrent = aList.mCurrent->mNext;
		Enemy* anEnemy = aList.Current();
		double aDistance = sqrt(pow(anEnemy->mY - mTrailY[0], 2.0) + pow(anEnemy->mX - mTrailX[0], 2.0));
		if (aDistance < aBest && !anEnemy->mIsFriendly && anEnemy != mLastTarget && anEnemy->mIsAlive &&
			anEnemy->mY < ENEMY_BOTTOM)
		{
			aBest = aDistance;
			aResult = anEnemy;
		}
	}
	aList.mCurrent = aSavedCurrent;
	if (aResult != NULL)
		mLastTarget = aResult;
	return aResult;
}

// 0x447d00: the path as a chain of bolt pictures, added, brighter at the head
void StaticStrike::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	double aShade = 255.0;
	for (int i = 0; i < TRAIL_LENGTH - 1; i++)
	{
		double aStep = 0.0;
		do
		{
			g->SetColor(Color((int)aShade, (int)aShade, (int)aShade));
			mSprite->Draw(g, (int)(mTrailX[i] + aStep * (mTrailX[i + 1] - mTrailX[i])),
				(int)(mTrailY[i] + aStep * (mTrailY[i + 1] - mTrailY[i])), true, 4 - i / 2, 0, false);
			aShade -= 2.55;
			aStep += 0.1;
		}
		while (aStep < 0.9);
	}
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x44aca0: the head moves at most 20 pixels an update to the target (or, with none, toward the top of the screen's
// middle), the path behind it shifting and its second point jittering; on reaching the target it hurts it
void StaticStrike::Update()
{
	if (mTarget == NULL)
	{
		mTarget = FindTarget();
	}
	else
	{
		ObjectList<Enemy>& aList = mBoard->mEnemies;
		bool aFound = false;
		for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
		{
			aList.mCurrent = aList.mCurrent->mNext;
			if (aList.Current() == mTarget)
			{
				aFound = true;
				break;
			}
		}
		if (!aFound)
			mTarget = NULL;
	}

	if (mHitsLeft == 0 || mTrailX[0] < -420.0 || 420.0 < mTrailX[0] || mTrailY[0] < -100.0 || 580.0 < mTrailY[0])
	{
		Remove();
		return;
	}

	for (int i = TRAIL_LENGTH - 1; i > 0; i--)
	{
		mTrailX[i] = mTrailX[i - 1];
		mTrailY[i] = mTrailY[i - 1];
	}
	mTrailX[1] = (double)(Rand() % 2000) * 0.01 - 10.0 + mTrailX[1];
	mTrailY[1] = (double)(Rand() % 2000) * 0.01 - 10.0 + mTrailY[1];

	if (mTarget == NULL)
	{
		mVelX = -mTrailX[0];
		mVelY = -120.0 - mTrailY[0];
	}
	else
	{
		mVelX = mTarget->mX - mTrailX[0];
		mVelY = mTarget->mY - mTrailY[0];
	}
	double aLength = sqrt(mVelY * mVelY + mVelX * mVelX);
	if (aLength > 20.0)
	{
		mVelX = mVelX / aLength * 20.0;
		mVelY = mVelY / aLength * 20.0;
	}
	mTrailX[0] = mTrailX[0] + mVelX;
	mTrailY[0] = mTrailY[0] + mVelY;

	if (mTarget != NULL && mTrailX[0] == mTarget->mX && mTrailY[0] == mTarget->mY)
	{
		Enemy* aTarget = mTarget;
		aTarget->mHealth -= (double)mBoard->mApp->mWeapons[HeavyWeaponApp::WEAPON_STATIC];
		aTarget->mHitFlash = 255;
		mBoard->mParticles.Emit(mBoard->mApp->mSprites[IMAGE_STATICSPARK], aTarget->mX, aTarget->mY, aTarget->mVelX,
			aTarget->mVelY, (int)(mBoard->mApp->mParticles * 5.0), 3.0, 0.05, 3.0, 100, true, -1);
		mBoard->mApp->PlaySample(SOUND_STATICHIT, (int)(aTarget->mX * 3.0));
		Spark* aSpark = new Spark();
		mBoard->mSparks.Append(aSpark);
		aSpark->mX = aTarget->mX;
		aSpark->mY = aTarget->mY;
		aSpark->mVelX = aTarget->mVelX;
		aSpark->mVelY = aTarget->mVelY;
		aSpark->mBrightness = 0;
		aSpark->mFrame = 0;
		aSpark->mLife = 20;
		if (aTarget->mHealth < 0.0)
		{
			aTarget->Killed();
			if (aTarget != mBoard->mBoss)
				aTarget->Remove();
		}
		mTarget = FindTarget();
		mHitsLeft--;
	}
}

//
// HomingMissile
//

// 0x449830
HomingMissile::HomingMissile(Board* theBoard, double x, double y) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mFrame = 5.0;
	mIsDestructible = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_MISSILE];
	mBoard->mApp->PlaySample(SOUND_SMALLMISSILE, (int)(x * 3.0));
	EmitLaunchSmoke(mBoard, mX, mY, 0.0, 0.0);
}

// 0x449920
HomingMissile::~HomingMissile()
{
}

// 0x44a520: the nearest living enemy within a heading of the missile's (and not below it) that is not the player's
Enemy* HomingMissile::FindTarget()
{
	double aBest = SEARCH_START;
	Enemy* aResult = NULL;
	ObjectList<Enemy>& aList = mBoard->mEnemies;
	ObjectList<Enemy>::Node* aSavedCurrent = aList.mCurrent;
	for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
	{
		aList.mCurrent = aList.mCurrent->mNext;
		Enemy* anEnemy = aList.Current();
		double aDistance = sqrt(pow(anEnemy->mY - mY, 2.0) + pow(anEnemy->mX - mX, 2.0));
		if (aDistance < aBest &&
			(double)abs((int)(mFrame * HEADING_PER_FRAME - (atan2(mX - anEnemy->mX, mY - anEnemy->mY) + HEADING_OFFSET))) <
				3.142 &&
			!anEnemy->mIsFriendly && anEnemy->mY < mY + 1.0 && anEnemy->mIsAlive)
		{
			aBest = aDistance;
			aResult = anEnemy;
		}
	}
	aList.mCurrent = aSavedCurrent;
	return aResult;
}

// 0x44b510: it flies along its heading, steers toward its target, and hits the first living enemy that is not the
// player's it touches
void HomingMissile::Update()
{
	double anAngle = mFrame * HEADING_PER_FRAME;
	mVelX = cos(anAngle) * 4.0;
	mVelY = sin(anAngle) * 4.0;
	mX = mX + mVelX;
	mY = mY - mVelY;
	EmitTrail(mBoard, mX - mVelX * 5.0, mVelY * 5.0 + mY, mVelX * 0.5, mVelY * -0.5);

	Enemy* aTarget = FindTarget();
	if (aTarget != NULL)
	{
		double anAim = atan2(mX - aTarget->mX, mY - aTarget->mY) + HEADING_OFFSET;
		if (anAngle < anAim)
			mFrame = mFrame + 0.1;
		if (anAim < anAngle)
			mFrame = mFrame - 0.1;
		if (mFrame < 0.0)
			mFrame = 0.0;
		if (10.0 < mFrame)
			mFrame = 10.0;
	}

	ObjectList<Enemy>& aList = mBoard->mEnemies;
	for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
	{
		aList.mCurrent = aList.mCurrent->mNext;
		Enemy* anEnemy = aList.Current();
		if (anEnemy->mIsAlive && !anEnemy->mIsFriendly && TouchesEnemy(this, anEnemy))
		{
			HitEnemy(this, anEnemy, 15.0);
			return;
		}
	}
	FinishShot(this);
}

//
// Rocket
//

// 0x449c80: it starts at the gun, 12 pixels out along its heading and theSideOffset to its side
Rocket::Rocket(Board* theBoard, double x, double y, int theSideOffset) : Projectile(theBoard)
{
	mFrame = mBoard->mTurretFrame;
	mIsDestructible = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_ROCKET];
	double aHeading = (mFrame * 9.0 - 4.0) * 0.017453292520882225;
	mX = (mBoard->mTankX - cos(aHeading) * 12.0) - cos(aHeading + 1.5707963268794) * (double)theSideOffset;
	mY = ((double)(mBoard->mTankY - 12) - sin(aHeading) * 12.0) - sin(aHeading + 1.5707963268794) * (double)theSideOffset;
	mVelX = cos(aHeading) * -4.0;
	mVelY = sin(aHeading) * 4.0;
	EmitLaunchSmoke(mBoard, mX, mY, mVelX * -0.5, mVelY * 0.5);
}

// 0x449df0
Rocket::~Rocket()
{
}

// 0x44a6a0: it flies straight, and hits the first living enemy it touches
void Rocket::Update()
{
	mX = mX + mVelX;
	mY = mY - mVelY;
	EmitTrail(mBoard, mX - mVelX * 5.0, mVelY * 5.0 + mY, mVelX * 0.5, mVelY * -0.5);

	ObjectList<Enemy>& aList = mBoard->mEnemies;
	for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
	{
		aList.mCurrent = aList.mCurrent->mNext;
		Enemy* anEnemy = aList.Current();
		if (anEnemy->mIsAlive && TouchesEnemy(this, anEnemy))
		{
			HitEnemy(this, anEnemy, 6.0);
			return;
		}
	}
	FinishShot(this);
}

//
// FlakBurst
//

// 0x44a220
FlakBurst::FlakBurst(Board* theBoard)
{
	mBoard = theBoard;
	mX = (double)(mBoard->mMouseX - FIELD_EDGE_X);
	mY = (double)mBoard->mMouseY;
	mBurstX = (int)mX;
	mBurstY = (int)mY;
	mPhase = 0.0;
	mBurstIndex = 0;
	int aLevel = mBoard->mApp->mWeapons[HeavyWeaponApp::WEAPON_FLAK];
	mMaxBursts = (aLevel + 1) * 5;
	mBurstRadius = aLevel * 8 + 8;
	mNode = mBoard->mTankShots.Append(this);
	mBoard->mApp->PlaySample(SOUND_FLAK, (int)(mX * 3.0));
}

// 0x448620
FlakBurst::~FlakBurst()
{
}

// 0x448630
void FlakBurst::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	mBoard->mApp->mSprites[IMAGE_FLAKFLASH]->Draw(g, mBurstX, mBurstY, true, (int)mPhase, 0, false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x44b120: on its first frame a flash hurts the enemies it touches (0.75 each) and the projectiles (0.5), and leaves
// smoke; the flash then runs 5 frames and the next burst is at a random point on the circle round the mouse's place,
// until the last
void FlakBurst::Update()
{
	if (mPhase == 0.0)
	{
		Sprite* aFlash = mBoard->mApp->mSprites[IMAGE_FLAKFLASH];
		ObjectList<Enemy>& aList = mBoard->mEnemies;
		for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
		{
			aList.mCurrent = aList.mCurrent->mNext;
			Enemy* anEnemy = aList.Current();
			if (anEnemy->mIsAlive && aFlash->Collides(mBurstX, mBurstY, 0, 0, false, anEnemy->GetSprite(), (int)anEnemy->mX,
				(int)anEnemy->mY, (int)anEnemy->mFrame, 0, anEnemy->mMirrored, true))
			{
				anEnemy->mHitFlash = 255;
				anEnemy->mHealth -= 0.75;
				if (anEnemy->mHealth >= 0.0)
				{
					mBoard->mApp->PlaySample(SOUND_BULLETHIT, mBurstX);
				}
				else
				{
					anEnemy->Killed();
					if (anEnemy != mBoard->mBoss)
						anEnemy->Remove();
				}
			}
		}
		mBoard->HitProjectiles(aFlash, mBurstX, mBurstY, 0, 0, 0.5);
		mBoard->mParticles.Emit(mBoard->mApp->mSprites[IMAGE_SMOKE], (double)mBurstX, (double)mBurstY, -1.0, 0.0, 1, 0.0,
			-0.01, -0.1, 120, true, -1);
	}

	mPhase = mPhase + 1.0;
	if (mPhase > 4.0)
	{
		mPhase = 0.0;
		mBurstIndex++;
		if (mMaxBursts < mBurstIndex)
		{
			mBoard->mTankShots.Remove(mNode);
			return;
		}
		double anAngle = (double)(Rand() % 6282) * 0.001;
		Rand();
		mBurstX = (int)(cos(anAngle) * (double)mBurstRadius + mX);
		mBurstY = (int)(sin(anAngle) * (double)mBurstRadius + mY);
	}
}
