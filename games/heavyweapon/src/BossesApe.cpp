// BossesApe.cpp: the ape boss (images\Ape): it stands at the right edge, throws bursting rockets three times, then
// jumps across the field, and stomps the tank it lands on.
// Original: 0x437de0 (destructor body), 0x437e50 Update, 0x4382a0 Draw, 0x438340 Killed, 0x438400 its data, 0x438be0
// constructor.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"

using namespace Sexy;

namespace
{
	const double START_X = 380.0;				// it starts off the right edge
	const double GROUND_Y = 315.0;				// its feet are on the ground here
	const int SHADOW_Y = 455;
	const int SHADOW_OFFSET_X = 20;				// its shadow is 20 to the left of it, or 20 to the right facing left
	const double SCROLL_ARRIVAL_SPEED = 0.1;	// it walks in until the scroll has slowed to this
	const double SCROLL_ARRIVAL_FACTOR = 1.35;	// and moves this much of the scroll speed an update
	const int START_WAIT = 100;					// updates it stands before its first throw
	const int JUMPS_AFTER_THROWS = 3;			// throws between jumps
	const int LANDING_CROUCH_TIME = 50;			// updates it crouches before a jump
	const int CENTRE_RANGE = 100;				// it crouches again, not turns, where it lands this near the middle
	const double FALL_FRAME_SPEED = 0.5;		// it shows its falling frame once it falls this fast
	const double THROW_END_FRAME = 5.9;			// its throw is over at this frame, the rocket thrown at frame 4
	const double THROW_FRAME = 4.0;
	const double ROCKET_HEIGHT = 120.0;			// the rocket starts this far above its feet
	const int ROCKET_SPEED = 3;					// the rocket's sideways speed, away from the way it faces
	const int ROCKET_BULLETS = 6;
	const int LANDING_SHAKE = 0x80;				// the screen shakes this hard when it lands
	const int SHAKE_DECAY = 2;
	const int FRAME_CROUCH = 6;
	const int FRAME_JUMP = 7;
	const int FRAME_FALL = 8;
}

// 0x438be0: it loads its armour, points, jump and throw speed from the data, and its sprites
ApeBoss::ApeBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Ape", aBlock);						// 0x438400
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mThrowSpeed = aData.Double("", "throw");
	mJumpVelX = aData.Double("Jump", "xspeed");
	mJumpVelY = aData.Double("Jump", "yspeed");
	mGravity = aData.Double("Jump", "gravity");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\Ape\\Ape", 9, 1);
	mShadow = new Sprite(mApp);
	mShadow->Load("images\\Ape\\shadow", 1, 1);
	mMirrored = true;
	mX = START_X;
	mY = GROUND_Y;
	mVelX = 0.0;
	mVelY = 0.0;
	mIsThrowing = false;
	mWaitTime = START_WAIT;
	mLandTime = 0;
	mJumpCount = 0;
	mShake = 0;
	mIsAlive = false;
}

// 0x437de0 (deleting 0x4383d0)
ApeBoss::~ApeBoss()
{
	delete mSprite;
	delete mShadow;
}

// 0x437e50: once the level has run out it walks in with the ground, then waits, throws bursting rockets at the tank
// three times, and jumps (crouching, then up and across); it turns where it lands, away from the middle, and crouches
// for the next jump where it lands near it. The tank dies under it if they touch.
void ApeBoss::Update()
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	Enemy::Update();
	Board* aBoard = mBoard;
	if (SCROLL_ARRIVAL_SPEED <= aBoard->mScrollSpeed)
	{
		mX = mX - aBoard->mScrollSpeed * SCROLL_ARRIVAL_FACTOR;
		return;
	}

	mIsAlive = true;
	mX = mX + mVelX;
	if (aBoard->mTankDeadTime != 0)
		mJumpCount = JUMPS_AFTER_THROWS;

	if (mWaitTime == 0)
	{
		if (!mIsThrowing || aBoard->mBossDeath != NULL)
		{
			if (mLandTime == 0)
			{
				// in the air
				mVelY = mGravity + mVelY;
				if (mJumpVelY < mVelY)
					mVelY = mJumpVelY;
				if (FALL_FRAME_SPEED < mVelY)
					mFrame = (double)FRAME_FALL;
				mY = mVelY + mY;
				if (GROUND_Y < mY)
				{
					if (aBoard->mBossDeath == NULL)
					{
						mShake = LANDING_SHAKE;
						mApp->PlaySample(SOUND_BIGTHUD, (int)(mX * 3.0));
					}
					mY = GROUND_Y;
					mVelX = 0.0;
					mVelY = 0.0;
					mFrame = 0.0;
					int aDistance = (int)mX;
					if (aDistance < 0)
						aDistance = -aDistance;
					if (aDistance < CENTRE_RANGE)
					{
						mLandTime = LANDING_CROUCH_TIME;
						mFrame = (double)FRAME_CROUCH;
					}
					else
					{
						mWaitTime = 1;
						mFrame = 0.0;
						if (aBoard->mBossDeath == NULL)
							mMirrored = !mMirrored;
					}
				}
			}
			else
			{
				// crouching: at the end it jumps, toward the way it faces
				mLandTime--;
				if (mLandTime == 0)
				{
					mFrame = (double)FRAME_JUMP;
					mVelY = -mJumpVelY;
					mVelX = -((double)((mMirrored ? 1 : 0) * 2 - 1) * mJumpVelX);
				}
			}
		}
		else
		{
			// throwing: the rocket leaves at frame 4
			mFrame = mFrame + mThrowSpeed;
			if (THROW_END_FRAME < mFrame)
			{
				mFrame = 0.0;
				mJumpCount++;
				mWaitTime = 1;
			}
			if (THROW_FRAME <= mFrame && mFrame - mThrowSpeed < THROW_FRAME && aBoard->mBossDeath == NULL)
			{
				double aVelX = (double)((mMirrored ? 1 : 0) * -2 * ROCKET_SPEED + ROCKET_SPEED);
				new BurstRocket(mBoard, mX, mY - ROCKET_HEIGHT, aVelX, mMirrored, ROCKET_BULLETS);
			}
		}
	}
	else
	{
		mWaitTime--;
		if (mWaitTime == 0)
		{
			if (mJumpCount == JUMPS_AFTER_THROWS)
			{
				mLandTime = LANDING_CROUCH_TIME;
				mFrame = (double)FRAME_CROUCH;
				mJumpCount = 0;
				mIsThrowing = false;
			}
			else
				mIsThrowing = true;
		}
	}

	// the screen shakes while it settles (the board's own shake does the same with its own counter)
	if (mShake != 0)
	{
		int aShake = mShake;
		int aY = Rand() % (aShake / 16 + 1) - aShake / 32;
		int aX = Rand() % (aShake / 16 + 1) - aShake / 32;
		mBoard->Move(aX, aY);
		mShake -= SHAKE_DECAY;
	}

	// with the tank dead it is gone from under the ape's next landing (the board goes on from state 2 to 3)
	if (mBoard->mTankDeadTime == 2 &&
		((mJumpCount == JUMPS_AFTER_THROWS && mMirrored) || (!mMirrored && mX < 0.0)))
		mBoard->mTankDeadTime = 3;

	aBoard = mBoard;
	if (aBoard->mBossDeath == NULL && aBoard->mTankDeadTime == 0)
	{
		if (mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, mApp->mSprites[IMAGE_TANK],
			(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
		{
			mBoard->KillTank();
			return;
		}
	}
}

// 0x4382a0: its shadow, fading as it climbs, then the ape
void ApeBoss::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(255.0 - (GROUND_Y - mY))));
	mShadow->Draw(g, (int)((double)((mMirrored ? 1 : 0) * 2 * SHADOW_OFFSET_X - SHADOW_OFFSET_X) + mX), SHADOW_Y, true, 0, 0, false);
	g->SetColorizeImages(false);
	Enemy::Draw(g);
}

// 0x438340: the points, and its death
void ApeBoss::Killed()
{
	if (mBoard->mBossDeath != NULL)
		return;
	Enemy::Killed();
	mBoard->AddScore(mPoints, (int)mX, (int)mY);
	mBoard->mBossDeath = new BossDeath(mBoard);
}
