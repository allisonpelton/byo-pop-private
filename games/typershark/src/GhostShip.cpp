#include "GhostShip.h"
#include "CannonBall.h"
#include "WinWSApp.h"
#include "WSTimer.h"
#include "Board.h"
#include "LessonManager.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "Common.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): GameObject slots 0 (0x40e8a0, to the deleting destructor 0x40e8e0
// with the part destructor 0x409920), 1 (0x40f890, to 0x40e930), 2 (0x40e8d0, to 0x404210), 3 and 4 (0x40e8b0 and
// 0x40e8c0, to the Enemy level 0x402180 and 0x4021a0) and 7 (0x40e920, to TorpedoBoss's 0x40ca50)

// 0x40e600: TorpedoBoss (0x40c630) first, then the galleon's pictures and way (the way runs from the left of the
// screen's middle, mMinX, as TorpedoBoss made it; the bobbing from 30 to the same height above the status bar), the
// places of its three guns, the rudder, and the sonar timer paused (it then never reaches 2 seconds, so the ghost ship
// never pings). In the abyss mode it has more hit points and speeds by the score
GhostShip::GhostShip(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig) :
	TorpedoBoss(theApp, thePlayer, theConfig),
	mRudderRising(false),
	mRudderFrame(0),
	mRudderHeight(0),
	mRudderWidth(0),
	mRudderFrames(0)
{
	mImages[0] = mApp->mImages[IMAGE_BOSS_GALLEON];
	mImages[2] = mApp->mImages[IMAGE_BOSS_GALLEON_DEATH];
	mFrameWidth = mImages[0]->GetWidth();
	mFrameHeight = mImages[0]->GetHeight();
	mMinY = 30.0;
	mMaxY = (double)(mApp->mHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - mFrameHeight - 0x32);
	mMaxX = (double)(mFrameWidth >> 1) + mMinX;
	mY = mMaxY;
	mX = (double)mApp->mWidth;
	mSlotOffsetY[0] = 0xdc;
	mSlotOffsetY[1] = 0x10a;
	mSlotOffsetY[2] = 0x10a;
	mRudderFrame = 0;
	mRudderRising = true;
	mRudderHeight = 0x2f;
	mRudderWidth = 0x15;
	mRudderFrames = 5;
	mSonarTimer->SetDisabled(true);

	if (mConfig->mAbyssMode != 0)
	{
		double aMore = 0.0;
		mField10c = 250;
		mHitPoints = 250;
		int aScore = mApp->mBoard->mField1c4;	// +0x1c4 of the board: the score
		if (aScore >= 18000)
		{
			aMore = 1.6;
			mField10c = 450;
			mHitPoints = 450;
		}
		else if (aScore >= 14000)
		{
			aMore = 1.3;
			mField10c = 350;
			mHitPoints = 350;
		}
		else if (aScore >= 10000)
		{
			aMore = 1.0;
			mField10c = 350;
			mHitPoints = 350;
		}
		else if (aScore >= 7000)
		{
			aMore = 0.8;
			mField10c = 300;
			mHitPoints = 300;
		}
		mField130 = 2;
		mMaxShots = 3;
		mShotSpeedMin = 0.8 + aMore;
		mShotSpeedMax = aMore + 0.9;
	}
}

GhostShip::~GhostShip()
{
}

// 0x408060: the lengths are not looked at. The word of a ball is one capital letter, drawn from the game's generator
// (rand % 26 + 'A') and drawn again, with the shots looked through from the first again, while a shot in flight has
// the same word (Enemy::GetWord, 0x4070c0, is the word)
std::string GhostShip::GetWord(int theMinLength, int theMaxLength)
{
	std::string aWord = StrFormat("%c", (int)(gGameRand->NextNoAssert() % 26) + 'A');
	size_t i = 0;
	while (i < mShots.size())
	{
		if (mShots[i]->mWord == aWord)
		{
			aWord = StrFormat("%c", (int)(gGameRand->NextNoAssert() % 26) + 'A');
			i = 0;
		}
		else
			i++;
	}
	return aWord;
}

// 0x40e9a0: a cannon ball (new, 0x218 bytes, with the ship as the target it can hit and the player's cannon) 100 to the
// left of the ship, shown
Projectile* GhostShip::CreateProjectile()
{
	CannonBall* aShot = new CannonBall(mApp, this, mPlayer);
	aShot->SetX(mX - 100.0);
	aShot->SetVisible(true);
	return aShot;
}

// 0x40e930: TorpedoBoss::Update (0x40cdc0), then, every 13th tick while the ship lives, the rudder's frame moves one
// on: up to the last frame, then down to the first, and so on
void GhostShip::Update()
{
	TorpedoBoss::Update();

	if (mState != 3)
	{
		if (mTick % 13 == 0)
		{
			if (mRudderRising)
			{
				mRudderFrame++;
				if (mRudderFrame >= mRudderFrames)
				{
					mRudderRising = false;
					mRudderFrame = mRudderFrame - 1;
					return;
				}
			}
			else
			{
				mRudderFrame = mRudderFrame - 1;
				if (mRudderFrame < 0)
				{
					mRudderFrame = mRudderFrame + 1;
					mRudderRising = true;
				}
			}
		}
	}
}

// 0x404210: as TorpedoBoss::Draw, but alive the ship is the galleon picture and the rudder, a frame of
// IMAGE_GALLEON_RUDDER (mRudderWidth wide, mRudderHeight high), 121 to the right and 300 below the ship's position;
// no rotor. Dying it is the same: the picture coloured white with the alpha mAlpha, and the explosions. Last the shots
void GhostShip::Draw(Graphics* g)
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
		Rect aSrc(mRudderFrame * mRudderWidth, 0, mRudderWidth, mRudderHeight);
		g->DrawImage(mApp->mImages[IMAGE_GALLEON_RUDDER], (int)(mLastX + 121.0), (int)(mLastY + 300.0), aSrc);
	}

	size_t i = 0;
	while (i < mShots.size())
	{
		mShots[i]->Draw(g);
		i++;
	}
}
