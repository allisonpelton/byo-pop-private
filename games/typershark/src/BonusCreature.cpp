#include "BonusCreature.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x40dd00, 0x40dd10, 0x40dd20, 0x40dd30, 0x40dd40, 0x40dd50,
// 0x40e060 (jump to the virtual functions below or to the Enemy level), 0x40dd60 (deleting destructor), 0x40dda0 (part
// destructor: the vtables, then Enemy's 0x4078b0)

// 0x40db40: GameObject (0x4611b0) and Enemy (0x40a3f0) first. The first frame is drawn from the game's generator
BonusCreature::BonusCreature(WinWSApp* theApp) :
	Enemy(theApp),
	mAlpha(0xff),
	mQuarterVelY(0.0),
	mFrameOffset(0)
{
	mState = 0;
	mNumFrames = 20;
	mFrame = gGameRand->NextNoAssert() % mNumFrames;
	mFrameDelay = 6;
	mFrameWidth = mApp->mImages[IMAGE_BONUS_CREATURE_SWIM]->GetWidth() / 20;
	mFrameHeight = mApp->mImages[IMAGE_BONUS_CREATURE_SWIM]->GetHeight();
	mImages[0] = mApp->mImages[IMAGE_BONUS_CREATURE_SWIM];
	mImages[2] = mApp->mImages[IMAGE_BONUS_CREATURE_DEATH];
	mVelX = 0.0;
	mImage = mImages[0];
	mTypedFont = mApp->mFont12pt;
	mFont = mApp->mFont12pt;
	mStealthy = false;
}

BonusCreature::~BonusCreature()
{
}

// 0x40dde0 slot 1: one tick (a hidden creature only counts it). The frame advances every mFrameDelay ticks; the
// creature rises by mVelY, and when it has gone above the top of the screen (higher than its picture is tall) it dies
// with events 6 and 5. In state 3 the death picture stops at frame 7 and reports 6 and 5 once
void BonusCreature::Update()
{
	IncTick();
	if (mVisible)
	{
		if (mTick % mFrameDelay == 0)
		{
			mFrame++;
			if (mFrame >= mNumFrames)
			{
				if (mState == 3)
				{
					mFrame = 7;
					if (!mDeathFinished)
					{
						mDeathFinished = true;
						mEvents.push_back(6);
						mEvents.push_back(5);
					}
				}
				else
					mFrame = 0;
			}
			mFrameOffset = mFrameWidth * mFrame;
		}

		mY = mY - mVelY;
		if (mState == 0 && (double)(-mImages[0]->GetHeight()) > mY)
		{
			mState = 3;
			mFrame = 0;
			mDeathFinished = true;
			mEvents.push_back(6);
			mEvents.push_back(5);
		}
	}
}

// 0x408e10 slot 2: the picture frame, then the word over it in state 0 and not while the board is paused: the typed
// letters in red (235, 0, 0) with the typed font, the rest in black with the other font, the whole word centred on the
// creature (4 right of the middle, 14 above it)
void BonusCreature::Draw(Graphics* g)
{
	if (mVisible)
	{
		GameObject::Draw(g);
		Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
		g->DrawImage(mImage, (int)mLastX, (int)mLastY, aSrc);
		int aCenterX = (mFrameWidth >> 1) + 4;
		if (mState == 0)
		{
			Board* aBoard = mApp->mBoard;
			if (aBoard == NULL || !aBoard->mPaused)
			{
				if (mNumTyped > 0)
				{
					g->SetFont(mTypedFont);
					g->SetColor(Color(235, 0, 0));
					std::string aTyped = mWord.substr(0, mNumTyped);
					double aCenter = (double)aCenterX;
					int aY = (int)((double)(mFrameHeight >> 1) + mLastY - 14.0);
					int aX = (int)(aCenter + mLastX - (double)(mTypedFont->StringWidth(mWord) >> 1) - 5.0);
					g->DrawString(aTyped, aX, aY);

					g->SetFont(mFont);
					g->SetColor(Color(0, 0, 0));
					std::string aTyped2 = mWord.substr(0, mNumTyped);
					std::string aRest = mWord.substr(mNumTyped, mWord.size());
					aY = (int)((double)(mFrameHeight >> 1) + mLastY - 14.0);
					int aTypedWidth = mFont->StringWidth(aTyped2);
					double aRestX = (double)aTypedWidth + mLastX + aCenter;
					aX = (int)(aRestX - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
					g->DrawString(aRest, aX, aY);
				}
				else
				{
					g->SetFont(mFont);
					g->SetColor(Color(0, 0, 0));
					int aY = (int)((double)(mFrameHeight >> 1) + mLastY - 14.0);
					double aCenter = (double)aCenterX + mLastX;
					int aX = (int)(aCenter - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
					g->DrawString(mWord, aX, aY);
				}
			}
		}
	}
}

// 0x406180 slot 7: state 3 swaps to the death picture (8 frames, 3 ticks each, the creature moved 12 left and 20 up so
// the bigger picture is centred), plays the jellyfish sound unless mKilledByEmp is set; state 0 restores the swim picture
void BonusCreature::SetState(int theState)
{
	mState = theState;
	if (theState == 3)
	{
		mImage = mImages[2];
		mFrameOffset = 0;
		if (!mKilledByEmp)
			mApp->PlaySample(SOUND_JELLYFISH);
		mFrame = 0;
		mFrameDelay = 3;
		mNumFrames = 8;
		mFrameWidth = mImages[2]->GetWidth() / 8;
		mFrameHeight = mImages[2]->GetHeight();
		mX = mX - 12.0;
		mY = mY - 20.0;
	}
	else if (theState == 0)
	{
		mImage = mImages[0];
	}
	mFrameOffset = mFrameWidth * mFrame;
}

// 0x401fb0 slot 10: the speed up the screen; a quarter of it is kept in the field at +0xf0 (read by nobody here)
void BonusCreature::SetVelY(double theVelY)
{
	mVelY = theVelY;
	mQuarterVelY = mVelY * 0.25;
}
