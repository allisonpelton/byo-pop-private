// The effects of Atomica Deluxe (see Effects.h).

#include "Effects.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "Graphics.h"
#include "Font.h"
#include "Atom.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// port: a colour whose alpha is cut to 0 to 255
static inline Color ClampedColor(int r, int g, int b, int a)
{
	return Color(r, g, b, a > 255 ? 255 : (a < 0 ? 0 : a));
}

// ---------------------------------------------------------------------------------------------------------
// Particle

// 0x418ca0
Particle::Particle(AtomicaApp* theApp, Sprite* theSprite) :
	Effect(theApp),
	mSprite(theSprite),
	mAlpha(0xff),
	mFade(2)
{
}

// 0x418ce0
bool Particle::Update()
{
	mX += mVX;
	mAlpha -= mFade;
	mY += mVY;
	return mAlpha > 0;
}

// 0x418d40
void Particle::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(ClampedColor(0xff, 0xff, 0xff, mAlpha));
	mSprite->DrawCenteredF(g, mX, mY);
	g->SetColorizeImages(false);
}

// ---------------------------------------------------------------------------------------------------------
// Flash

// 0x416660
Flash::Flash(AtomicaApp* theApp, Sprite* theSprite, int theR, int theG, int theB, int theA) :
	Effect(theApp),
	mLife(300),
	mSprite(theSprite),
	mColor(theR, theG, theB, theA)
{
}

// 0x4166f0
bool Flash::Update()
{
	mX += mVX;
	mLife--;
	mY += mVY;
	return mLife > 0;
}

// 0x416740
void Flash::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(ClampedColor(mColor.mRed, mColor.mGreen, mColor.mBlue, mLife));
	mSprite->DrawCenteredF(g, mX, mY);
	g->SetColorizeImages(false);
}

// ---------------------------------------------------------------------------------------------------------
// Spark

// 0x417a70
Spark::Spark(AtomicaApp* theApp) :
	Effect(theApp),
	mAlpha(0x15e),
	mColor(0, 0, 0, 255)
{
}

// 0x417ae0
bool Spark::Update()
{
	mAlpha -= 4;
	if (mAlpha < 0)
		return false;
	mX += mVX;
	mY += mVY;
	return true;
}

// 0x417b30
void Spark::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(ClampedColor(mColor.mRed, mColor.mGreen, mColor.mBlue, mAlpha));
	mApp->mElectronDot->DrawCenteredF(g, mX, mY);
	g->SetColorizeImages(false);
}

// ---------------------------------------------------------------------------------------------------------
// AtomPop

// 0x40c250
AtomPop::AtomPop(AtomicaApp* theApp) :
	Effect(theApp),
	mFrame(0)
{
	mApp->mSndNukeBlast->Play();
}

// 0x40c2c0
bool AtomPop::Update()
{
	mFrame += 0.25f;
	return mFrame < 20.0f;
}

// 0x40c310
void AtomPop::Draw(Graphics* g)
{
	int aFrame = (int) mFrame;
	mApp->mBigExplosion[aFrame]->DrawCenteredF(g, mX, mY);
}

// ---------------------------------------------------------------------------------------------------------
// Blast

// 0x418da0
Blast::Blast(AtomicaApp* theApp, int theDelay, bool theKills, int theTier) :
	Effect(theApp),
	mRadius(0),
	mGrow(0),
	mFrame(0),
	mDelay(theDelay),
	mKills(theKills),
	mGrid(theApp->mBoard->mGrid),
	mTier(theTier)
{
	if (theTier == 1)
		mGrow = 7.5f;
	else if (theTier == 2)
		mGrow = 15.0f;
	else if (theTier == 3)
		mGrow = 22.5f;
}

// 0x418e60
bool Blast::Update()
{
	mDelay--;
	if (mDelay < 1)
	{
		mDelay = 0;
		mFrame += 0.25f;
		if (mFrame >= 20.0f)
			return false;
		mRadius = mGrow * 0.15f + mRadius;
		if (mKills)
		{
			int aX = (int) mX;
			int aY = (int) mY;
			for (int aCol = 0; aCol < Grid::COLS; aCol++)
			{
				for (int aRow = 0; aRow < Grid::ROWS; aRow++)
				{
					Atom* anAtom = mGrid->mCells[aCol][aRow];
					if ((anAtom != NULL) && !anAtom->mPre)
					{
						int aPixel[2] = {0, 0};
						Grid::CellToPixel(aCol, aRow, aPixel);
						int aDX = aPixel[0] - aX;
						int aDY = aPixel[1] - aY;
						int aDist = (int) sqrt((double) (aDX * aDX + aDY * aDY));
						if ((float) aDist < mRadius)
						{
							if (mApp->mMode == 1)
								mApp->mBoard->mMeter.Hit(5);
							AtomPop* aPop = new AtomPop(mApp);
							aPop->mX = (float) (mGrid->mX + aPixel[0]);
							aPop->mY = (float) (mGrid->mY + aPixel[1]);
							mGrid->mAtoms.Remove(anAtom);
							delete anAtom;
							mGrid->mCells[aCol][aRow] = NULL;
							mApp->mBoard->mEffects.Add(aPop);
						}
					}
				}
			}
		}
	}
	return true;
}

// 0x419060
void Blast::Draw(Graphics* g)
{
	if (mDelay < 1)
		mApp->mExplosion[mTier - 1][(int) mFrame / 2]->DrawCenteredF(g, mX, mY);
}

// ---------------------------------------------------------------------------------------------------------
// Shatter

// 0x4170d0
Shatter::Shatter(AtomicaApp* theApp, Sprite* theSprite, int theX, int theY) :
	Effect(theApp),
	mSprite(theSprite),
	mAlpha(0xff)
{
	mX = (float) theX;
	mY = (float) theY;
	for (int x = 0; x < mSprite->mWidth; x += 2)
	{
		for (int y = 0; y < mSprite->mHeight; y += 2)
		{
			Piece* aPiece = new Piece();
			aPiece->mSrcX = mSprite->mSrc.mX + x;
			aPiece->mSrcY = mSprite->mSrc.mY + y;
			aPiece->mW = 2;
			aPiece->mH = 2;
			aPiece->mX = (float) (x - mSprite->mWidth / 2);
			aPiece->mY = (float) (y - mSprite->mHeight / 2);
			aPiece->mAngle = (float) CompassAngle(0, 0, (int) aPiece->mX, (int) aPiece->mY);
			aPiece->mAngle = (float) ((int) aPiece->mAngle + mApp->mRand.Get(4, true));
			aPiece->mSpeed = mApp->mRand.Get(5, false) + 1;
			mPieces.Add(aPiece);
		}
	}
}

// 0x416f00: the pieces still flying are freed
Shatter::~Shatter()
{
	mPieces.Reset();
	Piece* aPiece = mPieces.Next();
	while (aPiece != NULL)
	{
		delete aPiece;
		aPiece = mPieces.Next();
	}
}

// 0x416f90
bool Shatter::Update()
{
	mAlpha -= 2;
	if (mAlpha < 1)
		return false;
	mPieces.Reset();
	Piece* aPiece = mPieces.Next();
	while (aPiece != NULL)
	{
		aPiece->mX = aPiece->mX - (1.0f / (float) aPiece->mSpeed) * SinDeg((int) aPiece->mAngle);
		aPiece->mY = aPiece->mY - (1.0f / (float) aPiece->mSpeed) * NegCosDeg((int) aPiece->mAngle);
		aPiece = mPieces.Next();
	}
	return true;
}

// 0x417000
void Shatter::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(ClampedColor(0xff, 0xff, 0xff, mAlpha));
	mPieces.Reset();
	Piece* aPiece = mPieces.Next();
	while (aPiece != NULL)
	{
		g->DrawImage(mSprite->mImage, (int) (mX + aPiece->mX), (int) (mY + aPiece->mY),
			Rect(aPiece->mSrcX, aPiece->mSrcY, aPiece->mW, aPiece->mH));
		aPiece = mPieces.Next();
	}
	g->SetColorizeImages(false);
}

// ---------------------------------------------------------------------------------------------------------
// FloatyText

// 0x429e30
FloatyText::FloatyText(AtomicaApp* theApp, int theX, int theY, const char* theText, Font* theFont) :
	Effect(theApp),
	mFont(theFont),
	mLife(0xff),
	mColor(0xff, 0xff, 0xff, 0xff)
{
	mX = (float) theX;
	mY = (float) theY;
	mText = (char*) malloc(strlen(theText) + 1);
	strcpy(mText, theText);
	mHalfWidth = mFont->StringWidth(mText) / 2;
}

// 0x429da0
FloatyText::~FloatyText()
{
	free(mText);
}

// 0x429dd0
bool FloatyText::Update()
{
	mX += mVX;
	mLife -= 2;
	mY += mVY;
	return mLife > 0;
}

// 0x429f40
void FloatyText::Draw(Graphics* g)
{
	g->SetFont(mFont);
	int aLife = mLife > 0xfe ? 0xff : mLife;
	g->SetColor(Color(0, 0, 0, aLife));
	g->DrawString(mText, (int) ((mX - (float) mHalfWidth) + 3.0f), (int) (mY + 3.0f));
	g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, aLife));
	g->DrawString(mText, (int) (mX - (float) mHalfWidth), (int) mY);
}

// ---------------------------------------------------------------------------------------------------------
// Bolt

// 0x417300
Bolt::Bolt(AtomicaApp* theApp, Grid* theGrid, int theCol1, int theRow1, int theCol2, int theRow2, const Color& theColor,
	bool thePlaySound) :
	Effect(theApp),
	mGrid(theGrid),
	mCol1(theCol1),
	mRow1(theRow1),
	mCol2(theCol2),
	mRow2(theRow2),
	mTwist(0x3c),
	mColor(theColor)
{
	if (thePlaySound)
		mApp->mSndElectric[mApp->mRand.Get(3)]->Play();
}

// 0x417270
bool Bolt::Update()
{
	if ((mGrid->mCells[mCol1][mRow1] != NULL) && (mGrid->mCells[mCol2][mRow2] != NULL))
	{
		mTwist += -4 - mApp->mRand.Get(4);
		if ((mApp->mRand.Get(10) == 5) && (mTwist < 0x1e))
			mTwist += 0x19;
		return mTwist > -0x47;
	}
	return false;
}

// 0x4173f0: jagged lightning from one atom to the other; every part is drawn as a line of dots (0x4255e0), faint
// first and bright after
void Bolt::Draw(Graphics* g)
{
	int aPixel1[2] = {0, 0};
	int aPixel2[2] = {0, 0};
	Grid::CellToPixel(mCol1, mRow1, aPixel1);
	Grid::CellToPixel(mCol2, mRow2, aPixel2);
	int aX1 = aPixel1[0] + mGrid->mX;
	int aY1 = aPixel1[1] + mGrid->mY;
	int aX2 = aPixel2[0] + mGrid->mX;
	int aY2 = aPixel2[1] + mGrid->mY;
	DrawPixels(g, aX1, aY1, aX2, aY2);
}

// 0x4255e0: a line of dots (Bresenham, every other point a dot), of the spark picture or the mini spark's
static void DrawDots(Graphics* g, AtomicaApp* theApp, int x1, int y1, int x2, int y2, bool theMini)
{
	Sprite* aDot = theMini ? theApp->mMiniSpark : theApp->mSpark;
	int aDX = x2 - x1;
	int aDY = y2 - y1;
	int aStepX = 1;
	int aStepY = 1;
	if (aDX < 0)
	{
		aDX = -aDX;
		aStepX = -1;
	}
	if (aDY < 0)
	{
		aDY = -aDY;
		aStepY = -1;
	}
	int aMajorX, aMajorY, aMinor, aMajor;
	if (aDX < aDY)
	{
		aMajorX = 0;
		aMajorY = aStepY;
		aMinor = aDX;
		aMajor = aDY;
	}
	else
	{
		aMajorX = aStepX;
		aMajorY = 0;
		aMinor = aDY;
		aMajor = aDX;
	}
	int aError = aMinor * 2 - aMajor;
	bool aDraw = true;
	aDot->Draw(g, x1 - (aDot->mWidth >> 1), y1 - (aDot->mHeight >> 1));
	for (int i = aMajor; i > 0; i--)
	{
		int aSX, aSY;
		int aDelta;
		if (aError < 0)
		{
			aDelta = aMinor * 2;
			aSX = aMajorX;
			aSY = aMajorY;
		}
		else
		{
			aDelta = (aMinor - aMajor) * 2;
			aSX = aStepX;
			aSY = aStepY;
		}
		aError += aDelta;
		x1 += aSX;
		y1 += aSY;
		if (aDraw)
			aDot->Draw(g, x1 - (aDot->mWidth >> 1), y1 - (aDot->mHeight >> 1));
		aDraw = !aDraw;
	}
}

// the drawing of 0x4173f0 between two places on the screen (Grid::DrawBond draws the bonds of a molecule with it)
void Bolt::DrawPixels(Graphics* g, int aX1, int aY1, int aX2, int aY2)
{
	g->SetColorizeImages(true);
	int aAngle1 = CompassAngle(aX1, aY1, aX2, aY2) + mTwist;
	int aAngle2 = CompassAngle(aX2, aY2, aX1, aY1) - mTwist;
	if (aAngle1 > FULL_TURN - 1)
		aAngle1 -= FULL_TURN;
	if (aAngle1 < 0)
		aAngle1 += FULL_TURN;
	if (aAngle2 > FULL_TURN - 1)
		aAngle2 -= FULL_TURN;
	if (aAngle2 < 0)
		aAngle2 += FULL_TURN;

	// the two ends leave the atoms at 25 pixels in the twisted directions
	float aStartX = SinDeg(aAngle1) * 25.0f + (float) aX1;
	float aStartY = NegCosDeg(aAngle1) * 25.0f + (float) aY1;
	float aEndX = SinDeg(aAngle2) * 25.0f + (float) aX2;
	float aEndY = NegCosDeg(aAngle2) * 25.0f + (float) aY2;
	// the line bends at a point beside the middle of the ends (a quarter of the way from the first atom's line, along the
	// axis the atoms differ on) and runs start, first jitter, the bend, second jitter, end
	float aMidX = (aEndX + aStartX) * 0.5f;
	float aMidY = (aEndY + aStartY) * 0.5f;
	int aOffsetX = 0;
	int aOffsetY = 0;
	if (aX1 == aX2)
		aOffsetX = (int) ((aMidX - (float) aX1) * 0.25f);
	else
		aOffsetY = (int) ((aMidY - (float) aY1) * 0.25f);
	float aBendX = (float) aOffsetX + aMidX;
	float aBendY = (float) aOffsetY + aMidY;
	aBendX = (float) mApp->mRand.Get(8, true) + aBendX;
	aBendY = (float) mApp->mRand.Get(8, true) + aBendY;
	float aJX1 = (aBendX + aStartX) * 0.5f;
	float aJY1 = (aBendY + aStartY) * 0.5f;
	aJX1 = (float) mApp->mRand.Get(8, true) + aJX1;
	aJY1 = (float) mApp->mRand.Get(8, true) + aJY1;
	float aJX2 = (aBendX + aEndX) * 0.5f;
	float aJY2 = (aBendY + aEndY) * 0.5f;
	aJX2 = (float) mApp->mRand.Get(8, true) + aJX2;
	aJY2 = (float) mApp->mRand.Get(8, true) + aJY2;

	mColor.mAlpha = 0x20;
	g->SetColor(mColor);
	DrawDots(g, mApp, (int) aStartX, (int) aStartY, (int) aJX1, (int) aJY1, false);
	DrawDots(g, mApp, (int) aJX2, (int) aJY2, (int) aEndX, (int) aEndY, false);
	mColor.mAlpha = 0x80;
	g->SetColor(mColor);
	DrawDots(g, mApp, (int) aJX1, (int) aJY1, (int) aBendX, (int) aBendY, false);
	DrawDots(g, mApp, (int) aBendX, (int) aBendY, (int) aJX2, (int) aJY2, false);
	g->SetColor(Color(0xff, 0xff, 0xff, 0x20));
	DrawDots(g, mApp, (int) aStartX, (int) aStartY, (int) aJX1, (int) aJY1, true);
	DrawDots(g, mApp, (int) aJX2, (int) aJY2, (int) aEndX, (int) aEndY, true);
	g->SetColor(Color(0xff, 0xff, 0xff, 0x80));
	DrawDots(g, mApp, (int) aJX1, (int) aJY1, (int) aBendX, (int) aBendY, true);
	DrawDots(g, mApp, (int) aBendX, (int) aBendY, (int) aJX2, (int) aJY2, true);

	// sparks scattered about both ends, then one on each of the lines 10 pixels from the atoms
	for (int i = 0; i < 4; i++)
	{
		float aSparkY = (float) mApp->mRand.Get(6, true) + aStartY;
		float aSparkX = (float) mApp->mRand.Get(6, true) + aStartX;
		mApp->mMiniSpark->DrawCenteredF(g, aSparkX, aSparkY);
		aSparkY = (float) mApp->mRand.Get(6, true) + aEndY;
		aSparkX = (float) mApp->mRand.Get(6, true) + aEndX;
		mApp->mMiniSpark->DrawCenteredF(g, aSparkX, aSparkY);
	}
	float aNearX1 = SinDeg(aAngle1) * 10.0f + (float) aX1;
	float aNearY1 = NegCosDeg(aAngle1) * 10.0f + (float) aY1;
	float aNearX2 = SinDeg(aAngle2) * 10.0f + (float) aX2;
	float aNearY2 = NegCosDeg(aAngle2) * 10.0f + (float) aY2;
	mApp->mMiniSpark->DrawCenteredF(g, aNearX1, aNearY1);
	mApp->mMiniSpark->DrawCenteredF(g, aNearX2, aNearY2);
	g->SetColorizeImages(false);
}

// 0x4255e0 for a bond (Grid::DrawBond): a line of the mini spark
void Bolt::DrawMiniLine(Graphics* g, int theX1, int theY1, int theX2, int theY2)
{
	DrawDots(g, mApp, theX1, theY1, theX2, theY2, true);
}

// ---------------------------------------------------------------------------------------------------------
// InfectBolt

// port: an angle brought back into 0 to 359 after a step of less than a turn (InfectBolt: 0x425230, 0x425730)
static int WrapDegrees(int theAngle)
{
	if (theAngle > FULL_TURN - 1)
		theAngle -= FULL_TURN;
	if (theAngle < 0)
		theAngle += FULL_TURN;
	return theAngle;
}

// 0x425120: the three electric sounds at once
InfectBolt::InfectBolt(AtomicaApp* theApp, int theStartX, int theStartY, int theTargetX, int theTargetY) :
	Effect(theApp),
	mX1(0),
	mY1(0),
	mStartX(0),
	mStartY(0),
	mEndX(0),
	mEndY(0),
	mTargetX(0),
	mTargetY(0),
	mTimer(0x5a),
	mAlpha(0x168),
	mAngle(0),
	mRadius(0),
	mMoving(false),
	mSoundTimer(0)
{
	mApp->mSndElectric[0]->Play();
	mApp->mSndElectric[1]->Play();
	mApp->mSndElectric[2]->Play();
	mSoundTimer = 10;
	mAngle = mApp->mRand.Get(FULL_TURN, false);
	mStartX = theStartX;
	mTargetY = theTargetY;
	mTargetX = theTargetX;
	mStartY = theStartY;
	mMoving = true;
}

// 0x425230: the end of the line circles the target, wider and wider; two sparks fly off (one from each end); after 90 updates
// 150 sparks burst from a point 25 pixels from the target
bool InfectBolt::Update()
{
	mSoundTimer--;
	if (mSoundTimer < 1)
	{
		mApp->mSndElectric[mApp->mRand.Get(3, false)]->Play();
		mSoundTimer = 10;
	}
	if (mMoving)
	{
		mAngle = WrapDegrees(mAngle + 8);
		mEndX = (int) (SinDeg(mAngle) * mRadius + (float) mTargetX);
		mEndY = (int) (NegCosDeg(mAngle) * mRadius + (float) mTargetY);
		mRadius = mRadius + 0.28f;
		int anAngle = CompassAngle(mStartX, mStartY, mEndX, mEndY);
		mX1 = (int) (SinDeg(anAngle) * 24.0f + (float) mStartX);
		mY1 = (int) (NegCosDeg(anAngle) * 24.0f + (float) mStartY);
		int aDirection = WrapDegrees(mApp->mRand.Get(0x3c, true) + anAngle);
		Particle* aSpark = new Particle(mApp, mApp->mMiniSpark);
		aSpark->mX = (float) mX1;
		aSpark->mY = (float) mY1;
		float aSpeed = (float) (mApp->mRand.Get(10, false) + 1) * 0.02f;
		aSpark->mVX = aSpeed * SinDeg(aDirection);
		aSpark->mVY = aSpeed * NegCosDeg(aDirection);
		mApp->mBoard->mEffects.Add(aSpark);
		int aDirection2 = mApp->mRand.Get(FULL_TURN, false);
		aSpark = new Particle(mApp, mApp->mMiniSpark);
		aSpark->mX = (float) mEndX;
		aSpark->mY = (float) mEndY;
		aSpeed = (float) (mApp->mRand.Get(10, false) + 1) * 0.02f;
		aSpark->mVX = aSpeed * SinDeg(aDirection2);
		aSpark->mVY = aSpeed * NegCosDeg(aDirection2);
		mApp->mBoard->mEffects.Add(aSpark);
	}
	else
	{
		mX1 = mStartX;
		mY1 = mStartY;
		mEndX = mTargetX;
		mEndY = mTargetY;
	}
	mAlpha -= 3;
	if (mAlpha < 0)
		mAlpha = 0;
	mTimer--;
	if (mTimer > 0)
		return true;
	if (mMoving)
	{
		for (int i = 0; i < 0x96; i++)
		{
			Particle* aSpark = new Particle(mApp, mApp->mMiniSpark);
			aSpark->mX = (float) mTargetX;
			aSpark->mY = (float) mTargetY;
			int aDirection = mApp->mRand.Get(FULL_TURN, false);
			aSpark->mVX = SinDeg(aDirection);
			aSpark->mVY = NegCosDeg(aDirection);
			aSpark->mX = aSpark->mVX * 25.0f + aSpark->mX;
			aSpark->mY = aSpark->mVY * 25.0f + aSpark->mY;
			float aSpeed = (float) (mApp->mRand.Get(10, false) + 1) * 0.033333335f;
			aSpark->mVX = aSpeed * aSpark->mVX;
			aSpark->mVY = aSpeed * aSpark->mVY;
			mApp->mBoard->mEffects.Add(aSpark);
		}
	}
	return false;
}

// 0x425730: a jagged line of dots from the start through three points, each moved at random, to the end
void InfectBolt::Draw(Graphics* g)
{
	int aAngle1 = CompassAngle(mX1, mY1, mEndX, mEndY);
	int aAngle2 = CompassAngle(mEndX, mEndY, mX1, mY1);
	int aAngle3 = CompassAngle(mEndX, mEndY, mX1, mY1);
	int aDX = mEndX - mX1;
	int aDY = mEndY - mY1;
	float aLength = sqrtf((float) (aDY * aDY + aDX * aDX)) * 0.25f;
	int aJitter1 = WrapDegrees(mApp->mRand.Get(0x37, true) + aAngle1);
	int aJitter2 = WrapDegrees(mApp->mRand.Get(0x37, true) + aAngle2);
	int aJitter3 = WrapDegrees(mApp->mRand.Get(0x37, true) + aAngle3);
	float aAX = aLength * SinDeg(aJitter1) + (float) mX1;
	float aAY = aLength * NegCosDeg(aJitter1) + (float) mY1;
	float aBX = aLength * SinDeg(aJitter2) + (float) mEndX;
	float aBY = aLength * NegCosDeg(aJitter2) + (float) mEndY;
	float aCX = (aLength + aLength) * SinDeg(aJitter3) + (float) mEndX;
	float aCY = (aLength + aLength) * NegCosDeg(aJitter3) + (float) mEndY;
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, mAlpha > 0xfe ? 0xff : mAlpha));
	DrawDots(g, mApp, mX1, mY1, (int) aAX, (int) aAY, false);
	DrawDots(g, mApp, (int) aAX, (int) aAY, (int) aCX, (int) aCY, false);
	DrawDots(g, mApp, (int) aCX, (int) aCY, (int) aBX, (int) aBY, false);
	DrawDots(g, mApp, (int) aBX, (int) aBY, mEndX, mEndY, false);
	g->SetColorizeImages(false);
}

// ---------------------------------------------------------------------------------------------------------
// Sparkle

// 0x41c5f0
Sparkle::Sparkle(AtomicaApp* theApp, int theCol, int theRow, int theCorner) :
	Effect(theApp),
	mTrailIndex(0),
	mAlpha(0xaf),
	mRevealing(true),
	mFading(false),
	mFadeStep(1)
{
	mDir = mApp->mRand.Get(2, false);
	mSpeed = mApp->mRand.Get(3, false) + 2;
	mDead = false;
	for (int i = 0; i < 20; i++)
	{
		mTrail[i][0] = -1000.0f;
		mTrail[i][1] = -1000.0f;
	}
	mX = (float) (theCol * 0x32);
	mY = (float) (theRow * 0x32);
	if (theCorner == 1)
		mX = mX + 50.0f;
	else if ((theCorner == 2) || (theCorner == 3))
	{
		if (theCorner == 3)
			mX = mX + 50.0f;
		mY = (float) (theRow * 0x32) + 50.0f;
	}
	Reset();
}

// 0x41c4c0
void Sparkle::Reset()
{
	float anOldVX = mVX;
	mTimer = 0x33;
	mVX = 0;
	mVY = 0;
	if ((mY <= -50.0f) || (450.0f <= mY))
	{
		mDead = true;
		return;
	}
	if (mApp->mRand.Get(2, false) == 0)
	{
		mVY = (mDir == 0) ? -1.0f : 1.0f;
		return;
	}
	mVX = (float) mApp->mRand.Get(2, false);
	if (mVX == 0.0f)
		mVX = -1.0f;
	if (-anOldVX == mVX)
		mVX = anOldVX;
	if (mX <= 0.0f)
		mVX = 1.0f;
	if (450.0f <= mX)
		mVX = -1.0f;
}

// 0x41c710
bool Sparkle::Update()
{
	for (int i = 0; i < mSpeed; i++)
	{
		mTimer--;
		if (mTimer < 1)
			Reset();
		else
		{
			if (i % 4 == 0)
			{
				mTrailIndex++;
				if (mTrailIndex > 19)
					mTrailIndex = 0;
				mTrail[mTrailIndex][0] = mX;
				mTrail[mTrailIndex][1] = mY;
			}
			mX = mVX + mX;
			mY = mVY + mY;
		}
	}
	if (mRevealing && !mApp->mBoard->mMeter.mFull)
	{
		int aCell[2] = {0, 0};
		Grid::PixelToCell((int) mX, (int) mY, aCell);
		Grid* aGrid = mApp->mBoard->mGrid;
		if ((aCell[0] > -1) && (aCell[1] > -1) && (aCell[0] < Grid::COLS) && (aCell[1] < Grid::ROWS) && (aGrid->mCellState[aCell[0]][aCell[1]] < -100))
			aGrid->mCellState[aCell[0]][aCell[1]] = 0x40;
	}
	if (!mFading)
	{
		mAlpha = mAlpha - mFadeStep;
		if (mApp->mBoard->mMeter.mFull)
			mAlpha -= 2;
		if (mAlpha < 0)
		{
			mAlpha = 0;
			mDead = true;
		}
	}
	return !mDead;
}

// 0x41c890
void Sparkle::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(ClampedColor(0xff, 0xff, 0xff, mAlpha));
	Grid* aGrid = mApp->mBoard->mGrid;
	if ((0.0f < mY) && (mY < (float) aGrid->mHeight))
		mApp->mGridBug->DrawCenteredF(g, mX, mY);
	int aIndex = mTrailIndex;
	int aHalf = mAlpha / 2;
	int aStep = aHalf / 0x28;
	int aAlpha = aHalf - aStep;
	for (int i = 0; i < 20; i++)
	{
		g->SetColor(ClampedColor(0xff, 0xff, 0xff, aAlpha));
		if ((0.0f < mTrail[aIndex][1]) && (mTrail[aIndex][1] < (float) aGrid->mHeight))
			mApp->mGridBug->DrawCenteredF(g, mTrail[aIndex][0], mTrail[aIndex][1]);
		aAlpha -= aStep;
		aIndex--;
		if (aIndex < 0)
			aIndex = 19;
	}
	g->SetColorizeImages(false);
}

// 0x422860
NukeFlash::NukeFlash(AtomicaApp* theApp) :
	Effect(theApp),
	mAlpha(0xff)
{
}

// 0x4228a0
bool NukeFlash::Update()
{
	mAlpha -= 2;
	return mAlpha > 0;
}

// 0x4228e0
void NukeFlash::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	int anAlpha = mAlpha;
	if (0xaf < anAlpha)
		anAlpha = 0xff;
	g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
	mApp->mNukeFlash->DrawCenteredF(g, mX, mY);
	g->SetColorizeImages(false);
}

// 0x421e40
Explosion::Explosion(AtomicaApp* theApp) :
	Effect(theApp),
	mFrame1(0),
	mStep1(0.15f),
	mFrame2(0),
	mStep2(0.2f),
	mRise(5.0f)
{
}

// 0x421e90
bool Explosion::Update()
{
	mFrame1 = mFrame1 + mStep1;
	mFrame2 = mFrame2 + mStep2;
	mRise = mRise + 0.5f;
	return (mFrame1 < 16.0f) || (mFrame2 < 20.0f);
}

// 0x421f10
void Explosion::Draw(Graphics* g)
{
	if (mFrame2 < 20.0f)
		mApp->mBigExplosion[(int) mFrame2]->DrawCenteredF(g, mX, mY);
	if (mFrame1 < 16.0f)
		mApp->mAtomicExplosion[(int) mFrame1]->DrawCenteredF(g, mX, mY - mRise);
}
