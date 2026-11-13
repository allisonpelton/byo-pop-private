// BossesWrecker.cpp: the wrecker boss (images\wrecker): a crane on four wheels that drives in from the right and swings
// a wrecking ball on a chain over the field; the ball is its only weapon, and its chain pays out as long as the
// tank lives. Killing it drops the ball.
// Original: 0x445e90 (destructor body), 0x445f60 Update, 0x4466f0 Draw, 0x446a30 Killed, 0x446b50 SilenceSounds,
// 0x446b80 RestoreSounds, 0x446c30 its data, 0x447210 constructor.

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
#include <math.h>
#include <stdlib.h>

using namespace Sexy;

namespace
{
	const double START_Y = 190.0;				// the tower's height (its hook, where the chain hangs from)
	const double START_APPROACH_X = 640.0;		// it starts this far off to the right
	const double START_APPROACH_VEL = -1.0;
	const double ARRIVAL_X = 20.0;				// it slows down within this
	const double ARRIVAL_FACTOR = -0.05;		// to this much of its distance a tick
	const double STOPPED_X = 0.5;				// and has arrived within this
	const double START_PHASE = 1.570796;		// its two swings start a quarter turn on
	const double PHASE_SPEED_A = 0.005;			// radians a swing advances an update
	const double PHASE_SPEED_B = 0.015;
	const double PHASE_PERIOD = 6.28318;
	const double SWING_RADIUS = 100.0;			// the hook swings 100 either way, for each swing
	const double START_CHAIN_LENGTH = 200.0;
	const double CHAIN_STEP = 0.5;				// pixels the chain pays out (or draws in) an update
	const double START_BALL_Y = 250.0;
	const double HOOK_Y = 50.0;					// the chain hangs from this height
	const double BALL_GRAVITY = 0.5;
	const double BALL_PULL_X = 0.00025;			// the ball is pulled toward under the hook by this
	const double BALL_PULL_Y = 0.01;			// and toward the end of the chain by this
	const double WHEEL_FRAMES = 12.0;
	const double WHEEL_FACTOR = -0.4;			// the wheels turn with its speed
	const int ENGINE_NEAR = 320;				// its engine is at full volume this near the middle
	const double ENGINE_FAR_X = 640.0;
	const double ENGINE_VOLUME_FACTOR = 0.005;
	const double ENGINE_PITCH_START = 1.0;		// and it winds down as it brakes
	const double ENGINE_PITCH_FACTOR = -0.1;
	const double SLAM_Y = 410.0;				// the ball slams the ground below this
	const int SLAM_LEVEL = 12;					// it throws boulders only on the later encounter
	const int SLAM_BOULDERS = 10;
	const double SLAM_BOULDER_Y = 460.0;
	const int SLAM_SHAKE = 0x80;
	const int SHAKE_DECAY = 2;
	const int BODY_OFFSET_Y = 181;
	const int WHEEL_OFFSET_Y = 225;
	const int WHEEL_OUTER_X = 215;
	const int WHEEL_INNER_X = 133;
	const double SHADOW_FADE_Y = 200.0;			// the ball's shadow shows once it is this low, fading in
	const int SHADOW_Y = 0x1d1;
	const double LINK_STEP = 16.0;				// the chain's links are 16 apart
	const double LINK_STOP_Y = 48.0;
	const double LINK_ANGLE_OFFSET = 1.570795;
	const int CAB_X = 12;
	const int CAB_Y = 0x23;
	const int CAB_WIDTH = 0x18;
	const int CAB_HEIGHT = 10;
	const double DEATH_BALL_GRAVITY = 0.1;		// the ball falls this fast again each update once it is dead
	const double DEATH_CHAIN_STEP = 2.0;
	const int SOUND_PAN_FACTOR = 3;
}

// 0x447210: it loads its armour, points and chain length from the data, its sprites, and its engine's sound (silent
// at first and always running)
WreckerBoss::WreckerBoss(Board* theBoard) : Enemy(theBoard)
{
	char aBlock[16];
	GetBossBlockName(mApp, aBlock);
	BossData aData("Wrecker", aBlock);					// 0x446c30
	mHealth = (double)aData.Int("", "armor");
	mPoints = aData.Int("", "score");
	mLongChain = aData.Attribute("", "longchain") == "yes" ? 0x170 : 0x160;

	mMaxHealth = mHealth;
	mSprite = new Sprite(mApp);
	mSprite->Load("images\\wrecker\\tower", 1, 1);
	mBody = new Sprite(mApp);
	mBody->Load("images\\wrecker\\body", 1, 1);
	mWheel = new Sprite(mApp);
	mWheel->Load("images\\wrecker\\wheel", 12, 1);
	mLink = new Sprite(mApp);
	mLink->Load("images\\wrecker\\link", 1, 1);
	mBall = new Sprite(mApp);
	mBall->Load("images\\wrecker\\ball", 1, 1);
	mShadow = new Sprite(mApp);
	mShadow->Load("images\\wrecker\\shadow", 1, 1);

	mSwingA = START_PHASE;
	mSwingB = START_PHASE;
	mFrame = 0.0;
	mWheelFrame = 0.0;
	mX = 0.0;
	mY = START_Y;
	mVelX = 0.0;
	mVelY = 0.0;
	mApproachX = START_APPROACH_X;
	mApproachVel = START_APPROACH_VEL;
	mIsAlive = false;
	mChainLength = START_CHAIN_LENGTH;
	mBallX = 0.0;
	mBallY = START_BALL_Y;
	mBallVelX = 0.0;
	mBallVelY = 0.0;
	mChainSound = NULL;
	mFallingBall = NULL;
	mHasSlammed = false;
	mShake = 0;
	mEngineSound = mApp->mSoundManager->GetSoundInstance(SOUND_DIESEL);
	if (mEngineSound != NULL)
	{
		mEngineSound->SetVolume(0.0);
		mEngineSound->Play(true, true);
	}
}

// 0x445e90 (deleting 0x446c00)
WreckerBoss::~WreckerBoss()
{
	delete mSprite;
	delete mBody;
	delete mWheel;
	delete mLink;
	delete mBall;
	delete mShadow;
	delete mFallingBall;
	if (mEngineSound != NULL)
		mEngineSound->Release();
	if (mChainSound != NULL)
		mChainSound->Release();
}

// 0x445f60: once the level has run out it drives in (braking near the stop), swings its hook from two sines, and
// whirls the ball around it on its chain (a spring toward the end of the chain); the ball kills the tank it touches
// and, at the long chain on the later encounter, throws boulders and dust where it slams the ground; the chain pays
// out while the tank lives and draws in while it is dead (with a chain sound on while it moves). Once the boss is
// dead its ball falls away.
void WreckerBoss::Update()
{
	if (mBoard->mLevelTime < mBoard->mLevelLength)
		return;

	Enemy::Update();
	if (mFallingBall != NULL)
	{
		mFallingBall->mX = mFallingBall->mVelX + mFallingBall->mX;
		mFallingBall->mY = mFallingBall->mVelY + mFallingBall->mY;
		mFallingBall->mVelY = mFallingBall->mVelY + DEATH_BALL_GRAVITY;
		mChainLength = mChainLength - DEATH_CHAIN_STEP;
		if (mChainLength < 0.0)
			mChainLength = 0.0;
	}

	if (mBoard->mBossDeath == NULL)
	{
		mSwingA = mSwingA + PHASE_SPEED_A;
		mSwingB = mSwingB + PHASE_SPEED_B;
	}

	if (!mIsAlive)
	{
		mApproachX = mApproachX + mApproachVel;
		mWheelFrame = mWheelFrame - mApproachVel * WHEEL_FACTOR;
		if (mWheelFrame < 0.0)
			mWheelFrame = mWheelFrame + WHEEL_FRAMES;
		if (mEngineSound != NULL)
		{
			mEngineSound->SetPan((int)(mApproachX * SOUND_PAN_FACTOR));
			if (abs((int)mApproachX) <= ENGINE_NEAR)
				mEngineSound->SetVolume(1.0);
			else
				mEngineSound->SetVolume((ENGINE_FAR_X - mApproachX) * ENGINE_VOLUME_FACTOR * mApp->GetSfxVolume());
		}
		if (mApproachX < ARRIVAL_X)
		{
			mApproachVel = mApproachX * ARRIVAL_FACTOR;
			if (mApproachX < STOPPED_X)
			{
				mIsAlive = true;
				mApp->PlaySample(SOUND_AIRBRAKE, 0);
			}
		}
	}

	if (mEngineSound != NULL && ENGINE_PITCH_START < mApproachX && mApproachX < ARRIVAL_X)
	{
		mEngineSound->SetPan((int)(mApproachX * SOUND_PAN_FACTOR));
		mEngineSound->AdjustPitch((ARRIVAL_X - mApproachX) * ENGINE_PITCH_FACTOR);
	}

	if (PHASE_PERIOD <= mSwingA)
		mSwingA = mSwingA - PHASE_PERIOD;
	if (PHASE_PERIOD <= mSwingB)
		mSwingB = mSwingB - PHASE_PERIOD;
	mX = (cos(mSwingA) + cos(mSwingB)) * SWING_RADIUS + mApproachX;

	// the ball and its chain
	double aBallX = mBallX + mBallVelX;
	mBallX = aBallX;
	double aBallY = mBallY + mBallVelY + BALL_GRAVITY;
	mBallY = aBallY;
	double aLength = sqrt(pow(aBallX - mX, 2.0) + pow(aBallY - HOOK_Y, 2.0));
	double aScale = mChainLength / aLength;
	mBallX = (aBallX - mX) * aScale + mX;
	mBallY = (aBallY - HOOK_Y) * aScale + HOOK_Y;
	mBallVelX = (mX - mBallX) * BALL_PULL_X + mBallVelX;
	mBallVelY = ((mChainLength + HOOK_Y) - mBallY) * BALL_PULL_Y;

	Board* aBoard = mBoard;
	if (aBoard->mBossDeath == NULL && aBoard->mTankDeadTime == 0)
	{
		if (mBall->Collides((int)mBallX, (int)mBallY, 0, 0, false, mApp->mSprites[IMAGE_TANK],
			(int)aBoard->mTankX, aBoard->mTankY, (int)aBoard->mTreadFrame, 0, false, true))
			mBoard->KillTank();
	}

	if (mShake != 0)
	{
		int aShake = mShake;
		int aY = Rand() % (aShake / 16 + 1) - aShake / 32;
		int aX = Rand() % (aShake / 16 + 1) - aShake / 32;
		mBoard->Move(aX, aY);
		mShake -= SHAKE_DECAY;
	}

	if (mBoard->mBossDeath != NULL)
		return;

	if (SLAM_Y <= mBallY)
	{
		if ((double)mLongChain == mChainLength && mApp->mLevel == SLAM_LEVEL && !mHasSlammed)
		{
			for (int i = SLAM_BOULDERS; i != 0; i--)
			{
				double aVelY = (double)(Rand() % 200) * 0.01 - 5.0;
				double aVelX = (mBallVelX * 0.25 + (double)(Rand() % 600) * 0.01) - 3.0;
				double aBoulderX = ((double)(Rand() % 80) + mBallX) - 40.0;
				new Boulder(mBoard, aBoulderX, SLAM_BOULDER_Y, aVelX, aVelY, true);
			}
			mBoard->mParticles.Emit(mApp->mSprites[IMAGE_SANDDUST], mBallX, SLAM_BOULDER_Y, mBallVelX * 0.25, -1.0, 20,
				2.0, -0.01, -0.5, 200, true, -1);
			mBoard->mParticles.Emit(mApp->mSprites[IMAGE_ROCK], mBallX, SLAM_BOULDER_Y, mBallVelX * 0.25, -2.0, 10,
				1.0, 0.05, 3.0, 200, false, -1);
			mHasSlammed = true;
			mShake = SLAM_SHAKE;
			mApp->PlaySample(SOUND_BIGTHUD, (int)(mBallX * SOUND_PAN_FACTOR));
		}
	}
	else
		mHasSlammed = false;

	// the chain pays out to its full length while the tank lives and the crane is in place, and draws back to 200 while
	// the tank is dead (and a tank is left); its chain sound runs while it moves
	if (mBoard->mTankDeadTime == 0 && mIsAlive)
	{
		if ((double)mLongChain <= mChainLength)
		{
			ReleaseChainSound();
			return;
		}
		mChainLength = mChainLength + CHAIN_STEP;
		if (mChainSound != NULL)
			return;
		mChainSound = mApp->mSoundManager->GetSoundInstance(SOUND_CHAIN);
	}
	else
	{
		if (mApp->mLives < 1)
			return;
		if (mChainLength <= START_CHAIN_LENGTH)
		{
			ReleaseChainSound();
			return;
		}
		mChainLength = mChainLength - CHAIN_STEP;
		if (mChainSound != NULL)
			return;
		mChainSound = mApp->mSoundManager->GetSoundInstance(SOUND_CHAIN);
	}
	if (mChainSound != NULL)
		mChainSound->Play(true, true);
}

// port: the shared end of both branches of the chain's control: its sound stops when the chain does (0x4466c6)
void WreckerBoss::ReleaseChainSound()
{
	if (mChainSound != NULL)
	{
		mChainSound->Release();
		mChainSound = NULL;
	}
}

// 0x4466f0: its body, the tower, the ball's shadow, four wheels, the chain's links from the ball up to the hook, the ball
// and a small light on the cab
void WreckerBoss::Draw(Graphics* g)
{
	if (mBoard->mLevelLength > mBoard->mLevelTime)
		return;

	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mBody->Draw(g, (int)mApproachX, (int)(mY + BODY_OFFSET_Y), true, 0, 0, false);
	mSprite->Draw(g, (int)mX, (int)mY, true, 0, 0, false);
	int aShadowAlpha = (int)(mBallY - SHADOW_FADE_Y);
	if (0 < aShadowAlpha)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, aShadowAlpha));
		mShadow->Draw(g, (int)mBallX, SHADOW_Y, true, 0, 0, false);
		g->SetColorizeImages(false);
	}
	mWheel->Draw(g, (int)(mApproachX - WHEEL_OUTER_X), (int)(mY + WHEEL_OFFSET_Y), true, (int)mWheelFrame, 0, false);
	mWheel->Draw(g, (int)(mApproachX + WHEEL_OUTER_X), (int)(mY + WHEEL_OFFSET_Y), true, (int)mWheelFrame, 0, false);
	mWheel->Draw(g, (int)(mApproachX - WHEEL_INNER_X), (int)(mY + WHEEL_OFFSET_Y), true, (int)mWheelFrame, 0, false);
	mWheel->Draw(g, (int)(mApproachX + WHEEL_INNER_X), (int)(mY + WHEEL_OFFSET_Y), true, (int)mWheelFrame, 0, false);
	g->SetColorizeImages(false);

	double aLinkX = mBallX;
	double aBearing = atan2(mBallX - mX, mBallY - HOOK_Y);
	double aLinkY = mBallY;
	if (LINK_STOP_Y < aLinkY)
	{
		double aStepX = cos(aBearing - LINK_ANGLE_OFFSET) * LINK_STEP;
		double aStepY = sin(aBearing - LINK_ANGLE_OFFSET) * LINK_STEP;
		do
		{
			mLink->Draw(g, (int)aLinkX, (int)aLinkY, true, 0, 0, false);
			aLinkX = aLinkX - aStepX;
			aLinkY = aStepY + aLinkY;
		}
		while (LINK_STOP_Y < aLinkY);
	}

	if (mFallingBall == NULL)
		mBall->Draw(g, (int)mBallX, (int)mBallY, true, 0, 0, false);
	else
		mBall->Draw(g, (int)mFallingBall->mX, (int)mFallingBall->mY, true, 0, 0, false);
	g->SetColor(Color(0xc0, 0xc0, 0xc0));
	g->FillRect((int)(mX - CAB_X), CAB_Y, CAB_WIDTH, CAB_HEIGHT);
}

// 0x446a30: the points, its death, and its ball comes off its chain; the chain's sound ends
void WreckerBoss::Killed()
{
	if (mBoard->mBossDeath == NULL)
	{
		Enemy::Killed();
		mBoard->AddScore(mPoints, (int)mX, (int)mY);
		mBoard->mBossDeath = new BossDeath(mBoard);
		mFallingBall = new FallingBall;
		mFallingBall->mX = mBallX;
		mFallingBall->mY = mBallY;
		mFallingBall->mVelX = mBallVelX;
		mFallingBall->mVelY = mBallVelY;
		if (mChainSound != NULL)
		{
			mChainSound->Release();
			mChainSound = NULL;
		}
	}
}

// 0x446b50: both its sounds go quiet while the game is paused
void WreckerBoss::SilenceSounds()
{
	if (mEngineSound != NULL)
		mEngineSound->SetVolume(0.0);
	if (mChainSound != NULL)
		mChainSound->SetVolume(0.0);
}

// 0x446b80: and come back (the engine by its distance)
void WreckerBoss::RestoreSounds()
{
	if (mEngineSound != NULL)
	{
		if (abs((int)mApproachX) <= ENGINE_NEAR)
			mEngineSound->SetVolume(1.0);
		else
			mEngineSound->SetVolume((ENGINE_FAR_X - mApproachX) * ENGINE_VOLUME_FACTOR * mApp->GetSfxVolume());
	}
	if (mChainSound != NULL)
		mChainSound->SetVolume(1.0);
}
