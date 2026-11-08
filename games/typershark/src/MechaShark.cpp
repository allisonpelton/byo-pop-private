#include "MechaShark.h"
#include "MechaTorpedo.h"
#include "WinWSApp.h"
#include "Board.h"
#include "LessonManager.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): GameObject slots 0 (0x40eeb0, to the deleting destructor 0x40eed0
// with the part destructor 0x40ef10 = a jump to TorpedoBoss's 0x408d00), 2 (0x40eec0, to 0x404460) and 7 (0x40f880, to
// 0x40ef20)

// 0x40ebe0: TorpedoBoss (0x40c630) first, then the shark's pictures, the fin in the rotor's place (the rotor's frame
// count from TorpedoBoss stays as it was), the way (as TorpedoBoss made it, with the new height), 10 frames, the
// places of its guns and the fin's place. In the abyss mode the shots can be faster (the fastest by 0.1) and the boss
// has hit points and words by the score
MechaShark::MechaShark(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig) :
	TorpedoBoss(theApp, thePlayer, theConfig)
{
	mImages[0] = mApp->mImages[IMAGE_BOSS_MECHA];
	mImages[2] = mApp->mImages[IMAGE_BOSS_MECHA_DEATH];
	mFrameWidth = mImages[0]->GetWidth();
	mFrameHeight = mImages[0]->GetHeight();
	mRotorImage = mApp->mImages[IMAGE_BOSS_MECHA_FIN];
	mMinX = (double)(mApp->mWidth - 0x109);
	mMaxX = (double)(mFrameWidth >> 1) + mMinX;
	mMinY = (double)(mApp->mHeight - mFrameHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - 0xa0);
	mMaxY = (double)(mApp->mHeight - mFrameHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight() - 0x32);
	mNumFrames = 10;
	mY = mMaxY;
	mX = (double)mApp->mWidth;

	if (mConfig->mAbyssMode != 0)
	{
		mShotSpeedMax = mShotSpeedMax + 0.1;
		mField10c = 200;
		mHitPoints = 200;
		mMinWordLength = 4;
		mMaxWordLength = 5;
		int aScore = mApp->mBoard->mField1c4;	// +0x1c4 of the board: the score
		if (aScore >= 11000)
		{
			mField10c = 400;
			mHitPoints = 400;
			mMinWordLength = 7;
			mMaxWordLength = 7;
		}
		else if (aScore >= 8000)
		{
			mMinWordLength = 6;
			mField10c = 300;
			mHitPoints = 300;
			mMaxWordLength = 7;
		}
		else if (aScore > 5000)
		{
			mMinWordLength = 5;
			mField10c = 300;
			mHitPoints = 300;
			mMaxWordLength = 7;
		}
	}

	mSlotOffsetY[0] = 0x62;
	mSlotOffsetY[1] = 0x83;
	mSlotOffsetY[2] = 0xa5;
	mRotorOffsetX = 0xb8;
	mRotorOffsetY = 0x3c;
	mRotorWidth = 0x27;
}

MechaShark::~MechaShark()
{
}

// 0x40f0a0: a mecha torpedo (new, 0x218 bytes, with the shark as the target it can hit and the player's cannon) 100 to
// the left of the shark, shown
Projectile* MechaShark::CreateProjectile()
{
	MechaTorpedo* aShot = new MechaTorpedo(mApp, this, mPlayer);
	aShot->SetX(mX - 100.0);
	aShot->SetVisible(true);
	return aShot;
}

// 0x40ef20: as TorpedoBoss::SetState (0x40ca50), but the states 0, 6 and 7 have 10 frames
void MechaShark::SetState(int theState)
{
	ProjectileBoss::SetState(theState);

	if (theState == 6 || theState == 0 || theState == 7)
	{
		mFrameWidth = mImages[0]->GetWidth();
		mFrameHeight = mImages[0]->GetHeight();
		mNumFrames = 10;
	}
	else if (theState == 3)
	{
		ExplosionRecord aRecord;
		aRecord.mFrame = 0;
		aRecord.mX = (double)(gGameRand->NextNoAssert() % (unsigned int)(int)((double)mApp->mWidth - mMinX)) + mMinX;
		aRecord.mY = (double)(gGameRand->NextNoAssert() % (unsigned int)(int)(mMaxY - mMinY)) + mMinY;
		mExplosions.push_back(aRecord);
		mFrameWidth = mImages[2]->GetWidth();
		mFrameHeight = mImages[2]->GetHeight();
		mNumFrames = 1;
	}
}

// 0x404460: as TorpedoBoss::Draw (0x405270), but alive the fin is not turned by the rotor's counter: it is the frame
// mFrame (mRotorWidth pixels apart) of the fin picture at the place mRotorOffsetX, mRotorOffsetY
void MechaShark::Draw(Graphics* g)
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
		Rect aSrc(mFrame * mRotorWidth, 0, mRotorWidth, mRotorImage->GetHeight());
		g->DrawImage(mRotorImage, (int)((double)mRotorOffsetX + mLastX), (int)((double)mRotorOffsetY + mLastY), aSrc);
	}

	size_t i = 0;
	while (i < mShots.size())
	{
		mShots[i]->Draw(g);
		i++;
	}
}
