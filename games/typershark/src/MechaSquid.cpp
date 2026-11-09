#include "MechaSquid.h"
#include "SquidCannonBall.h"
#include "WinWSApp.h"
#include "WSTimer.h"
#include "Board.h"
#include "LessonManager.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): GameObject slots 0 (0x40f650, to the deleting destructor 0x40f690
// with the part destructor 0x40f6d0 = a jump to TorpedoBoss's 0x408d00), 1 (0x40f870, to 0x40f6f0), 2 (0x40f680, to
// 0x4046a0), 3 and 4 (0x40f660 and 0x40f670, to the Enemy level 0x402180 and 0x4021a0) and 7 (0x40f6e0, to TorpedoBoss's
// 0x40ca50)

// 0x40f390: GhostShip (0x40e600) first, then the squid's pictures, the way (the bobbing from 100 down to the height the
// frame leaves above the status bar, the sideways from 0x113 left of the right edge for 50), the places of its three
// guns, the fin (GhostShip's rudder fields: 67x47 pixels, 10 frames), the eye, and the sonar timer paused again. In the
// abyss mode it has hit points and speeds by the score
MechaSquid::MechaSquid(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig) :
	GhostShip(theApp, thePlayer, theConfig),
	mEyeFrame(0),
	mEyeRising(false)
{
	mImages[0] = mApp->mImages[IMAGE_BOSS_SQUID];
	mImages[2] = mApp->mImages[IMAGE_BOSS_SQUID_DEATH];
	mFrameWidth = mImages[0]->GetWidth();
	mFrameHeight = mImages[0]->GetHeight();
	mMinY = 100.0;
	mMaxY = (double)(mApp->mHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - mFrameHeight - 0x32);
	mMinX = (double)(mApp->mWidth - 0x113);
	mMaxX = mMinX + 50.0;
	mY = mMaxY;
	mX = (double)mApp->mWidth;
	mSlotOffsetY[0] = 7;
	mSlotOffsetY[1] = 0x3a;
	mSlotOffsetY[2] = 0x78;
	mRudderHeight = 0x2f;
	mRudderWidth = 0x43;
	mRudderFrames = 10;
	mEyeFrame = 0;
	mEyeRising = true;
	mSonarTimer->SetDisabled(true);

	if (mConfig->mAbyssMode != 0)
	{
		double aMore = 0.0;
		mField10c = 250;
		mHitPoints = 250;
		int aScore = mApp->mBoard->mScore;	// +0x1c4 of the board: the score
		if (aScore >= 12000)
		{
			aMore = 1.7;
			mField10c = 400;
			mHitPoints = 400;
		}
		else if (aScore >= 9000)
		{
			aMore = 1.3;
			mField10c = 300;
			mHitPoints = 300;
		}
		else if (aScore >= 6000)
		{
			aMore = 0.9;
			mField10c = 300;
			mHitPoints = 300;
		}
		else if (aScore > 4000)
		{
			mField10c = 250;
			aMore = 0.8;
			mHitPoints = 250;
		}
		mField130 = 2;
		mMaxShots = 3;
		mShotSpeedMin = 0.7 + aMore;
		mShotSpeedMax = aMore + 0.8;
	}
}

MechaSquid::~MechaSquid()
{
}

// 0x40f7a0: a squid cannon ball (new, 0x21c bytes, with the squid as the target it can hit and the player's cannon)
// 100 to the left of the squid, shown
Projectile* MechaSquid::CreateProjectile()
{
	SquidCannonBall* aShot = new SquidCannonBall(mApp, this, mPlayer);
	aShot->SetX(mX - 100.0);
	aShot->SetVisible(true);
	return aShot;
}

// 0x40f6f0: TorpedoBoss::Update (0x40cdc0; not GhostShip's, so no rudder), then, while the squid lives, every 5th tick
// the fin goes on a frame (round and round) and every 8th tick the eye goes one frame up to the last, then down to the
// first, and so on
void MechaSquid::Update()
{
	TorpedoBoss::Update();

	if (mState != 3)
	{
		if (mTick % 5 == 0)
		{
			mRudderFrame++;
			if (mRudderFrame >= mRudderFrames)
				mRudderFrame = 0;
		}
	}

	if (mState != 3)
	{
		if (mTick % 8 == 0)
		{
			if (mEyeRising)
			{
				mEyeFrame++;
				if (mEyeFrame >= 5)
				{
					mEyeRising = false;
					mEyeFrame = mEyeFrame - 1;
					return;
				}
			}
			else
			{
				mEyeFrame = mEyeFrame - 1;
				if (mEyeFrame < 0)
				{
					mEyeFrame = mEyeFrame + 1;
					mEyeRising = true;
				}
			}
		}
	}
}

// 0x4046a0: as TorpedoBoss::Draw (0x405270) for a dying squid (the picture coloured white with the alpha mAlpha, and
// the explosions); alive the squid picture, over it the fin (frame mRudderFrame of 67x47 pixels, from IMAGE_BOSS_SQUID_FIN)
// 217 to the right and 60 below the position, and the eye (frame mEyeFrame of 12x12, from IMAGE_BOSS_SQUID_EYE) 133 to
// the right and 80 below. Last the shots
void MechaSquid::Draw(Graphics* g)
{
	GameObject::Draw(g);

	int aExplosionHeight = mApp->mImages[IMAGE_TORPEDO_EXPLODE]->GetHeight();
	int aExplosionWidth = mApp->mImages[IMAGE_TORPEDO_EXPLODE]->GetWidth() / 8;

	if (mState == 3)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mAlpha));
		g->DrawImage(mImages[2], (int)mLastX, (int)mLastY);
		g->SetColorizeImages(false);

		size_t i = 0;
		while (i < mExplosions.size())
		{
			ExplosionRecord* aRecord = &mExplosions[i];
			Rect aSrc(aRecord->mFrame * aExplosionWidth, 0, aExplosionWidth, aExplosionHeight);
			g->DrawImage(mApp->mImages[IMAGE_TORPEDO_EXPLODE], (int)aRecord->mX, (int)aRecord->mY, aSrc);
			i++;
		}
	}
	else
	{
		g->DrawImage(mImages[0], (int)mLastX, (int)mLastY);

		Rect aFinSrc(mRudderFrame * mRudderWidth, 0, mRudderWidth, mRudderHeight);
		g->DrawImage(mApp->mImages[IMAGE_BOSS_SQUID_FIN], (int)(mLastX + 217.0), (int)(mLastY + 60.0), aFinSrc);

		Rect aEyeSrc(mEyeFrame * 12, 0, 12, 12);
		g->DrawImage(mApp->mImages[IMAGE_BOSS_SQUID_EYE], (int)(mLastX + 133.0), (int)(mLastY + 80.0), aEyeSrc);
	}

	size_t i = 0;
	while (i < mShots.size())
	{
		mShots[i]->Draw(g);
		i++;
	}
}
