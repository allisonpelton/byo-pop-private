// BossesRobot.cpp: the robot boss (images\robot): a giant robot that hops across the field after the tank, stamping the
// ground (and the tank) with its foot. Its two arms must be shot off first, and then its launchers fire hellfire and
// it fires a pair of lasers.
// Original: 0x441f10 (destructor body), 0x441fb0 Update, 0x442490 Draw, 0x442600 Killed, 0x4426c0 its data, 0x443210
// constructor; its arms 0x443720 (constructor), 0x4437d0 Update, 0x443850 Killed; its lasers 0x4438f0 (constructor),
// 0x443a00 Update, 0x443ba0 Draw; its launchers 0x443d00 (constructor), 0x443db0 Update (Killed 0x4409b0 is the
// helicopter's).

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"
#include "X87.h"
#include <math.h>

using namespace Sexy;

namespace
{
	const double START_Y = -240.0;				// it falls in from above the field
	const double STAND_Y = 210.0;				// it pushes up from the ground when it is lower than this
	const double MAX_Y = 260.0;					// and never gets lower than this
	const double FOOT_OFFSET_Y = 208.0;			// its foot is this far below its middle
	const double GROUND_FOOT_Y = 410.0;			// and has landed when it is this low
	const double PANEL_SPEED = 0.1;				// frames of its chest panel an update
	const double PANEL_FRAMES = 4.0;
	const int LANDING_SHAKE = 0x80;				// the screen shakes this hard when it lands
	const int SHAKE_DECAY = 2;
	const double TARGET_RANGE_X = 240.0;		// it jumps toward the tank, but never beyond this
	const double TANK_LEFT_LIMIT = -300.0;		// a tank off this far to the left is "behind" it, and it jumps to the right
	const double TARGET_FACTOR = 0.02;			// its sideways speed is this much of the way to the target
	const int JUMP_RANDOM_RANGE = 1000;			// scaled by 0.5 to 1.0 at random
	const double JUMP_RANDOM_STEP = 0.0005;
	const double JUMP_RANDOM_MIN = 0.5;
	const double MAX_JUMP_VEL_X = 1.5;
	const double FIRE_RANGE_X = 300.0;			// its launchers and lasers fire only this near the middle
	const double LASER_Y = 55.0;				// its lasers leave this far above its middle
	const double LASER_X = 10.0;				// and this far to each side
	const double PANEL_Y = 37.0;				// its panel, waist and foot are this far below its middle (the panel)
	const double WAIST_Y = 126.0;				// (the waist)
	const double EYE_Y = 55.0;					// its eye glows this far above its middle
	const double COLOUR_MAX = 255.0;
	const int ARM_OFFSET_X = 0x3b;				// an arm hangs this far to the side of it
	const int ARM_OFFSET_Y = 0x3c;				// and this far below
	const int LAUNCHER_OFFSET_X = 0x34;
	const int LAUNCHER_OFFSET_Y = -0x1a;
	const int SHOT_PAN = 3;						// a sound is placed on the screen at three times its x
	const double LASER_SPEED = 4.0;				// pixels an update
	const double LASER_BEARING_OFFSET = 1.57;	// its heading is the tank's bearing turned a quarter turn
	const double LASER_LENGTH = 50.0;			// its tail starts to follow once the head is this far on
	const double LASER_EDGE_X = 320.0;			// and it ends past the sides of the field
	const double LASER_GROUND_Y = 460.0;		// or at the ground
	const int LASER_WIDTH_OUTER = 4;			// the width of its two glows is these times 1 plus
}

//
// RobotBoss
//

// 0x443210: it loads its armour, points, fire delay, launchers, arms and jump from the data, makes its sprites, two
// arms and two launchers; it starts above the field with no ground under its foot
RobotBoss::RobotBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Robot", aBlock);					// 0x4426c0
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mFire = aData.Int("", "fire");
	mLauncherArmor = (double)aData.Int("Launcher", "armor");
	mLauncherFire = aData.Int("Launcher", "fire");
	mArmArmor = (double)aData.Int("Arm", "armor");
	mDownAccel = aData.Double("Jump", "downaccel");
	mDownMax = aData.Double("Jump", "downmax");
	mUpAccel = aData.Double("Jump", "upaccel");
	mUpMax = aData.Double("Jump", "upmax");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\robot\\body", 1, 1);
	mWaist = new Sprite(mApp);
	mWaist->Load("images\\robot\\waist", 1, 1);
	mFoot = new Sprite(mApp);
	mFoot->Load("images\\robot\\foot", 1, 1);
	mArmSprite = new Sprite(mApp);
	mArmSprite->Load("images\\robot\\arms", 2, 1);
	mLauncherSprite = new Sprite(mApp);
	mLauncherSprite->Load("images\\robot\\launchers", 2, 1);
	mPanel = new Sprite(mApp);
	mPanel->Load("images\\robot\\panel", 4, 1);
	mEyeGlow = new Sprite(mApp);
	mEyeGlow->Load("images\\robot\\eyeglow", 1, 1);

	mArms[0] = new RobotArm(theBoard, this, 0);
	mArms[1] = new RobotArm(theBoard, this, 1);
	mLaunchers[0] = new RobotLauncher(theBoard, this, 0);
	mLaunchers[1] = new RobotLauncher(theBoard, this, 1);

	mFrame = 0.0;
	mFireTimer = mFire;
	mX = 0.0;
	mY = START_Y;
	mVelX = 0.0;
	mVelY = 0.0;
	mFootY = 0.0;			// uninitialised in the original
	mPrevFootY = 0.0;
	mIsAlive = true;
	mPanelFrame = 0.0;
	mShake = 0;
}

// 0x441f10 (deleting 0x442690): its arms and launchers are on the board's list, and the board's to delete
RobotBoss::~RobotBoss()
{
	delete mSprite;
	delete mWaist;
	delete mFoot;
	delete mArmSprite;
	delete mLauncherSprite;
	delete mPanel;
	delete mEyeGlow;		// port: the original never deletes it
}

// 0x441fb0: it spins its chest panel; it pushes up from the ground when it is low and falls when it is high (its
// jump); its foot's height is held to 410 at the lowest, where it stamps (a sound and a shake) and, once it lifts off
// again, it picks a place to jump to, the tank's (kept within 240 of the middle). Its foot kills the tank under it.
// With both its arms gone its launchers fire, and so does it, two lasers at a time, on a timer.
void RobotBoss::Update()
{
	Enemy::Update();
	mPanelFrame = mPanelFrame + PANEL_SPEED;
	if (PANEL_FRAMES <= mPanelFrame)
		mPanelFrame = mPanelFrame - PANEL_FRAMES;

	if (STAND_Y <= mY)
	{
		if (mBoard->mBossDeath == NULL)
		{
			mVelY = mVelY - mUpAccel;
			if (mVelY < -mUpMax)
				mVelY = -mUpMax;
		}
	}
	else
	{
		mVelY = mDownAccel + mVelY;
		if (mDownMax < mVelY)
			mVelY = mDownMax;
	}

	double aFootY = mY + FOOT_OFFSET_Y;
	if (mFootY < aFootY)
		mFootY = aFootY;
	if (GROUND_FOOT_Y <= mFootY)
		mFootY = GROUND_FOOT_Y;
	if (mFootY == GROUND_FOOT_Y && mPrevFootY < GROUND_FOOT_Y)
	{
		mApp->PlaySample(SOUND_ROBOTSMASH, (int)(mX * SHOT_PAN));
		mShake = LANDING_SHAKE;
	}
	aFootY = mY + FOOT_OFFSET_Y;
	if (aFootY < mFootY)
		mFootY = aFootY;

	if (mPrevFootY == GROUND_FOOT_Y && mFootY < GROUND_FOOT_Y)
	{
		// it has lifted off: it jumps toward the tank
		Board* aBoard = mBoard;
		double aTarget;
		if (aBoard->mTankDeadTime != 0 || aBoard->mTankX < TANK_LEFT_LIMIT)
			aTarget = TARGET_RANGE_X;
		else
		{
			aTarget = aBoard->mTankX;
			if (aTarget < -TARGET_RANGE_X)
				aTarget = -TARGET_RANGE_X;
			else if (TARGET_RANGE_X < aTarget)
				aTarget = TARGET_RANGE_X;
		}
		mVelX = (aTarget - mX) * TARGET_FACTOR * ((double)(Rand() % JUMP_RANDOM_RANGE) * JUMP_RANDOM_STEP + JUMP_RANDOM_MIN);
		if (MAX_JUMP_VEL_X < mVelX)
			mVelX = MAX_JUMP_VEL_X;
		if (mVelX < -MAX_JUMP_VEL_X)
			mVelX = -MAX_JUMP_VEL_X;
	}

	Board* aBoard = mBoard;
	mPrevFootY = mFootY;
	if (aBoard->mBossDeath == NULL && aBoard->mTankDeadTime == 0)
	{
		if (mFoot->Collides((int)mX, (int)mFootY, 0, 0, false, mApp->mSprites[IMAGE_TANK],
			(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
			mBoard->KillTank();
	}

	if (mFootY < GROUND_FOOT_Y)
		mX = mVelX + mX;
	if (mY < MAX_Y)
		mY = mVelY + mY;

	aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 && mArms[0] == NULL && mArms[1] == NULL)
		mFireTimer = mFireTimer - 1;
	else
		mFireTimer = mFire;
	if (-FIRE_RANGE_X < mX && mX < FIRE_RANGE_X && mFireTimer < 1 && aBoard->mNukeFlash == 0.0 &&
		aBoard->mBossDeath == NULL)
	{
		new RobotLaser(mBoard, mX - LASER_X, mY - LASER_Y);
		new RobotLaser(mBoard, mX + LASER_X, mY - LASER_Y);
		mFireTimer = mFire;
	}

	if (mShake != 0)
	{
		int aShake = mShake;
		int aY = Rand() % (aShake / 16 + 1) - aShake / 32;
		int aX = Rand() % (aShake / 16 + 1) - aShake / 32;
		mBoard->Move(aX, aY);
		mShake -= SHAKE_DECAY;
	}
}

// 0x442490: the body, its chest panel, its waist and foot, and (while the launchers charge, arms gone) its eye glowing
// brighter as the next shot nears
void RobotBoss::Draw(Graphics* g)
{
	Enemy::Draw(g);
	mPanel->Draw(g, (int)mX, (int)(mY + PANEL_Y), true, (int)mPanelFrame, 0, false);
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mWaist->Draw(g, (int)mX, (int)(mY + WAIST_Y), true, 0, 0, false);
	mFoot->Draw(g, (int)mX, (int)mFootY, true, 0, 0, false);
	if (0 < mFireTimer && mArms[0] == NULL && mArms[1] == NULL)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aBrightness = (int)(COLOUR_MAX - COLOUR_MAX / (double)mFire * (double)mFireTimer);
		g->SetColor(Color(aBrightness, aBrightness, aBrightness));
		mEyeGlow->Draw(g, (int)mX, (int)(mY - EYE_Y), true, 0, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColorizeImages(false);
}

// 0x442600: the points, and its death
void RobotBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
	}
}

//
// RobotArm
//

// 0x443720: it takes the robot's arm armour and sprite (the arms sprite's frame is the side: 0 the left); it is not
// counted among the enemies spawned
RobotArm::RobotArm(Board* theBoard, RobotBoss* theRobot, int theSide) : Enemy(theBoard)
{
	mRobot = theRobot;
	mHealth = theRobot->mArmArmor;
	mMaxHealth = mHealth;
	mSprite = theRobot->mArmSprite;
	mPoints = 0;
	mFrame = (double)theSide;
	mIsAlive = true;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	double anX = mRobot->mX;
	if (mFrame == 0.0)
		anX = anX - 59.0;
	else
		anX = anX + 59.0;
	mX = anX;
	mY = mRobot->mY + 60.0;
}

// 0x4437c0: the sprite is the robot's
RobotArm::~RobotArm()
{
}

// 0x4437d0: it hangs from the robot's sides
void RobotArm::Update()
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
	{
		Enemy::Update();
		int anX;
		if (mFrame == 0.0)
			anX = (int)mRobot->mX - ARM_OFFSET_X;
		else
			anX = (int)mRobot->mX + ARM_OFFSET_X;
		mX = (double)anX;
		mY = (double)((int)mRobot->mY + ARM_OFFSET_Y);
	}
}

// 0x443850: the robot forgets this arm, and it is counted back out of the enemies destroyed
void RobotArm::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	if (mFrame == 0.0)
		mRobot->mArms[0] = NULL;
	if (mFrame == 1.0)
		mRobot->mArms[1] = NULL;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}

//
// RobotLauncher
//

// 0x443d00: it takes the robot's launcher armour, sprite and delay (the sprite's frame is the side: 0 the left); it
// is not counted among the enemies spawned
RobotLauncher::RobotLauncher(Board* theBoard, RobotBoss* theRobot, int theSide) : Enemy(theBoard)
{
	mRobot = theRobot;
	mHealth = theRobot->mLauncherArmor;
	mMaxHealth = mHealth;
	mSprite = theRobot->mLauncherSprite;
	mPoints = 0;
	mFireTimer = theRobot->mLauncherFire;
	mFrame = (double)theSide;
	mIsAlive = true;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	double anX = mRobot->mX;
	if (mFrame == 0.0)
		anX = anX - 52.0;
	else
		anX = anX + 52.0;
	mX = anX;
	mY = mRobot->mY - 26.0;
}

// 0x443da0: the sprite is the robot's
RobotLauncher::~RobotLauncher()
{
}

// 0x443db0: it hangs from the robot's shoulders and fires a hellfire missile at the robot's velocity every so often
// while it is near the middle of the field
void RobotLauncher::Update()
{
	if (mBoard->mLevelLength <= mBoard->mLevelTime)
	{
		Enemy::Update();
		RobotBoss* aRobot = mRobot;
		int anX;
		if (mFrame == 0.0)
			anX = (int)aRobot->mX - LAUNCHER_OFFSET_X;
		else
			anX = (int)aRobot->mX + LAUNCHER_OFFSET_X;
		mX = (double)anX;
		mY = (double)((int)aRobot->mY + LAUNCHER_OFFSET_Y);
		mVelX = aRobot->mVelX;
		mVelY = aRobot->mVelY;
		Board* aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0)
			mFireTimer = mFireTimer - 1;
		else
			mFireTimer = aRobot->mLauncherFire;
		if (-FIRE_RANGE_X < mX && mX < FIRE_RANGE_X && mFireTimer < 1 && aBoard->mNukeFlash == 0.0)
		{
			aBoard->AddHellfire((int)mX, (int)mY, mVelX, mVelY);
			mFireTimer = mRobot->mLauncherFire;
		}
	}
}

// 0x4409b0 (shared with the helicopter's guns): killed, exploding, and counted back out of the enemies destroyed
void RobotLauncher::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}

//
// RobotLaser
//

// 0x4438f0: a bolt aimed at the tank's place now; it can't be shot down
RobotLaser::RobotLaser(Board* theBoard, double x, double y) : Projectile(theBoard)
{
	mY = y;
	mX = x;
	mTailX = x;
	mTailY = y;
	mVelX = 0.0;
	mVelY = 0.0;
	mIsShootable = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_ROCK];
	mHasLanded = false;
	mAngle = X87Atan2(x - mBoard->mTankX, (double)mBoard->mTankY - y) + LASER_BEARING_OFFSET;
	mDirX = X87Cos(mAngle) * LASER_SPEED;
	mDirY = X87Sin(mAngle) * LASER_SPEED;
	mBoard->mApp->PlaySample(SOUND_ROBOTLASER, (int)(x * SHOT_PAN));
}

// 0x4439f0 (deleting 0x443b70)
RobotLaser::~RobotLaser()
{
}

// 0x443a00: the head flies on until it reaches the ground (a flash there); the tail follows once the bolt is 50 long,
// and, after the head has landed, on and on. It ends past the sides or the ground, and the tank dies if it is hit.
void RobotLaser::Update()
{
	double aLength = sqrt(pow(mY - mTailY, 2.0) + pow(mX - mTailX, 2.0));
	bool aMoveTail = true;
	if (LASER_GROUND_Y <= mY)
	{
		if (!mHasLanded)
		{
			mBoard->AddGroundFlash((int)mX, 0);
			mHasLanded = true;
		}
	}
	else
	{
		mX = mDirX + mX;
		mY = mDirY + mY;
		if (aLength <= LASER_LENGTH)
			aMoveTail = false;
	}
	if (aMoveTail)
	{
		mTailX = mDirX + mTailX;
		mTailY = mDirY + mTailY;
	}

	if (-LASER_EDGE_X <= mTailX && mTailX <= LASER_EDGE_X && mTailY <= LASER_GROUND_Y)
	{
		Board* aBoard = mBoard;
		if (aBoard->mTankDeadTime == 0)
		{
			if (mSprite->Collides((int)mX, (int)mY, 0, 0, false, aBoard->mApp->mSprites[IMAGE_TANK],
				(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
			{
				mBoard->KillTank();
				Remove();
				return;
			}
		}
		Projectile::Update();
		return;
	}
	Remove();
}

// 0x443ba0: two glowing bands along the bolt from its tail to its head, each as wide as it is long (a dark red one
// under a white one), added to the picture
void RobotLaser::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	Point aPoints[4];
	for (int aBand = 2; aBand > 0; aBand--)
	{
		double aAngle = mAngle + LASER_BEARING_OFFSET;
		double aCos = cos(aAngle);
		double aOuterX = (double)(aBand * LASER_WIDTH_OUTER + 1) * aCos;
		double aSin = sin(aAngle);
		double aOuterY = (double)(aBand * LASER_WIDTH_OUTER + 1) * aSin;
		double aInnerX = (double)(aBand + 1) * aCos;
		double aInnerY = (double)(aBand + 1) * aSin;
		aPoints[0] = Point((int)(aOuterX + mX), (int)(aOuterY + mY));
		aPoints[1] = Point((int)(mX - aOuterX), (int)(mY - aOuterY));
		aPoints[2] = Point((int)(mTailX - aInnerX), (int)(mTailY - aInnerY));
		aPoints[3] = Point((int)(aInnerX + mTailX), (int)(aInnerY + mTailY));
		if (aBand == 2)
			g->SetColor(Color(0x80, 0x20, 0x40));
		else
			g->SetColor(Color(0xff, 0xff, 0xff));
		g->PolyFill(aPoints, 4, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
