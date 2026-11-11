// The tunnel behind the game and the atoms that circle in it (see Swirl.h).

#include "Swirl.h"
#include "Atom.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Graphics.h"
#include "Image.h"

#include <cmath>
#include <cstring>
#include <vector>

using namespace Sexy;

float Swirl::sRadiusA = 150.0f;
float Swirl::sRadiusAStep = -3.0f;
float Swirl::sRadiusB = 50.0f;
float Swirl::sRadiusBStep = -2.0f;
int Swirl::sAngle = 0;

namespace
{
	inline int ClampByte(int theValue)
	{
		return theValue < 0 ? 0 : (theValue > 255 ? 255 : theValue);
	}
}

// 0x428100: a ring as wide as the screen that will shrink; where its centre is to go, how it is moved at each point
Swirl::Swirl(Board* theBoard) :
	mBoard(theBoard),
	mApp(theBoard->mApp),
	mClone(false),
	mShrink(1.0f)
{
	memset(mPoints, 0, sizeof(mPoints));
	memset(mJitter, 0, sizeof(mJitter));
	mX = 320.0f;
	mY = 240.0f;
	mTargetX = sRadiusA * SinDeg(sAngle) + 320.0f;
	mTargetY = sRadiusA * NegCosDeg(sAngle) + 240.0f;
	mOffsetX = (float) mApp->mRand.Get(0x14, true);
	mOffsetY = (float) mApp->mRand.Get(0x14, true);
	int anAngle = sAngle + 4;
	if (0x167 < anAngle)
		anAngle = sAngle - 0x164;
	if (anAngle < 0)
		anAngle += 0x168;
	sRadiusA = sRadiusAStep + sRadiusA;
	if (200.0f < sRadiusA)
		sRadiusAStep = -fabsf(sRadiusAStep);
	if (sRadiusA < 100.0f)
		sRadiusAStep = fabsf(sRadiusAStep);
	sAngle = anAngle;
	for (int i = 0; i < NUM_POINTS; i++)
	{
		mJitter[i][0] = mApp->mRand.Get(7, true);
		mJitter[i][1] = mApp->mRand.Get(7, true);
	}
	mMinRadius = sRadiusB;
	mRadius = sRadiusB;
	mSpeed = 0.025f;
	mShrink = (float) (1.003 - (double) mApp->mRand.Get(0x28, false) * 1e-05);
	sRadiusB = sRadiusBStep + sRadiusB;
	if (60.0f < sRadiusB)
		sRadiusBStep = -fabsf(sRadiusBStep);
	if (sRadiusB < 30.0f)
		sRadiusBStep = fabsf(sRadiusBStep);
	mRadius = 650.0f;
	mSpeed = 1.5f;
}

// 0x427ba0: the atoms that were in it go in a flash of their colour
Swirl::~Swirl()
{
	if (6 < mAtoms.Size())
		mApp->mSndWhoosh->Play();
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		mBoard->mFlash.mRed = anAtom->mColor.mRed;
		mBoard->mFlash.mGreen = anAtom->mColor.mGreen;
		mBoard->mFlash.mBlue = anAtom->mColor.mBlue;
		delete anAtom;
		mApp->mSndLowWhoosh->Play();
		anAtom = mAtoms.Next();
	}
	mAtoms.Clear();
}

// 0x427c70: it shrinks (a clone faster and faster), the centre moves toward its target, the atoms in it turn
bool Swirl::Update()
{
	if ((0 < mBoard->mI184) && (mAtoms.Size() == 0) && !mBoard->mB98)
		return true;
	mRadius = mRadius - mSpeed;
	if (!mClone)
		mSpeed = mSpeed * 0.997009f;
	else
		mSpeed = mSpeed / mShrink;
	if ((double) mSpeed < 0.15)
		mSpeed = 0.15f;
	if (mRadius < 250.0f)
		mRadius = mRadius - mSpeed * 0.16666667f;
	if (mRadius < 150.0f)
		mRadius = mRadius - mSpeed * 0.25f;
	if (mRadius <= mMinRadius)
		return false;
	bool aSkip = mClone ? (1000.0f <= mRadius) : (mMinRadius + 350.0f <= mRadius);
	if (!aSkip)
	{
		int anAngle = CompassAngle((int) mX, (int) mY, (int) mTargetX, (int) mTargetY);
		mX = SinDeg(anAngle) * 0.1f + mX;
		mY = NegCosDeg(anAngle) * 0.1f + mY;
		if (mRadius < 250.0f)
		{
			mX = SinDeg(anAngle) * 0.1f + mX;
			mY = NegCosDeg(anAngle) * 0.1f + mY;
		}
		if (mRadius < 150.0f)
		{
			mX = SinDeg(anAngle) * 0.1f + mX;
			mY = NegCosDeg(anAngle) * 0.1f + mY;
		}
	}
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		if (anAtom->mFlySize < anAtom->mFlyTurn)
			anAtom->mFlySize = anAtom->mFlySize + 0.0015f;
		anAtom->mFlyX = anAtom->mFlyX - anAtom->mFlyGrow * 0.33333334f;
		anAtom->mFlyGrow = anAtom->mFlyGrow + 0.0033333332f;
		if (anAtom->mF_b0 < anAtom->mFlyGrow)
			anAtom->mFlyGrow = anAtom->mF_b0;
		anAtom = mAtoms.Next();
	}
	return true;
}

// 0x427ed0: where a point is as seen from the centre: the angle to it and how far, as a part of the radius
void Swirl::Aim(int theX, int theY, float* theAngle, float* theRatio)
{
	int aCenterX = (int) mX;
	int aCenterY = (int) mY;
	int anAngle = CompassAngle(theX, theY, aCenterX, aCenterY);
	*theAngle = (float) anAngle;
	int aDX = aCenterX - theX;
	int aDY = aCenterY - theY;
	*theRatio = sqrtf((float) (aDY * aDY + aDX * aDX)) / mRadius;
}

// 0x428870: a copy of the swirl (made afresh, so that the random numbers go on) with its place, size and points
Swirl* Swirl::Clone()
{
	Swirl* aCopy = new Swirl(mBoard);
	aCopy->mApp = mApp;
	aCopy->mX = mX;
	aCopy->mY = mY;
	aCopy->mOffsetX = mOffsetX;
	aCopy->mOffsetY = mOffsetY;
	aCopy->mTargetX = mTargetX;
	aCopy->mTargetY = mTargetY;
	aCopy->mRadius = mRadius;
	aCopy->mSpeed = mSpeed;
	aCopy->mMinRadius = mMinRadius;
	memcpy(aCopy->mPoints, mPoints, sizeof(mPoints));
	memcpy(aCopy->mJitter, mJitter, sizeof(mJitter));
	aCopy->mClone = mClone;
	return aCopy;
}

// 0x4283a0 with 0x427f50: a triangle in the colour of the graphics, written straight to the screen (no clipping or
// moving but the screen's), only on the rows that the stride divides. The original fills each triangle's rows as it
// comes; the port adds them to theSpans and Draw fills a ring's rows in one call, the same rows in the same order and
// colour (GAME_NOTES.md, "Performance": a frame has about 600 triangles and 21,000 rows)
void Swirl::DrawTriangle(std::vector<Span>& theSpans, const int* theA, const int* theB, const int* theC, int theStride)
{
	const int* aTop = theA;
	if (theB[1] < theA[1])
		aTop = theB;
	if (theC[1] < aTop[1])
		aTop = theC;
	const int* aBottom = theA;
	if (theA[1] < theB[1])
		aBottom = theB;
	if (aBottom[1] < theC[1])
		aBottom = theC;
	const int* aMiddle = theA;
	if ((aMiddle == aTop) || (aMiddle == aBottom))
		aMiddle = theB;
	if ((aMiddle == aTop) || (aMiddle == aBottom))
		aMiddle = theC;

	int aX1 = aTop[0];
	int aY1 = aTop[1];
	int aX3 = aBottom[0];
	int aY3 = aBottom[1];
	int aX2 = aMiddle[0];
	int aY2 = aMiddle[1];
	float aSlopeLong = (float) ((double) (aX3 - aX1) / (double) (aY3 - aY1));
	float aSlopeUpper = (float) ((double) (aX2 - aX1) / (double) (aY2 - aY1));
	// the long edge is on the left when it passes left of the middle point
	bool aLongLeft = (double) aSlopeLong * (double) (aY2 - aY1) + (double) aX1 <= (double) aX2;

	double aLong = aX1;
	double aShort = aX1;
	double aY = aY1;
	for (int aPart = 0; aPart < 2; aPart++)
	{
		float aSlopeShort = aSlopeUpper;
		double aEnd = aY2;
		bool aInclusive = false;
		if (aPart == 1)
		{
			aSlopeShort = (float) ((double) (aX3 - aX2) / (double) (aY3 - aY2));
			aShort = aX2;
			aEnd = aY3;
			aInclusive = true;
		}
		double aCounter = (aPart == 0) ? (double) aY1 : (double) aY2;
		if (aInclusive ? ((double) aY2 > (double) aY3) : !(aCounter < (double) aY2))
			continue;
		do
		{
			double aLeft = aLongLeft ? aLong : aShort;
			double aRight = aLongLeft ? aShort : aLong;
			if ((0.0 <= aY) && (aY < 480.0) && (aLeft < 640.0))
			{
				int aRow = (int) aY;
				if (aRow % theStride == 0)
				{
					Span aSpan;
					aSpan.mY = aRow;
					aSpan.mX = (int) aLeft;
					aSpan.mWidth = (int) aRight - aSpan.mX;
					if (aSpan.mX < 0)
					{
						aSpan.mWidth += aSpan.mX;
						aSpan.mX = 0;
					}
					if (0x27f < aSpan.mWidth + aSpan.mX)
						aSpan.mWidth = 0x280 - aSpan.mX;
					if (0 < aSpan.mWidth)
						theSpans.push_back(aSpan);
				}
			}
			aLong = (double) aSlopeLong + aLong;
			aY = aY + 1.0;
			aShort = (double) aSlopeShort + aShort;
			aCounter = aCounter + 1.0;
		} while (aInclusive ? (aCounter <= aEnd) : (aCounter < aEnd));
	}
}

// 0x428940: the ring as a band between its points and those of the swirl before it (the first as a disc up to the middle),
// then the atoms that circle in it
void Swirl::Draw(Graphics* g, Swirl* thePrevious)
{
	float anAngle = mBoard->mAngle;
	int aShadeInt = ClampByte((int) ((mRadius - (mMinRadius + 10.0f)) * 0.0125f * 255.0f));
	float aShade = (float) aShadeInt * 0.003921569f;
	int aBlue = (int) (mRadius * 0.00125f * 512.0f);
	if (aBlue > 255)
		aBlue = 255;
	float aGrow = mRadius * 0.005f;
	if (aGrow > 1.0f)
		aGrow = 1.0f;
	else if (aGrow < 0.0f)
		aGrow = 0.0f;
	aGrow = 1.0f - aGrow;

	int aRed = (int) ((float) mBoard->mFlash.mRed * aGrow);
	int aGreen = (int) ((float) mBoard->mFlash.mGreen * aGrow);
	int aBlueFlash = (int) ((float) mBoard->mFlash.mBlue * aGrow);
	int aMax = aRed;
	if (aMax <= aGreen)
		aMax = aGreen;
	if (aMax <= aBlueFlash)
		aMax = aBlueFlash;
	int aStride = mBoard->mDetail;
	if (aMax > 0x40)
		aStride = aStride / 2;
	if (aMax > 0x80)
		aStride = aStride / 2;
	if (aMax > 0xc0)
		aStride = aStride / 2;
	if (aStride < 1)
		aStride = 1;

	if (this != mBoard->mGlowSwirl)
	{
		// the field frozen: every swirl glows a little
		float aLimit = (float) mBoard->mI180 * aShade;
		aRed = ((float) aRed > aLimit) ? aRed : (int) aLimit;
		aLimit = (float) mBoard->mI180 * aShade;
		aGreen = ((float) aGreen > aLimit) ? aGreen : (int) aLimit;
		aLimit = (float) mBoard->mI180 * aShade;
		aBlueFlash = ((float) aBlueFlash > aLimit) ? aBlueFlash : (int) aLimit;
	}
	else
	{
		// the one the freeze has come to glows brighter
		float aLimit = aShade * 192.0f;
		aRed = ((float) aRed > aLimit) ? aRed : (int) aLimit;
		aGreen = ((float) aGreen > aLimit) ? aGreen : (int) aLimit;
		float aBlueLimit = aShade * 255.0f;
		aBlueFlash = ((float) aBlueFlash > aBlueLimit) ? aBlueFlash : (int) aBlueLimit;
	}
	int aColorRed = (int) (aShade * 70.0f);
	if (aColorRed <= aRed)
		aColorRed = aRed;
	int aColorGreen = aGreen < 0 ? 0 : aGreen;
	int aColorBlue = (int) ((float) aBlue * aShade);
	if (aColorBlue <= aBlueFlash)
		aColorBlue = aBlueFlash;
	if (mBoard->mTintOn)
	{
		aColorRed = (int) ((float) aColorRed * mBoard->mTintF[0]);
		aColorGreen = (int) ((float) aColorGreen * mBoard->mTintF[1]);
		aColorBlue = (int) ((float) aColorBlue * mBoard->mTintF[2]);
	}
	g->SetColor(Color(ClampByte(aColorRed), ClampByte(aColorGreen), ClampByte(aColorBlue), 255));
	if (thePrevious == NULL)
		g->SetColor(mBoard->mFlash);
	int aFrame = (int) ((1.0f - mRadius * 0.0026666666f) * 24.0f);
	if (aFrame > 24)
		aFrame = 24;
	else if (aFrame < 0)
		aFrame = 0;
	g->SetColorizeImages(true);

	if (!mClone)
	{
		// port: the rows of the ring's triangles, filled together below; the list is kept between draws, so it is allocated once
		static std::vector<Span> sSpans;
		sSpans.clear();
		for (int i = 0; i < 20; i++)
		{
			float anAngle1 = anAngle * mApp->mPi * 0.0055555557f;
			mPoints[i * 18][0] = (int) (sinf(anAngle1) * mRadius + (mOffsetX + mX));
			mPoints[i * 18][1] = (int) (cosf(anAngle1) * mRadius + (mOffsetY + mY));
			float anAngle2 = anAngle + 18.0f;
			if (anAngle2 >= 360.0f)
				anAngle2 -= 360.0f;
			float anAngle2Rad = anAngle2 * mApp->mPi * 0.0055555557f;
			mPoints[(i + 1) * 18][0] = (int) (sinf(anAngle2Rad) * mRadius + (mOffsetX + mX));
			mPoints[(i + 1) * 18][1] = (int) (cosf(anAngle2Rad) * mRadius + (mOffsetY + mY));
			int aNext = (i == 19) ? 0 : (i + 1) * 18;
			int aThis = i * 18;
			int aA[2] = {mJitter[aThis][0] + mPoints[aThis][0], mJitter[aThis][1] + mPoints[aThis][1]};
			int aB[2] = {mJitter[aNext][0] + mPoints[aNext][0], mJitter[aNext][1] + mPoints[aNext][1]};
			if (thePrevious == NULL)
			{
				int aCenter[2] = {(int) mX, (int) mY};
				DrawTriangle(sSpans, aA, aB, aCenter, aStride);
			}
			else
			{
				int aC[2] = {thePrevious->mJitter[aNext][0] + thePrevious->mPoints[aNext][0],
					thePrevious->mJitter[aNext][1] + thePrevious->mPoints[aNext][1]};
				int aD[2] = {thePrevious->mJitter[aThis][0] + thePrevious->mPoints[aThis][0],
					thePrevious->mJitter[aThis][1] + thePrevious->mPoints[aThis][1]};
				DrawTriangle(sSpans, aA, aB, aC, aStride);
				DrawTriangle(sSpans, aA, aC, aD, aStride);
			}
			anAngle = anAngle2;
		}
		if (!sSpans.empty())
			g->mDestImage->FillScanLines(&sSpans[0], (int) sSpans.size(), g->GetColor(), Graphics::DRAWMODE_NORMAL);
	}

	// the atoms
	int aFlicker = mApp->mRand.Get(0x32, true) + 0x40;
	g->SetColor(Color(255, 255, 255, ClampByte((int) ((float) aFlicker * aShade))));
	aShade = aShade + aShade;
	if (aShade > 1.0f)
		aShade = 1.0f;
	int aGrey = ClampByte((int) (aShade * 255.0f));
	g->SetColor(Color(aGrey, aGrey, aGrey, 255));
	mAtoms.Reset();
	Atom* anAtom = mAtoms.Next();
	while (anAtom != NULL)
	{
		float anAtomAngle = anAtom->mFlyX * mApp->mPi * 0.0055555557f;
		float aRadius = anAtom->mFlySize * mRadius;
		float aX = (mOffsetX + mX) - sinf(anAtomAngle) * aRadius;
		float aY = cosf(anAtomAngle) * aRadius + (mOffsetY + mY);
		g->SetColor(Color(aGrey, aGrey, aGrey, 255));
		Sprite* aSprite = mApp->mAtomShrink[anAtom->mType][aFrame];
		if (aSprite->mImage != NULL)
			aSprite->DrawCenteredF(g, aX, aY);
		if (anAtom->mKind == KIND_CHROMIUM)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, 100));
			Sprite* aChromium = ChromiumFrame(mApp, aFrame);
			if (aChromium->mImage != NULL)
				aChromium->DrawCenteredF(g, aX, aY);
			g->SetColorizeImages(false);
		}
		anAtom = mAtoms.Next();
	}
	g->SetColor(Color(ClampByte((int) (aShade * 70.0f)), 0, ClampByte((int) ((float) aBlue * aShade)), 255));
	g->SetColorizeImages(false);
}
