// BossesEye.cpp: the eye boss (images\eye): a floating eye with six jointed arms ending in hands. It opens and fires
// energy shots when it is open, and shuts while one of its hands charges a bolt of lightning down to the ground.
// Original: 0x43a650 (destructor body), 0x43a700 Draw, 0x43a9e0 Killed, 0x43aaa0 Update, 0x43b350 its data, 0x43ba90
// constructor; the hands 0x43c000 (constructor), 0x43c070, 0x43c080 Update, 0x43c0d0 Killed.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Res.h"
#include "Sprite.h"
#include "X87.h"

#include <math.h>
#include <vector>

using namespace Sexy;

namespace
{
	// Where each of the six arms is fixed to the eye, from its centre: three on each side (0x5307f0)
	const int ARM_ROOTS[EyeBoss::NUM_ARMS][2] =
	{
		{ -54, -54 }, { -76, 0 }, { -54, 54 }, { 54, -54 }, { 76, 0 }, { 54, 54 }
	};

	const double START_Y = -100.0;				// it comes down from above the field
	const double START_TARGET_Y = 190.0;
	const int START_CYCLE = 499;
	const int ARMS_PER_SIDE = 3;
	const int HAND_START_SHIFT = 192;			// a hand starts this far to the right of the left side's, less 96
	const int HAND_START_BASE = -96;
	const int ARM_JOINT_STEP = 12;				// an arm's joints are 12 pixels apart
	const double ARM_JOINT_LENGTH = 12.0;

	const double EYE_LOW_Y = 160.0;				// below this it can open its eye and fire
	const int CYCLE_LENGTH = 1000;				// its eye is open for the second half of each 1000 updates
	const int CYCLE_OPEN_AT = 500;
	const double EYE_STEP = 0.2;				// the frames its eye opens or closes an update
	const double GLOW_FRAME_SCALE = 0.2;		// its glow is this much of its frame, times GLOW_BRIGHTNESS
	const double EYE_OPEN_FRAME = 5.0;
	const int GLOW_CYCLE = 63;
	const double GLOW_ANGLE_STEP = 0.1;
	const double GLOW_BRIGHTNESS = 127.0;
	const int RETARGET_INTERVAL = 50;			// every 50 updates it and its hands choose new places to go
	const int TARGET_X_RANGE = 400;
	const double TARGET_X_MIN = 200.0;
	const int TARGET_Y_RANGE = 20;
	const double TARGET_Y_MIN = 180.0;
	const int HAND_JITTER_RANGE = 40;
	const double HAND_JITTER_MIN = 20.0;
	const double EYE_MAX_SPEED_X = 1.5;
	const double EYE_ACCEL_X = 0.02;
	const double EYE_MAX_SPEED_Y = 0.5;
	const double EYE_ACCEL_Y = 0.01;
	const double HAND_MAX_SPEED = 1.0;
	const double HAND_ACCEL = 0.01;
	const int SHOT_ANGLE_RANGE = 157;			// the energy shot's angle: Rand() % this hundredths of a radian
	const double SHOT_ANGLE_SCALE = 0.01;
	const double SHOT_ANGLE_BASE = 3.926;
	const double SHOT_SPEED = -3.0;
	const double BOLT_MIN_HAND_Y = 80.0;		// a hand below this may be chosen to charge a bolt
	const double BOLT_CHARGE_STEP = 2.5;
	const double BOLT_FULL = 255.0;
	const int BOLT_TIME = 50;					// updates the bolt strikes
	const double BOLT_TANK_RANGE = 40.0;		// it kills a tank this near its hand's x
	const int BOLT_GLOW_CYCLE = 10;
	const double BOLT_GLOW_STEP = 25.5;
	const double BOLT_OFFSET_X = 30.0;			// the bolt's picture is drawn this far from the hand
	const double BOLT_OFFSET_Y = 22.0;
	const int FLASH_X = -320;					// and the whole field flashes
	const int FLASH_Y = 0;
	const int FLASH_WIDTH = 640;
	const int FLASH_HEIGHT = 480;
	const int NO_HAND = -1;
}

// 0x43ba90: its data and sprites, then its six arms and the hands on them
EyeBoss::EyeBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Eye", aBlock);						// 0x43b350
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mFire = aData.Int("", "fire");
	mHandArmor = aData.Int("Hand", "armor");
	mHandFire = aData.Int("Hand", "fire");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\eye\\eye", 6, 1);
	mBody = new Sprite(mApp);
	mBody->Load("images\\eye\\body", 1, 1);
	mArm = new Sprite(mApp);
	mArm->Load("images\\eye\\arm", 1, 1);
	mGlowSprite = new Sprite(mApp);
	mGlowSprite->Load("images\\eye\\glow", 1, 1);
	mHandSprite = new Sprite(mApp);
	mHandSprite->Load("images\\eye\\hand", 1, 1);
	mBoltSprite = new Sprite(mApp);
	mBoltSprite->Load("images\\eye\\bolt", 1, 1);
	mHandGlowSprite = new Sprite(mApp);
	mHandGlowSprite->Load("images\\eye\\handglow", 1, 1);

	mShotDelay = mFire;
	mIsAlive = false;
	mTargetX = 0.0;
	mTargetY = START_TARGET_Y;
	mBoltDelay = mHandFire;
	mBoltHand = NO_HAND;
	mBoltTime = 0;
	mBoltCharge = 0.0;
	mHandsLeft = NUM_ARMS;
	mCycle = START_CYCLE;
	mGlow = 0;
	mX = 0.0;
	mY = START_Y;

	for (int i = 0; i < NUM_ARMS; i++)
	{
		int aSide = i / ARMS_PER_SIDE;
		mHands[i].mOffsetX = (double)(aSide * HAND_START_SHIFT + HAND_START_BASE + ARM_ROOTS[i][0]);
		mHands[i].mOffsetY = (double)ARM_ROOTS[i][1];
		mHands[i].mVelX = 0.0;
		mHands[i].mVelY = 0.0;
		mHands[i].mHand = new EyeHand(mBoard, this, i);

		int aStep = 0;
		int aJoint = NUM_JOINTS;
		ArmJoint* aPoint = mArms[i];
		do
		{
			aPoint->mX = (double)ARM_ROOTS[i][0] + (double)aStep + mX;
			aStep = aStep + aSide * ARM_JOINT_STEP * 2 - ARM_JOINT_STEP;
			aPoint->mY = (double)ARM_ROOTS[i][1] + mY;
			aPoint++;
			aJoint--;
		}
		while (aJoint != 0);
	}
}

// 0x43a650 (deleting 0x43aa70): the hands are the board's to delete
EyeBoss::~EyeBoss()
{
	delete mSprite;
	delete mBody;
	delete mArm;
	delete mGlowSprite;
	delete mHandSprite;
	delete mBoltSprite;
	delete mHandGlowSprite;
}

// 0x43a700: each arm's joints from the hand back to the shoulder, its hand (flashing red when hit, lit while it
// charges a bolt), then the body, the eye (and the glow of its lid) and the bolt with a flash over the field
void EyeBoss::Draw(Graphics* g)
{
	for (int i = 0; i < NUM_ARMS; i++)
	{
		for (int aJoint = NUM_JOINTS - 1; aJoint >= 0; aJoint--)
			mArm->Draw(g, (int)mArms[i][aJoint].mX, (int)mArms[i][aJoint].mY, true, 0, 0, false);

		EyeHand* aHand = mHands[i].mHand;
		if (aHand != NULL)
		{
			if (aHand->mHitFlash != 0)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255 - aHand->mHitFlash, 255 - aHand->mHitFlash, 255));
			}
			mHandSprite->Draw(g, (int)aHand->mX, (int)aHand->mY, true, 0, 0, false);
			if (0.0 < mBoltCharge && mBoltHand == i)
			{
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColorizeImages(true);
				int aBrightness = (int)mBoltCharge;
				g->SetColor(Color(aBrightness, aBrightness, aBrightness));
				mHandGlowSprite->Draw(g, (int)aHand->mX, (int)aHand->mY, true, 0, 0, false);
				g->SetColorizeImages(false);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
	}

	mBody->Draw(g, (int)mX, (int)mY, true, 0, 0, false);
	if (mIsAlive)
	{
		Enemy::Draw(g);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(mGlow, mGlow, mGlow));
		mGlowSprite->Draw(g, (int)mX, (int)mY, true, 0, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	if (mBoltTime != 0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int aBrightness = (int)((double)(mApp->mUpdateCount % BOLT_GLOW_CYCLE) * BOLT_GLOW_STEP);
		g->SetColor(Color(aBrightness, aBrightness, aBrightness));
		EyeHand* aHand = mHands[mBoltHand].mHand;
		mBoltSprite->Draw(g, (int)(aHand->mX - BOLT_OFFSET_X), (int)(aHand->mY - BOLT_OFFSET_Y), false, 0, 0, false);
		g->FillRect(FLASH_X, FLASH_Y, FLASH_WIDTH, FLASH_HEIGHT);
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x43a9e0
void EyeBoss::Killed()
{
	if (mBoard->mBossDeath != NULL)
		return;
	Enemy::Killed();
	mBoard->AddScore(mPoints, (int)mX, (int)mY);
	mBoard->mBossDeath = new BossDeath(mBoard);
}

// port: an arm's joints held 12 pixels apart: each joint of theJoints, in the order the pointers step, is moved along
// the line from its neighbour (theNeighbour, the previous joint in the walk's direction) to be that far from it
void EyeBoss::PullJoint(ArmJoint* theJoint, const ArmJoint* theNeighbour)
{
	double aDX = theJoint->mX - theNeighbour->mX;
	double aDY = theJoint->mY - theNeighbour->mY;
	double aDistance = sqrt(pow(aDX, 2.0) + pow(aDY, 2.0));
	if ((int)aDistance != (int)ARM_JOINT_LENGTH)
	{
		theJoint->mX = aDX * (ARM_JOINT_LENGTH / aDistance) + theNeighbour->mX;
		theJoint->mY = aDY * (ARM_JOINT_LENGTH / aDistance) + theNeighbour->mY;
	}
}

// 0x43aaa0: the eye opens and shuts (open for half of each 1000 updates once it is low enough, and always once its
// hands are gone or it is dying), drifts toward a place it picks every 50 updates, its hands toward places of their
// own; its arms follow. Open it fires an energy shot every so often; shut it picks a hand to charge a bolt, which
// strikes the ground below that hand and kills a tank under it.
void EyeBoss::Update()
{
	Enemy::Update();
	if (EYE_LOW_Y < mY)
		mCycle++;

	if ((mCycle % CYCLE_LENGTH < CYCLE_OPEN_AT || mBoard->mBossDeath != NULL) && 0 < mHandsLeft)
	{
		mFrame = mFrame - EYE_STEP;
		if (mFrame < 0.0)
			mFrame = 0.0;
	}
	else
	{
		mFrame = mFrame + EYE_STEP;
		if (EYE_OPEN_FRAME < mFrame)
			mFrame = EYE_OPEN_FRAME;
	}

	if (mFrame <= 0.0)
	{
		mIsAlive = false;
		mGlow = 0;
	}
	else
	{
		mIsAlive = true;
		int aPhase = mApp->mUpdateCount % GLOW_CYCLE;
		mGlow = (int)((X87Cos((double)aPhase * GLOW_ANGLE_STEP) + 1.0) * (mFrame * GLOW_FRAME_SCALE * GLOW_BRIGHTNESS));
	}

	if (mApp->mUpdateCount % RETARGET_INTERVAL == 0)
	{
		mTargetX = (double)(Rand() % TARGET_X_RANGE) - TARGET_X_MIN;
		mTargetY = (double)(Rand() % TARGET_Y_RANGE) + TARGET_Y_MIN;
		for (int i = 0; i < NUM_ARMS; i++)
		{
			mHands[i].mOffsetX = ((double)(Rand() % HAND_JITTER_RANGE) - HAND_JITTER_MIN) + (double)ARM_ROOTS[i][0];
			mHands[i].mOffsetY = ((double)(Rand() % HAND_JITTER_RANGE) - HAND_JITTER_MIN) + (double)ARM_ROOTS[i][1];
		}
	}

	if (mX < mTargetX && mVelX < EYE_MAX_SPEED_X)
		mVelX = mVelX + EYE_ACCEL_X;
	if (mTargetX < mX && -EYE_MAX_SPEED_X < mVelX)
		mVelX = mVelX - EYE_ACCEL_X;
	if (mY < mTargetY && mVelY < EYE_MAX_SPEED_Y)
		mVelY = mVelY + EYE_ACCEL_Y;
	if (mTargetY < mY && -EYE_MAX_SPEED_Y < mVelY)
		mVelY = mVelY - EYE_ACCEL_Y;
	mX = mVelX + mX;
	mY = mVelY + mY;

	// each hand speeds toward its place (the shoulder's, plus its own offset), the joint at its end moving with it and
	// the rest of the arm pulled after it, then pulled out again from the shoulder
	for (int i = 0; i < NUM_ARMS; i++)
	{
		double aTargetX = (double)ARM_ROOTS[i][0] + mHands[i].mOffsetX + mX;
		double aTargetY = (double)ARM_ROOTS[i][1] + mHands[i].mOffsetY + mY;
		ArmJoint* aEnd = &mArms[i][NUM_JOINTS - 1];
		if (aEnd->mX < aTargetX && mHands[i].mVelX < HAND_MAX_SPEED)
			mHands[i].mVelX = mHands[i].mVelX + HAND_ACCEL;
		if (aTargetX < aEnd->mX && -HAND_MAX_SPEED < mHands[i].mVelX)
			mHands[i].mVelX = mHands[i].mVelX - HAND_ACCEL;
		if (aEnd->mY < aTargetY && mHands[i].mVelY < HAND_MAX_SPEED)
			mHands[i].mVelY = mHands[i].mVelY + HAND_ACCEL;
		if (aTargetY < aEnd->mY && -HAND_MAX_SPEED < mHands[i].mVelY)
			mHands[i].mVelY = mHands[i].mVelY - HAND_ACCEL;
		aEnd->mX = mHands[i].mVelX + mVelX + aEnd->mX;
		aEnd->mY = mHands[i].mVelY + mVelY + aEnd->mY;

		int aJoint = NUM_JOINTS - 1;
		ArmJoint* aPoint = &mArms[i][NUM_JOINTS - 2];
		do
		{
			PullJoint(aPoint, aPoint + 1);
			aPoint--;
			aJoint--;
		}
		while (aJoint != 0);
	}
	for (int i = 0; i < NUM_ARMS; i++)
	{
		mArms[i][0].mX = (double)ARM_ROOTS[i][0] + mX;
		mArms[i][0].mY = (double)ARM_ROOTS[i][1] + mY;
		int aJoint = NUM_JOINTS - 1;
		ArmJoint* aPoint = &mArms[i][1];
		do
		{
			PullJoint(aPoint, aPoint - 1);
			aPoint++;
			aJoint--;
		}
		while (aJoint != 0);
	}

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime != 0 || aBoard->mNukeFlash != 0.0 || mY < EYE_LOW_Y)
		mShotDelay = mFire;
	else if (mShotDelay != 0)
		mShotDelay--;

	if (!mIsAlive)
		ChooseBoltHand();
	else if (mShotDelay == 0 && mFrame == EYE_OPEN_FRAME && mBoard->mBossDeath == NULL)
	{
		Bullet* aBullet = new Bullet();
		mBoard->mBullets.Append(aBullet);
		aBullet->mX = mX;
		aBullet->mY = mY;
		int aRand = Rand();
		aBullet->mIsEnemy = true;
		aRand = aRand % SHOT_ANGLE_RANGE;
		aBullet->mGravity = 0.0;
		double anAngle = (double)aRand * SHOT_ANGLE_SCALE + SHOT_ANGLE_BASE;
		aBullet->mVelX = X87Cos(anAngle) * SHOT_SPEED;
		aBullet->mVelY = X87Sin(anAngle) * SHOT_SPEED;
		aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
		aBullet->mColumn = 0;
		aBullet->mRow = 0;
		mShotDelay = mFire;
		mApp->PlaySample(SOUND_ENERGYFIRE, (int)(mX * 3.0));
	}

	if (-1 < mBoltHand)
	{
		if (mBoard->mNukeFlash == 0.0)
		{
			if (mBoltCharge < BOLT_FULL && mBoltTime == 0)
			{
				mBoltCharge = mBoltCharge + BOLT_CHARGE_STEP;
				if (BOLT_FULL <= mBoltCharge)
				{
					mBoltCharge = BOLT_FULL;
					mBoltTime = BOLT_TIME;
					mApp->PlaySample(SOUND_THUNDER, (int)(mX * 3.0));
				}
			}
		}
		else
		{
			mBoltHand = NO_HAND;
			mBoltTime = 0;
			mBoltCharge = 0.0;
		}
	}

	if (mBoltTime != 0 && -1 < mBoltTime)
	{
		Board* aStrikeBoard = mBoard;
		EyeHand* aHand = mHands[mBoltHand].mHand;
		if (aStrikeBoard->mTankDeadTime == 0 && aHand->mX < aStrikeBoard->mTankX + BOLT_TANK_RANGE &&
			aStrikeBoard->mTankX - BOLT_TANK_RANGE < aHand->mX)
			aStrikeBoard->KillTank();
		mBoltTime--;
		if (mBoltTime == 0)
		{
			mBoltCharge = 0.0;
			mBoltHand = NO_HAND;
		}
	}
}

// port: part of 0x43aaa0 (its shut-eye branch): with no bolt being charged and the eye low enough, the bolt's delay
// counts down (it resets while the tank is dead or the field is flashing) and at 0 a random hand below the top of the
// field is chosen
void EyeBoss::ChooseBoltHand()
{
	if (mBoltHand != NO_HAND || !(EYE_LOW_Y < mY))
		return;

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0 && aBoard->mNukeFlash == 0.0)
	{
		if (mBoltDelay != 0)
			mBoltDelay--;
	}
	else
		mBoltDelay = mHandFire;

	if (mBoltDelay == 0 && aBoard->mBossDeath == NULL)
	{
		std::vector<int> aCandidates;
		for (int i = 0; i < NUM_ARMS; i++)
		{
			EyeHand* aHand = mHands[i].mHand;
			if (aHand != NULL && BOLT_MIN_HAND_Y < aHand->mY)
				aCandidates.push_back(i);
		}
		if (!aCandidates.empty())
		{
			unsigned int aRand = (unsigned int)Rand();
			mBoltDelay = mHandFire;
			mBoltHand = aCandidates[aRand % aCandidates.size()];
			mApp->PlaySample(SOUND_BOLTCHARGE, (int)(mX * 3.0));
		}
	}
}

//
// EyeHand
//

// 0x43c000: it is not counted among the enemies spawned, nor (in Killed) among those destroyed
EyeHand::EyeHand(Board* theBoard, EyeBoss* theEye, int theIndex) : Enemy(theBoard)
{
	mEye = theEye;
	mHealth = (double)theEye->mHandArmor;
	mMaxHealth = mHealth;
	mFrame = 0.0;
	mPoints = 0;
	mIsAlive = false;
	mSprite = theEye->mHandSprite;
	mIndex = theIndex;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
}

// 0x43c070: the sprite is the eye's
EyeHand::~EyeHand()
{
}

// 0x43c080: it sits at the end of its arm, and can be hurt once the eye can
void EyeHand::Update()
{
	Enemy::Update();
	EyeBoss::ArmJoint* anEnd = &mEye->mArms[mIndex][EyeBoss::NUM_JOINTS - 1];
	mX = anEnd->mX;
	mY = anEnd->mY;
	if (mEye->mIsAlive)
		mIsAlive = true;
}

// 0x43c0d0: a bolt it was charging is lost, and the eye forgets it
void EyeHand::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	if (mEye->mBoltHand == mIndex)
	{
		mEye->mBoltDelay = mEye->mHandFire;
		mEye->mBoltHand = NO_HAND;
		mEye->mBoltTime = 0;
		mEye->mBoltCharge = 0.0;
	}
	mEye->mHands[mIndex].mHand = NULL;
	mEye->mHandsLeft--;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}
