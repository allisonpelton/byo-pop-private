#include "ImageBlast.h"

#include <cfloat>

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "Common.h"
#include "Cubby.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

static const double RAND_SCALE = 4.656612875245797e-10;		// 1 / 2^31: Rand() as a number from 0 to 1

// 0x440030: the picture is cut into theRows by theCols pieces, which the last row and column fill out. A piece flies off
// the way the point it started from lies from the centre of the picture; the pieces that start in the lower half stand
// on the ground a little in front of the platform, and with a cubby and the flag set only the pieces above the
// platform's centre are made. (The draws are made in the original's order, so the game's random numbers stay in step.)
ImageBlast::ImageBlast(int theStyle, double theX, double theY, Image* theImage, const Rect& theSrcRect, int theRows,
		int theCols, double theGroundY, Cubby* theCubby, bool theFlag, bool theShatter) :
	mTick(0),
	mX(theX),
	mY(theY),
	mSrcRect(theSrcRect),
	mStyle(theStyle),
	mRadius(0.0),
	mRows(theRows),
	mCols(theCols),
	mImage(theImage),
	mShatter(theShatter),
	mFlag(theFlag),
	mFinished(false)
{
	if (mRows <= 0 || mCols <= 0)
	{
		mFinished = true;
		return;
	}

	int aPieceWidth = mSrcRect.mWidth / mCols;
	int aPieceHeight = mSrcRect.mHeight / mRows;
	mRadius = sqrt((double)(aPieceHeight * aPieceHeight + aPieceWidth * aPieceWidth)) * 0.5;

	double aHalfPieceHeight = (double)(aPieceHeight / 2);
	double aHalfPieceWidth = (double)(aPieceWidth / 2);
	double aPieceHeightValue = (double)aPieceHeight;
	int aPlatformHeight = IMAGE_CUBBY_PLATFORM->GetHeight();

	int aRowOffset = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		int aColOffset = 0;
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			BlastPiece aPiece;
			aPiece.mSrcRect.mX = mSrcRect.mX + aColOffset;
			aPiece.mSrcRect.mY = mSrcRect.mY + aRowOffset;
			aPiece.mSrcRect.mHeight = aPieceHeight;
			if (aRow == mRows - 1)
				aPiece.mSrcRect.mHeight = mSrcRect.mHeight % mRows + aPieceHeight;
			aPiece.mSrcRect.mWidth = aPieceWidth;
			if (aCol == mCols - 1)
				aPiece.mSrcRect.mWidth = mSrcRect.mWidth % mCols + aPieceWidth;

			aPiece.mX = (double)aColOffset + mX;
			aPiece.mY = (double)aRowOffset + mY;
			double aCenterX = (double)(mSrcRect.mWidth / 2) + mX;

			// how far from the centre of the picture, in pictures, the piece lies
			int aRand = Rand();
			double aOffsetY = (((double)(mSrcRect.mHeight / 2) + mY) - aPiece.mY - aHalfPieceHeight) / (double)mSrcRect.mHeight;
			double aVelocityY = (((double)aRand * RAND_SCALE - 0.5) - (aOffsetY + aOffsetY + 5.0)) * 0.4;
			aRand = Rand();
			double aOffsetX = ((aCenterX - aPiece.mX) - aHalfPieceWidth) / (double)mSrcRect.mWidth;
			double aVelocityX = (((double)aRand * RAND_SCALE - 0.5) - (aOffsetX + aOffsetX)) * 0.4;
			if (theCubby != nullptr)
			{
				aVelocityX = aVelocityX * 0.5;
				aVelocityY = aVelocityY * 1.1;
			}

			aRand = Rand();
			double aSpin = ((double)aRand * RAND_SCALE) * 0.1 - 0.05;
			aRand = Rand();
			double aLife = ((double)aRand * RAND_SCALE - 0.5) + 1.0;

			if (!mShatter)
			{
				if (mStyle == 1 || mStyle == 2)
				{
					aVelocityX = aVelocityX * 5.0;
					aVelocityY = aVelocityY - 4.0;
					aRand = Rand();
					aSpin = ((double)aRand * RAND_SCALE) * 0.2 - 0.1;
					if (0.0 <= aSpin)
						aSpin = aSpin + 0.1;
					else
						aSpin = aSpin - 0.1;
					aRand = Rand();
					aLife = ((double)aRand * RAND_SCALE - 0.5) + 1.5;
				}
			}
			else
			{
				aVelocityX = aVelocityX * 10.0;
				aVelocityY = aVelocityY - 3.0;
				aRand = Rand();
				aSpin = ((double)aRand * RAND_SCALE) * 0.2 - 0.1;
				if (0.0 <= aSpin)
					aSpin = aSpin + 0.1;
				else
					aSpin = aSpin - 0.1;
				aRand = Rand();
				aLife = ((double)aRand * RAND_SCALE - 0.5) + 1.5;
			}

			bool aLowerHalf = (double)(mSrcRect.mHeight / 2) + mY <= aPiece.mY;
			double aFloorY;
			aRand = Rand();
			if (aLowerHalf)
				aFloorY = (((double)aRand * RAND_SCALE) * ((double)aPlatformHeight * 0.25) + (double)(aPlatformHeight / 2)) + theGroundY;
			else
				aFloorY = (theGroundY - (double)(aPlatformHeight / 2)) - ((double)aRand * RAND_SCALE) * ((double)aPlatformHeight * 0.25);
			aFloorY = aFloorY - aPieceHeightValue;

			aPiece.mVelocityX = aVelocityX;
			aPiece.mVelocityY = aVelocityY;
			aPiece.mAngle = 0.0;
			aPiece.mSpin = aSpin;
			aPiece.mLife = aLife;
			aPiece.mLifeStart = aLife;
			aPiece.mFloorY = aFloorY;
			aPiece.mBounced = false;
			aPiece.mFront = !aLowerHalf;

			bool anAdded = true;
			if (theCubby != nullptr)
			{
				if (!mFlag)
				{
					aPiece.mFloorY = DBL_MAX;
					aPiece.mFront = true;
				}
				else
				{
					double aCenter[2];
					theCubby->GetPlatformCenter(aCenter);
					if (aPiece.mY < aCenter[1] - aPieceHeightValue)
					{
						aPiece.mVelocityX = aPiece.mVelocityX + aPiece.mVelocityX;
						aPiece.mVelocityY = aPiece.mVelocityY * 1.2;
						int aRand1 = Rand();
						theCubby->GetPlatformCenter(aCenter);
						int aRand2 = Rand();
						aPiece.mFloorY = ((aCenter[1] - aPieceHeightValue) + ((double)aRand1 * RAND_SCALE) * ((double)aPlatformHeight * 0.125)) -
							((double)aRand2 * RAND_SCALE) * ((double)aPlatformHeight * 0.125);
					}
					else
						anAdded = false;
				}
			}

			if (anAdded)
				mPieces.push_back(aPiece);

			aColOffset += aPieceWidth;
		}
		aRowOffset += aPieceHeight;
	}

	if (theCubby != nullptr)
		mCubby.Set(theCubby);
}

// 0x43fb10
ImageBlast::~ImageBlast()
{
}

// 0x43fb70
void ImageBlast::Update()
{
	if (mFinished)
		return;

	mTick++;
	if (mStyle == 2 && (mTick & 3) != 0)
		return;

	int aDead = 0;
	for (size_t i = 0; i < mPieces.size(); i++)
	{
		BlastPiece& aPiece = mPieces[i];
		if (aPiece.mLife <= 0.0)
		{
			aDead++;
			continue;
		}

		aPiece.mX = aPiece.mX + aPiece.mVelocityX;
		aPiece.mY = aPiece.mVelocityY + aPiece.mY;
		if (aPiece.mY < aPiece.mFloorY)
			aPiece.mBounced = true;
		if (aPiece.mBounced && aPiece.mFloorY < aPiece.mY && 0.0 < aPiece.mVelocityY)
		{
			aPiece.mVelocityY = aPiece.mVelocityY * -0.6;
			aPiece.mSpin = aPiece.mSpin * -0.9;
			aPiece.mVelocityX = aPiece.mVelocityX * 0.9;
		}
		aPiece.mVelocityY = aPiece.mVelocityY + 0.13;
		aPiece.mAngle = aPiece.mSpin + aPiece.mAngle;
		aPiece.mLife = aPiece.mLife - 0.01;
	}

	if (aDead == mCols * mRows)
		mFinished = true;
}

// 0x43fca0
void ImageBlast::Draw(Graphics* g, bool theFront)
{
	Graphics aGraphics(*g);
	aGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);

	Cubby* aCubby = dynamic_cast<Cubby*>(mCubby.Get());

	for (size_t i = 0; i < mPieces.size(); i++)
	{
		BlastPiece& aPiece = mPieces[i];
		if (aPiece.mFront == theFront && 0.0 < aPiece.mLife)
		{
			if (aPiece.mLife <= 0.5)
			{
				aGraphics.SetColorizeImages(true);
				aGraphics.SetColor(Color(255, 255, 255, (int)(aPiece.mLife * 510.0)));
			}
			else
				aGraphics.SetColorizeImages(false);

			if (0.0 <= aPiece.mX && aPiece.mX + mRadius < (double)gSexyAppBase->mWidth &&
				0.0 <= aPiece.mY && mRadius + aPiece.mY < (double)gSexyAppBase->mHeight)
			{
				aGraphics.DrawImageRotatedF(mImage, (float)aPiece.mX, (float)aPiece.mY, aPiece.mAngle, &aPiece.mSrcRect);
			}
		}
	}

	if (theFront && aCubby != nullptr)
		aCubby->DrawCover(&aGraphics);
}

// 0x440730
ImageBlast* Sexy::MakeImageBlast(int theStyle, double theX, double theY, Image* theImage, const Rect* theSrcRect,
	int theRows, int theCols, double theGroundY, Cubby* theCubby, bool theFlag, bool theShatter)
{
	return new ImageBlast(theStyle, theX, theY, theImage, *theSrcRect, theRows, theCols, theGroundY, theCubby, theFlag,
		theShatter);
}
