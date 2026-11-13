// BossesBattleship.cpp: the battleship boss (images\battleship): a ship that sails in from the right with four gun
// turrets, which fire down at the tank, and a launcher, which fires hellfire missiles.
// Original: 0x438d80 (destructor body), 0x438df0 Update, 0x439000 Draw, 0x439060 Killed, 0x439120 its data, 0x439e60
// constructor; the turrets 0x43a1c0 (constructor), 0x43a250, 0x43a260 Killed, 0x43a300 Update.

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
	const double START_X = 770.0;				// it sails in from the right
	const double START_Y = 260.0;
	const double START_VEL_X = -1.0;
	const int POINTS = 25000;					// what killing it scores (the data's score is read and then overwritten)
	const double HULL_OFFSET_Y = 89.0;			// its hull is drawn this far below its deck
	const double BRAKING_X = 240.0;				// the ship slows once it has sailed this far in
	const double BRAKE_STEP = 0.005;
	const double LEFT_END_X = -50.0;			// with its left turrets gone it sails this far left, and stops
	const double MAX_SPEED = 1.0;
	const double HELLFIRE_OFFSET_X = 120.0;		// its hellfire missile leaves this far toward the middle of the field
	const double HELLFIRE_Y = 300.0;
	const double HELLFIRE_SCALE = 0.75;

	// where each turret sits from the ship's centre: the left ones are 0 and 1
	const double TURRET_OFFSET_X[BattleshipBoss::NUM_TURRETS] = { -219.0, -306.0, 219.0, 306.0 };
	const double TURRET_OFFSET_Y[BattleshipBoss::NUM_TURRETS] = { 51.0, 47.0, 49.0, 43.0 };

	const double TURRET_RESET_X = 357.0;		// while the ship is farther right than this a turret is whole and at rest
	const double TURRET_REST_FRAME = 10.0;		// its middle frame
	const int NUM_LEFT_TURRETS = 2;
	const double LEFT_SWEEP_MIN = 7.0;			// the left turrets' gun sweeps from this frame to the last
	const int RIGHT_SWEEP_EXTRA = 4;			// the right turrets' from 0 to this many frames past the middle one
	const double TURRET_MIN_SHIP_Y = 50.0;		// a turret fires only while its ship is below this
	const int SHOTS_FIRST_OFFSET = -12;			// a turret fires two shots, 24 apart
	const int SHOTS_STEP = 24;
	const int SHOTS_END_OFFSET = 13;
	const double SHOT_MUZZLE_Y = 17.0;
	const double SHOT_ANGLE_PER_FRAME = 0.6369426751592356;
	const double SHOT_ANGLE_BASE = 0.7854;
	const double SHOT_SPEED = -3.75;
	const double SHOT_GRAVITY = 0.05;
	const double SMOKE_OFFSET_Y = 20.0;
	const int SMOKE_COUNT = 4;
	const int SMOKE_LIFE = 100;
}

// 0x439e60: its data and sprites, then the four turrets (made in the order 0, 2, 1, 3)
BattleshipBoss::BattleshipBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Battleship", aBlock);				// 0x439120
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mTurretArmor = aData.Int("", "turretarmor");
	mTurretFire[0] = aData.Int("Turret1", "fire");
	mTurretSpeed[0] = aData.Double("Turret1", "speed");
	mTurretFire[1] = aData.Int("Turret2", "fire");
	mTurretSpeed[1] = aData.Double("Turret2", "speed");
	mTurretFire[2] = aData.Int("Turret3", "fire");
	mTurretSpeed[2] = aData.Double("Turret3", "speed");
	mTurretFire[3] = aData.Int("Turret4", "fire");
	mTurretSpeed[3] = aData.Double("Turret4", "speed");
	mLauncherFire = aData.Int("Launcher", "fire");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\battleship\\main", 1, 1);
	mHull = new Sprite(mApp);
	mHull->Load("images\\battleship\\hull", 1, 1);
	mGunSprite = new Sprite(mApp);
	mGunSprite->Load("images\\battleship\\gun", 21, 1);

	static const int CREATION_ORDER[NUM_TURRETS] = { 0, 2, 1, 3 };
	for (int i = 0; i < NUM_TURRETS; i++)
	{
		int aTurret = CREATION_ORDER[i];
		mTurrets[aTurret] = new BattleshipTurret(mBoard, this, aTurret);
		mTurrets[aTurret]->mOffsetX = TURRET_OFFSET_X[aTurret];
		mTurrets[aTurret]->mOffsetY = TURRET_OFFSET_Y[aTurret];
	}

	mLauncherDelay = mLauncherFire;
	mX = START_X;
	mY = START_Y;
	mVelX = START_VEL_X;
	mVelY = 0.0;
	mPoints = POINTS;
	mIsAlive = false;
}

// 0x438d80 (deleting 0x4390f0): the turrets are the board's to delete
BattleshipBoss::~BattleshipBoss()
{
	delete mSprite;
	delete mHull;
	delete mGunSprite;
}

// 0x438df0: it sails in and slows to a stop (past the middle of the field, or at its left end once its left turrets
// are both gone); it can be hurt once it has stopped, and then fires a hellfire missile every so often
void BattleshipBoss::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	mX = mX + mVelX;
	if (mTurrets[0] == NULL && mTurrets[1] == NULL)
	{
		if (mX <= LEFT_END_X)
		{
			mVelX = mVelX + BRAKE_STEP;
			if (0.0 < mVelX)
				mVelX = 0.0;
		}
		else
			mVelX = mVelX - BRAKE_STEP;
	}
	else if (mX < BRAKING_X)
	{
		mVelX = mVelX + BRAKE_STEP;
		if (0.0 < mVelX)
			mVelX = 0.0;
	}

	if (mVelX < -MAX_SPEED)
		mVelX = -MAX_SPEED;
	if (MAX_SPEED < mVelX)
		mVelX = MAX_SPEED;
	if (mVelX == 0.0)
		mIsAlive = true;

	if (mLauncherDelay != 0)
		mLauncherDelay--;
	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime != 0)
		mLauncherDelay = mLauncherFire;
	if (mVelX != 0.0 && !mIsAlive)
		return;
	if (mLauncherDelay != 0 || aBoard->mTankDeadTime != 0 || aBoard->mNukeFlash != 0.0 || aBoard->mBossDeath != NULL)
		return;

	if (0.0 <= mX)
		new Hellfire(mBoard, mX - HELLFIRE_OFFSET_X, HELLFIRE_Y, 0.0, 0.0, HELLFIRE_SCALE);
	else
		new Hellfire(mBoard, mX + HELLFIRE_OFFSET_X, HELLFIRE_Y, 0.0, 0.0, HELLFIRE_SCALE);
	mLauncherDelay = mLauncherFire;
}

// 0x439000: its hull, then the ship
void BattleshipBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
	{
		mHull->Draw(g, (int)mX, (int)(mY + HULL_OFFSET_Y), true, 0, 0, false);
		Enemy::Draw(g);
	}
}

// 0x439060
void BattleshipBoss::Killed()
{
	if (mBoard->mBossDeath != NULL)
		return;
	Enemy::Killed();
	mBoard->AddScore(mPoints, (int)mX, (int)mY);
	mBoard->mBossDeath = new BossDeath(mBoard);
}

//
// BattleshipTurret
//

// 0x43a1c0: it is not counted among the enemies spawned, nor (in Killed) among those destroyed
BattleshipTurret::BattleshipTurret(Board* theBoard, BattleshipBoss* theShip, int theIndex) : Enemy(theBoard)
{
	mShip = theShip;
	mIndex = theIndex;
	mHealth = (double)theShip->mTurretArmor;
	mMaxHealth = mHealth;
	mSprite = theShip->mGunSprite;
	mFireDelay = theShip->mTurretFire[theIndex];
	mPoints = 0;
	mIsAlive = false;
	mGunStep = theShip->mTurretSpeed[theIndex];
	mOffsetX = 0.0;
	mOffsetY = 0.0;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
}

// 0x43a250: the sprite is the ship's
BattleshipTurret::~BattleshipTurret()
{
}

// 0x43a260: the ship forgets it. The original then counts the turrets from the first one on while they are there
// and halves the launcher's delay if all four are: after this one's own slot is cleared that never happens.
void BattleshipTurret::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mShip->mTurrets[mIndex] = NULL;
	int aTurrets = 0;
	BattleshipTurret** aTurret = mShip->mTurrets;
	do
	{
		if (*aTurret == NULL)
			break;
		aTurrets++;
		aTurret++;
	}
	while (aTurrets < BattleshipBoss::NUM_TURRETS);
	if (aTurrets == BattleshipBoss::NUM_TURRETS)
		mShip->mLauncherFire = mShip->mLauncherFire / 2;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}

// 0x43a300: it rides on the ship, takes its speed, and once the ship has stopped sweeps its gun to and fro (the left
// turrets over their last frames, the right ones over their first 14), firing two shots at the gun's angle now and then
void BattleshipTurret::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	if (TURRET_RESET_X < mX)
	{
		mFrame = TURRET_REST_FRAME;
		mFireDelay = mShip->mTurretFire[mIndex];
		mHealth = (double)mShip->mTurretArmor;
	}

	BattleshipBoss* aShip = mShip;
	mX = (double)(int)aShip->mX + mOffsetX;
	mY = (double)(int)aShip->mY + mOffsetY;
	mVelX = aShip->mVelX;
	mVelY = aShip->mVelY;
	if (!aShip->mIsAlive)
		return;

	double aFrame = mGunStep + mFrame;
	mIsAlive = true;
	mFrame = aFrame;
	if (mIndex < NUM_LEFT_TURRETS)
	{
		int aColumns = mSprite->mColumns;
		if (aFrame < (double)aColumns)
		{
			if (aFrame < LEFT_SWEEP_MIN)
			{
				mFrame = LEFT_SWEEP_MIN;
				mGunStep = aShip->mTurretSpeed[mIndex];
			}
		}
		else
		{
			mFrame = (double)(aColumns - 1);
			mGunStep = -aShip->mTurretSpeed[mIndex];
		}
	}
	else
	{
		int aMiddle = mSprite->mColumns / 2;
		if (aFrame < (double)(aMiddle + RIGHT_SWEEP_EXTRA))
		{
			if (aFrame < 0.0)
			{
				mFrame = 0.0;
				mGunStep = aShip->mTurretSpeed[mIndex];
			}
		}
		else
		{
			mFrame = (double)(aMiddle + RIGHT_SWEEP_EXTRA - 1);
			mGunStep = -aShip->mTurretSpeed[mIndex];
		}
	}

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0)
		mFireDelay--;
	else
		mFireDelay = aShip->mTurretFire[mIndex];
	if (mFireDelay < 1 && TURRET_MIN_SHIP_Y < aShip->mY && aBoard->mTankDeadTime == 0 && aBoard->mNukeFlash == 0.0 &&
		aBoard->mBossDeath == NULL)
	{
		int aOffset = SHOTS_FIRST_OFFSET;
		do
		{
			Bullet* aBullet = new Bullet();
			mBoard->mBullets.Append(aBullet);
			double aGunFrame = mFrame - TURRET_REST_FRAME;
			aBullet->mX = (aGunFrame + aGunFrame + mX) - (double)aOffset;
			aBullet->mY = mY - SHOT_MUZZLE_Y;
			aBullet->mGravity = SHOT_GRAVITY;
			aBullet->mIsEnemy = true;
			double anAngle = mFrame / ((double)mSprite->mColumns * SHOT_ANGLE_PER_FRAME) + SHOT_ANGLE_BASE;
			aBullet->mVelX = cos(anAngle) * SHOT_SPEED;
			aBullet->mVelY = sin(anAngle) * SHOT_SPEED;
			aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
			aBullet->mColumn = 0;
			aBullet->mRow = 0;
			mBoard->AddMuzzleFlash(aBullet->mX, aBullet->mY, 0.0, 0.0);
			mBoard->mParticles.Emit(mApp->mSprites[IMAGE_SMOKE], aBullet->mX, aBullet->mY - SMOKE_OFFSET_Y, -0.5, 0.0,
				SMOKE_COUNT, 0.5, -0.01, -0.5, SMOKE_LIFE, true, -1);
			aOffset += SHOTS_STEP;
		}
		while (aOffset < SHOTS_END_OFFSET);
		mApp->PlaySample(SOUND_ENEMYTANKGUN, (int)(mX * 3.0));
		mFireDelay = aShip->mTurretFire[mIndex];
	}
}
