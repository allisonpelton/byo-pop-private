// Hyperspace.cpp: Sexy::Hyperspace (Hyperspace.h), the jump between two levels.
//   Draw2dPortal 0x5c5a68, Draw3dPortal 0x5c620e, Draw 0x5c6beb, destructor 0x5c7033, constructor 0x5c72d1,
//   Update2d 0x5c7532, DoUpdate 0x5c7957, Update 0x5c7fd3.
// The original is x87 code: every value it stores in a float is rounded there and nowhere else, and the constants it
// multiplies by are the floats that its source named (0.006f is 0.006000000052154064 in the code), so the sums here are made in
// double and rounded to float where the original stores (the casts) and the constants are float literals.

#include "Hyperspace.h"
#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "StarField.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "DDInterface.h"
#include "TriVertex.h"
#include "Common.h"
#include "Compat2004.h"
#include <math.h>

using namespace Sexy;

extern float gSinTable[];           // BoardCtor.cpp: the sine and the cosine of the 4096 steps of a turn (0x6713c8 and 0x6755c8)
extern float gCosTable[];

// the C runtime's sinf (0x404960) as the framework's other ports make it
static inline float Sinf(float theAngle)
{
	return (float)sin((double)theAngle);
}

// The original sets the texture addressing of Direct3D's stage 0 round the tunnel's triangles (SetTextureStageState with
// 0xd and 0xe: u and v; 1 is wrap, 3 clamp): wrap, so that the picture repeats where a point's coordinate goes past 1,
// and clamp for a v below 0. The framework's triangle drawing has no such control (the picture is clamped).
// TODO(coordinator): DDInterface::DrawTrianglesTex needs an address mode for u and v.
static void SetTextureAddress(int theU, int theV)
{
	(void)theU;
	(void)theV;
}

// 0x5c72d1
Hyperspace::Hyperspace(WinDMApp* theApp)
{
	mFade = 0;
	mWaveA = 0;
	mApp = theApp;
	mWaveB = 0;
	mStep = 0;
	mMouseVisible = false;
	mWaveC = 0;
	mDone = false;
	mWaveD = 0;
	mDoneDelay = 0;
	mWaveE = 0;
	mPhase = 0;
	mWaveF = 0;
	mFadeDelay = 0x78;
	mCenter2X = 0;
	mReady = false;
	mCenter2Y = 0;
	mStartDelay = 200;
	mCenterX = 0;
	mCenterY = 0;
	mProgress = 0;
	for (int aRing = 0; aRing < 24; aRing++)
	{
		TunnelRing& aR = mRings[aRing];
		aR.mCurZ = 0;
		aR.mNextZ = 0;
		aR.mCurX = 0;
		aR.mCurY = 0;
		aR.mNextX = 0;
		aR.mNextY = 0;
		aR.mLerpX = 0;
		aR.mLerpY = 0;
		aR.mScreenX = 0;
		aR.mScreenY = 0;
		aR.mSize = 0;       // not set by the original, DoUpdate sets it before it is read
		float aV = (float)((double)aRing * 0.02 - 0.48);
		for (int aPoint = 0; aPoint < 24; aPoint++)
		{
			float aU = (float)((double)aPoint / 24.0);
			TunnelPoint& aP = aR.mPoints[aPoint];
			aP.mX = 0;      // set by DoUpdate
			aP.mY = 0;
			aP.mU[0] = aU;
			aP.mU[1] = aU;
			aP.mV[0] = aV;
			aP.mV[1] = aV;
		}
	}
	m3D = theApp->Is3DAccelerated();
	mPosA = 4.0f;
	mSpeedA = 0.3f;
	mFrame = 0;
	mPosB = 0;
	mSpeedB = 0;
	mStarAcc = 0;
	mStarRate = 0;
	mStarSpeed = 0.12f;
	mJitter = 0;
	mNeedBgCache = true;
	mStretch = 1.0f;
	mStretchSpeed = 0;
	if (!m3D)
		mPhase = 2;
	else
	{
		static_cast<MemoryImage*>(IMAGE_TUNNEL_END)->mD3DFlags = 1;
		static_cast<MemoryImage*>(IMAGE_FIRE_RING)->mD3DFlags = 1;
	}
}

// 0x5c7033: the lines of stars go with the vector
Hyperspace::~Hyperspace()
{
}

// 0x5c7fd3: the mode with the hyper cube runs the jump twice as fast
void Hyperspace::Update()
{
	DoUpdate();
	if (mApp->mBoard->mIsHyper)
		DoUpdate();
}

// 0x5c7532: the 2D jump: the glow flies up and down, lines of stars fall from the top and bottom of the screen, the picture
// of the board shakes and is stretched. The jump ends 500 updates after it began.
void Hyperspace::Update2d()
{
	mFrame++;
	if (mFrame >= 0x78)
		mStarRate = (float)((double)mStarRate + 0.017f);
	if (mPosA < (float)mHeight)
	{
		mStarAcc = (float)((double)mStarRate + (double)mStarAcc);
		while (mStarAcc >= 1.0f)
		{
			WarpLine aLine;
			aLine.mX = Rand() % mWidth;
			aLine.mA = 0;
			aLine.mB = 0;
			aLine.mVelA = 3.0f;
			aLine.mVelB = 0;
			mLines.push_back(aLine);
			mStarAcc = (float)((double)mStarAcc - 1.0);
		}
	}
	if (mFrame >= 300)
	{
		float aSpeed = (float)((double)mStarSpeed + 0.02f);
		mStarSpeed = (float)((double)aSpeed * 0.95f);
	}
	for (int aLine = 0; aLine < (int)mLines.size(); aLine++)
	{
		WarpLine& aWarp = mLines[aLine];
		aWarp.mVelA = (float)((double)mStarSpeed + (double)aWarp.mVelA);
		aWarp.mVelB = (float)((double)mStarSpeed * 0.97f + (double)aWarp.mVelB);
		aWarp.mA = (float)((double)aWarp.mA + (double)aWarp.mVelA);
		aWarp.mB = (float)((double)aWarp.mVelB + (double)aWarp.mB);
		if ((float)(mHeight / 2) <= aWarp.mB)
		{
			mLines.erase(mLines.begin() + aLine);
			aLine--;
		}
	}
	if (!mReady && mApp->mBoard->m15fb0 == 0.0f)
	{
		mReady = true;
		mNeedBgCache = true;
	}
	if (mFrame < 400)
	{
		float aSpeed;
		if (mFrame < 180)
		{
			float aT = (float)((double)mSpeedA + 0.0023f);
			aSpeed = (float)((double)aT * 0.997f);
		}
		else
		{
			float aT = (float)((double)mSpeedA - 0.008f);
			aSpeed = (float)((double)aT * 0.96f);
		}
		mSpeedA = aSpeed;
		mPosA = (float)((double)aSpeed + (double)mPosA);
	}
	else
	{
		if (mSpeedA < 0.0f)
			mSpeedA = 2.0f;
		if (mPosA < 600.0f)
		{
			float aT = (float)((double)mSpeedA + 0.03f);
			float aT2 = (float)((double)aT * 0.05f + (double)aT);
			mSpeedA = aT2;
			float aU = (float)((double)aT2 * (double)aT2 * 0.001f + (double)mSpeedA);
			float aV = (float)((double)aU * 0.997f);
			mSpeedA = aV;
			mPosA = (float)((double)aV + (double)mPosA);
		}
	}
	if (mFrame > 150)
	{
		float aSpeed = (float)((double)mSpeedB + 0.01f);
		mSpeedB = aSpeed;
		if (2.0f > aSpeed)
			mSpeedB = 2.0f;
		if (mPosB < 600.0f)
		{
			float aW = (float)((double)mSpeedB * 0.02f + (double)mSpeedB);
			mSpeedB = aW;
			float aU = (float)((double)aW + (double)aW * (double)aW * 0.001f);
			float aV = (float)(0.997f * (double)aU);
			mSpeedB = aV;
			mPosB = (float)((double)aV + (double)mPosB);
		}
	}
	if (mFrame > 100)
	{
		double aJitter = (double)mJitter + 0.1;
		mJitter = (float)(5.0 <= aJitter ? 5.0 : aJitter);
	}
	if (mFrame > 200)
	{
		double aSpeed = (double)mStretchSpeed + 5e-05;
		mStretchSpeed = (float)(0.004 <= aSpeed ? 0.004 : aSpeed);
	}
	mStretch = (float)((double)mStretch - (double)mStretchSpeed);
	if (mUpdateCnt == 500)
		mDone = true;
	if (!mApp->mBoard->mIsPuzzle && mUpdateCnt == 0x1c2)
		mApp->PlaySample(SOUND_GET_READY);
	if (!mReady)
		MarkDirty();
	else
		MarkDirtyFull();
}

// 0x5c7957: one step of the 3D jump. The flash fades in, then the tunnel flies until mProgress is 1. The far ring sways on
// sines, and every ring takes the place that the one behind it had a step ago (a step is a quarter of an update).
void Hyperspace::DoUpdate()
{
	if (mApp->mBoard->mPauseCount > 0)
		return;
	Widget::Update();
	if (!m3D)
	{
		Update2d();
		return;
	}
	if (mFadeDelay > 0)
	{
		mFadeDelay--;
		if (mFadeDelay == 0)
		{
			mFade = 1.0f;
			mPhase = 1;
			mReady = true;
		}
		if (mFadeDelay < 20)
		{
			mFade = (float)((double)(20 - mFadeDelay) / 20.0);
			MarkDirty();
		}
	}
	else if (0.0f < mFade)
	{
		mFade = (float)((double)mFade - 0.015f);
		MarkDirty();
	}
	if (mReady)
	{
		if (mStartDelay > 0)
			mStartDelay--;
		else if (mProgress < 1.0f)
		{
			float aInc = (float)((double)mProgress / 50.0 + 0.001f);
			float aSlow = (float)((1.001f - (double)mProgress) * 5.0);
			if (aSlow < 1.0f)
				aInc = (float)((double)aSlow * (double)aInc);
			mProgress = (float)((double)mProgress + (double)aInc);
		}
		if (mDoneDelay > 0)
		{
			mDoneDelay--;
			if (mDoneDelay == 0)
				mDone = true;
		}
		else if (mProgress >= 1.0f)
		{
			mProgress = 1.0f;
			mDoneDelay = 10;
		}
		mWaveA = (float)((double)mWaveA + 0.006f);
		mWaveB = (float)((double)mWaveB + 0.0093f);
		mWaveC = (float)((double)(float)((double)0.006f + (double)mWaveC) + 0.0017f);
		mWaveE = (float)((double)(float)((double)mWaveE + 0.01f) + 0.003f);
		TunnelRing& aFar = mRings[23];
		float aSwayZ = (float)((double)Sinf(mWaveA) * 0.002f);
		aFar.mCurZ = (float)((double)aSwayZ + (double)aFar.mCurZ);
		float aSwayZ2 = (float)((double)Sinf(mWaveB) * 0.001f);
		float aZ = (float)((double)aSwayZ2 + (double)aFar.mCurZ);
		aFar.mCurZ = aZ;
		aFar.mNextZ = aZ;
		float aSinC = Sinf(mWaveC);
		float aSinD = Sinf(mWaveD);
		float aPartD = (float)((double)aSinD * 5.0);
		float aPartC = (float)((double)aSinC * 4.0);
		float aX = (float)((double)aPartD + (double)aPartC + (double)aFar.mCurX);
		aFar.mCurX = aX;
		aFar.mNextX = aX;
		float aSinE = Sinf(mWaveE);
		float aSinF = Sinf(mWaveF);
		float aPartF = (float)((double)aSinF * 5.0);
		float aPartE = (float)((double)aSinE * 4.0);
		float aY = (float)((double)aPartF + (double)aPartE + (double)aFar.mCurY);
		aFar.mCurY = aY;
		aFar.mNextY = aY;
	}
	mStep = (float)((double)mStep + 0.25);
	if (mStep >= 1.0f)
	{
		for (int aRing = 0; aRing < 23; aRing++)
		{
			TunnelRing& aR = mRings[aRing];
			aR.mCurX = aR.mNextX;
			aR.mNextX = mRings[aRing + 1].mCurX;
			aR.mCurY = aR.mNextY;
			aR.mNextY = mRings[aRing + 1].mCurY;
			aR.mCurZ = aR.mNextZ;
			aR.mNextZ = mRings[aRing + 1].mCurZ;
		}
		mStep = (float)((double)mStep - 1.0);
	}
	// the middle of the tunnel follows the first ring
	float aDiff = (float)((double)mRings[0].mLerpX - (double)mCenterX);
	float aMove = (float)(0.09 * (double)aDiff);
	mCenterX = (float)((double)aMove + (double)mCenterX);
	aDiff = (float)((double)mRings[0].mLerpY - (double)mCenterY);
	aMove = (float)(0.09 * (double)aDiff);
	mCenterY = (float)((double)aMove + (double)mCenterY);
	aDiff = (float)((double)mRings[1].mLerpX - (double)mCenter2X);
	aMove = (float)(0.08 * (double)aDiff);
	mCenter2X = (float)((double)aMove + (double)mCenter2X);
	aDiff = (float)((double)mRings[1].mLerpY - (double)mCenter2Y);
	aMove = (float)(0.08 * (double)aDiff);
	mCenter2Y = (float)((double)aMove + (double)mCenter2Y);
	if (mProgress < 1.0f)
	{
		for (int aRing = 0; aRing < 24; aRing++)
		{
			TunnelRing& aR = mRings[aRing];
			double aOneMinusStep = 1.0 - (double)mStep;
			float aZ = (float)((double)mStep * (double)aR.mNextZ + (double)aR.mCurZ * aOneMinusStep);
			aR.mLerpX = (float)((double)aR.mNextX * (double)mStep + (double)aR.mCurX * aOneMinusStep);
			aR.mLerpY = (float)(aOneMinusStep * (double)aR.mCurY + (double)aR.mNextY * (double)mStep);
			int aAngle = FTol((double)aZ * 4096.0 / (double)3.14159f * 2.0);
			if (aAngle < 0)
				aAngle = 0x1000 - aAngle;
			float aRemain = (float)(1.0 - (double)mProgress);
			double aRingScaled = (double)aRing * (double)aRemain;
			float aInner = (float)(800.0 - 5000.0 * aRingScaled / 24.0);
			float aZoom = (float)(1024.0 / (1024.0 - (double)aInner));
			int aRadius = FTol(200.0 - aRingScaled * 150.0 / 24.0);
			aR.mScreenX = (float)(((double)aR.mLerpX - (double)mCenterX) * (double)aRemain * (double)aZoom + 512.0);
			aR.mScreenY = (float)(((double)aR.mLerpY - (double)mCenterY) * (double)aRemain * (double)aZoom + 384.0);
			float aRadiusF = (float)aRadius;
			aR.mSize = (float)((double)aRadiusF * (double)aZoom);
			for (int aPoint = 0, aStep = 0; aStep < 0x18000; aStep += 0x1000, aPoint++)
			{
				int anIndex = (aStep / 0x18 + aAngle) & 0xfff;
				TunnelPoint& aP = aR.mPoints[aPoint];
				aP.mX = (float)((((double)aR.mLerpX - (double)mCenterX) * (double)aRemain +
					(double)gCosTable[anIndex] * (double)aRadiusF) * (double)aZoom + 512.0);
				aP.mY = (float)((((double)aR.mLerpY - (double)mCenterY) * (double)aRemain +
					(double)gSinTable[anIndex] * (double)aRadiusF) * (double)aZoom + 384.0);
				aP.mV[0] = (float)((double)aP.mV[0] + 0.006f);
				if (1.0f < aP.mV[0])
					aP.mV[0] = (float)((double)aP.mV[0] - 1.0);
				aP.mV[1] = (float)((double)aP.mV[1] + 0.004f);
				if (1.0f < aP.mV[1])
					aP.mV[1] = (float)((double)aP.mV[1] - 1.0);
			}
		}
	}
	if (!mReady)
		MarkDirtyFull();
	else
		MarkDirty();
}

// 0x5c5a68: the 2D jump: the board's picture, a line at a time, shaken and stretched; the strips of the backdrop that open
// from the middle (unless the star field is on); the glows above and below; the lines of stars
void Hyperspace::Draw2dPortal(Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	if (mReady && aBoard->m15f60 != NULL)
	{
		if (mNeedBgCache)
		{
			aBoard->DrawAllToBackgroundCache();
			mNeedBgCache = false;
		}
		int aShakeX = FTol((double)(CrtRand() % 20000 - 10000) / 10000.0 * (double)mJitter);
		int aShakeY = FTol((double)(CrtRand() % 20000 - 10000) / 10000.0 * (double)mJitter);
		int aHeight = mHeight;
		int aStretched = FTol((double)aHeight / (double)mStretch);
		float aSourceY = (float)((double)(aStretched - aHeight) * 0.5 * (double)mStretch);
		if (aHeight > 0)
		{
			int aRow = 0;
			do
			{
				g->DrawImage(aBoard->m15f60, aShakeX, aShakeY + aRow, Rect(0, FTol((double)aSourceY), mWidth, 1));
				aRow++;
				aSourceY = (float)((double)mStretch + (double)aSourceY);
			} while (aRow < mHeight);
		}
	}
	if (aBoard->m165bc == NULL)
	{
		float aOpen = (float)((double)mPosA + (double)mPosA);
		double aOpenScaled = (double)mApp->ScaleNum(aOpen, 0.0f);
		int aHeight1 = FTol(aOpenScaled - (double)mApp->ScaleNum(0x8c, 0));
		int aHeight2 = aHeight1 - mApp->ScaleNum(0x19, 0) * 2;
		if (aHeight1 > mApp->ScaleNum(0x300, 0))
			aHeight1 = mApp->ScaleNum(0x300, 0);
		if (aHeight2 > mApp->ScaleNum(0x300, 0))
			aHeight2 = mApp->ScaleNum(0x300, 0);
		if (aHeight1 > 0)
		{
			int aThird = mWidth / 6;
			int aMiddle = mWidth - aThird * 2;
			int aHalfHeight2 = aHeight2 / 2;
			int aY1 = mApp->ScaleNum(0x180, 0) - aHalfHeight2;
			g->DrawImage(aBoard->mBackdropImage, 0, aY1, Rect(0, aY1, aThird, aHeight2));
			int aY2 = mApp->ScaleNum(0x180, 0) - aHeight1 / 2;
			g->DrawImage(aBoard->mBackdropImage, aThird, aY2, Rect(aThird, aY2, aMiddle, aHeight1));
			int aY3 = mApp->ScaleNum(0x180, 0) - aHalfHeight2;
			int aX3 = aMiddle + aThird;
			g->DrawImage(aBoard->mBackdropImage, aX3, aY3, Rect(aX3, aY3, aThird, aHeight2));
		}
	}
	// the glows: the top one, then the bottom one, each twice (in colour and in white)
	Graphics aTopGraphics(*g);
	aTopGraphics.SetDrawMode(1);
	aTopGraphics.ClipRect(0, 0, mApp->ScaleNum(0x400, 0), mApp->ScaleNumUp(0x181, 0));
	aTopGraphics.DrawImage(IMAGE_WARP_TOP_GLOW, 0, mApp->ScaleNum(0x180 - FTol((double)mPosA), 0));
	aTopGraphics.SetColorizeImages(true);
	aTopGraphics.SetColor(Color(255, 255, 255, 255));
	aTopGraphics.DrawImage(IMAGE_WARP_TOP_GLOW, 0, mApp->ScaleNum(0x180 - FTol((double)mPosB), 0));
	Graphics aBottomGraphics(*g);
	aBottomGraphics.SetDrawMode(1);
	aBottomGraphics.ClipRect(0, mApp->ScaleNum(0x17f, 0), mApp->ScaleNum(0x400, 0), mApp->ScaleNumUp(0x181, 0));
	aBottomGraphics.DrawImage(IMAGE_WARP_BOTTOM_GLOW, 0, mApp->ScaleNum(FTol((double)mPosA) + 0xf5, 0));
	aBottomGraphics.SetColorizeImages(true);
	aBottomGraphics.SetColor(Color(255, 255, 255, 255));
	aBottomGraphics.DrawImage(IMAGE_WARP_BOTTOM_GLOW, 0, mApp->ScaleNum(FTol((double)mPosB) + 0xf5, 0));
	// the lines of stars: the first set in the lower half of the screen, the second set (mirrored) in the upper half
	Graphics aLowerGraphics(*g);
	int aLowerTop = mApp->ScaleNum(FTol((double)mPosA) + 0x130, 0);
	if (mApp->ScaleNum(0x180, 0) < aLowerTop)
		aLowerTop = mApp->ScaleNum(FTol((double)mPosA) + 0x130, 0);
	else
		aLowerTop = mApp->ScaleNum(0x180, 0);
	aLowerGraphics.ClipRect(0, aLowerTop, mWidth, mApp->ScaleNum(0x180, 0));
	Graphics aUpperGraphics(*g);
	int aUpperHeight = mApp->ScaleNum(0x1d0 - FTol((double)mPosA), 0);
	int aUpperRows;
	if (aUpperHeight < mApp->ScaleNum(0x180, 0))
		aUpperRows = 0x1d0 - FTol((double)mPosA);
	else
		aUpperRows = 0x180;
	aUpperGraphics.ClipRect(0, 0, mWidth, mApp->ScaleNum(aUpperRows, 0));
	for (int aLine = 0; aLine < (int)mLines.size(); aLine++)
	{
		WarpLine& aWarp = mLines[aLine];
		int aHalfWidth = mWidth / 2;
		int aHalfHeight = mHeight / 2;
		float aScaledB = mApp->ScaleNum(aWarp.mB, 0.0f);
		int aX1 = FTol((double)(aWarp.mX - aHalfWidth) * ((double)aScaledB * 0.01 + 1.0)) + aHalfWidth;
		int aY1 = FTol((double)mApp->ScaleNum(aWarp.mB, 0.0f)) + aHalfHeight;
		float aScaledA = mApp->ScaleNum(aWarp.mA, 0.0f);
		int aX2 = FTol((double)(aWarp.mX - aHalfWidth) * ((double)aScaledA * 0.01 + 1.0)) + aHalfWidth;
		int aY2 = FTol((double)mApp->ScaleNum(aWarp.mA, 0.0f)) + aHalfHeight;
		aLowerGraphics.SetColor(Color(255, 255, 255));
		aLowerGraphics.DrawLine(aX1, aY1, aX2, aY2);
		aUpperGraphics.SetColor(Color(255, 255, 255));
		aUpperGraphics.DrawLine(mWidth - aWarp.mX, mHeight - aY1, mWidth - aWarp.mX, mHeight - aY2);
	}
}

// 0x5c620e: the 3D jump. First the backdrop (or the star field), opened from the middle as the tunnel nears its end; then,
// once the jump has begun, the picture at the end of the tunnel with black round it, and the ring of fire; then the rings
// of the tunnel as triangles in two layers (the picture and its lines), the nearer rings brighter; then the flash.
void Hyperspace::Draw3dPortal(Graphics* g)
{
	if (mPhase == 1)
		mPhase = 2;
	Board* aBoard = mApp->mBoard;
	// (the original makes Direct3D ready to draw here: 0x448670; the framework does that itself)
	if (mReady)
	{
		SetTextureAddress(3, 3);
		int aAlpha = FTol((double)mProgress * 300.0);
		if (aAlpha > 255)
			aAlpha = 255;
		else if (aAlpha < 0)
			aAlpha = 0;
		double aOpen = (double)mProgress * (double)mProgress * (double)mProgress * 0.95 + 0.2;
		int aWidth = FTol(aOpen * 1024.0);
		if (aWidth >= 0x400)
			aWidth = 0x400;
		int aHeight = FTol(aOpen * 768.0);
		if (aHeight >= 0x300)
			aHeight = 0x300;
		Image* aBackdrop = aBoard->mBackdropImage;
		float aCenterX = (float)((double)mProgress * 1024.0 * 0.5 + (double)mRings[23].mScreenX * (1.0 - (double)mProgress));
		float aCenterY = (float)((1.0 - (double)mProgress) * (double)mRings[23].mScreenY + (double)mProgress * 768.0 * 0.5);
		if (aWidth > 0 && aHeight > 0)
		{
			if (aBoard->m165bc == NULL)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(aAlpha, aAlpha, aAlpha));
				Rect aSrc(0, 0, mApp->ScaleNum(0x400, 0), mApp->ScaleNum(0x300, 0));
				int aDestHeight = mApp->ScaleNum(aHeight, 0);
				int aDestWidth = mApp->ScaleNum(aWidth, 0);
				int aDestY = FTol((double)mApp->ScaleNum((float)((double)aCenterY - (double)(aHeight / 2)), 0.0f));
				int aDestX = FTol((double)mApp->ScaleNum((float)((double)aCenterX - (double)(aWidth / 2)), 0.0f));
				g->DrawImage(aBackdrop, Rect(aDestX, aDestY, aDestWidth, aDestHeight), aSrc);
				g->SetColorizeImages(false);
			}
			else
				aBoard->m165bc->Draw(g, aBoard->m165b8);
		}
		// the picture at the far end of the tunnel, with black round it
		int aEndSize = FTol((double)mRings[23].mSize * 2.5);
		int aEndHeight = mApp->ScaleNum(aEndSize, 0);
		int aEndWidth = mApp->ScaleNum(aEndSize, 0);
		float aEndHalf = (float)(aEndSize / 2);
		int aEndY = FTol((double)mApp->ScaleNum((float)((double)mRings[23].mScreenY - (double)aEndHalf), 0.0f));
		int aEndX = FTol((double)mApp->ScaleNum((float)((double)mRings[23].mScreenX - (double)aEndHalf), 0.0f));
		Rect anEnd(aEndX, aEndY, aEndWidth, aEndHeight);
		if (mProgress < 1.0f)
			g->DrawImage(IMAGE_TUNNEL_END, anEnd, Rect(0, 0, mApp->ScaleNum(0x80, 0), mApp->ScaleNum(0x80, 0)));
		g->SetColor(Color(0, 0, 0));
		g->FillRect(0, 0, anEnd.mX, mHeight);
		g->FillRect(anEnd.mX, 0, anEnd.mWidth, anEnd.mY);
		g->FillRect(anEnd.mX, anEnd.mHeight + anEnd.mY - 1, anEnd.mWidth, mHeight - anEnd.mHeight - anEnd.mY + 1);
		g->FillRect(anEnd.mWidth + anEnd.mX - 1, 0, mWidth - anEnd.mWidth - anEnd.mX + 1, mHeight);
		// the ring of fire round the end of the tunnel
		g->SetDrawMode(1);
		g->SetColorizeImages(true);
		int aFireAlpha = FTol((double)mProgress * 400.0);
		if (aFireAlpha > 255)
			aFireAlpha = 255;
		if (aFireAlpha > 0)
		{
			g->SetColor(Color(aFireAlpha, aFireAlpha, aFireAlpha));
			int aFireSize = FTol((double)mRings[23].mSize * 5.0);
			int aFireWidth = mApp->ScaleNumUp(aFireSize, 0);
			int aFireHeight = mApp->ScaleNumUp(aFireSize, 0);
			float aFireHalf = (float)(aFireSize / 2);
			int aFireY = FTol((double)mApp->ScaleNum((float)((double)mRings[23].mScreenY - (double)aFireHalf), 0.0f));
			int aFireX = FTol((double)mApp->ScaleNum((float)((double)mRings[23].mScreenX - (double)aFireHalf), 0.0f));
			Rect aFireDest(aFireX, aFireY, aFireWidth, aFireHeight);
			if (mProgress < 1.0f)
			{
				int aFireFrame = (mUpdateCnt / 2) % 10;
				Rect aFireSrc(mApp->ScaleNum(aFireFrame * 200, 0), 0, mApp->ScaleNum(200, 0), mApp->ScaleNum(200, 0));
				g->DrawImage(IMAGE_FIRE_RING, aFireDest, aFireSrc);
			}
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(0);
	}
	if (mProgress < 1.0f)
	{
		// the rings from the farthest (22, drawn between 22 and 23) to the nearest, in one layer before the jump has
		// begun and two after
		int aLayers = (mReady ? 1 : 0) + 1;
		for (int aLayer = 0; aLayer < aLayers; aLayer++)
		{
			int aBrightness = 0x1ef0;       // 360 for each ring
			for (int aRing = 22; aBrightness >= 0; aRing--, aBrightness -= 0x168)
			{
				int aGrey = 0xff;
				int aDim = 0x180 - aBrightness / 0x18;
				if (aDim <= 0xff)
					aGrey = aDim;
				TunnelRing& aNear = mRings[aRing];
				TunnelRing& aFarRing = mRings[aRing + 1];
				for (int aPoint = 0; aPoint < 24; aPoint++)
				{
					int aNext = (aPoint + 1) % 24;
					TunnelPoint& aA = aNear.mPoints[aPoint];
					TunnelPoint& aB = aNear.mPoints[aNext];
					TunnelPoint& aC = aFarRing.mPoints[aPoint];
					TunnelPoint& aD = aFarRing.mPoints[aNext];
					TriVertex aVerts[4];
					aVerts[0] = TriVertex(mApp->ScaleNum(aA.mX, 0.0f), mApp->ScaleNum(aA.mY, 0.0f), aA.mU[aLayer], aA.mV[aLayer]);
					aVerts[1] = TriVertex(mApp->ScaleNum(aB.mX, 0.0f), mApp->ScaleNum(aB.mY, 0.0f), aB.mU[aLayer], aB.mV[aLayer]);
					aVerts[2] = TriVertex(mApp->ScaleNum(aC.mX, 0.0f), mApp->ScaleNum(aC.mY, 0.0f), aC.mU[aLayer], aC.mV[aLayer]);
					aVerts[3] = TriVertex(mApp->ScaleNum(aD.mX, 0.0f), mApp->ScaleNum(aD.mY, 0.0f), aD.mU[aLayer], aD.mV[aLayer]);
					if (aPoint == 23)
					{
						// the last quad of a ring goes round to the first point: its u continues past 1
						aVerts[1].u = (float)((double)aVerts[1].u + 1.0);
						aVerts[3].u = (float)((double)aVerts[3].u + 1.0);
					}
					if (aVerts[2].v < aVerts[0].v)
					{
						// the v coordinates of this quad wrap round in the picture: the far side goes past 1
						aVerts[2].v = (float)((double)aVerts[2].v + 1.0);
						aVerts[3].v = (float)(1.0 + (double)aVerts[3].v);
					}
					SetTextureAddress(1, 0.0f <= aVerts[0].v ? 1 : 3);
					Image* aTexture = IMAGE_HYPERSPACE_INITIAL;
					if (mReady)
						aTexture = aLayer == 0 ? IMAGE_HYPERSPACE : IMAGE_WARP_LINES;
					// the original draws a triangle at a time with x moved by 1 (it passes the 1 that stands for "blend"
					// to the float that moves the vertices)
					TriVertex aFirst[1][3] = {{aVerts[0], aVerts[1], aVerts[2]}};
					mApp->mDDInterface->DrawTrianglesTex(aTexture, aFirst, 1, NULL, Color(aGrey, aGrey, aGrey, 255), 1, 1.0f, 0.0f, true);
					TriVertex aSecond[1][3] = {{aVerts[1], aVerts[2], aVerts[3]}};
					mApp->mDDInterface->DrawTrianglesTex(aTexture, aSecond, 1, NULL, Color(aGrey, aGrey, aGrey, 255), 1, 1.0f, 0.0f, true);
				}
			}
		}
	}
	SetTextureAddress(3, 3);
	if (0.0f < mFade)
	{
		int aFadeAlpha = FTol((double)mFade * 255.0);
		if (aFadeAlpha >= 255)
			aFadeAlpha = 255;
		g->SetColor(Color(255, 255, 255, aFadeAlpha));
		g->FillRect(0, 0, mWidth, mHeight);
	}
}

// 0x5c6beb
void Hyperspace::Draw(Graphics* g)
{
	if (!m3D)
		Draw2dPortal(g);
	else
		Draw3dPortal(g);
}
