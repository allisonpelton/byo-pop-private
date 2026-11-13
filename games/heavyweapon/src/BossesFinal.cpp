// BossesFinal.cpp: the final boss (images\finalboss): a flying fortress that comes down from above the field and
// fights in three phases, each with new weapons: lasers that drop bomb fragments, then a spinning turret and fat boys,
// then hellfire and head missiles.
// Original: 0x43c1a0 (destructor body), 0x43c240 Draw, 0x43c480 Killed, 0x43c510 Explode, 0x43c620 Update, 0x43d480 its
// data, 0x43de30 constructor.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"

#include <math.h>

using namespace Sexy;

namespace
{
	const double START_Y = -140.0;				// it comes down from above the field
	const double START_VEL_Y = 0.5;
	const double START_TARGET_Y = 160.0;
	const int START_ATTACK_DELAY = 80;
	const int FIRST_LASER_KIND = 2;
	const double ARRIVAL_Y = 150.0;				// it fights once it is below this

	const double TARGET_NEAR = 10.0;			// in phase 0 it fires when it is this near its target
	const double LASER_FIRE_MIN = 1.5;			// bomb fragments leave while the laser's charge is in this range
	const double LASER_FIRE_MAX = 1.54;
	const int NUM_LASER_KINDS = 3;
	const double LASER_LEFT_X = -156.0;			// the bomb fragments leave from either laser, 10 below its middle
	const double LASER_RIGHT_X = 157.0;
	const double LASER_Y = 10.0;
	const double LASER_STEP = 0.04;
	const double LASER_CYCLE = 0.5;				// the turret turns by this much of its charge's sine
	const double LASER_FRAMES = 5.0;			// the laser sprite's frames, which its turn wraps round
	const double PI = 3.141592653589793;
	const int TARGET_X_RANGE_0 = 200;			// phase 0's targets: x within 100 of the middle, y 140 to 179
	const double TARGET_X_MIN_0 = 100.0;
	const int TARGET_Y_RANGE_0 = 40;
	const double TARGET_Y_MIN_0 = 140.0;
	const double MIN_TARGET_DISTANCE = 60.0;
	const double EASE = 0.2;					// phase 0 eases toward its target at this part of the way an update
	const double EASE_ACCEL_X = 0.1;
	const double EASE_ACCEL_Y = 0.03;

	const int RETARGET_INTERVAL = 50;			// phases 1 and 2 pick a new target every 50 updates
	const int TARGET_X_RANGE = 400;
	const double TARGET_X_MIN = 200.0;
	const int TARGET_Y_RANGE = 40;
	const double TARGET_Y_MIN = 20.0;
	const int TARGET_Y_BASE_OFFSET = 16;		// its target's y is 10 times (phase + 16), plus or minus 20
	const int TARGET_Y_BASE_SCALE = 10;
	const double DRIFT_MAX_SPEED_X = 2.0;
	const double DRIFT_ACCEL_X = 0.04;
	const double DRIFT_MAX_SPEED_Y = 0.5;
	const double DRIFT_ACCEL_Y = 0.01;

	const double PHASE_1_HEALTH = 0.66;			// the part of its health at which each phase begins
	const double PHASE_2_HEALTH = 0.33;

	const double TURRET_SPIN = 0.2;				// phase 1: the turret's frames an update, of 16
	const double TURRET_FRAMES = 16.0;
	const double TURRET_ANGLE_PER_FRAME = 0.19630938358853553;
	const double TURRET_RADIUS = 45.0;
	const double TURRET_CENTRE_Y = 84.0;
	const double TURRET_SHOT_SPEED = 2.5;
	const double TURRET_SHOT_GRAVITY = 0.1;
	const double TURRET_SHOT_OPPOSITE = 3.141;	// its second shot goes the other way
	const double FAT_BOY_OFFSET_Y = 100.0;

	const double HELLFIRE_LEFT_X = -84.0;		// phase 2
	const double HELLFIRE_RIGHT_X = 85.0;
	const double HELLFIRE_Y = 3.0;
	const double HELLFIRE_SCALE = 1.0;
	const double HEAD_MISSILE_LEFT_X = -52.0;
	const double HEAD_MISSILE_RIGHT_X = 53.0;
	const double HEAD_MISSILE_Y = -52.0;

	const int SHAKE = 0x80;						// its explosion shakes the screen
	const double EXPLOSION_SCALE = 1.5;
	const double EXPLOSION_SPEED = 0.02;
	const double DEBRIS_GRAVITY = 0.02;
	const int NUM_DEBRIS = 6;

	// Its smoke trails from its thrusters, a particle every other update (x and y from its centre)
	const double SMOKE_SPREAD = 0.5;
	const double SMOKE_FALL = 2.0;
	const int SMOKE_LIFE = 20;
	const double SMOKE_PHASE_0_X[6] = { 92.0, -91.0, 71.0, -70.0, 42.0, -41.0 };
	const double SMOKE_PHASE_0_Y[6] = { 105.0, 105.0, 109.0, 109.0, 110.0, 110.0 };
	const double SMOKE_PHASE_1_X[2] = { 98.0, -97.0 };
	const double SMOKE_PHASE_1_Y = 20.0;
	const double SMOKE_PHASE_2_X[2] = { 75.0, -74.0 };
	const double SMOKE_PHASE_2_Y = 64.0;

	const int LASER_OFFSET_LEFT_X = 156;		// Draw: its two lasers
	const int LASER_OFFSET_RIGHT_X = 157;
	const int LASER_OFFSET_Y = 15;
	const int TURRET_OFFSET_Y = 84;
}

// 0x43de30: its data and sprites; the first body is its sprite
FinalBoss::FinalBoss(Board* theBoard) : Enemy(theBoard)
{
	BossData aData("Final", "Level");					// 0x43d480
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mLaserFire = aData.Int("Laser", "fire");
	mTurretFire = aData.Int("Turret", "fire");
	mSmallLauncherFire = aData.Int("SmallLauncher", "fire");
	mBigLauncherFire = aData.Int("BigLauncher", "fire");
	mBombFire = aData.Int("Bomb", "fire");

	mMaxHealth = mHealth;
	mLastHealth = mHealth;
	mBody1 = new Sprite(mApp);
	mBody1->Load("images\\finalboss\\body1", 1, 1);
	mBody2 = new Sprite(mApp);
	mBody2->Load("images\\finalboss\\body2", 1, 1);
	mBody3 = new Sprite(mApp);
	mBody3->Load("images\\finalboss\\body3", 1, 1);
	mTurretSprite = new Sprite(mApp);
	mTurretSprite->Load("images\\finalboss\\turret", 16, 1);
	mLaserSprite = new Sprite(mApp);
	mLaserSprite->Load("images\\finalboss\\laser", 5, 1);
	mLaserGlowSprite = new Sprite(mApp);
	mLaserGlowSprite->Load("images\\finalboss\\laserglow", 5, 1);

	mSprite = mBody1;
	mSmallLauncherDelay = mSmallLauncherFire;
	mBigLauncherDelay = mBigLauncherFire;
	mX = 0.0;
	mY = START_Y;
	mVelX = 0.0;
	mVelY = START_VEL_Y;
	mTargetX = 0.0;
	mTargetY = START_TARGET_Y;
	mPhase = 0;
	mTurretFrame = 0.0;
	mAttackDelay = START_ATTACK_DELAY;
	mLaserCharge = 0.0;
	mIsAlive = false;
	mBombDelay = mBombFire;
	mLastLaserKind = FIRST_LASER_KIND;
}

// 0x43c1a0 (deleting 0x43c5f0): its sprite is the first body, which it deletes there
FinalBoss::~FinalBoss()
{
	delete mBody1;
	delete mBody2;
	delete mBody3;
	delete mTurretSprite;
	delete mLaserSprite;
	delete mLaserGlowSprite;
}

// port: the smoke of its thrusters, which every other update leaves behind it
void FinalBoss::EmitSmoke(double theOffsetX, double theOffsetY, double theVelX)
{
	mBoard->mParticles.Emit(mApp->mSprites[IMAGE_METEORITE], mX + theOffsetX, mY + theOffsetY, theVelX,
		mVelY + SMOKE_FALL, 1, SMOKE_SPREAD, 0.0, 0.0, SMOKE_LIFE, true, -1);
}

// 0x43c240: the body, then in phase 0 its two lasers (the right one mirrored, lit while the charge builds) and in
// phase 1 its turret
void FinalBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	Enemy::Draw(g);
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}

	if (mPhase == 0)
	{
		int aFrame = (int)mTurretFrame;
		mLaserSprite->Draw(g, (int)mX - LASER_OFFSET_LEFT_X, (int)mY - LASER_OFFSET_Y, true, aFrame, 0, false);
		mLaserSprite->Draw(g, (int)mX + LASER_OFFSET_RIGHT_X, (int)mY - LASER_OFFSET_Y, true, aFrame, 0, true);
		if (0.0 < mLaserCharge)
		{
			bool wasColorized = g->mColorizeImages;
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			int aBrightness = (int)(sin(mLaserCharge) * 255.0);
			g->SetColor(Color(aBrightness, aBrightness, aBrightness));
			int aGlowFrame = (int)mTurretFrame;
			mLaserGlowSprite->Draw(g, (int)mX - LASER_OFFSET_LEFT_X, (int)mY - LASER_OFFSET_Y, true, aGlowFrame, 0, false);
			mLaserGlowSprite->Draw(g, (int)mX + LASER_OFFSET_RIGHT_X, (int)mY - LASER_OFFSET_Y, true, aGlowFrame, 0, true);
			g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(wasColorized);
		}
	}
	else if (mPhase == 1)
	{
		mTurretSprite->Draw(g, (int)mX, (int)mY - TURRET_OFFSET_Y, true, (int)mTurretFrame, 0, false);
		g->SetColorizeImages(false);
		return;
	}
	g->SetColorizeImages(false);
}

// 0x43c480
void FinalBoss::Killed()
{
	if (mBoard->mBossDeath != NULL)
		return;
	Enemy::Killed();
	mBoard->AddScore(mPoints, (int)mX, (int)mY);
	mBoard->mBossDeath = new BossDeath(mBoard);
}

// 0x43c510: a phase's end: an explosion half as big again as its body, the screen shaking, and six pieces of debris
void FinalBoss::Explode()
{
	int aHeight = (int)((double)mSprite->mCelHeight * EXPLOSION_SCALE);
	int aWidth = (int)((double)mSprite->mCelWidth * EXPLOSION_SCALE);
	mBoard->AddExplosion(mX, mY, aWidth, aHeight, 0.0, 0.0, EXPLOSION_SPEED);
	mBoard->mShake = SHAKE;
	for (int i = 0; i < NUM_DEBRIS; i++)
		mBoard->mParticles.Emit(mApp->mSprites[IMAGE_BIGDEBRIS], mX, mY, 0.0, 0.0, 2, 2.0, DEBRIS_GRAVITY, 4.0, 300, false, i);
}

// 0x43c620: in phase 0 it flies to a place within reach of the middle and holds there to charge its two lasers, which
// drop bomb fragments; below two thirds of its health it turns to phase 1 (a spinning turret of shots, and fat boys), below
// a third to phase 2 (hellfire and head missiles). In phases 1 and 2 it drifts about the top of the field.
void FinalBoss::Update()
{
	Board* aBoard = mBoard;
	if (aBoard->mLevelTime < aBoard->mLevelLength)
		return;

	if (!mIsAlive)
	{
		if (ARRIVAL_Y < mY)
			mIsAlive = true;
	}
	else if (mPhase == 0)
	{
		if (fabs(mX - mTargetX) < TARGET_NEAR && fabs(mY - mTargetY) < TARGET_NEAR)
		{
			if (aBoard->mTankDeadTime == 0)
				mAttackDelay--;
			else
				mAttackDelay = START_ATTACK_DELAY;

			if (mAttackDelay < START_ATTACK_DELAY && aBoard->mNukeFlash == 0.0)
			{
				if (mLaserCharge == 0.0)
					mApp->PlaySample(SOUND_BOSSLASER, (int)(mX * 3.0));
				if (LASER_FIRE_MIN < mLaserCharge && mLaserCharge < LASER_FIRE_MAX)
				{
					int aKind;
					do
					{
						aKind = Rand() % NUM_LASER_KINDS;
					}
					while (aKind == mLastLaserKind);
					mLastLaserKind = aKind;
					new BombFragment(mBoard, mX + LASER_LEFT_X, mY + LASER_Y, aKind, 1);
					new BombFragment(mBoard, mX + LASER_RIGHT_X, mY + LASER_Y, aKind, -1);
					mApp->PlaySample(SOUND_BOSSLASERBLAST, (int)(mX * 3.0));
				}
			}

			if ((mAttackDelay < START_ATTACK_DELAY || (0.0 < mLaserCharge && mBoard->mTankDeadTime != 0)) &&
				(mBoard->mNukeFlash == 0.0 || mLaserCharge != 0.0) && mLaserCharge < PI)
			{
				double aFrame = sin(mLaserCharge) * LASER_CYCLE + mTurretFrame;
				mTurretFrame = aFrame;
				if (LASER_FRAMES <= aFrame)
					mTurretFrame = aFrame - LASER_FRAMES;
				double aCharge = mLaserCharge;
				mLaserCharge = aCharge + LASER_STEP;
				if (PI < aCharge + LASER_STEP)
					mLaserCharge = PI;
			}

			if (mAttackDelay == 0)
			{
				do
				{
					mTargetX = (double)(Rand() % TARGET_X_RANGE_0) - TARGET_X_MIN_0;
					mTargetY = (double)(Rand() % TARGET_Y_RANGE_0) + TARGET_Y_MIN_0;
				}
				while (sqrt(pow(mTargetY - mY, 2.0) + pow(mTargetX - mX, 2.0)) < MIN_TARGET_DISTANCE);
				mLaserCharge = 0.0;
				mAttackDelay = START_ATTACK_DELAY;
			}
		}

		double aGoalVelX = (mTargetX - mX) * EASE;
		double aGoalVelY = (mTargetY - mY) * EASE;
		if (mVelX < aGoalVelX)
			mVelX = mVelX + EASE_ACCEL_X;
		if (aGoalVelX < mVelX)
			mVelX = mVelX - EASE_ACCEL_X;
		if (mVelY < aGoalVelY)
			mVelY = mVelY + EASE_ACCEL_Y;
		if (aGoalVelY < mVelY)
			mVelY = mVelY - EASE_ACCEL_Y;
		if (fabs(aGoalVelX) < fabs(mVelX))
			mVelX = aGoalVelX;
		if (fabs(aGoalVelY) < fabs(mVelY))
			mVelY = aGoalVelY;
	}
	else
	{
		if (mApp->mUpdateCount % RETARGET_INTERVAL == 0)
		{
			mTargetX = (double)(Rand() % TARGET_X_RANGE) - TARGET_X_MIN;
			mTargetY = (double)((mPhase + TARGET_Y_BASE_OFFSET) * TARGET_Y_BASE_SCALE) +
				((double)(Rand() % TARGET_Y_RANGE) - TARGET_Y_MIN);
		}
		if (mX < mTargetX && mVelX < DRIFT_MAX_SPEED_X)
			mVelX = mVelX + DRIFT_ACCEL_X;
		if (mTargetX < mX && -DRIFT_MAX_SPEED_X < mVelX)
			mVelX = mVelX - DRIFT_ACCEL_X;
		if (mY < mTargetY && mVelY < DRIFT_MAX_SPEED_Y)
			mVelY = mVelY + DRIFT_ACCEL_Y;
		if (mTargetY < mY && -DRIFT_MAX_SPEED_Y < mVelY)
			mVelY = mVelY - DRIFT_ACCEL_Y;
	}

	// the phases change as its health crosses two thirds and one third
	if (mLastHealth <= mMaxHealth * PHASE_1_HEALTH || mMaxHealth * PHASE_1_HEALTH < mHealth)
	{
		if (mMaxHealth * PHASE_2_HEALTH < mLastHealth && mHealth <= mMaxHealth * PHASE_2_HEALTH)
		{
			Explode();
			mPhase = 2;
			mSprite = mBody3;
		}
	}
	else
	{
		Explode();
		mPhase = 1;
		mSprite = mBody2;
		mTurretFrame = 0.0;
		mAttackDelay = mTurretFire;
	}

	mX = mVelX + mX;
	mLastHealth = mHealth;
	mY = mVelY + mY;

	if (mPhase == 0)
	{
		if (mApp->mUpdateCount % 2 == 0)
		{
			for (int i = 0; i < 6; i++)
				EmitSmoke(SMOKE_PHASE_0_X[i], SMOKE_PHASE_0_Y[i], mVelX);
		}
	}
	else if (mPhase == 1)
	{
		mTurretFrame = mTurretFrame + TURRET_SPIN;
		if (TURRET_FRAMES <= mTurretFrame)
			mTurretFrame = mTurretFrame - TURRET_FRAMES;
		if (mApp->mUpdateCount % 2 == 0)
		{
			for (int i = 0; i < 2; i++)
				EmitSmoke(SMOKE_PHASE_1_X[i], SMOKE_PHASE_1_Y, mVelX);
		}

		aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0 && mIsAlive)
		{
			mAttackDelay--;
			mBombDelay--;
		}
		else
		{
			mAttackDelay = mTurretFire;
			mBombDelay = mBombFire;
		}
		if (aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
		{
			if (mBombDelay < 1)
			{
				new FatBoy(mBoard, mX, mY + FAT_BOY_OFFSET_Y, mVelX, mVelY, false, true);
				mBombDelay = mBombFire;
			}
			if (mAttackDelay < 1)
			{
				double anOffset = 0.0;
				do
				{
					double aFrame = mTurretFrame;
					Bullet* aBullet = new Bullet();
					mBoard->mBullets.Append(aBullet);
					double anAngle = aFrame * TURRET_ANGLE_PER_FRAME + anOffset;
					double aCos = cos(anAngle);
					aBullet->mX = TURRET_RADIUS * aCos + mX;
					double aSin = sin(anAngle);
					aBullet->mGravity = TURRET_SHOT_GRAVITY;
					aBullet->mIsEnemy = true;
					aBullet->mY = (mY - TURRET_CENTRE_Y) - aSin * TURRET_RADIUS;
					aBullet->mVelX = aCos * TURRET_SHOT_SPEED;
					aBullet->mVelY = aSin * -TURRET_SHOT_SPEED;
					aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
					aBullet->mColumn = 0;
					aBullet->mRow = 0;
					mBoard->AddMuzzleFlash(aBullet->mX, aBullet->mY, mVelX, mVelY);
					anOffset = anOffset + TURRET_SHOT_OPPOSITE;
					mAttackDelay = mTurretFire;
				}
				while (anOffset <= TURRET_SHOT_OPPOSITE);
			}
		}
	}
	else if (mPhase == 2)
	{
		if (mApp->mUpdateCount % 2 == 0)
		{
			EmitSmoke(SMOKE_PHASE_2_X[0], SMOKE_PHASE_2_Y, mVelX + 0.5);
			EmitSmoke(SMOKE_PHASE_2_X[1], SMOKE_PHASE_2_Y, mVelX - 0.5);
		}

		aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0 && mIsAlive)
			mSmallLauncherDelay--;
		else
			mSmallLauncherDelay = mSmallLauncherFire;
		if (mSmallLauncherDelay < 1 && aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
		{
			new Hellfire(mBoard, mX + HELLFIRE_LEFT_X, mY + HELLFIRE_Y, mVelX, mVelY, HELLFIRE_SCALE);
			new Hellfire(mBoard, mX + HELLFIRE_RIGHT_X, mY + HELLFIRE_Y, mVelX, mVelY, HELLFIRE_SCALE);
			mSmallLauncherDelay = mSmallLauncherFire;
		}

		aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0 && mIsAlive)
			mBigLauncherDelay--;
		else
			mBigLauncherDelay = mBigLauncherFire;
		if (mBigLauncherDelay < 1 && aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
		{
			new HeadMissile(mBoard, mX + HEAD_MISSILE_LEFT_X, mY + HEAD_MISSILE_Y, true);
			new HeadMissile(mBoard, mX + HEAD_MISSILE_RIGHT_X, mY + HEAD_MISSILE_Y, false);
			mBigLauncherDelay = mBigLauncherFire;
		}
	}
	Enemy::Update();
}
