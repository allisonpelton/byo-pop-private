#include "ToxicShark.h"
#include "WinWSApp.h"
#include "WSTimer.h"
#include "WidgetManager.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x402290, 0x4022a0, 0x4022b0, 0x4022c0, 0x4065b0, 0x4065c0
// (jump to Shark's and GameObject's functions with the pointer adjusted), 0x40e0d0, 0x40e0e0, 0x409bc0 (deleting
// destructor)

// 0x4109e0: Shark (0x4101d0) first. mImage stays Shark's picture (Draw does not use it)
ToxicShark::ToxicShark(WinWSApp* theApp) :
	Shark(theApp),
	mScramblePeriod(0),
	mTimer(NULL)
{
	mImages[0] = mApp->mSharkImages[4][0];
	mImages[3] = mApp->mToxicSwimDim;
	mImages[2] = mApp->mSharkImages[4][2];
	mImages[4] = mApp->mSharkImages[4][4];
	mTypedColor = Color(255, 255, 255);
	mTextColor = Color(0, 0, 0);
	mTextOffsetX = 7;
	mScramblePeriod = 1000;
	mTimer = new WSTimer();
	mApp->mWidgetManager->AddWidget(mTimer);
}

// 0x401cf0 (the destructor of the whole object)
ToxicShark::~ToxicShark()
{
	mApp->mWidgetManager->RemoveWidget(mTimer);
	delete mTimer;
}

// 0x40d650: the next scramble comes sooner on the harder levels
void ToxicShark::TypeLetter()
{
	Enemy::TypeLetter();
	switch (mApp->mField690)
	{
	case 0:
		mScramblePeriod = 2500;
		return;
	case 1:
	case 2:
		mScramblePeriod = 2000;
		return;
	case 3:
		mScramblePeriod = 1700;
		return;
	case 4:
		mScramblePeriod = 1300;
		return;
	default:
		mScramblePeriod = 1500;
		return;
	}
}

// 0x40d6e0: Shark::Update, then while the shark swims with a word and the timer has run mScramblePeriod: the letters
// from the first one not typed on (from the second one while nothing is typed) are drawn out of a pile one at a time,
// each with a draw of the game's MTRand, and put back in that order; the timer starts again
void ToxicShark::Update()
{
	Shark::Update();
	if (!mWord.empty() && mState == 0 && mScramblePeriod <= mTimer->Elapsed())
	{
		unsigned int aRemaining = (unsigned int)mWord.size() - (unsigned int)mNumTyped;
		if (1 < (int)aRemaining)
		{
			unsigned int aStart = mNumTyped;
			if (aStart == 0)
				aStart = 1;

			std::string aPile = mWord.substr(aStart, aRemaining);
			std::string aShuffled = "";
			while (!aPile.empty())
			{
				unsigned int aDraw = gGameRand->NextNoAssert();
				unsigned int aSize = (unsigned int)aPile.size();
				char aLetter = aPile[aDraw % aSize];
				aShuffled.append(1, aLetter);
				aPile.erase(aDraw % aSize, 1);
			}

			mWord = mWord.substr(0, aStart) + aShuffled;
			mTyped = mWord.substr(0, mNumTyped);
			mRemaining = mWord.substr(mNumTyped, mWord.size());
		}
		mTimer->Reset();
	}
}

// the glow half of each case of Draw: the glow picture added, 10 pixels beyond the body on every side. The frame of
// the glow sheet is 20 larger than the body's
void ToxicShark::DrawGlow(Graphics* g, Image* theGlow)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	Rect aGlowSrc(mFrame * (mFrameWidth + 20), 0, mFrameWidth + 20, mFrameHeight + 20);
	g->DrawImage(theGlow, (int)(mLastX - 10.0), (int)(mLastY - 10.0), aGlowSrc);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x4055f0: by state: 0 and 4 the glow and the swim picture, 1 (mirrored) the app's mirrored copies, 3 the death
// pictures, 5 only the shock picture while mFlag30 is set (otherwise glow and swim picture), 6 the red picture while
// mFlashed (otherwise glow and swim picture), 2 nothing. Then the word as in Shark::Draw (states 0, 4 and 6, not
// while the board is paused)
void ToxicShark::Draw(Graphics* g)
{
	if (!mVisible)
		return;

	GameObject::Draw(g);
	Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
	switch (mState)
	{
	case 0:
	case 4:
		DrawGlow(g, mApp->mImages[IMAGE_TOXIC_GLOW_SWIM]);
		g->DrawImage(mApp->mImages[IMAGE_TOXIC_BASIC_SWIM], (int)mLastX, (int)mLastY, aSrc);
		break;
	case 1:
		DrawGlow(g, mApp->mToxicGlowSwimDim);
		g->DrawImage(mApp->mToxicSwimDim, (int)mLastX, (int)mLastY, aSrc);
		break;
	case 3:
		DrawGlow(g, mApp->mImages[IMAGE_TOXIC_GLOW_DEATH]);
		g->DrawImage(mApp->mImages[IMAGE_TOXIC_BASIC_DEATH], (int)mLastX, (int)mLastY, aSrc);
		break;
	case 5:
		if (mFlag30)
		{
			g->DrawImage(mApp->mImages[IMAGE_TOXIC_BASIC_SHOCK], (int)mLastX, (int)mLastY);
			break;
		}
		DrawGlow(g, mApp->mImages[IMAGE_TOXIC_GLOW_SWIM]);
		g->DrawImage(mApp->mImages[IMAGE_TOXIC_BASIC_SWIM], (int)mLastX, (int)mLastY, aSrc);
		break;
	case 6:
		if (mFlashed)
		{
			g->DrawImage(mImages[4], (int)mLastX, (int)mLastY, aSrc);
			break;
		}
		DrawGlow(g, mApp->mImages[IMAGE_TOXIC_GLOW_SWIM]);
		g->DrawImage(mApp->mImages[IMAGE_TOXIC_BASIC_SWIM], (int)mLastX, (int)mLastY, aSrc);
		break;
	default:
		break;
	}

	int aCenterX = (mFrameWidth >> 1) + mTextOffsetX;
	if (mState != 0 && mState != 4 && mState != 6)
		return;
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL && aBoard->mPaused)
		return;

	if (mNumTyped > 0)
	{
		g->SetFont(mTypedFont);
		g->SetColor(mTypedColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aX = (double)aCenterX + mLastX;
		int aTypedX = (int)(aX - (double)(mTypedFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mTyped, aTypedX, aY);

		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY2 = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aTypedWidth = (double)mFont->StringWidth(mTyped);
		int aRemainingX = (int)((aTypedWidth + mLastX + (double)aCenterX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mRemaining, aRemainingX, aY2);
	}
	else
	{
		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		int aWordX = (int)(((double)aCenterX + mLastX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mWord, aWordX, aY);
	}
}
