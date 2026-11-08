#include "ProjectileBoss.h"
#include "Projectile.h"
#include "WinWSApp.h"
#include "WSTimer.h"
#include "WidgetManager.h"
#include "LessonManager.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): GameObject slots 0 (0x4083c0, deleting destructor), 1 (0x40e080,
// to 0x40b360), 2 (0x4083d0, to 0x401a70 = GameObject::Draw), 3 and 4 (0x4083e0 and 0x4083f0, to the Enemy level
// 0x402180 and 0x4021a0) and 7 (0x40e090, to 0x40b240)

// 0x40b030: Enemy (0x40a3f0) first, then the timer and the numbers of the boss from the first record of the lesson
// manager (the abyss mode has its own). The boss is then floating in (state 0) at the speed 0.5 and says it was created
ProjectileBoss::ProjectileBoss(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig) :
	Enemy(theApp),
	mTimer(NULL),
	mFireInterval(0.0),
	mHitPoints(0),
	mField10c(0),
	mMinY(0.0),
	mMaxY(0.0),
	mMinX(0.0),
	mMaxX(0.0),
	mField130(0),
	mMaxShots(0),
	mBobVelY(0.0),
	mBobVelX(0.0),
	mShotSpeedMin(0.0),
	mShotSpeedMax(0.0),
	mConfig(NULL),
	mAlpha(0),
	mPlayer(NULL)
{
	mPlayer = thePlayer;
	mAlpha = 255;
	mTimer = new WSTimer();
	mTimer->SetDisabled(true);
	mApp->mWidgetManager->AddWidget(mTimer);
	mField10c = 100;
	mHitPoints = 100;
	mConfig = theConfig;
	if (mConfig->mAbyssMode == 0)
	{
		mFireInterval = mConfig->GetBossRecord(0)->mFireInterval;
		mField10c = mConfig->GetBossRecord(0)->mHitPoints;
		mHitPoints = mField10c;
		mBobVelY = mConfig->GetBossRecord(0)->mBobSpeed;
	}
	else
	{
		mFireInterval = 1000.0;
		mField10c = 200;
		mHitPoints = 200;
		mBobVelY = 0.2;
	}
	mBobVelX = mBobVelY;
	mState = 0;
	mEvents.push_back(10);
	mVelX = 0.5;
}

// 0x4082a0: each shot is deleted, the timer leaves the widget manager. Then Enemy's destructor (0x4078b0)
ProjectileBoss::~ProjectileBoss()
{
	for (size_t i = 0; i < mShots.size(); i++)
	{
		if (mShots[i] != NULL)
			delete mShots[i];
	}
	mShots.clear();
	mApp->mWidgetManager->RemoveWidget(mTimer);
	delete mTimer;	// port: the original does not free the timer (a leak)
}

// 0x4019b0: the bobbing. y moves by the speed mBobVelY, which turns round outside mMinY..mMaxY; while it fires (state
// 7) x moves too, by mBobVelX, which turns round outside mMinX..mMaxX
void ProjectileBoss::Move()
{
	mY = mY + mBobVelY;
	if (mY < mMinY || mMaxY < mY)
		mBobVelY = -mBobVelY;

	if (mState == 7)
	{
		if (mX < mMinX || mMaxX < mX)
			mBobVelX = -mBobVelX;
		mX = mBobVelX + mX;
	}
}

// 0x4083b0: the boss that fires has its own (TorpedoBoss); this one has nothing to do with a shot's events
bool ProjectileBoss::HandleShot(Projectile* theShot)
{
	return false;
}

// 0x40b360: the shots move first, each is shown to HandleShot, which says true when the shot is over (it is then
// removed from the list, not deleted: the boss that says so has taken it). Every mFrameDelay ticks the picture
// advances one frame; at the end it starts again, and a dying boss (state 3) says by Enemy's flag 0x18 that its
// fall is through. Then the state's move: 0 floats in to mMinX and bobs, 6 bobs and waits a second, 8 bobs until the
// player's cannon is ready, 3 falls up out of the picture
void ProjectileBoss::Update()
{
	IncTick();

	size_t i = 0;
	while (i < mShots.size())
	{
		mShots[i]->Update();
		if (HandleShot(mShots[i]))
		{
			mShots.erase(mShots.begin() + i);
			i--;
		}
		i++;
	}

	if (mTick % mFrameDelay == 0)
	{
		mFrame++;
		if (mFrame >= mNumFrames)
		{
			if (mState != 3)
				mFrame = 0;
			else
			{
				mFrame = 0;
				if (!mFlag18)
					mFlag18 = true;
			}
		}
	}

	if (mState == 0)
	{
		Move();
		mX = mX - mVelX;
		if (mX <= mMinX)
		{
			mX = mMinX;
			SetState(6);
		}
	}
	else if (mState == 6)
	{
		Move();
		if (mTimer->Elapsed() >= 1000)
			SetState(7);
	}
	else if (mState == 8)
	{
		Move();
		if (mPlayer->mState == 9)
			SetState(7);
	}
	else if (mState == 3)
	{
		mY = mY - 0.5;
		if (mY <= (double)(-mFrameHeight))
			mEvents.push_back(5);
	}
}

// 0x40b240: 6 starts the timer for the wait, 7 restarts it and says so (event 0xb), 8 with no hit points left is the
// end: state 3, event 0xc, and every shot still flying that is not exploding already is set to explode (state 3, after
// the speed is set to 1e-5, so that it counts as flying right)
void ProjectileBoss::SetState(int theState)
{
	mState = theState;
	if (theState == 6)
	{
		mTimer->SetDisabled(false);
		mTimer->Reset();
		return;
	}
	if (theState == 7)
	{
		mTimer->Reset();
		mEvents.push_back(0xb);
		return;
	}
	if (theState == 8)
	{
		if (mHitPoints > 0)
			return;
		SetState(3);
		mEvents.push_back(0xc);
		size_t i = 0;
		while (i < mShots.size())
		{
			if (mShots[i]->mState != 3)
			{
				mShots[i]->SetVelX(0.00001);
				mShots[i]->SetState(3);
			}
			i++;
		}
	}
}
