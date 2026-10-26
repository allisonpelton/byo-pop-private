// A piece (see Piece.h).

#include "Piece.h"
#include "Board.h"
#include "Effects.h"
#include "AlchemyApp.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Compat2004.h"
#include <math.h>
#include <stdlib.h>

using namespace Sexy;

// The seven piece colours (rdata 0x4de390; the gray one is not used by a piece)
const int Sexy::gPieceColors[9][3] =
{
	{255, 32, 32}, {0, 255, 0}, {64, 64, 255}, {255, 0, 255}, {255, 255, 0}, {32, 255, 255}, {255, 168, 0},
	{255, 255, 255}, {128, 128, 128}
};

// 0x40e433
Piece::Piece(AlchemyApp* theApp)
{
	mApp = theApp;
	mImage = NULL;
	mCol = 0;
	mRow = 0;
	mShape = 0;
	mColor = 0;
	mDelay = 0;
	mShrink = 0;
	mShrinkDir = 0;
	mFollowMouse = true;
	mFlyX = mFlyY = mFlyVX = mFlyVY = 0;
	mSpeed = 0;
	mSparkleAcc = 0;
	mPulse = 0;
	mPulseDir = 1;
	mStuck = false;
	mBomb = false;
	mBombFrame = 0;
	mSkull = false;

	mMouseVisible = false;
	mHasTransparencies = true;
}

// 0x40e4c2
void Piece::Update()
{
	Widget::Update();

	// the red ring of a piece with no place to go pulses
	if (mStuck && mVisible && mShrink == 0)
	{
		mPulse = mPulseDir * 0.085 + mPulse;
		if (mPulse < 0.0)
		{
			mPulseDir = 1;
			mPulse = 0.0;
		}
		if (mPulse > 1.5)
		{
			mPulseDir = -1;
			mPulse = 1.5;
		}
		MarkDirty();
	}

	// shrinking (placed and then gone) or growing back
	if (mShrinkDir != 0)
	{
		if (mDelay > 0)
			mDelay--;
		else
		{
			mShrink += mShrinkDir;
			if (mShrink < 0)
			{
				mShrink = 0;
				mShrinkDir = 0;
			}
			else if (mShrink > 0x13)
			{
				mShrink = 0x13;
				mShrinkDir = 0;
			}
			MarkDirty();
		}
	}

	// the bomb's fuse
	if (mUpdateCnt % 3 == 0 && mBomb)
	{
		mBombFrame = (mBombFrame + 1) % 0x20;
		MarkDirty();
	}

	if (mFollowMouse)
	{
		int aX = mWidgetManager->mLastMouseX - 0x30;
		int aY = mWidgetManager->mLastMouseY - 0x30;

		if (!mVisible)
		{
			mSpeed = 0;
			mSparkleAcc = 0;
		}
		else
		{
			int aDX = aX - mX;
			int aDY = aY - mY;
			int aDist = (int) sqrt((double) (aDY * aDY + aDX * aDX));
			mSpeed = mSpeed * 0.9 + aDist * 0.1;
			if (mSpeed > 20.0)
				mSpeed = 20.0;
		}

		Move(aX, aY);

		// the piece stands in for the mouse pointer while the mouse is over the board
		bool isHidden = !mWidgetManager->mMouseIn || mApp->mMinimized || !mApp->mDialogMap.empty() ||
			(mApp->mBoard != NULL && mApp->mBoard->mTrialPause != 0) || mApp->mHelp != NULL ||
			(mX < 0x66 && (mY < 0x113 || mY > 0x16a));
		SetVisible(!isHidden);
	}

	// a moving piece gives off sparkles
	mSparkleAcc = mSpeed * 0.025 + mSparkleAcc + 0.015;
	while (mSparkleAcc >= 1.0 && mVisible)
	{
		bool isBig = (CrtRand() % 2) == 0;
		int aY = CrtRand() % 0x22 + 0x14 + mY;
		int aX = CrtRand() % 0x22 + 0x14 + mX;
		mApp->mEffects->AddSparkle(aX, aY, true, isBig);
		mSparkleAcc -= 1.0;
	}
}

// 0x40e773: the symbol and its light
void Piece::Draw(Graphics* g)
{
	if (mImage != NULL)
	{
		g->DrawImage(mImage, 0xc, 0xc);
		return;
	}

	if (mShrink == 0)
	{
		if (mBomb)
		{
			g->DrawImage(mApp->mImages[IMAGE_SKULLS], 0xc, 0xc,
				Rect((mBombFrame % 8) * 0x32, (mBombFrame / 8) * 0x32, 0x32, 0x32));
			goto Stuck;
		}
		if (mSkull)
		{
			g->DrawImage(mApp->mImages[IMAGE_NULLBLOCK], 0xc, 0xc);
			goto Stuck;
		}

		g->SetColorizeImages(true);
		g->SetColor(Color(gPieceColors[mColor][0], gPieceColors[mColor][1], gPieceColors[mColor][2]));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mApp->mImages[mApp->mBoard->mHoverX == -1 ? IMAGE_LIGHT : IMAGE_BRITELIGHT], 0, 0);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0xc, 0xc, Rect(mShape * 0x32, 0, 0x32, 0x32));
	}
	else
	{
		if (mShrink <= 0x13)
		{
			int aPicture = mShape;
			if (mBomb)
				aPicture = 0x17;
			else if (mSkull)
				aPicture = 0x16;

			if (aPicture < 0x16)
			{
				// the light fades as the symbol shrinks
				g->SetColorizeImages(true);
				double aFade = (0x14 - mShrink) * 0.05;
				int aB = (int) (gPieceColors[mColor][2] * aFade);
				int aG = (int) (gPieceColors[mColor][1] * aFade);
				int aR = (int) (gPieceColors[mColor][0] * aFade);
				g->SetColor(Color(aR, aG, aB));
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[mApp->mBoard->mHoverX == -1 ? IMAGE_LIGHT : IMAGE_BRITELIGHT], 0, 0);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColor(Color(gPieceColors[mColor][0], gPieceColors[mColor][1], gPieceColors[mColor][2]));
			}
			g->DrawImage(mApp->mShapeImages[mShrink][aPicture], 0xc, 0xc);
		}
	}
	g->SetColorizeImages(false);

Stuck:
	if (mStuck)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mPulse * 255.0)));
		g->DrawImage(mApp->mImages[IMAGE_REDCIRCLE], 5, 5);
		g->SetColorizeImages(false);
	}
}
