// Board: the helpers of the drawing (DrawCel and DrawCelStretched, the game's own picture-cel calls) and the backdrop
// (0x437bd0). Draw itself (0x461e50) is in BoardDrawLayers.cpp, with the parts it is made of; the other sub-draws each
// have a file of their own. The original works out its positions on the x87 stack at 24 bits of precision (UpdateFrames sets
// it): every sum is a float before it is cut to a whole pixel, as _ftol does.

#include "Board.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x4b83f0: the game's own DrawImageCel. The cels of an image with several rows count down the rows, those of one
// with one row along the columns; a cel outside the image draws nothing.
void Sexy::DrawCel(Graphics* g, Image* theImage, int theX, int theY, int theCel)
{
	int aCol = 0;
	int aRow = 0;
	if (theImage->mNumRows > 1)
		aRow = theCel;
	else
		aCol = theCel;
	if (aRow < 0 || aCol < 0 || aRow >= theImage->mNumRows || aCol >= theImage->mNumCols)
		return;
	int aCelW = theImage->mWidth / theImage->mNumCols;
	int aCelH = theImage->mHeight / theImage->mNumRows;
	g->DrawImage(theImage, theX, theY, Rect(aCelW * aCol, aCelH * aRow, aCelW, aCelH));
}

// 0x4b8430: the same into a rectangle (the picture is stretched to it)
void Sexy::DrawCelStretched(Graphics* g, Image* theImage, const Rect& theDest, int theCel)
{
	int aCol = 0;
	int aRow = 0;
	if (theImage->mNumRows > 1)
		aRow = theCel;
	else
		aCol = theCel;
	if (aRow < 0 || aCol < 0 || aRow >= theImage->mNumRows || aCol >= theImage->mNumCols)
		return;
	int aCelW = theImage->mWidth / theImage->mNumCols;
	int aCelH = theImage->mHeight / theImage->mNumRows;
	g->DrawImage(theImage, theDest, Rect(aCelW * aCol, aCelH * aRow, aCelW, aCelH));
}

// 0x436200: four floats cut to a rectangle of whole pixels
static Rect FloatRect(const float theRect[4])
{
	return Rect((int) theRect[0], (int) theRect[1], (int) theRect[2], (int) theRect[3]);
}

//////////////////////////////////////////////////////////////////////////
// The backdrop

// 0x437bd0: the scenery layer, or on the warp and upgrade screens the planets; the lightning; the warp stars; the
// streaks of light (a pixel and its tail); the stars (3D only)
void Board::DrawBackdrop(Graphics* g)
{
	bool is3D = mApp->Is3DAccelerated();

	if (!mWarpFlight && !mAtWarpGate && !mWarpGatePicked)
	{
		// the scenery: the picture now on the screen and the one that follows it
		g->DrawImage(mSceneryLayer.mImage, (int) mSceneryX, (int) mSceneryLayer.mCurY, FloatRect(mSceneryLayer.mCur));
		if (mSceneryLayer.mNext[3] > 0.0f)
			g->DrawImage(mSceneryLayer.mImage, (int) mSceneryX, (int) mSceneryLayer.mNextY, FloatRect(mSceneryLayer.mNext));
	}
	else
	{
		g->SetColor(Color(0, 0, 0));
		g->FillRect(0, 0, mWidth, mHeight);

		// the planets (the second and the third only with 3D, the last two add their light)
		int anOffset = 0;
		do
		{
			if (is3D || (anOffset != 0x3c && anOffset != 0x78))
			{
				if (anOffset > 0x78)
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				PlanetRec& aPlanet = mPlanets[anOffset / 0x3c];
				g->DrawImage(aPlanet.mImage, (int) aPlanet.mRangeStart, (int) aPlanet.mCurY, FloatRect(aPlanet.mCur));
				if (aPlanet.mNext[3] > 0.0f)
					g->DrawImage(aPlanet.mImage, (int) aPlanet.mRangeStart, (int) aPlanet.mNextY, FloatRect(aPlanet.mNext));
				if (anOffset > 0x78)
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
			anOffset += 0x3c;
		} while (anOffset < 300);
	}

	if (mWarpLightningAlpha > 0 && (mWarpFlight || mAtWarpGate || mWarpGatePicked) && mWarpLightningImage != NULL && is3D)
	{
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(mWarpLightningAlpha, mWarpLightningAlpha, mWarpLightningAlpha, mWarpLightningAlpha));
		g->DrawImage(mWarpLightningImage, 100, mWarpLightningY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	// the warp stars (the original also steps a trail that nothing reads: PointerTrail::Step, 0x40b230)
	for (int i = 0; i < (int) mWarpStars.size(); i++)
	{
		WarpStar* aStar = mWarpStars[i];
		int aCel = (aStar->mAnim.mNumFrames == 1) ? 0 : aStar->mAnim.mFrame;
		DrawCel(g, aStar->mImage, (int) aStar->mX, (int) aStar->mY, aCel);
	}

	// the streaks: a pixel, and a tail of the picture behind it (above it on the warp screens)
	int aStreaks = (is3D ? 0x4b : 0) + 0x19;
	for (int i = 0; i < aStreaks; i++)
	{
		StreakRec& aStreak = mStreaks[i];
		float anX = aStreak.mX;
		float aY = aStreak.mY;
		int aMargin = aStreak.mMargin;
		g->SetColor(aStreak.mColor);
		int aPixelX = (int) anX;
		g->FillRect(aPixelX, (int) aY, 1, 1);

		bool aTailDown = mWarpCharging || mWarpActive || (mBonusWarpIn && (mBonusWarpPhase == 1 || mBonusWarpPhase == 2));
		bool aTailUp = mWarpFlight || mStreaksFadingOut || (mBonusWarpIn && mBonusWarpPhase == 3);
		if (aTailDown)
		{
			if (!mWarpFlight)
			{
				if (aMargin > 0)
				{
					int aLength = aMargin;
					float aTailTop = aY - (float) aMargin;    // (x87 at 24 bits: a float)
					if (480.0f < aTailTop)
						aLength = (int) (480.0f - aTailTop);
					g->DrawImage(aStreak.mImage, aPixelX, (int) (aY - (float) aStreak.mMargin), Rect(0, 0, 1, aLength));
					continue;
				}
				aTailUp = mWarpFlight || mStreaksFadingOut || (mBonusWarpIn && mBonusWarpPhase == 3);
			}
			else
				aTailUp = true;
		}
		if (aTailUp && aMargin > 0)
		{
			int aLength = aMargin;
			float aTailTop = aY - (float) aMargin;
			if (480.0f < aTailTop)
				aLength = (int) (480.0f - aTailTop);
			g->DrawImage(aStreak.mImage, aPixelX, (int) (aY - (float) aStreak.mMargin),
				Rect(0, 0x1e0 - aStreak.mMargin, 1, aLength));
		}
	}

	// the stars
	if (!mWarpFlight && !mAtWarpGate && !mWarpGatePicked && !mGameOver && is3D)
	{
		for (int i = 0; i < (int) mBackdropStars.size(); i++)
		{
			StarRec& aStar = mBackdropStars[i];
			float aY = aStar.mY;
			if ((float) -aStar.mImage->mHeight <= aY && aY <= (float) mHeight)
				g->DrawImageF(aStar.mImage, aStar.mX, aY);
		}
	}
}
