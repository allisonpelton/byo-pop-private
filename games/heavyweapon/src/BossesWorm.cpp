// BossesWorm.cpp: the worm boss (images\worm): a giant sand worm that leaps out of the ground in arcs, a chain of 30
// ribs behind its head, with three turrets on its back that drop iron bombs. Shoot the turrets off and it leaps faster
// and fires hellfire; it throws boulders where it dives back into the sand.
// Original: 0x443f00 (destructor body), 0x443f80 Draw, 0x444120 Killed, 0x4441b0 (a new jump), 0x4442b0 Update, 0x444a60
// its data, 0x445800 constructor; its turrets 0x445b90 (constructor), 0x445c30 Update, 0x445e20 Killed.

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
	const int PART_SEGMENT_FIRST = 9;			// its turrets sit on ribs 9, 19 and 29 (ten ribs apart)
	const int PART_SEGMENT_STEP = 10;
	const double GROUND_Y = 460.0;				// the sand's surface
	const double SURFACED_Y = 480.0;			// it counts as in the field above this
	const double TURN_Y = 520.0;				// its head turns toward the tank when it has sunk this far
	const double TAIL_GONE_Y = 795.0;			// and it jumps again once its tail has sunk this far
	const double SPLASH_Y = 500.0;				// it throws sand up when it leaves going up from this deep
	const double HELLFIRE_MAX_Y = 280.0;		// it fires hellfire only above this
	const double HEAD_FRAME_SPEED = 0.3;
	const double HEAD_FRAMES = 10.0;
	const double TURN_FACTOR = 0.01;			// its head's sideways speed is this much of the way to the tank
	const double RIB_FOLLOW = 0.125;			// each rib closes this much of the way to the one before it an update
	const int DEATH_THRASH_PERIOD = 50;			// while it dies it picks a new thrashing speed this often
	const double DEATH_X_PULL = 0.0015625;
	const double DEATH_Y_PULL = 0.0020833333333333333;
	const double DEATH_CENTRE_Y = 240.0;
	const int THRASH_RANGE = 200;
	const double THRASH_STEP = 0.01;
	const int JUMP_SIDE_RANGE = 160;			// the next jump comes up this far (plus 100) to either side of the middle
	const double JUMP_SIDE_MIN = 100.0;
	const double JUMP_FIRST_X = -200.0;			// the first one comes up on the left
	const double TANK_LEFT_LIMIT = -300.0;		// a tank off this far to the left is "behind" it, and it comes up on the right
	const double JUMP_LEFT_X = 200.0;
	const int SAND_FADE_IN = 8;
	const int SAND_FADE_OUT = -2;
	const int SAND_MAX_ALPHA = 255;
	const int SAND_SOUND_PAN = 3;
	const int SAND_PUFF_PERIOD = 5;
	const int SAND_PUFF_RANGE = 100;
	const double SAND_PUFF_Y = 500.0;
	const double SAND_PUFF_SPREAD = 0.5;
	const double SAND_PUFF_GRAVITY = -0.01;
	const double SAND_PUFF_VEL_Y = -1.0;
	const int SAND_PUFF_LIFE = 120;
	const int SAND_DRAW_Y = 0x1cc;
	const int SAND_FRAME_PERIOD = 8;
	const int BOULDER_X_RANGE = 80;				// a boulder starts anywhere within 40 of the head
	const int BOULDER_VEL_X_RANGE = 400;
	const int BOULDER_VEL_Y_RANGE = 200;
	const double BOULDER_Y = 460.0;
	const double TURRET_AIM_FACTOR = 0.0125;	// a turret's bomb leans toward the tank this much of the way
	const double TURRET_AIM_MAX = 3.0;
	const double TURRET_FIRE_MAX_Y = 280.0;
	const double TURRET_FIRE_MAX_VEL_Y = 1.0;
	const double TURRET_HIDE_Y = 460.0;
	const double TURRET_FRAME_SPEED = 0.05;
}

// 0x445800: it loads its armour, points, fire delay, boulders, turrets and jumps from the data, makes its sprites,
// three turrets, and takes its first jump
WormBoss::WormBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Worm", aBlock);						// 0x444a60
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mFire = aData.Int("", "fire");
	mBoulders = aData.Int("", "boulders");
	mTurretArmor = aData.Int("Turret", "armor");
	mTurretFire = aData.Int("Turret", "fire");
	mJumpSpeed[0] = aData.Double("Jump", "jump");
	mJumpDepth[0] = aData.Int("Jump", "depth");
	mJumpGravity[0] = aData.Double("Jump", "gravity");
	mJumpLateral[0] = aData.Double("Jump", "lateral");
	mJumpSpeed[1] = aData.Double("JumpFast", "jump");
	mJumpDepth[1] = aData.Int("JumpFast", "depth");
	mJumpGravity[1] = aData.Double("JumpFast", "gravity");
	mJumpLateral[1] = aData.Double("JumpFast", "lateral");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\worm\\head", 10, 1);
	mRib = new Sprite(mApp);
	mRib->Load("images\\worm\\rib", 4, 1);
	mTurretSprite = new Sprite(mApp);
	mTurretSprite->Load("images\\worm\\turret", 1, 1);
	mSand = new Sprite(mApp);
	mSand->Load("images\\worm\\sand", 4, 1);

	mCurrentJumpSpeed = mJumpSpeed[0];
	mCurrentGravity = mJumpGravity[0];
	mDepth = mJumpDepth[0];
	mCurrentLateral = mJumpLateral[0];
	mIsAlive = true;
	mTurrets[0] = new WormTurret(theBoard, this, 0);
	mTurrets[1] = new WormTurret(theBoard, this, 1);
	mTurrets[2] = new WormTurret(theBoard, this, 2);
	mSandAlpha = 0;
	mSandFade = 0;
	mFireTimer = mFire;
	mIsFast = false;
	mFrame = 0.0;			// the original leaves its frame to the Enemy constructor
	mSandX = 0;
	NewJump(true);
}

// 0x443f00 (deleting 0x444280): its turrets are on the board's list, and the board's to delete
WormBoss::~WormBoss()
{
	delete mSprite;
	delete mRib;
	delete mTurretSprite;
	delete mSand;
}

// 0x4441b0: it comes up again: all its ribs are gathered at one place at its dig depth (the first time on the left, after
// that a random place 100 to 260 to either side of the middle; once the turrets are gone under the tank, and on the
// right if the tank is far to the left), and it starts to rise
void WormBoss::NewJump(bool isFirst)
{
	double aX;
	if (!mIsFast)
	{
		if (!isFirst)
		{
			int aDistance = Rand();
			int aSide = Rand() % 2;
			aX = ((double)aSide + (double)aSide - 1.0) * ((double)(aDistance % JUMP_SIDE_RANGE) + JUMP_SIDE_MIN);
		}
		else
			aX = JUMP_FIRST_X;
	}
	else
		aX = mBoard->mTankX;
	if (mBoard->mTankX < TANK_LEFT_LIMIT)
		aX = JUMP_LEFT_X;

	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		mSegX[i] = aX;
		mSegY[i] = (double)mDepth;
	}
	mY = (double)mDepth;
	mVelY = -mCurrentJumpSpeed;
}

// 0x4442b0: once the level has run out it picks its jump (the slow one while a turret lives, the fast one after), throws
// hellfire when it has none; its head arcs up out of the sand and down into it (turning toward the tank as it sinks),
// its ribs follow its head, sand sprays where it enters and leaves, boulders fly where it dives in, and every rib
// that touches the tank kills it
void WormBoss::Update()
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	Enemy::Update();
	int aTurret = 0;
	bool aHasTurret = false;
	while (aTurret < NUM_TURRETS)
	{
		if (mTurrets[aTurret] != NULL)
		{
			aHasTurret = true;
			break;
		}
		aTurret++;
	}

	if (aHasTurret)
	{
		mCurrentJumpSpeed = mJumpSpeed[0];
		mCurrentGravity = mJumpGravity[0];
		mIsFast = false;
		mDepth = mJumpDepth[0];
		mCurrentLateral = mJumpLateral[0];
	}
	else
	{
		mCurrentJumpSpeed = mJumpSpeed[1];
		mCurrentGravity = mJumpGravity[1];
		mDepth = mJumpDepth[1];
		Board* aBoard = mBoard;
		mIsFast = true;
		mCurrentLateral = mJumpLateral[1];
		if (aBoard->mTankDeadTime == 0)
			mFireTimer = mFireTimer - 1;
		else
			mFireTimer = mFire;
		if (mY < HELLFIRE_MAX_Y && mFireTimer < 1 && aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
		{
			new Hellfire(mBoard, mX, mY, mVelX, mVelY, 1.0);
			mFireTimer = mFire;
		}
	}

	if (mBoard->mBossDeath == NULL)
	{
		mFrame = mFrame + HEAD_FRAME_SPEED;
		if (HEAD_FRAMES <= mFrame)
			mFrame = mFrame - HEAD_FRAMES;
		mSegY[0] = mVelY + mSegY[0];
		if (TURN_Y <= mSegY[0])
		{
			mVelX = (mBoard->mTankX - mSegX[0]) * TURN_FACTOR;
			if (mCurrentLateral < mVelX)
				mVelX = mCurrentLateral;
			if (mVelX < -mCurrentLateral)
				mVelX = -mCurrentLateral;
		}
		else
		{
			mSegX[0] = mSegX[0] + mVelX;
			mVelY = mVelY + mCurrentGravity;
			if (mCurrentJumpSpeed < mVelY)
				mVelY = mCurrentJumpSpeed;
		}
	}
	else
	{
		// dying, it thrashes about, pulled toward the middle of the field
		if (mApp->mUpdateCount % DEATH_THRASH_PERIOD == 0)
		{
			mVelX = (double)(Rand() % THRASH_RANGE) * THRASH_STEP - 1.0;
			mVelX = mVelX - mSegX[0] * DEATH_X_PULL;
			mVelY = ((double)(Rand() % THRASH_RANGE) * THRASH_STEP - 1.0) -
				(mSegY[0] - DEATH_CENTRE_Y) * DEATH_Y_PULL;
		}
		mSegX[0] = mSegX[0] + mVelX;
		mSegY[0] = mVelY + mSegY[0];
	}

	mY = mSegY[0];
	mX = mSegX[0];
	mIsAlive = !(SURFACED_Y <= mSegY[0]);
	if (TAIL_GONE_Y < mSegY[NUM_SEGMENTS - 1] && (double)mDepth < mSegY[0])
		NewJump(false);

	for (int i = 1; i < NUM_SEGMENTS; i++)
	{
		mSegX[i] = mSegX[i] - (mSegX[i] - mSegX[i - 1]) * RIB_FOLLOW;
		mSegY[i] = mSegY[i] - (mSegY[i] - mSegY[i - 1]) * RIB_FOLLOW;
	}

	// the sand: it sprays while the worm's body crosses the surface (head and tail on different sides) or it leaves
	// the deep going up
	if ((mSegY[0] < GROUND_Y && GROUND_Y <= mSegY[NUM_SEGMENTS - 1]) ||
		(GROUND_Y < mSegY[0] && mSegY[NUM_SEGMENTS - 1] < GROUND_Y) || (SPLASH_Y < mSegY[0] && mVelY < 0.0))
	{
		if (mSandFade != SAND_FADE_IN)
		{
			mSandX = (int)mSegX[0];
			mSandFade = SAND_FADE_IN;
			mApp->PlaySample(SOUND_EARTHQUAKE, mSandX * SAND_SOUND_PAN);
		}
	}
	else
		mSandFade = SAND_FADE_OUT;

	// where its head dives under the surface it throws boulders
	if (GROUND_Y < mSegY[0] && mVelY + mSegY[0] < GROUND_Y && 0 < mBoulders)
	{
		int aBoulder = 0;
		do
		{
			double aVelY = (double)(Rand() % BOULDER_VEL_Y_RANGE) * THRASH_STEP - 5.0;
			double aVelX = ((double)(Rand() % BOULDER_VEL_X_RANGE) * THRASH_STEP + mVelX) - 2.0;
			double aBoulderX = ((double)(Rand() % BOULDER_X_RANGE) + mX) - 40.0;
			new Boulder(mBoard, aBoulderX, BOULDER_Y, aVelX, aVelY, false);
			aBoulder++;
		}
		while (aBoulder < mBoulders);
	}

	mSandAlpha = mSandAlpha + mSandFade;
	if (mSandAlpha < 256)
	{
		if (mSandAlpha < 0)
			mSandAlpha = 0;
	}
	else
	{
		mSandAlpha = SAND_MAX_ALPHA;
		if (mApp->mUpdateCount % SAND_PUFF_PERIOD == 0)
		{
			double aPuffX = (double)(Rand() % SAND_PUFF_RANGE - SAND_PUFF_RANGE / 2 + mSandX);
			mBoard->mParticles.Emit(mApp->mSprites[IMAGE_SANDDUST], aPuffX, SAND_PUFF_Y, 0.0, SAND_PUFF_VEL_Y, 1,
				SAND_PUFF_SPREAD, SAND_PUFF_GRAVITY, SAND_PUFF_VEL_Y, SAND_PUFF_LIFE, true, -1);
		}
	}

	Board* aBoard = mBoard;
	if (aBoard->mBossDeath == NULL && aBoard->mTankDeadTime == 0)
	{
		for (int i = 0; i < NUM_SEGMENTS; i++)
		{
			if (mRib->Collides((int)mSegX[i], (int)mSegY[i], 0, 0, false, mApp->mSprites[IMAGE_TANK],
				(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
				mBoard->KillTank();
		}
		// oddity: the original takes its head's place from one past its rib arrays, which is its head's y for the x
		// and, for the y, the armour and fire delay of its turrets as a double (0 for any value they take)
		if (mSprite->Collides((int)mSegY[0], 0, (int)mFrame, 0, false, mApp->mSprites[IMAGE_TANK],
			(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
			mBoard->KillTank();
	}

	if (mBoard->mTankDeadTime == 2 && (mY < GROUND_Y || mSegY[NUM_SEGMENTS - 1] < GROUND_Y))
		mBoard->mTankDeadTime = 3;
}

// 0x443f80: its ribs from the tail to the head, its turrets, the head, and the sand it sprays
void WormBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	for (int i = NUM_SEGMENTS - 1; i >= 0; i--)
		mRib->Draw(g, (int)mSegX[i], (int)mSegY[i], true, i % 4, 0, false);

	for (int i = 0; i < NUM_TURRETS; i++)
	{
		WormTurret* aTurret = mTurrets[i];
		if (aTurret != NULL)
		{
			if (aTurret->mHitFlash != 0)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255 - aTurret->mHitFlash, 255 - aTurret->mHitFlash, 255));
			}
			mTurretSprite->Draw(g, (int)aTurret->mX, (int)aTurret->mY, true, (int)aTurret->mFrame, 0, false);
			g->SetColorizeImages(false);
		}
	}

	Enemy::Draw(g);
	if (mSandAlpha != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mSandAlpha));
		mSand->Draw(g, mSandX, SAND_DRAW_Y, true, (mApp->mUpdateCount / SAND_FRAME_PERIOD) % 4, 0, false);
		g->SetColorizeImages(false);
	}
}

// 0x444120: the points, and its death
void WormBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
	}
}

//
// WormTurret
//

// 0x445b90: it takes the worm's turret armour, sprite and delay, and is not counted among the enemies spawned
WormTurret::WormTurret(Board* theBoard, WormBoss* theWorm, int theIndex) : Enemy(theBoard)
{
	mWorm = theWorm;
	mHealth = (double)theWorm->mTurretArmor;
	mMaxHealth = mHealth;
	mSprite = theWorm->mTurretSprite;
	mFrame = 0.0;
	mPoints = 0;
	mFireTimer = theWorm->mTurretFire;
	mIndex = theIndex;
	mIsAlive = true;
	mFrameSpeed = TURRET_FRAME_SPEED;		// nothing reads it
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
}

// 0x445c20: the sprite is the worm's
WormTurret::~WormTurret()
{
}

// 0x445c30: it rides its rib, and bombs the tank from there while it is above the sand and not rising too fast: an iron
// bomb leaning toward the tank, thrown with the turret's velocity
void WormTurret::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	WormBoss* aWorm = mWorm;
	int aSegment = PART_SEGMENT_FIRST + mIndex * PART_SEGMENT_STEP;
	mVelX = aWorm->mSegX[aSegment] - mX;
	mVelY = aWorm->mSegY[aSegment] - mY;
	mX = aWorm->mSegX[aSegment];
	mY = aWorm->mSegY[aSegment];
	if (TURRET_HIDE_Y <= mY)
		return;

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime == 0)
		mFireTimer = mFireTimer - 1;
	else
		mFireTimer = aWorm->mTurretFire;
	if (0 < mFireTimer)
		return;
	if (TURRET_FIRE_MAX_Y <= mY)
		return;
	if (aBoard->mNukeFlash != 0.0)
		return;
	if (TURRET_FIRE_MAX_VEL_Y <= mVelY)
		return;
	if (aBoard->mBossDeath != NULL)
		return;

	mFireTimer = aWorm->mTurretFire;
	double aLean = (aBoard->mTankX - mX) * TURRET_AIM_FACTOR;
	if (aLean <= TURRET_AIM_MAX)
	{
		if (-TURRET_AIM_MAX > aLean)
			aLean = -TURRET_AIM_MAX;
	}
	else
		aLean = TURRET_AIM_MAX;
	double aVelX = ((double)(Rand() % THRASH_RANGE) * THRASH_STEP - 1.0) + aLean;
	new IronBomb(mBoard, mX, mY, aVelX, mVelY, aVelX < 0.0);
}

// 0x445e20: the worm forgets this turret, and it is counted back out of the enemies destroyed
void WormTurret::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	mWorm->mTurrets[mIndex] = NULL;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]--;
	mBoard->mEnemiesDestroyed--;
}
