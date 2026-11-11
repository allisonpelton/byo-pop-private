#include "Player.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated: 0x4614f0 (GameObject's 7-byte part destructor, reached from the deleting destructor 0x461e80)

// 0x461320: GameObject (0x4611b0) first. The diver starts in state 1 at x 30, y 0 (GameObject's zero) with a y speed
// of 1.0 (mVelY, which the state 2 rise uses), visible, and the music of order 0 is started. The score at which
// mLives grows first is picked by the mode (WinWSApp +0x690): 450000, 525000, 650000, 700000 or 900000. Not set here
// (and left to chance in the original, which does not clear its memory): mHoseFrameUp, mHoseY, mAlpha
Player::Player(WinWSApp* theApp) :
	mApp(theApp),
	mScore(0),
	mEmpCharge(0.0),
	mEmpActive(false),
	mEmpX(0.0),
	mLives(2),
	mLevelNumber(0),
	mDiveActive(false),
	mHoseFrame(0),
	mHoseFrameUp(false),
	mHoseY(0.0),
	mAlpha(0),
	mCloudRounds(0),
	mNextLevelScore(0),
	mExtraLivesOff(false),
	mExtraLifeFlashing(false),
	mMode(0),
	mBittenByShark(false)
{
	mVisible = true;
	mVelY = 1.0;
	mX = 30.0;
	mState = 1;
	mApp->ChangeMusic(0);
	mNumFrames = 8;
	mFrameDelay = 0x19;
	mFrameWidth = 0x32;
	mEmpX = mX + 50.0;
	mFrameHeight = mApp->mImages[IMAGE_DIVER_IDLE]->GetHeight();
	mMode = mApp->mDifficulty;
	switch (mMode)
	{
	case 0:
		mNextLevelScore = 450000;
		break;
	case 1:
		mNextLevelScore = 525000;
		break;
	case 2:
		mNextLevelScore = 650000;
		break;
	case 3:
		mNextLevelScore = 700000;
		break;
	case 4:
		mNextLevelScore = 900000;
		break;
	}
}

Player::~Player()
{
}

// 0x461520 slot 7: what each state sets up. The state is stored first. 4: the flag at +0x118 is cleared. 1: the diver
// climbs down (sound, x 30, y 0, 8 frames of 50 wide, a frame every 22 ticks). 3: beaten: the dust cloud (7 frames, a
// frame every 4 ticks) at x -25 and 25 higher (not when he comes from state 4); 2: the beaten picture at x 21, from
// the y of the hose, and the music of order 0 unless the board has a number in +0xeb8; 10: the same x; 6 and 11 change
// the music to order 0x2e
void Player::SetState(int theState)
{
	int aOldState = mState;
	int aRise = (aOldState == 4) ? 0 : 0x19;
	mState = theState;
	if (theState == 4)
	{
		mDiveActive = false;
		return;
	}
	if (theState == 1)
	{
		mApp->PlaySample(SOUND_DIVERENTERS);
		mHoseFrame = 0;
		mY = 0.0;
		mHoseY = 0.0;
		mX = 30.0;
		mNumFrames = 8;
		mFrameDelay = 0x16;
		mFrameWidth = 0x32;
		mFrameHeight = mApp->mImages[IMAGE_DIVER_IDLE]->GetHeight();
		return;
	}
	if (theState == 3)
	{
		// the original also reads the byte at +0x144 (mBittenByShark, set by the Board's event loops) and does nothing with it
		mFrameDelay = 4;
		mCloudRounds = 0;
		mFrame = 0;
		mY = mY - (double)aRise;
		mAlpha = 0xff;
		mX = -25.0;
		mFrameWidth = mApp->mImages[IMAGE_RUMBLECLOUD]->GetWidth() / 7;
		mFrameHeight = mApp->mImages[IMAGE_RUMBLECLOUD]->GetHeight();
		mNumFrames = 7;
		return;
	}
	if (theState == 2)
	{
		mFrameWidth = mApp->mImages[IMAGE_DIVER_BEATEN]->GetWidth();
		mFrameHeight = mApp->mImages[IMAGE_DIVER_BEATEN]->GetHeight();
		mY = mHoseY;
		mX = 21.0;
		if (mApp->mBoard->mBoss == NULL)
			mApp->ChangeMusic(0);
		return;
	}
	if (theState == 10)
	{
		mX = 21.0;
		return;
	}
	if (theState == 6 || theState == 0xb)
		mApp->ChangeMusic(0x2e);
}

// 0x461740 slot 1: one tick (nothing in state 8). Every mFrameDelay ticks the frame advances (not in states 4 to 7, 11
// and 12) and wraps at mNumFrames, counting a finished dust cloud in state 3. Every 22 ticks the hose frame moves one
// step up or down between 0 and 7. Then the state: 1 climbs down (2 pixels a tick, 1 below the top picture's height,
// until y 235, where the diver arrives: flag at +0x118 and state 0); 3 fades the dust cloud after four rounds and goes
// to state 2 when it is faint; 2 fades and rises with the hose; 4 and 12 sink to y 325. The EMP wave moves right
void Player::Update()
{
	mTick++;
	int aState = mState;
	if (aState != 8)
	{
		if (mTick % mFrameDelay == 0 && aState != 5 && aState != 4 && aState != 6 && aState != 7 && aState != 0xb && aState != 0xc)
		{
			mFrame++;
			if (mFrame >= mNumFrames)
			{
				mFrame = 0;
				if (aState == 3)
					mCloudRounds++;
			}
		}

		if (mTick % 0x16 == 0)
		{
			if (!mHoseFrameUp)
			{
				mHoseFrame--;
				if (mHoseFrame < 0)
				{
					mHoseFrame = 1;
					mHoseFrameUp = true;
				}
			}
			else
			{
				mHoseFrame++;
				if (mHoseFrame >= 8)
				{
					mHoseFrame = mHoseFrame - 2;
					mHoseFrameUp = false;
				}
			}
		}

		if (aState == 1)
		{
			mHoseY = mY;
			if (!(mY < 235.0))
			{
				mDiveActive = true;
				mState = 0;
			}
			else if (!(mY < 235.0))
			{
				// the same test as above: never reached
				mY = mY + 0.5;
			}
			else if ((double)mApp->mImages[IMAGE_TOPBACKGROUND]->GetHeight() < mY)
				mY = mY + 1.0;
			else
				mY = mY + 2.0;
		}
		else if (aState == 3)
		{
			if (mCloudRounds >= 4)
			{
				mAlpha -= 8;
				if (mAlpha < 0)
					mAlpha = 0;
				if (mAlpha <= 0x80)
					SetState(2);
			}
		}
		else if (aState == 2)
		{
			mAlpha -= 8;
			if (mAlpha < 0)
				mAlpha = 0;
			double aHoseY = mHoseY - mVelY;
			mHoseY = aHoseY;
			mY = aHoseY;
		}
		else if (aState == 4 || aState == 0xc)
		{
			if (mY < 325.0)
			{
				mY = mY + 0.5;
				mHoseY = mHoseY + 0.5;
			}
		}

		if (mEmpActive)
		{
			mEmpX = mEmpX + 10.0;
			if ((double)mApp->mWidth < mEmpX)
			{
				mEmpActive = false;
				mEmpX = (double)mFrameWidth + mX;
			}
		}
	}
}

// 0x461f00 slot 2 (nothing in state 8): the hose first, two pieces of the hose picture (9 wide, one frame of 8, as
// tall as the picture) at x 40, the lower end 3 below the hose's y, one above the other. Then the diver: the dust cloud
// state draws the beaten diver at 48 right and 5 below, then the cloud with the colour alpha mAlpha; states 2 and 10
// the beaten picture; any other state the frame of the diver
void Player::Draw(Graphics* g)
{
	if (mState != 8)
	{
		int aHoseHeight = mApp->mImages[IMAGE_UPPER_HOSE]->GetHeight();
		GameObject::Draw(g);
		if (mVisible)
		{
			int aHoseY = aHoseHeight;
			int aCount = 2;
			do
			{
				Rect aSrc(mHoseFrame * 9, 0, 9, aHoseHeight);
				g->DrawImage(mApp->mImages[IMAGE_UPPER_HOSE], 0x28, (int)(mHoseY - (double)aHoseY + 3.0), aSrc);
				aHoseY = aHoseY + aHoseHeight;
				aCount--;
			} while (aCount != 0);

			if (mState == 3)
			{
				Image* aCloud = mApp->mImages[IMAGE_RUMBLECLOUD];
				Image* aBeaten = mApp->mImages[IMAGE_DIVER_BEATEN];
				Rect aBeatenSrc(0, 0, aBeaten->GetWidth(), aBeaten->GetHeight());
				g->DrawImage(aBeaten, (int)(mLastX + 48.0), (int)(mLastY + 5.0), aBeatenSrc);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
				Rect aCloudSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
				g->DrawImage(aCloud, (int)mLastX, (int)mLastY, aCloudSrc);
				g->SetColorizeImages(false);
			}
			else if (mState == 2 || mState == 10)
			{
				Rect aSrc(0, 0, mFrameWidth, mFrameHeight);
				g->DrawImage(mApp->mImages[IMAGE_DIVER_BEATEN], (int)mLastX, (int)mLastY, aSrc);
			}
			else
			{
				Rect aSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
				g->DrawImage(mApp->mImages[IMAGE_DIVER_IDLE], (int)mLastX, (int)mLastY, aSrc);
			}

			if (mEmpActive)
				g->DrawImage(mApp->mImages[IMAGE_EMPWAVE], (int)mEmpX, 0);
		}
	}
}

// 0x4619d0: adds to the score. When the score has reached the next threshold (and mExtraLivesOff is not set), mLives grows by
// one, mExtraLifeFlashing is set, and the threshold moves on: by mode, a fixed list for the first thresholds, then a fixed step
// (400000, 600000, 800000, 1500000 and 2000000 for the modes 0 to 4)
void Player::AddScore(int theScore)
{
	mScore = mScore + theScore;
	if (!mExtraLivesOff && mNextLevelScore <= mScore)
	{
		mLives++;
		mExtraLifeFlashing = true;
		switch (mMode)
		{
		case 0:
			if (mNextLevelScore != 450000)
				mNextLevelScore = mNextLevelScore + 400000;
			else
				mNextLevelScore = 625000;
			break;
		case 1:
			if (mNextLevelScore == 525000)
				mNextLevelScore = 775000;
			else if (mNextLevelScore != 775000)
				mNextLevelScore = mNextLevelScore + 600000;
			else
				mNextLevelScore = 1150000;
			break;
		case 2:
			if (mNextLevelScore == 650000)
				mNextLevelScore = 900000;
			else if (mNextLevelScore != 900000)
				mNextLevelScore = mNextLevelScore + 800000;
			else
				mNextLevelScore = 1550000;
			break;
		case 3:
			if (mNextLevelScore == 700000)
				mNextLevelScore = 1250000;
			else if (mNextLevelScore != 1250000)
				mNextLevelScore = mNextLevelScore + 1500000;
			else
				mNextLevelScore = 1750000;
			break;
		case 4:
			if (mNextLevelScore == 900000)
				mNextLevelScore = 1500000;
			else if (mNextLevelScore == 1500000)
				mNextLevelScore = 2000000;
			else
				mNextLevelScore = mNextLevelScore + 2000000;
			break;
		}
	}
}

// 0x461b50: sets the score and works out the threshold that goes with it (the one AddScore would have reached after the
// same score), without touching mLives. It agrees with the progression of AddScore except in mode 1, where a score
// from 750000 to 774999 gets 1150000, not 775000
void Player::SetScore(int theScore)
{
	mScore = theScore;
	switch (mMode)
	{
	case 0:
		if (theScore >= 625000)
		{
			mNextLevelScore = 625000;
			do
			{
				mNextLevelScore = mNextLevelScore + 400000;
			} while (mNextLevelScore <= mScore);
			return;
		}
		mNextLevelScore = ((theScore < 450000) ? 0 : 175000) + 450000;
		return;
	case 1:
		if (theScore >= 1150000)
		{
			mNextLevelScore = 1150000;
			do
			{
				mNextLevelScore = mNextLevelScore + 600000;
			} while (mNextLevelScore <= mScore);
			return;
		}
		if (theScore < 750000)
		{
			mNextLevelScore = ((theScore < 525000) ? 0 : 250000) + 525000;
			return;
		}
		mNextLevelScore = 1150000;
		return;
	case 2:
		if (theScore >= 1550000)
		{
			mNextLevelScore = 1550000;
			do
			{
				mNextLevelScore = mNextLevelScore + 800000;
			} while (mNextLevelScore <= mScore);
			return;
		}
		if (theScore < 900000)
		{
			mNextLevelScore = ((theScore < 650000) ? 0 : 250000) + 650000;
			return;
		}
		mNextLevelScore = 1550000;
		return;
	case 3:
		if (theScore >= 1750000)
		{
			mNextLevelScore = 1750000;
			do
			{
				mNextLevelScore = mNextLevelScore + 1500000;
			} while (mNextLevelScore <= mScore);
			return;
		}
		if (theScore < 1250000)
		{
			mNextLevelScore = ((theScore < 700000) ? 0 : 550000) + 700000;
			return;
		}
		mNextLevelScore = 1750000;
		return;
	case 4:
		if (theScore >= 2000000)
		{
			mNextLevelScore = 2000000;
			do
			{
				mNextLevelScore = mNextLevelScore + 2000000;
			} while (mNextLevelScore <= mScore);
			return;
		}
		if (theScore < 1500000)
		{
			mNextLevelScore = ((theScore < 900000) ? 0 : 600000) + 900000;
			return;
		}
		mNextLevelScore = 2000000;
		return;
	}
}
