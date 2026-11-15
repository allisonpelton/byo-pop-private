// BossesRainer.cpp: the rainer boss (images\rainer): a flying saucer with a propeller to each side and a dish under it
// that raises a tractor beam, drops air mines, and calls down meteors.
// Original: 0x440c90 (destructor body), 0x440d20 Update, 0x441130 Killed, 0x4411c0 SilenceSounds, 0x4411e0
// RestoreSounds, 0x441240 Draw, 0x441450 its data, 0x441c30 constructor.

#include "Bosses.h"
#include "Board.h"
#include "BossDeath.h"
#include "EnemyProjectiles.h"
#include "HeavyWeaponApp.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Sprite.h"
#include <math.h>

using namespace Sexy;

namespace
{
	const double START_X = 520.0;				// it comes in from the right edge
	const double HOME_Y = 200.0;				// and floats about this height
	const double START_VEL_X = -0.75;
	const double ARRIVAL_X = 20.0;				// it is in the field once it has come this near the middle
	const int DRIFT_RANGE = 100;				// its speeds are pushed by random amounts and pulled toward home
	const double DRIFT_CENTRE = 50.0;
	const double DRIFT_X = 0.0005;
	const double PULL_X = 0.00025;				// toward the middle, by its distance from it
	const double PULL_Y = 0.0005;				// toward its height, by its distance from it
	const double DRIFT_Y = 0.00025;
	const double MAX_VEL_X = 0.75;
	const double MAX_VEL_Y = 0.25;
	const double PROP_SPEED = 0.5;				// frames of the propellers an update
	const double PROP_FRAMES = 8.0;
	const double DISH_STEP = 0.01;				// radians the dish turns up or down an update
	const double DISH_UP_ANGLE = 1.5701;		// it turns up to a quarter turn
	const double DISH_FIRING_ANGLE = 1.56;		// and the beam shows past this
	const double DISH_DOWN_ANGLE = 0.02;		// it may drop a mine when it is down this near
	const double DISH_Y = 80.0;					// the dish's centre is this far above the saucer's, plus
	const double DISH_RADIUS = 44.0;			// the sine of its angle times this
	const double PROP_LEFT_X = 139.0;
	const double PROP_RIGHT_X = 140.0;
	const double PROP_Y = 96.0;
	const double BEAM_X = 25.0;					// the beam's top left is this far to the left of the saucer
	const double BEAM_Y = 299.0;				// and this far above
	const int BEAM_WIDTH = 50;
	const int BEAM_HEIGHT = 162;
	const int BEAM_SCROLL_PERIOD = 38;			// its picture scrolls a pixel every third of an update
	const int BEAM_SCROLL_SPEED = 3;
	const double MINE_Y = 115.0;				// a mine drops from this far above the saucer
	const int MINE_SPEED_RANGE = 400;
	const double MINE_SPEED_STEP = 0.01;
	const double MINE_SPEED_OFFSET = 2.0;
	const double MINE_VEL_Y = -1.0;
	const int MINE_PAN = 0;
}

// 0x441c30: it loads its armour, points, mine delay and dish times and meteors from the data, its sprites and its
// beam sound
RainerBoss::RainerBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Rainer", aBlock);					// 0x441450
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mMineFire = aData.Int("", "fire");
	mDishUpTime = aData.Int("Dish", "up");
	mDishDownTime = aData.Int("Dish", "down");
	mMeteors = aData.Int("Dish", "meteors");

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\rainer\\rainer", 1, 1);
	mDish = new Sprite(mApp);
	mDish->Load("images\\rainer\\dish", 1, 1);
	mProp = new Sprite(mApp);
	mProp->Load("images\\rainer\\prop", 8, 1);
	mBeam = new Sprite(mApp);
	mBeam->Load("images\\rainer\\beam", 1, 1);
	mX = START_X;
	mY = HOME_Y;
	mVelX = START_VEL_X;
	mVelY = 0.0;
	mIsAlive = false;
	mMineTimer = 0;		// uninitialised in the original, which sets it before it reads it
	mPropFrame = 0.0;
	mDishAngle = 0.0;
	mIsDishUp = false;
	mDishTimer = mDishDownTime;
	mSound = mApp->mSoundManager->GetSoundInstance(SOUND_TRACTORBEAM);
}

// 0x440c90 (deleting 0x441210)
RainerBoss::~RainerBoss()
{
	delete mSprite;
	delete mDish;
	delete mProp;
	delete mBeam;
	if (mSound != NULL)
		mSound->Release();
}

// 0x440d20: once the level has run out it flies in, then drifts about the middle of the field at its height; its dish
// turns up and down on a timer, raising the tractor beam (and calling its meteors as it comes down), and it drops an
// air mine every so often while the dish is down
void RainerBoss::Update()
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	Enemy::Update();
	if (ARRIVAL_X <= mX || mIsAlive)
	{
		mVelX = ((double)(Rand() % DRIFT_RANGE) - DRIFT_CENTRE) * DRIFT_X + mVelX;
		mVelX = mVelX - mX * PULL_X;
		mVelY = (HOME_Y - mY) * PULL_Y + ((double)(Rand() % DRIFT_RANGE) - DRIFT_CENTRE) * DRIFT_Y + mVelY;
		if (mVelX < -MAX_VEL_X)
			mVelX = -MAX_VEL_X;
		if (MAX_VEL_X < mVelX)
			mVelX = MAX_VEL_X;
		if (mVelY < -MAX_VEL_Y)
			mVelY = -MAX_VEL_Y;
		if (MAX_VEL_Y < mVelY)
			mVelY = MAX_VEL_Y;
	}
	else
		mIsAlive = true;

	mX = mVelX + mX;
	mY = mVelY + mY;
	mPropFrame = mPropFrame + PROP_SPEED;
	if (PROP_FRAMES <= mPropFrame)
		mPropFrame = mPropFrame - PROP_FRAMES;

	if (mIsAlive)
	{
		if (!mIsDishUp || mBoard->mTankDeadTime != 0)
		{
			// the dish is down (or the tank is dead and it goes down): it lowers, and when its time is up it rises
			mSound->Stop();
			mIsDishUp = false;
			if (0.0 < mDishAngle)
				mDishAngle = mDishAngle - DISH_STEP;
			int aTimer = mDishTimer;
			mDishTimer = aTimer - 1;
			if (aTimer < 0)
			{
				mIsDishUp = true;
				mDishTimer = mDishUpTime;
			}
		}
		else
		{
			// the dish is up: it turns up, the beam hums while it shows, and when the time is up it lowers and the
			// meteors fall
			if (mDishAngle < DISH_UP_ANGLE)
				mDishAngle = mDishAngle + DISH_STEP;
			if (mBoard->mNukeFlash == 0.0 && mBoard->mBossDeath == NULL)
			{
				int aTimer = mDishTimer;
				mDishTimer = aTimer - 1;
				if (aTimer < 0)
				{
					mSound->Stop();
					mIsDishUp = false;
					mDishTimer = mDishDownTime;
					for (int i = 0; i < mMeteors; i++)
						new Meteor(mBoard);
				}
				else if (DISH_FIRING_ANGLE < mDishAngle && !mSound->IsPlaying())
					mSound->Play(true, true);
			}
		}
	}

	Board* aBoard = mBoard;
	if (aBoard->mTankDeadTime != 0 || !mIsAlive || DISH_DOWN_ANGLE < mDishAngle)
		mMineTimer = mMineFire;
	else
		mMineTimer = mMineTimer - 1;
	if (mMineTimer < 1 && aBoard->mNukeFlash == 0.0 && aBoard->mBossDeath == NULL)
	{
		double aVelX = (double)(Rand() % MINE_SPEED_RANGE) * MINE_SPEED_STEP - MINE_SPEED_OFFSET;
		new AirMine(mBoard, mX, mY - MINE_Y, aVelX, MINE_VEL_Y);
		mApp->PlaySample(SOUND_AIRMINE, MINE_PAN);
		mMineTimer = mMineFire;
	}
	if (mBoard->mBossDeath != NULL && mSound->IsPlaying())
		mSound->Stop();
}

// 0x441240: the dish below the saucer, the saucer, a propeller to each side, and the beam (added to the picture)
void RainerBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mDish->Draw(g, (int)mX, (int)((mY - DISH_Y) - sin(mDishAngle) * DISH_RADIUS), true, 0, 0, false);
	g->SetColorizeImages(false);
	Enemy::Draw(g);
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mProp->Draw(g, (int)(mX - PROP_LEFT_X), (int)(mY - PROP_Y), true, (int)mPropFrame, 0, true);
	mProp->Draw(g, (int)(mX + PROP_RIGHT_X), (int)(mY - PROP_Y), true, (int)mPropFrame, 0, false);
	g->SetColorizeImages(false);
	if (mIsDishUp && DISH_FIRING_ANGLE < mDishAngle && mBoard->mBossDeath == NULL)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Rect aSource(0, (BEAM_SCROLL_PERIOD - 1) - (mApp->mUpdateCount * BEAM_SCROLL_SPEED) % BEAM_SCROLL_PERIOD,
			BEAM_WIDTH, BEAM_HEIGHT);
		g->DrawImage(mBeam->mImage, (int)(mX - BEAM_X), (int)(mY - BEAM_Y), aSource);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x441130: the points, and its death
void RainerBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
	}
}

// 0x4411c0: the beam's hum goes quiet while the game is paused
void RainerBoss::SilenceSounds()
{
	if (mSound != NULL)
		mSound->SetVolume(0.0);
}

// 0x4411e0: and comes back at the effects volume
void RainerBoss::RestoreSounds()
{
	if (mSound != NULL)
		mSound->SetVolume(mApp->GetSfxVolume());
}
