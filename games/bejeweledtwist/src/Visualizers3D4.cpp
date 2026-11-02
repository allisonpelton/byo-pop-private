// Gears, Slipstream, GravityField, VizMoleculeRedux, VizMagicSnakes, VizJellyBlob and VizBuckyShards (Visualizers3D.h).
// As in Visualizers3D.cpp the original's float code runs on the x87 stack (rounded to float only where it stores, and to
// 53 bits, the CRT's default precision), so expressions are written in double where the original's are. A constant that
// the export prints short in a float expression is the float the compiler widened to a double (read from the exe: 0.99
// is 0.9900000095367432), so it is written with the f suffix here (0.99f, a double in these expressions); a constant
// that is a real double in the exe (0.255) is written as it is.
#include "Visualizers3D.h"

#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>
#include <cstring>

using namespace Sexy;

// 0x401860: a colour as ARGB
static uint32_t PackArgb(const Color& theColor)
{
	return ((((uint32_t)theColor.mAlpha << 8 | (uint32_t)theColor.mRed) << 8 | (uint32_t)theColor.mGreen) << 8) |
		(uint32_t)theColor.mBlue;
}

// 0x67286a: the blend of the colours theFrom and theTo (as 0x401860 packs them: ARGB) by theFraction, each channel
// truncated (0x77e016, which cuts toward zero). The original puts the channels together as red, alpha, green and blue from
// the top, where 0x4016b0 (Color(int)) reads alpha, red, green and blue: the colour has the blended alpha as its red and
// the blended red as its alpha (an alpha of 0 is opaque).
static Color BlendedColor(uint32_t theFrom, uint32_t theTo, float theFraction)
{
	double aFraction = (double)theFraction;
	double aRest = 1.0 - aFraction;
	int aRed = Ftol((double)((theTo >> 16) & 0xff) * aFraction + (double)((theFrom >> 16) & 0xff) * aRest);
	int anAlpha = Ftol((double)(theTo >> 24) * aFraction + (double)(theFrom >> 24) * aRest);
	int aGreen = Ftol((double)((theTo >> 8) & 0xff) * aFraction + (double)((theFrom >> 8) & 0xff) * aRest);
	int aBlue = Ftol((double)(theTo & 0xff) * aFraction + (double)(theFrom & 0xff) * aRest);
	return Color((int)(uint32_t)((((aRed + anAlpha * 0x100) * 0x100 + aGreen) * 0x100) + aBlue));
}

// ---------------------------------------------------------------------------------------------------------------------
// Gears

// 0x673855: IMAGE_GEAR, IMAGE_GEAR_BLUR and IMAGE_GEARS_BG
Gears::Gears(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mAngleA = 0.0f;
	mUnknown3C = 0;
	mAngleB = 0.0f;
	mUnknown38 = 1;
	mGear = LoadVizImage(IMAGE_GEAR_ID);
	mGearBlur = LoadVizImage(IMAGE_GEAR_BLUR_ID);
	mBackground = LoadVizImage(IMAGE_GEARS_BG_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

// 0x673949
Gears::~Gears()
{
	DeleteVizImage(IMAGE_GEAR_ID);
	DeleteVizImage(IMAGE_GEAR_BLUR_ID);
	DeleteVizImage(IMAGE_GEARS_BG_ID);
	for (size_t i = 0; i < mBars.size(); i++)
		delete mBars[i];
	mBars.clear();
}

// 0x673abf: 100 bars, each started (0x68ed45, four of the C runtime's rand a try) again while it is within 100000 of the
// middle (x squared plus y squared)
void Gears::Init()
{
	int aCount = 100;
	do {
		Bar* aBar = new Bar();
		aBar->Start(mApp);  // 0x68eea5
		float aSquareY = aBar->mY * aBar->mY;
		float aSquareX = aBar->mX * aBar->mX;
		while ((double)aSquareX + (double)aSquareY < 100000.0) {
			aBar->Start(mApp);
			aSquareY = aBar->mY * aBar->mY;
			aSquareX = aBar->mX * aBar->mX;
		}
		mBars.push_back(aBar);  // 0x673a96
		aCount--;
	} while (aCount != 0);
}

// 0x673183
void Gears::Update()
{
	mCount++;
	float aPhase = (float)((double)(float)(mCount % 0x5dc) / 1500.0);
	aPhase = (float)((double)aPhase + (double)aPhase);
	mAngleA = (float)((double)(float)mUnknown38 * 0.0 / ((double)aPhase + 1.0) + 0.001f + (double)mAngleA);
	aPhase = (float)((double)aPhase * 3.1415927f);
	float aSin = sinf(aPhase);  // 0x401b70
	mAngleB = (float)(((double)aSin * 0.0 - 0.0005f) + (double)mAngleB);
	// both angles are kept between -2 pi and 2 pi
	float aTwoPi = 6.2831855f;  // 0x84e0ec
	float anAngle = mAngleB;
	while (aTwoPi < anAngle) {
		mAngleB = (float)((double)mAngleB - (double)aTwoPi);
		anAngle = mAngleB;
	}
	float aNegTwoPi = -aTwoPi;
	anAngle = mAngleB;
	while (anAngle < aNegTwoPi) {
		mAngleB = (float)((double)aTwoPi + (double)mAngleB);
		anAngle = mAngleB;
	}
	anAngle = mAngleA;
	while (aTwoPi < anAngle) {
		mAngleA = (float)((double)anAngle - (double)aTwoPi);
		anAngle = mAngleA;
	}
	while (anAngle < aNegTwoPi) {
		mAngleA = (float)((double)aTwoPi + (double)mAngleA);
		anAngle = mAngleA;
	}
	size_t anIndex = 0;
	while (anIndex != mBars.size()) {
		Bar* aBar = mBars[anIndex];
		float aZ = (float)((double)aBar->mZ + 0.01f);
		aBar->mZ = aZ;
		float aHalfWidth = (float)(mApp->mWidth / 2);
		float aScale = (float)((0.01f / (1.0 - ((double)aZ / (double)aHalfWidth + 0.5) * 0.99f)) * 0.95f + 0.05f);
		// (the original puts the y's place on the width's middle as well)
		float aCenter = (float)(gApp->mWidth / 2);
		float aPlaceX = (float)((double)aBar->mX * (double)aScale + (double)aCenter);
		float aPlaceY = (float)((double)aScale * (double)aBar->mY + (double)aCenter);
		if (aPlaceX < -30.0f || (float)(gApp->mWidth + 0x1e) < aPlaceX || aPlaceY < -50.0f ||
			(float)(gApp->mHeight + 0x32) < aPlaceY || aHalfWidth < aZ) {
			aBar->Start(mApp);  // 0x68ed45
			float aSquareY = aBar->mY * aBar->mY;
			float aSquareX = aBar->mX * aBar->mX;
			while ((double)aSquareX + (double)aSquareY < 100000.0) {
				aBar->Start(mApp);
				aSquareY = aBar->mY * aBar->mY;
				aSquareX = aBar->mX * aBar->mX;
			}
			aBar->mZ = (float)(-(mApp->mWidth / 2));
		}
		anIndex++;
	}
}

// 0x673435
void Gears::Draw(Graphics* g)
{
	g->SetClipRect(0, 0, gApp->mWidth, gApp->mHeight);  // 0x4222c0
	g->SetColor(Color(0xffffff));
	DrawImage(g, mBackground);  // 0x67fdb6
	g->SetColorizeImages(true);
	g->SetColor(Color(0));
	g->SetColor(BlendedColor(0, 0x303030, 1.0f));  // 0x67286a
	float aPhase = (float)((double)(float)(mCount % 0x5dc) / 1500.0);
	float aScale = (float)((double)(float)gApp->mWidth * 0.95f * 0.0009765625 *
		((double)aPhase * 0.0 * ((double)aPhase + 1.0) + 1.4f));
	g->SetColorizeImages(true);
	DrawGear(g, aScale, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), mAngleA, 0.0f, mUnknown3C);
	g->SetDrawMode(0);
	g->PushState();
	g->SetColorizeImages(true);
	unsigned int aNumber = 0;
	size_t anIndex = 0;
	while (anIndex != mBars.size()) {
		aNumber++;
		const Bar* aBar = mBars[anIndex];
		float aDepth = (float)((double)aBar->mZ / (double)(float)(mApp->mWidth / 2) + 0.5);
		if (0.0f <= aDepth) {
			if (1.0f < aDepth)
				aDepth = 1.0f;
		} else {
			aDepth = 0.0f;
		}
		float aStep = (float)((double)(float)(int)(aNumber % 4 + 1) * 0.25f);
		float aFade = (float)(((double)(float)(gApp->mWidth / 2) + (double)aBar->mZ) * 0.0003f);
		if (1.0f < aFade)
			aFade = 1.0f;
		int anAlpha = Ftol((double)aStep * 255.0 * (double)aFade);
		g->SetColor(Color(0xffffff, anAlpha));  // 0x4016f0
		int aBlurWidth = mGearBlur->mWidth;
		int aBlurHeight = mGearBlur->mHeight;
		float aSize = (float)(0.05f / (1.0 - (double)aDepth * 0.99f));
		Transform aTransform;  // 0x44f9d0
		float aSizeX = (float)((double)aSize * 0.35f);
		aTransform.Scale(aSizeX, (float)((double)aSize * 3.0));  // 0x44fae0
		float anAngle = (float)atan2((double)aBar->mX, (double)aBar->mY);  // 0x4c1b70
		aTransform.RotateRad((float)((double)anAngle + 0.003926999866962433 + (double)mAngleB));  // 0x44fa70
		float aCos = cosf(mAngleB);  // 0x401b20
		float aSin = sinf(mAngleB);  // 0x401b70
		float aTurnedX = (float)((double)aBar->mX * (double)aCos + (double)aBar->mY * (double)aSin);
		aCos = cosf(mAngleB);
		aSin = sinf(mAngleB);
		Rect aSource(0, 0, aBlurWidth, aBlurHeight);
		float aReach = (float)((double)aSize * 0.95f + 0.25f);
		float aDrawX = (float)((double)aReach * (double)aTurnedX + (double)(float)(gApp->mWidth / 2));
		float aDrawY = (float)(((double)aBar->mY * (double)aCos - (double)aBar->mX * (double)aSin) * (double)aReach +
			(double)(float)(gApp->mHeight / 2));
		g->DrawImageTransformF(mGearBlur, aTransform, aSource, aDrawX, aDrawY);  // 0x4220f0
		anIndex++;
	}
	g->PopState();  // 0x41fe00
}

// 0x672a71: a gear of scale theScale at theX, theY: first the four gears round it (a quarter of its size, at 0.13, 1.7,
// 3.27 and 4.84 radians from the angle) and the one at its own place turning the other way, each a gear in turn while
// it is over 0.1, then itself when it is on the screen; theBase is always 0 and theDepth is never read
void Gears::DrawGear(Graphics* g, float theScale, float theX, float theY, float theSpin, float theBase, int theDepth)
{
	if (0.1f < (double)theScale) {
		float aBase = (float)((double)theBase + 0.0);
		float aSmall = (float)((double)theScale * 0.271f);
		DrawGear(g, aSmall, theX, theY, (float)((double)theSpin * -3.0), aBase, theDepth + 2);
		float aReach = (float)((double)theScale * 0.255 * (double)mGear->mWidth);
		int aNextDepth = theDepth + 1;
		float aSpin = (float)((double)theSpin * 3.0);
		static const double kAngles[4] = {0.13f, 1.7f, 3.27f, 4.84f};
		for (int i = 0; i < 4; i++) {
			float anAngle = (float)((double)mAngleB + kAngles[i]);
			float aCos = cosf(anAngle);  // 0x401b20
			float aPlaceY = (float)((double)aCos * (double)aReach + (double)theY);
			float aSin = sinf(anAngle);  // 0x401b70
			float aPlaceX = (float)((double)aSin * (double)aReach + (double)theX);
			DrawGear(g, aSmall, aPlaceX, aPlaceY, aSpin, aBase, aNextDepth);
		}
	}
	float aWidth = (float)gApp->mWidth;
	if (0.025f < (double)theScale && (double)theScale < ((double)aWidth + (double)aWidth) / (double)(float)mGear->mWidth &&
		(double)fabsf((float)((double)(float)(gApp->mWidth / 2) - (double)theX)) <
			(double)(float)(gApp->mWidth / 2) + (double)(float)mGear->mWidth * 0.5f * (double)theScale &&
		(double)fabsf((float)((double)(float)(gApp->mHeight / 2) - (double)theY)) <
			(double)(float)(gApp->mHeight / 2) + (double)(float)mGear->mWidth * 0.5f * (double)theScale) {
		// the gear's colour: white fading to 0x222222 and nothing at all the smaller it is (a scale of 1 or more is white)
		float aLimit = 1.0f;
		float aFraction = 1.0f;
		if (!(1.0f <= theScale)) {
			aFraction = (float)(((double)theScale - 0.1f) * 0.75f + 0.65f);
			aLimit = aFraction;
			if (1.0f <= aFraction)
				aLimit = 1.0f;
		} else {
			aLimit = 1.0f;
		}
		float aBlend = 0.0f;
		if (0.0f < aLimit) {
			aBlend = 1.0f;
			if (aFraction < 1.0f)
				aBlend = aFraction;
		}
		uint32_t aWhite = PackArgb(Color(0xffffff, 0xff));  // 0x4016f0, 0x401860
		uint32_t aDark = PackArgb(Color(0x222222, 0));
		g->SetColor(BlendedColor(aDark, aWhite, aBlend));  // 0x67286a
		Transform aTransform;  // 0x44f9d0
		aTransform.Scale(theScale, theScale);  // 0x44fae0
		aTransform.RotateRad((float)((double)theSpin + (double)theBase + (double)mAngleB));  // 0x44fa70
		Rect aSource(0, 0, mGear->mWidth, mGear->mHeight);
		g->DrawImageTransformF(mGear, aTransform, aSource, theX, theY);  // 0x4220f0
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Slipstream

// 0x67a3d2 (through 0x67a79a, which clears the ints from +0x24): a place 100 or more from the middle (by |x| + |y|), tried
// again until it is: three of the C runtime's rand a try
Slipstream::Point::Point()
{
	while (true) {
		int aWidth = gApp->mWidth;
		mX = (float)(CrtRand() % (aWidth + 200) - aWidth / 2 - 100);
		mY = (float)((double)(CrtRand() % 0x18) + (double)mX - 12.0);
		aWidth = gApp->mWidth;
		mZ = (float)(CrtRand() % aWidth - aWidth / 2);
		mAngle = 0.0f;
		mUnknown0C = 0.0f;
		mUnknown10 = 0;
		float aDX = fabsf(mX);
		float aDY = fabsf(mY);
		if (100.0 <= (double)aDY + (double)aDX)
			break;
	}
}

// 0x67b8f4: IMAGE_SLIPSTREAM_POINTY, IMAGE_JELLYTRAIL_TEXTURE and IMAGE_SLIPSTREAM_BG
Slipstream::Slipstream(WinTwistApp* theApp) : Visualizer(theApp)
{
	mUnknown2C = 6.0f;
	mUnknown3C = 9.0f;
	mUnknown40 = 4.0f;
	mUnknown30 = 0xf;
	mUnknown44 = 20.0f;
	mUnknown34 = 0x28;
	mUnknown38 = 300;
	mUnknown48 = 6.0f;
	mUnknown4C = 40.0f;
	mUnknown50 = 4.0f;
	mUnknown54 = 0.0f;
	mUnknown58 = 0.0f;
	mPointy = LoadVizImage(IMAGE_SLIPSTREAM_POINTY_ID);
	mTrail = LoadVizImage(IMAGE_JELLYTRAIL_TEXTURE_ID);
	mBackground = LoadVizImage(IMAGE_SLIPSTREAM_BG_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

// 0x67b561: the images, then every point
Slipstream::~Slipstream()
{
	DeleteVizImage(IMAGE_SLIPSTREAM_POINTY_ID);
	DeleteVizImage(IMAGE_JELLYTRAIL_TEXTURE_ID);
	DeleteVizImage(IMAGE_SLIPSTREAM_BG_ID);
	for (size_t i = 0; i < mTrailPoints.size(); i++)
		delete mTrailPoints[i];
	mTrailPoints.clear();
	for (size_t i = 0; i < mDust.size(); i++)
		delete mDust[i];
	mDust.clear();
}

// 0x67ba37: 300 points of the trail on a spiral, 200 specks of dust at places of their own, and 30 updates
void Slipstream::Init()
{
	int anIndex = 0;
	do {
		Point* aPoint = new Point();  // 0x67a79a
		float aFloatIndex = (float)anIndex;
		aPoint->mSize = (float)(300.0 / ((double)aFloatIndex + 5.0));
		float anArg = (float)((double)aFloatIndex * 0.05f);
		double aRadius = (double)aFloatIndex * 1.5f + 0.0;
		aPoint->mX = (float)((double)sinf(anArg) * aRadius);  // 0x401b70
		float aCos = cosf(anArg);                             // 0x401b20
		aPoint->mY = (float)((double)aCos * aRadius);
		mTrailPoints.push_back(aPoint);  // 0x67b9e2
		anIndex++;
	} while (anIndex < 300);
	int aCount = 200;
	do {
		Point* aPoint = new Point();
		int aWidth = gApp->mWidth;
		aPoint->mX = (float)(CrtRand() % aWidth - aWidth / 2);
		aPoint->mY = (float)(CrtRand() % gApp->mHeight);
		mDust.push_back(aPoint);
		aCount--;
	} while (aCount != 0);
	int anUpdates = 0x1e;
	do {
		Update();
		anUpdates--;
	} while (anUpdates != 0);
}

// 0x67a7ba
void Slipstream::Update()
{
	mCount++;
	int anIndex = 0;
	if (0 < (int)mTrailPoints.size()) {
		do {
			Point* aPoint = mTrailPoints[anIndex];
			int aSlot = anIndex % 0x32;
			float aScale = gApp->mScale;  // +0xf30
			float aBig = (float)((double)aScale * 1200.0);
			aPoint->mSize = (float)((double)aBig / ((double)(100 - aSlot) * 0.002f + 5.0));
			float aStep = (float)((double)aScale * 50.0);
			int aFloor = Ftol((double)aScale * 1200.0);
			aPoint->mX = (float)((double)aStep * (double)aSlot - (double)aFloor);
			float aFloatIndex = (float)anIndex;
			float aCos1 = cosf((float)((double)mCount / 344.0 + (double)aFloatIndex * 0.05f));  // 0x401b20
			double aCos2 = (double)cosf((float)((double)mCount / 211.0 + (double)aFloatIndex * 0.15f));
			float aWave = (float)(((double)aFloatIndex * 0.1f + 30.0) * (double)aCos1 +
				aCos2 * ((double)aFloatIndex * 0.0125f + 5.0));
			aPoint->mY = aWave;
			// the trail's height by the stretch of it that it is on
			if (anIndex < 0x32) {
				aPoint->mY = (float)((double)aWave + (double)(float)((double)aScale * 100.0));
			} else if (anIndex < 100) {
				aPoint->mY = (float)((double)aWave + (double)(float)((double)aScale * 60.0));
			} else if (anIndex < 0x96) {
				aPoint->mY = (float)((double)aWave + (double)(float)((double)aScale * 20.0));
			} else if (anIndex < 200) {
				aPoint->mY = (float)((double)aWave - (double)(float)((double)aScale * 20.0));
			} else if (anIndex < 0xfa) {
				aPoint->mY = (float)((double)aWave - (double)(float)((double)aScale * 60.0));
			} else {
				aPoint->mY = (float)((double)aWave - (double)(float)((double)aScale * 100.0));
			}
			anIndex++;
		} while (anIndex < (int)mTrailPoints.size());
	}
	anIndex = 0;
	if (0 < (int)mDust.size()) {
		int aCountdown = 200;
		do {
			Point* aPoint = mDust[anIndex];
			float anOldX = aPoint->mX;
			float anOldY = aPoint->mY;
			float aDrift = (float)((double)gApp->mScale * -1.3f);
			float aNewX = (float)((double)aDrift + (double)aPoint->mX);
			aPoint->mX = aNewX;
			int aWidth = gApp->mWidth;
			if ((double)(-aWidth) * 0.55f > (double)aNewX)
				aPoint->mX = (float)((double)aWidth * 1.1f + (double)aNewX);
			int aHalfHeight = gApp->mHeight / 2;
			float aRise = (float)((double)aHalfHeight / ((double)aCountdown * 0.1f + 5.0));
			int aStep = mCount - anIndex;
			float aPhase = (float)((double)aPoint->mX * 0.75f + (double)aPoint->mY + (double)aStep);
			float aCosA = cosf((float)((double)aPhase * 0.0012f));
			float aCosB = cosf((float)((double)aPhase * 0.013f));
			double aCosC = (double)cosf((float)((double)(mCount + anIndex) * 0.0003f * (double)(anIndex % 0x32 + 100)));
			float aHeight = (float)(((double)aRise * 0.5f * (double)aCosA - 3.0 * (double)aCosB) + aCosC * 3.0);
			aPoint->mY = aHeight;
			float aScale = gApp->mScale;
			int aRow = Ftol((double)aScale * 20.0) * (anIndex % 0x19);
			aHeight = (float)((double)aHeight - (double)(Ftol((double)aScale * 240.0) - aRow));
			aPoint->mY = aHeight;
			if (anIndex < Ftol((double)aScale * 66.0))
				aPoint->mY = (float)((double)(gApp->mHeight / 5) + (double)aHeight);
			if (Ftol((double)aScale * 133.0) < anIndex)
				aPoint->mY = (float)((double)aPoint->mY - (double)(gApp->mHeight / 5));
			float aDeltaY = (float)((double)anOldY - (double)aPoint->mY);
			float aDeltaX = (float)((double)anOldX - (double)aPoint->mX);
			aPoint->mAngle = (float)atan2((double)aDeltaX, (double)aDeltaY);  // 0x4c1b70
			anIndex++;
			aCountdown--;
		} while (anIndex < (int)mDust.size());
	}
}

// 0x67ad27: the trail's point as four vertices of the strip across it (a thin, vertical one: the point's own place on
// the screen, and the point's size above and below it), in its blue and its pinker colour at the end. The point's
// extent (its +0x10, from 40 to 300, shrinks it) is never over 40, and its direction ints are never set.
void Slipstream::MakeStripVertices(Point* thePoint, TriVertex* theVertices)
{
	float aShrink = 0.0f;
	if (mUnknown34 < thePoint->mUnknown10)
		aShrink = (float)((double)(float)(thePoint->mUnknown10 - mUnknown34) * (double)mUnknown3C /
			(double)(mUnknown38 - mUnknown34));
	thePoint->mUnknown18 = 20.0f;
	float aSize = (float)((double)thePoint->mSize - (double)aShrink);
	thePoint->mFlag = aSize <= 0.0f;
	if (thePoint->mFlag)
		aSize = 0.0f;
	thePoint->mScreenX = Ftol((double)(gApp->mWidth / 2) + (double)thePoint->mX);
	thePoint->mScreenY = Ftol((double)(gApp->mHeight / 2) + (double)thePoint->mY);
	thePoint->mUnknown2C = 0;
	thePoint->mUnknown30 = 1;
	int aX = thePoint->mScreenX + thePoint->mUnknown34 * thePoint->mUnknown2C;
	int aY = thePoint->mScreenY + thePoint->mUnknown38 * thePoint->mUnknown30;
	int aSizeInt = Ftol((double)aSize);
	float aXs[4] = {(float)(aX - thePoint->mUnknown2C * aSizeInt), (float)aX, (float)aX,
		(float)(thePoint->mUnknown2C * aSizeInt + aX)};
	float aYs[4] = {(float)(aY - thePoint->mUnknown30 * aSizeInt), (float)aY, (float)aY,
		(float)(thePoint->mUnknown30 * aSizeInt + aY)};
	for (int i = 0; i < 4; i++) {
		theVertices[i].x = aXs[i];
		theVertices[i].y = aYs[i];
	}
	// the colours: Color(0x88ff) and Color(0xffbbff), the first with alpha 0x9b for the first three vertices, and the
	// last as 0.8 of the way (of each channel) to the second (the original blends 0.4 of the way too, and does not use it)
	Color aBlue(0x88ff);
	Color aPink(0xffbbff);
	uint32_t aBlueColor = ((((uint32_t)aBlue.mRed | 0xffff9b00) << 8 | (uint32_t)aBlue.mGreen) << 8) | (uint32_t)aBlue.mBlue;
	int aRed = Ftol((double)aBlue.mRed - (double)(aBlue.mRed - aPink.mRed) * 0.800000011920929);
	int aGreen = Ftol((double)aBlue.mGreen - (double)(aBlue.mGreen - aPink.mGreen) * 0.800000011920929);
	int aBlueChannel = Ftol((double)aBlue.mBlue - (double)(aBlue.mBlue - aPink.mBlue) * 0.800000011920929);
	theVertices[0].color = aBlueColor;
	theVertices[1].color = aBlueColor;
	theVertices[2].color = aBlueColor;
	theVertices[3].color = ((((uint32_t)aRed | 0xffff9b00) << 8 | (uint32_t)aGreen) << 8) | (uint32_t)aBlueChannel;
}

// 0x67af9b: the trail as a strip of the texture, in four triangles between each point and the one before it, except where
// the trail's 50 stretches meet (the points whose index is 0 or 1 more than a multiple of 50)
void Slipstream::DrawTrail(Graphics* g)
{
	// the strip's texture coordinates: along it (u, 0 to 1 over the two points), and across it (v: 0, a half, a half, 0)
	TriVertex aPrevious[4];
	TriVertex aCurrent[4];
	for (int i = 0; i < 4; i++) {
		aPrevious[i] = TriVertex(0.0f, 0.0f, 0.0f, (i == 1 || i == 2) ? 0.5f : 0.0f);
		aCurrent[i] = TriVertex(0.0f, 0.0f, 1.0f, (i == 1 || i == 2) ? 0.5f : 0.0f);
	}
	Image* anImage = mTrail;
	g->SetColor(Color::White);  // 0xe14068
	MakeStripVertices(mTrailPoints[0], aPrevious);
	int anIndex = 1;
	if (1 < (int)mTrailPoints.size()) {
		do {
			MakeStripVertices(mTrailPoints[anIndex], aCurrent);
			if (anIndex % 0x32 != 0 && anIndex % 0x32 != 1) {
				g->SetColor(Color(0, 0, 0xfa, 0xff));
				g->SetColorizeImages(true);
				g->DrawTriangleTex(anImage, aPrevious[0], aCurrent[1], aCurrent[0]);  // 0x422130
				g->DrawTriangleTex(anImage, aPrevious[0], aPrevious[1], aCurrent[1]);
				g->DrawTriangleTex(anImage, aPrevious[2], aPrevious[3], aCurrent[2]);
				g->DrawTriangleTex(anImage, aCurrent[2], aPrevious[3], aCurrent[3]);
			}
			anIndex++;
			for (int i = 0; i < 4; i++)
				aPrevious[i] = aCurrent[i];
		} while (anIndex < (int)mTrailPoints.size());
	}
}

// 0x67b308
void Slipstream::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	g->SetColorizeImages(false);
	g->SetColor(Color(0xff00ff));
	DrawImage(g, mBackground);  // 0x67fdb6
	g->SetColorizeImages(true);
	g->SetColor(Color(0xffffff));
	g->SetDrawMode(1);
	DrawTrail(g);
	// the specks of dust: IMAGE_SLIPSTREAM_POINTY, long and thin, turned the way each one moves
	size_t anIndex = 0;
	while (anIndex != mDust.size()) {
		Point* aPoint = mDust[anIndex];
		Transform aTransform;  // 0x44f9d0
		Image* aPointy = mPointy;
		g->SetDrawMode(1);
		g->SetColor(Color(0x22, 0x88, 0xff, 100));
		aTransform.Scale(0.3f, 1.6f);  // 0x44fae0
		aTransform.RotateRad((float)((double)aPoint->mAngle + 3.14f));  // 0x44fa70
		float aDrawY = (float)((double)(gApp->mHeight / 2) + (double)aPoint->mY);
		float aDrawX = (float)((double)(gApp->mWidth / 2) + (double)aPoint->mX);
		g->DrawImageTransform(aPointy, aTransform, aDrawX, aDrawY);  // 0x422050
		anIndex++;
	}
	g->PopState();  // 0x41fe00
}

