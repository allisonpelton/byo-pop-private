// BossesHelicopter.cpp: the helicopter boss (images\hugecopter): a huge helicopter that floats down into the field
// and wanders over it, firing hellfire missiles from two launchers and, from the second time it comes, bullets from
// two turrets.
// Original: 0x43f440 (destructor body), 0x43f4d0 Update, 0x43f6f0 Draw, 0x43f800 Killed, 0x43f890 SilenceSounds,
// 0x43f8b0 RestoreSounds, 0x43f910 its data, 0x440300 constructor; its launchers 0x440780 (constructor), 0x440820
// Update; its turrets 0x440a20 (constructor), 0x440ab0 Update; both 0x4409b0 Killed.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Sprite.h"
#include "X87.h"
#include <math.h>

using namespace Sexy;

namespace
{
	const double ROTOR_SPEED = 0.4;				// frames of the rotor an update
	const double ROTOR_FRAMES = 5.0;
	const double HOVER_TOP_Y = 140.0;			// it floats between these heights
	const double HOVER_BOTTOM_Y = 200.0;
	const double HOVER_SIDE_X = 120.0;			// and between these sides
	const double DRIFT_STEP = 0.01;				// its speed changes by this much an update, or by a random 0 to 0.019 inside
	const double DRIFT_RANDOM = 0.001;
	const int DRIFT_RANDOM_RANGE = 20;
	const double MAX_DRIFT_SPEED = 0.5;
	const double BLADE_OFFSET_X = 122.0;		// its two rotors are this far to each side of it
	const double BLADE_OFFSET_Y = 68.0;			// and this high
	const double FIRE_RANGE_X = 300.0;			// its guns fire only this near the middle
	const double FIRE_MIN_Y = 50.0;				// and once it is this far down
	const double LAUNCHER_OFFSET_X = 75.0;
	const double LAUNCHER_OFFSET_Y = 45.0;
	const double TURRET_OFFSET_X = 122.0;
	const double TURRET_OFFSET_Y = 33.0;
	const double TURRET_START_FRAME = 10.0;
	const double TURRET_FRAME_SPEED = 0.1;
	const double TURRET_ANGLE_PER_FRAME = 0.6369426751592356;	// one over 1.57: the sweep is a quarter turn
	const double TURRET_ANGLE_START = 0.7854;
	const double TURRET_MUZZLE = 30.0;			// the bullet leaves this far from the turret's centre
}

//
// HelicopterBoss
//

// 0x440300: it loads its armour, points, turrets and launchers from the data, makes its sprites, its two launchers,
// and (from the second time it comes) its two turrets; the rotor sound starts silent
HelicopterBoss::HelicopterBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Helicopter", aBlock);				// 0x43f910
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mTurretArmor = (double)aData.Int("Turret", "armor");
	mTurretFire = aData.Int("Turret", "fire");
	mTurretsMove = aData.Attribute("Turret", "stationary") != "yes";
	mLauncherArmor = (double)aData.Int("Launcher", "armor");
	mLauncherFire = aData.Int("Launcher", "fire");
	mLauncherScale = aData.Double("Launcher", "speed");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\hugecopter\\hugecopter", 1, 1);
	mBlades = new Sprite(mApp);
	mBlades->Load("images\\hugecopter\\copterblades", 5, 1);
	mLauncherSprite = new Sprite(mApp);
	mLauncherSprite->Load("images\\hugecopter\\launcher", 1, 1);

	mLaunchers[0] = new HelicopterLauncher(mBoard, this);
	mLaunchers[0]->mOffsetX = -LAUNCHER_OFFSET_X;
	mLaunchers[0]->mOffsetY = LAUNCHER_OFFSET_Y;
	mLaunchers[1] = new HelicopterLauncher(mBoard, this);
	mLaunchers[1]->mOffsetX = LAUNCHER_OFFSET_X;
	mLaunchers[1]->mOffsetY = LAUNCHER_OFFSET_Y;

	if (mApp->mLevel < 1)
	{
		mTurretSprite = NULL;
		mTurrets[0] = NULL;		// the original leaves these two unset
		mTurrets[1] = NULL;
	}
	else
	{
		mTurretSprite = new Sprite(mApp);
		mTurretSprite->Load("images\\hugecopter\\turret", 21, 1);
		mTurrets[0] = new HelicopterTurret(mBoard, this);
		mTurrets[0]->mOffsetX = -TURRET_OFFSET_X;
		mTurrets[0]->mOffsetY = TURRET_OFFSET_Y;
		mTurrets[0]->mFrame = TURRET_START_FRAME;
		mTurrets[0]->mFrameSpeed = -TURRET_FRAME_SPEED;
		mTurrets[1] = new HelicopterTurret(mBoard, this);
		mTurrets[1]->mOffsetX = TURRET_OFFSET_X;
		mTurrets[1]->mOffsetY = TURRET_OFFSET_Y;
		mTurrets[1]->mFrame = TURRET_START_FRAME;
		mTurrets[1]->mFrameSpeed = TURRET_FRAME_SPEED;
	}

	mFrame = 0.0;
	mRotorFrame = 0.0;
	mX = 0.0;
	mVelX = 0.0;
	mVelY = 0.0;
	mIsAlive = false;
	mY = (double)-GetCelHeight();
	mSound = mApp->mSoundManager->GetSoundInstance(SOUND_BOSSCOPTER);
	if (mSound != NULL)
	{
		mSound->SetVolume(0.0);
		mSound->Play(true, true);
	}
}

// 0x43f440 (deleting 0x43f8e0): its launchers and turrets are on the board's list, and the board's to delete
HelicopterBoss::~HelicopterBoss()
{
	delete mSprite;
	delete mBlades;
	delete mTurretSprite;
	delete mLauncherSprite;
	if (mSound != NULL)
		mSound->Release();
}

// 0x43f4d0: once the level has run out it spins its rotor and drifts inside its range (its speed pushed back by 0.01
// an update when outside, by a random amount near 0 when inside), capped at 0.5; its rotor sound's volume grows as it
// comes down the screen
void HelicopterBoss::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	mRotorFrame = mRotorFrame + ROTOR_SPEED;
	if (ROTOR_FRAMES <= mRotorFrame)
		mRotorFrame = mRotorFrame - ROTOR_FRAMES;

	mX = mX + mVelX;
	mY = mVelY + mY;
	if (HOVER_TOP_Y <= mY)
	{
		mIsAlive = true;
		if (mY <= HOVER_BOTTOM_Y)
			mVelY = ((double)(Rand() % DRIFT_RANDOM_RANGE) * DRIFT_RANDOM - DRIFT_STEP) + mVelY;
		else
			mVelY = mVelY - DRIFT_STEP;
	}
	else
		mVelY = mVelY + DRIFT_STEP;

	if (-HOVER_SIDE_X <= mX)
	{
		if (mX <= HOVER_SIDE_X)
			mVelX = ((double)(Rand() % DRIFT_RANDOM_RANGE) * DRIFT_RANDOM - DRIFT_STEP) + mVelX;
		else
			mVelX = mVelX - DRIFT_STEP;
	}
	else
		mVelX = mVelX + DRIFT_STEP;

	if (mVelX > MAX_DRIFT_SPEED)
		mVelX = MAX_DRIFT_SPEED;
	else if (mVelX < -MAX_DRIFT_SPEED)
		mVelX = -MAX_DRIFT_SPEED;
	if (mVelY > MAX_DRIFT_SPEED)
		mVelY = MAX_DRIFT_SPEED;
	else if (mVelY < -MAX_DRIFT_SPEED)
		mVelY = -MAX_DRIFT_SPEED;

	if (mSound != NULL)
	{
		mSound->SetPan((int)(mX + mX));
		if (mY < (double)GetCelHeight())
			mSound->SetVolume(((double)GetCelHeight() + mY) / ((double)GetCelHeight() + (double)GetCelHeight()));
		else
			mSound->SetVolume(1.0);
	}
}

// 0x43f6f0: its body, then a rotor to each side above it, only once the level has run out
void HelicopterBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
	{
		if (mHitFlash != 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
		}
		mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, mMirrored ? 1 : 0, false);
		mBlades->Draw(g, (int)(mX - BLADE_OFFSET_X), (int)(mY - BLADE_OFFSET_Y), true, (int)mRotorFrame, 0, false);
		mBlades->Draw(g, (int)(mX + BLADE_OFFSET_X), (int)(mY - BLADE_OFFSET_Y), true, (int)mRotorFrame, 0, false);
		g->SetColorizeImages(false);
	}
}

// 0x43f800: the points, and its death
void HelicopterBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
	}
}

// 0x43f890: the rotor sound goes quiet while the game is paused
void HelicopterBoss::SilenceSounds()
{
	if (mSound != NULL)
		mSound->SetVolume(0.0);
}

// 0x43f8b0: and comes back at the effects volume
void HelicopterBoss::RestoreSounds()
{
	if (mSound != NULL)
		mSound->SetVolume(mApp->GetSfxVolume());
}

//
// HelicopterLauncher
//

// 0x440780: it takes the helicopter's launcher armour, sprite and delay, and is not counted among the enemies spawned
HelicopterLauncher::HelicopterLauncher(Board* theBoard, HelicopterBoss* theHelicopter) : Enemy(theBoard)
{
	mHelicopter = theHelicopter;
	mHealth = theHelicopter->mLauncherArmor;
	mMaxHealth = mHealth;
	mSprite = theHelicopter->mLauncherSprite;
	mPoints = 0;
	mFireDelay = theHelicopter->mLauncherFire;
	mIsAlive = false;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	mScale = theHelicopter->mLauncherScale;
	mOffsetX = 0.0;
	mOffsetY = 0.0;
}

// 0x440810: the sprite is the helicopter's
HelicopterLauncher::~HelicopterLauncher()
{
}

// 0x440820: it hangs from the helicopter and, once the helicopter is in the field, fires a hellfire missile at the
// helicopter's own velocity every so often while it is near the middle of the field
void HelicopterLauncher::Update()
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
	{
		Enemy::Update();
		HelicopterBoss* aHelicopter = mHelicopter;
		mX = (double)(int)aHelicopter->mX + mOffsetX;
		mY = (double)(int)aHelicopter->mY + mOffsetY;
		mVelX = aHelicopter->mVelX;
		mVelY = aHelicopter->mVelY;
		if (aHelicopter->mIsAlive)
		{
			Board* aBoard = mBoard;
			mIsAlive = true;
			if (aBoard->mTankDeadTime == 0)
				mFireDelay = mFireDelay - 1;
			else
				mFireDelay = aHelicopter->mLauncherFire;
			if (-FIRE_RANGE_X < mX && mX < FIRE_RANGE_X && mFireDelay < 1 &&
				FIRE_MIN_Y < aHelicopter->mY && aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
			{
				new Hellfire(mBoard, mX, mY, mVelX, mVelY, mScale);
				mFireDelay = mHelicopter->mLauncherFire;
			}
		}
	}
}

// 0x4409b0 (shared with the turrets): killed, exploding, and counted back out of the enemies destroyed
void HelicopterLauncher::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}

//
// HelicopterTurret
//

// 0x440a20: it takes the helicopter's turret armour, sprite and delay; its frame starts at 10 (the turret's heading
// sweeps a quarter turn over its 21 frames)
HelicopterTurret::HelicopterTurret(Board* theBoard, HelicopterBoss* theHelicopter) : Enemy(theBoard)
{
	mHelicopter = theHelicopter;
	mHealth = theHelicopter->mTurretArmor;
	mMaxHealth = mHealth;
	mSprite = theHelicopter->mTurretSprite;
	mFrame = TURRET_START_FRAME;
	mPoints = 0;
	mFireDelay = theHelicopter->mTurretFire;
	mIsAlive = false;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	mOffsetX = 0.0;
	mOffsetY = 0.0;
	mFrameSpeed = 0.0;
}

// 0x440aa0: the sprite is the helicopter's
HelicopterTurret::~HelicopterTurret()
{
}

// 0x440ab0: it hangs from the helicopter, sweeps back and forth through its frames, and fires a bullet along its
// heading every so often
void HelicopterTurret::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	HelicopterBoss* aHelicopter = mHelicopter;
	mX = (double)(int)aHelicopter->mX + mOffsetX;
	mY = (double)(int)aHelicopter->mY + mOffsetY;
	mVelX = aHelicopter->mVelX;
	mVelY = aHelicopter->mVelY;
	if (!aHelicopter->mIsAlive)
		return;

	mFrame = mFrame + mFrameSpeed;
	int aFrames = mSprite->mColumns;
	if (mFrame < (double)aFrames)
	{
		if (mFrame < 0.0)
		{
			mFrameSpeed = TURRET_FRAME_SPEED;
			mFrame = 0.0;
		}
	}
	else
	{
		mFrameSpeed = -TURRET_FRAME_SPEED;
		mFrame = (double)(aFrames - 1);
	}

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0)
		mFireDelay = mFireDelay - 1;
	else
		mFireDelay = aHelicopter->mTurretFire;
	if (mFireDelay < 1 && FIRE_MIN_Y < aHelicopter->mY && aBoard->mTankDeadTime == 0 &&
		aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
	{
		double anAngle = mFrame / ((double)mSprite->mColumns * TURRET_ANGLE_PER_FRAME) + TURRET_ANGLE_START;
		int aY = (int)(X87Sin(anAngle) * TURRET_MUZZLE + mY);
		int aX = (int)(mX - X87Cos(anAngle) * TURRET_MUZZLE);
		aBoard->AddEnemyBullet(aX, aY, anAngle, 0.0);
		mFireDelay = mHelicopter->mTurretFire;
	}
}

// 0x4409b0 (shared with the launchers)
void HelicopterTurret::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}
