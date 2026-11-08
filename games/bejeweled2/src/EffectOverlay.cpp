// EffectOverlay.cpp: Sexy::EffectOverlay (EffectOverlay.h), the widget over the board.
//   DoStrechyText 0x5b524c, DoPortal 0x5b53b0, InterpolatePointF 0x5b592c, InterpolatePoint 0x5b5960, Draw 0x5b59b2,
//   DoUpdate 0x5b7cda, Update 0x5b871f, constructor 0x5b8b7e, destructor 0x5b8cee, InitFace 0x5b9071.
// The original is x87 code: every value it stores in a float is rounded there and nowhere else, and the constants it multiplies
// by are the floats that its source named (0.04f is 0.03999999910593033 in the code), so the sums here are made in double and
// rounded to float where the original stores (the casts) and the constants are float literals. The std::vector accessors of
// the original (the size, the range-checked at) are the containers' own.

#include "EffectOverlay.h"
#include "Board.h"
#include "WinDMApp.h"
#include "PuzzleStateOverlay.h"
#include "Profile.h"
#include "Res.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Buffer.h"
#include "Font.h"
#include "Common.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

using namespace Sexy;

namespace
{

// the C runtime's cosf (0x404940) and sinf (0x404960), as the framework's other ports make them
inline float Cosf(float theAngle)
{
	return (float)cos((double)theAngle);
}

inline float Sinf(float theAngle)
{
	return (float)sin((double)theAngle);
}

// 0x5b5229: powf as the original calls it, through the double pow
inline float Powf(float theBase, float theExponent)
{
	return (float)pow((double)theBase, (double)theExponent);
}

// A file of edata as the original reads it (fopen "r", fgets, feof of the C runtime's text mode): the pair CR LF reads as a
// line feed, a Ctrl-Z ends the file, and feof turns true only once a read has met the end, so that a file whose last line has
// its line feed gets one more (empty) read.
class EdataText
{
public:
	EdataText(const Buffer& theBuffer) :
		mData(theBuffer.GetDataPtr()),
		mSize(theBuffer.GetDataLen()),
		mPos(0),
		mEof(false)
	{
	}

	bool AtEof() const
	{
		return mEof;
	}

	// fgets: false (and the line untouched) when nothing could be read
	bool GetLine(char* theLine, int theSize)
	{
		int aCount = 0;
		while (aCount < theSize - 1)
		{
			int aChar = ReadChar();
			if (aChar < 0)
				break;
			theLine[aCount++] = (char)aChar;
			if (aChar == '\n')
				break;
		}
		if (aCount == 0)
			return false;
		theLine[aCount] = '\0';
		return true;
	}

private:
	int ReadChar()
	{
		if (mPos >= mSize)
		{
			mEof = true;
			return -1;
		}
		int aChar = mData[mPos++];
		if (aChar == 0x1a)
		{
			mPos = mSize;
			mEof = true;
			return -1;
		}
		if (aChar == '\r' && mPos < mSize && mData[mPos] == '\n')
		{
			mPos++;
			return '\n';
		}
		return aChar;
	}

private:
	const uchar*	mData;
	int				mSize;
	int				mPos;
	bool			mEof;
};

// The tiles of the 2D portal (0x62ad78: 16 rows of 16 ints): 0 nothing, 1 the ring's picture over what is there, 2 white,
// 3 the backdrop and the ring over it, 4 only the backdrop
const int kPortalTiles[256] =
{
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 1, 1, 1, 2, 2, 2, 1, 2, 1, 1, 1, 1, 0, 0,
	0, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 1, 1, 0,
	1, 1, 1, 2, 3, 3, 3, 4, 4, 3, 3, 3, 1, 1, 1, 0,
	1, 1, 1, 2, 3, 3, 4, 4, 4, 4, 4, 3, 1, 1, 1, 0,
	1, 1, 2, 2, 3, 4, 4, 4, 4, 4, 4, 3, 3, 1, 1, 1,
	1, 1, 1, 2, 3, 4, 4, 4, 4, 4, 4, 3, 3, 1, 1, 1,
	1, 1, 1, 2, 3, 4, 4, 4, 4, 4, 4, 3, 2, 1, 1, 1,
	1, 1, 1, 1, 3, 3, 4, 4, 4, 4, 3, 3, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1,
	0, 0, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};

}

// 0x5b8b7e
EffectOverlay::EffectOverlay(WinDMApp* theApp)
{
	mApp = theApp;

	mTextImage = new MemoryImage(mApp);
	mTextImage->SetImageMode(true, true);
	int aHeight = mApp->ScaleNumUp(0x6e, 0);
	int aWidth = mApp->ScaleNumUp(0x320, 0);
	mTextImage->Create(aWidth, aHeight);

	mPortalProgress = 0;
	mFlash = 0;
	mTextWidth = 0;
	mMouseVisible = false;
	mLineProgress = 0;
	mFlagC8 = false;
	mFlagC9 = false;
	mTextOn = 0;
	mFaceState = 0;
	mMorphTimer = 0;
	mLastHitSound = 0;
	mHasAlpha = true;

	// The original leaves the rest of the members as the heap had them. Nothing reads them before DoStrechyText, DoPortal or
	// InitFace has set them (Board reads mEndTimer and mFade only once the end screen runs), so the port zeroes them.
	mTextAlpha = 0;
	m9c = 0;
	mTextScale = 0;
	mTextScaleVel = 0;
	mTextStretch = 0;
	mTextStretchVel = 0;
	mTextAge = 0;
	mB4 = 0;
	mTextHold = 0;
	mTextFadeIn = 0;
	mTextSettleA = 0;
	mTextSettleB = 0;
	mPortalDelay = 0;
	mEndTimer = 0;
	mFaceDelay = 0;
	mWildX = 0;
	mWildY = 0;
	mWildT = 0;
	mFaceAlpha = 0;
	mFade = 0;
	mFadeHold = 0;
	mBarStep = 0;
}

// 0x5b8cee: the original never frees the text's picture; the port does
EffectOverlay::~EffectOverlay()
{
	delete mTextImage;
}

// 0x5b524c
void EffectOverlay::DoStrechyText(const std::string& theText, int theDelay)
{
	m9c = 0.73f;                    // 0x3f3ae148
	mTextOn = 1;
	mTextAlpha = 0.0f;
	mTextHold = 30;
	mTextStretch = 1.0f;
	mTextStretchVel = 0.0f;
	mTextAge = 0.0f;

	int aDelay;
	if (!mApp->Is3DAccelerated())
		aDelay = FTol((double)theDelay * (double)0.8f);
	else
		aDelay = theDelay;

	mB4 = 0.0f;
	mTextFadeIn = aDelay + 0x5a;
	mTextScale = 1.0f;
	mTextSettleA = aDelay + 0x3c;
	mTextScaleVel = 0.025f;         // 0x3ccccccd
	mTextSettleB = 0x3c;

	mTextImage->Clear();
	Graphics g(mTextImage);
	g.SetColor(Color(0xff, 0xff, 0xff));
	g.SetFont(FONT_HUGE);
	g.DrawString(theText, 8, mApp->ScaleNum(0x5e, 0));

	int aWidth = 0x320;
	if (FONT_HUGE->StringWidth(theText) + 0x10 < 0x320)
		aWidth = FONT_HUGE->StringWidth(theText) + 0x10;
	mTextWidth = aWidth;
}

// 0x5b53b0
void EffectOverlay::DoPortal()
{
	mPortalProgress = 0;
	mFlagC9 = true;
	mPortalDelay = 0x14;
	if (!mApp->Is3DAccelerated())
		mApp->KillMainMenu();
}

// 0x5b592c: the point between two points of the shape sets, a and b, t of the way
FPoint EffectOverlay::InterpolatePointF(const Point& theA, const Point& theB, float theT)
{
	double aOneMinusT = 1.0 - (double)theT;
	FPoint aResult;
	aResult.mX = (double)theB.mX * (double)theT + (double)theA.mX * aOneMinusT;
	aResult.mY = (double)theB.mY * (double)theT + (double)theA.mY * aOneMinusT;
	return aResult;
}

// 0x5b5960: the same for the shape that is drawn, rounded to whole pixels (the 0.05 keeps a sum that falls just short of a pixel
// from truncating to the pixel below)
Point EffectOverlay::InterpolatePoint(const FPoint& theA, const FPoint& theB, float theT)
{
	double aOneMinusT = 1.0 - (double)theT;
	Point aResult;
	aResult.mX = FTol((double)theT * theB.mX + theA.mX * aOneMinusT + 0.05);
	aResult.mY = FTol(aOneMinusT * theA.mY + (double)theT * theB.mY + 0.05);
	return aResult;
}

// 0x5b871f: the hyper mode updates twice
void EffectOverlay::Update()
{
	DoUpdate();
	if (mApp->mBoard != NULL && mApp->mBoard->mIsHyper)
		DoUpdate();
}

// 0x5b7cda
void EffectOverlay::DoUpdate()
{
	// the steps of the morph of the end screen (the original builds the table on the stack at every call)
	static const FaceStep kSteps[15] =
	{
		{  50, 3, 2,   0 },
		{  50, 0, 2,   0 },
		{  70, 0, 2,   0 },
		{  13, 0, 2, 100 },
		{  12, 0, 2,   0 },
		{  10, 0, 1, 100 },
		{  15, 0, 1,   0 },
		{  10, 0, 1,  45 },
		{   5, 0, 1,  65 },
		{   8, 0, 1,  20 },
		{  20, 0, 1,  25 },
		{  15, 0, 1,   0 },
		{  70, 0, 1,   0 },
		{ 100, 0, 3, 100 },
		{  -1, 0, 3,   0 }
	};

	if (!mApp->mHasFocus)
		return;

	Widget::Update();

	// the big text
	if (mTextOn != 0 && !(mApp->mBoard != NULL && mApp->mBoard->mPauseCount > 0))
	{
		if (mTextAge < 1.0f)
			mTextAge = (float)((double)mTextAge + (double)0.04f);

		// the stretch is a damped spring that is pulled towards 0
		double aDamping = (double)0.975f - (double)mTextAge * (double)0.12f;
		double aPull = (double)mTextAge * (double)0.01f + (double)0.002f;
		float aForce = (float)(-(double)mTextStretch * aPull + (double)mTextStretchVel);
		float aValue = (float)(aDamping * (double)aForce);
		mTextStretchVel = aValue;
		aValue = (float)((double)aValue + (double)mTextStretch);
		mTextStretch = aValue;
		if (fabsf(aValue) < 0.001f)
		{
			if (fabsf(mTextStretchVel) < 0.0001f)
			{
				mTextStretch = 0;
				mTextStretchVel = 0;
			}
		}

		// the size is a spring that aims at 1.2 first, then at 1.75, and then at 0
		if (mTextHold > 0)
		{
			mTextHold--;
		}
		else
		{
			float aScale = mTextScale;
			mTextSettleA--;
			int aSettleA = mTextSettleA;
			mTextSettleB--;
			float aDiff;
			double aStrength;
			if (mTextSettleB > 0)
			{
				aDiff = (float)((double)1.2f - (double)aScale);
				aStrength = (double)0.0015f;
			}
			else if (aSettleA > 0)
			{
				aDiff = (float)(1.75 - (double)aScale);
				aStrength = (double)0.0015f;
			}
			else
			{
				aDiff = (float)(0.0 - (double)aScale);
				aStrength = (double)0.002f;
			}
			mTextScaleVel = (float)((double)aDiff * aStrength + (double)mTextScaleVel);
			mTextScale = (float)((double)mTextScaleVel + (double)mTextScale);
		}

		if (mTextFadeIn > 0)
		{
			float aAlpha = (float)((double)mTextAlpha + (double)0.02f);
			mTextFadeIn--;
			mTextAlpha = aAlpha;
			if (1.0f < aAlpha)
				mTextAlpha = 1.0f;
		}
		else
		{
			mTextAlpha = (float)((double)mTextAlpha - (double)0.03f);
		}

		if (mTextAlpha < 0.0f || mTextScale <= 0.0f)
			mTextOn = 0;
		MarkDirty();
	}

	// the portal: it opens when the delay is over, and the board is shown when it has opened
	if (mFlagC9)
	{
		if (mPortalDelay > 0)
		{
			mPortalDelay--;
		}
		else
		{
			Board* aBoard = mApp->mBoard;
			if (aBoard->mUpdateCnt == 0)
				aBoard->Update();
			float aProgress = (float)((double)mPortalProgress + (double)0.01f);
			mPortalProgress = aProgress;
			if (1.0f < aProgress)
			{
				mFlagC9 = false;
				mApp->DoBoardShow();
			}
		}
		MarkDirty();
	}

	// the end screen
	if (mFaceDelay > 0)
		mFaceDelay--;
	if (mFaceState != 0)
	{
		mEndTimer++;
		MarkDirty();
	}

	if (mFaceState == 1 && mFaceDelay == 0)
	{
		if (mUpdateCnt % 4 == 0)
			mBarStep++;
		if (mBarStep == 0x38)
		{
			mFaceState = 2;
			mFaceDelay = 0;
		}
	}

	int aFaceState = mFaceState;
	if (aFaceState == 2)
	{
		// the gems fall (faster and faster) to their places
		if (mFaceDelay == 0)
		{
			int i = 0;
			if ((int)mGems.size() > 0)
			{
				do
				{
					FaceGem& aGem = mGems.at(i);
					if (aGem.mY < aGem.mTargetY)
					{
						float aSpeed = (float)((double)aGem.mVY + (double)0.02f);
						aGem.mVY = aSpeed;
						aSpeed = (float)((double)aGem.mY + (double)aSpeed);
						aGem.mY = aSpeed;
						if (aGem.mTargetY <= aSpeed)
						{
							aGem.mY = aGem.mTargetY;
							if (mUpdateCnt - mLastHitSound >= 10)
							{
								mApp->PlaySample(SOUND_GEM_HIT);
								mLastHitSound = mUpdateCnt;
							}
						}
					}
					i++;
				} while (i < (int)mGems.size());
			}
			if (mEndTimer == 0x2f8)
			{
				mFaceState = 3;
				mFaceDelay = 0;
			}
		}
	}
	else if (aFaceState == 3)
	{
		// the lines are drawn
		if (mFaceDelay == 0)
		{
			float aProgress = (float)((double)mLineProgress + (double)0.015f);
			mLineProgress = aProgress;
			if (1.0f < aProgress)
			{
				mLineProgress = 1.0f;
				mFaceState = 4;
				mFaceDelay = 10;
			}
		}
	}
	else if (aFaceState == 4)
	{
		if (mFaceDelay == 0)
		{
			mApp->PlaySample(SOUND_HYPERGEM_CREATE);
			mFlash = 1.0f;
			mGems.clear();
			mFaceState = 5;
			mFaceDelay = 0x32;
		}
	}
	else if (aFaceState == 5)
	{
		if (mFaceDelay == 0)
		{
			mFaceDelay = 200;
			mFaceState = 6;
		}
	}
	else if (aFaceState == 6)
	{
		if (mFaceDelay == 0)
		{
			mMorphTimer++;
			if (mMorphTimer == 0xb4)
				mApp->PlaySample(SOUND_EXCELLENT);

			// the step of the table that the timer is in, and how far in it is
			int aRemaining = mMorphTimer;
			int aStep = 0;
			if (aRemaining > 0)
			{
				const FaceStep* aEntry = kSteps;
				do
				{
					int aDuration = aEntry->mDuration;
					if (aDuration == -1 || aRemaining < aDuration)
						break;
					aRemaining -= aDuration;
					aStep++;
					aEntry++;
				} while (aRemaining > 0);
			}

			int aDuration = kSteps[aStep].mDuration;
			if (aDuration != -1)
			{
				if (aRemaining == 0)
					mCurShape = mShape;
				aRemaining++;
				float aTime = (float)((double)aRemaining / (double)aDuration);

				int i = 0;
				if ((int)mShape.size() > 0)
				{
					float aMix = (float)((double)kSteps[aStep].mPercent / 100.0);
					const std::vector<Point>* aTo = &mSets[kSteps[aStep].mTo];
					const std::vector<Point>* aFrom = &mSets[kSteps[aStep].mFrom];
					do
					{
						FPoint aMixed = InterpolatePointF(aFrom->at(i), aTo->at(i), aMix);
						FPoint aStart((double)mCurShape.at(i).mX, (double)mCurShape.at(i).mY);
						mShape.at(i) = InterpolatePoint(aStart, aMixed, aTime);
						i++;
					} while (i < (int)mShape.size());
				}
			}
		}

		if (mFadeHold == 0)
		{
			float aAlpha = (float)((double)mFaceAlpha - (double)0.01f);
			mFaceAlpha = aAlpha;
			if (aAlpha <= 0.0f)
			{
				mFaceAlpha = 0;
				mFaceState = 8;
				mFaceDelay = 0x1e;
			}
		}
		else
		{
			mFadeHold--;
		}
	}

	if (mFaceState == 7)
	{
		// (nothing in this class sets state 7)
		if (mFadeHold == 0)
		{
			float aAlpha = (float)((double)mFaceAlpha - (double)0.01f);
			mFaceAlpha = aAlpha;
			if (aAlpha <= 0.0f)
			{
				mFaceAlpha = 0;
				mFaceState = 8;
				mFaceDelay = 100;
			}
		}
	}
	else if (mFaceState == 8 && mFaceDelay == 0)
	{
		// the gem flies from the top of the middle to the bottom of it, round a circle that closes as it goes
		float aTime = (float)((double)mWildT * (double)0.004f + (double)0.003f + (double)mWildT);
		mWildT = aTime;
		FPoint aTop(513.0, 192.0);
		FPoint aBottom(513.0, 400.0);
		Point aMiddle = InterpolatePoint(aTop, aBottom, aTime);

		float aAngle = (float)((double)aTime * 12.0);
		float aTurn = Powf(aAngle, 1.2f);
		float aRing = (float)(((double)0.5 - (double)mWildT) * 2.0);
		aRing = fabsf(aRing);
		aRing = (float)(1.0 - (double)aRing);
		float aRadius = (float)((double)Powf(aRing, 0.6f) * 100.0);
		mWildX = (float)((double)Cosf(aTurn) * (double)aRadius + (double)aMiddle.mX);
		mWildY = (float)((double)Sinf(aTurn) * (double)aRadius + (double)aMiddle.mY);
		if (1.0f <= mWildT)
			mFaceState = 9;
	}

	if (mFaceState != 5)
	{
		if (0.0f < mFlash)
		{
			float aFlash = (float)((double)mFlash - (double)0.01f);
			mFlash = aFlash;
			if (aFlash < 0.0f)
				mFlash = 0;
		}
	}
}

// 0x5b59b2
void EffectOverlay::Draw(Graphics* g)
{
	bool a3D = mApp->Is3DAccelerated();
	int aCenterX = mApp->ScaleNum(0x28e, 0);
	int aCenterY = mApp->ScaleNum(0xf5, 0);

	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL)
	{
		if (aBoard->mScreenState == 1 && aBoard->mPauseCount == aBoard->mInterfacePauseCount && aBoard->mInterfacePauseCount > 0)
			return;
		if (aBoard->m15f24)
			aBoard->DrawBoard(g, true, false);

		// the arrow over the gem that a tip is about
		aBoard = mApp->mBoard;
		if (aBoard->mHintPiece != NULL)
		{
			float aCos = Cosf(aBoard->m1954);
			float aPhase = (float)(1.0 - (double)aCos);
			float aBob = (float)((double)aPhase * 0.5);
			int aGemY = FTol((double)aBoard->mHintPiece->mY);
			int aOffset = FTol((double)aBob * -10.0);
			int aTop = aGemY - aOffset - 0x31;
			int aArrowY = mApp->ScaleNum(aTop, 0);
			int aArrowX = FTol(mApp->ScaleNum((float)((double)mApp->mBoard->mHintPiece->mX - 8.0), 0.0f));
			g->DrawImage(IMAGE_HELP_ARROW, aArrowX, aArrowY);
			g->SetColorizeImages(true);
			g->SetDrawMode(1);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aBob * 255.0)));
			int aSizeH = mApp->ScaleNumUp(0x28, 0);
			int aSizeW = mApp->ScaleNumUp(0x28, 0);
			Rect aSrc(0, 0, aSizeW, aSizeH);
			int aIndicatorY = mApp->ScaleNum(aTop + 0x14, 0);
			int aIndicatorX = FTol(mApp->ScaleNum((float)((double)mApp->mBoard->mHintPiece->mX + 21.0), 0.0f));
			g->DrawImage(IMAGE_HELP_INDICATOR_ARROW, aIndicatorX, aIndicatorY, aSrc);
			g->SetDrawMode(0);
			g->SetColorizeImages(false);
		}

		// the arrow over the pod of the puzzle that is next
		aBoard = mApp->mBoard;
		if (aBoard->mScreenState == 1 && aBoard->mPuzzleOverlay != NULL && aBoard->m1850 && aBoard->mState == 0x11 && !aBoard->m191d)
		{
			Point aPos = aBoard->mPuzzleOverlay->mPods[mApp->mProfile->GetPuzzleNum() % 5].GetPos();
			double aCos = cos(mApp->mBoard->m1858);
			float aPhase = (float)(1.0 - aCos);
			float aBob = (float)((double)aPhase * 0.5);
			int aTop = aPos.mY - FTol((double)aBob * -6.0) - 0x20;
			int aArrowY = mApp->ScaleNum(aTop, 0);
			int aArrowX = mApp->ScaleNum(aPos.mX + 7, 0);
			g->DrawImage(IMAGE_HELP_ARROW, aArrowX, aArrowY);
			g->SetColorizeImages(true);
			g->SetDrawMode(1);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aBob * 255.0)));
			int aSizeH = mApp->ScaleNumUp(0x28, 0);
			int aSizeW = mApp->ScaleNumUp(0x28, 0);
			Rect aSrc(0, 0, aSizeW, aSizeH);
			int aIndicatorY = mApp->ScaleNum(aTop + 0x14, 0);
			int aIndicatorX = mApp->ScaleNum(aPos.mX + 0x25, 0);
			g->DrawImage(IMAGE_HELP_INDICATOR_ARROW, aIndicatorX, aIndicatorY, aSrc);
			g->SetDrawMode(0);
			g->SetColorizeImages(false);
		}
	}

	// the big text
	if (mTextOn != 0 && mApp->mBoard != NULL && mApp->mBoard->mScreenState == 1)
	{
		float aScale = (float)((double)mTextScale * (double)m9c);
		float aStretch = mTextStretch;
		if (!a3D)
		{
			aScale = (float)((double)aScale * (double)1.2f);
			if (1.0f < aScale)
				aScale = 1.0f;
			aStretch = (float)((double)aStretch - (double)0.1f);
			if (aStretch < 0.0f)
				aStretch = 0.0f;
		}

		float aWidth = (float)mTextWidth;
		aWidth = (float)((((double)aWidth * (double)aStretch) * 4.0 + (double)aWidth) * (double)aScale);
		aScale = (float)((110.0 - ((double)aStretch * 110.0) * (double)1.8f) * (double)aScale);
		if (aScale < 6.0f)
			aScale = 6.0f;

		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)mTextAlpha * 255.0)));
		if (a3D)
			g->SetFastStretch(false);
		int aSrcHeight = mApp->ScaleNumUp(0x6e, 0);
		Rect aSrc(0, 0, mTextWidth, aSrcHeight);
		int aHeight = FTol(mApp->ScaleNum(aScale, 0.0f));
		int aHalfHeight = FTol(mApp->ScaleNum((float)((double)aScale * 0.5), 0.0f));
		int aY = aCenterY - aHalfHeight;
		int aX = aCenterX - FTol((double)aWidth * 0.5);
		Rect aDest(aX, aY, FTol((double)aWidth), aHeight);
		g->DrawImage(mTextImage, aDest, aSrc);
		g->SetFastStretch(true);
		g->SetColorizeImages(false);
	}

	// the portal of the warp
	if (mFlagC9 && 0.0f < mPortalProgress)
	{
		// (the original waits here for the task that loads the backdrop; the port loads in line)
		double aProgress = (double)mPortalProgress;
		if (a3D)
		{
			// the backdrop of the board, zoomed out as the portal opens, and the ring of fire round it
			float aSizeF = (float)((aProgress * aProgress * 4.0 + (double)0.01f) * 1024.0);
			int aSize = FTol(mApp->ScaleNum(aSizeF, 0.0f));
			int aMidX = mApp->ScaleNum(0x1ff, 0);
			int aMidY = mApp->ScaleNum(0x156, 0);
			Image* aBackdrop = mApp->mBoard->mBackdropImage;
			int aZoomSize = FTol((double)aSize / (double)2.72f);
			int aHalfZoom = aZoomSize / 2;
			Rect aWindow(aMidX - aHalfZoom, aMidY - aHalfZoom, aZoomSize, aZoomSize);
			Rect aBounds(0, 0, mWidth, mHeight);
			Rect aClip = aWindow.Intersection(aBounds);

			float aInverse = 1.0f;
			if (mPortalProgress > 0.0001f)
			{
				double aDivisor = (double)mPortalProgress + 0.25;
				if (1.0 > aDivisor)
					aInverse = (float)(1.0 / aDivisor);
				else
					aInverse = 1.0f;
			}

			int aHalfWidth = mWidth / 2;
			int aHalfHeightOfThis = mHeight / 2;
			Rect aSource;
			aSource.mX = FTol((double)(aClip.mX - aHalfWidth) * (double)aInverse) + aHalfWidth;
			aSource.mY = FTol((double)(aClip.mY - aHalfHeightOfThis) * (double)aInverse) + aHalfHeightOfThis;
			aSource.mWidth = FTol((double)aClip.mWidth * (double)aInverse);
			aSource.mHeight = FTol((double)aClip.mHeight * (double)aInverse);
			if (aClip.mX < 0)
			{
				int aNegative = -aClip.mX;
				aClip.mX = 0;
				aClip.mWidth = mWidth;
				aSource.mX += FTol((double)aNegative * (double)aInverse);
			}
			if (aClip.mY < 0)
			{
				int aNegative = -aClip.mY;
				aClip.mY = 0;
				aClip.mHeight = mHeight;
				aSource.mY += FTol((double)aInverse * (double)aNegative);
			}
			if (aBackdrop != NULL)
				g->DrawImage(aBackdrop, aClip, aSource);

			int aRingHalf = aSize / 2;
			Rect aRing(aMidX - aRingHalf, aMidY - aRingHalf, aSize, aSize);
			g->SetDrawMode(1);
			g->SetFastStretch(false);
			if (mPortalProgress < 1.0f)
			{
				int aRingHeight = mApp->ScaleNumUp(200, 0);
				int aRingWidth = mApp->ScaleNumUp(200, 0);
				int aRingFrame = (mUpdateCnt / 2) % 10;
				Rect aRingSrc(mApp->ScaleNum(aRingFrame * 200, 0), 0, aRingWidth, aRingHeight);
				g->DrawImage(IMAGE_FIRE_RING, aRing, aRingSrc);
			}
			g->SetFastStretch(true);
			g->SetDrawMode(0);
		}
		else
		{
			// a grid of 16 by 16 tiles over the backdrop: each tile shows a piece of the ring's picture (three frames) or of the
			// backdrop, or is white; the grid grows as the portal opens
			int aFrame = (mUpdateCnt / 3) % 3;
			int aSize = FTol((aProgress * aProgress * 4.0 + (double)0.01f) * 1024.0);
			g->SetFastStretch(true);
			int aHalfSize = aSize / 2;
			int aBaseX = mApp->ScaleNum(0x1ff, 0) - aHalfSize;
			int aBaseY = mApp->ScaleNum(0x156, 0) - aHalfSize;
			Image* aBackdrop = mApp->mBoard->mBackdropImage;

			int aDestY = 0;
			int aSourceY = 0;
			const int* aTile = kPortalTiles;
			do
			{
				int aNextDestY = aSize + aDestY;
				int aRowY = aDestY / 16 + aBaseY;
				int aSourceRowY = aSourceY / 16;
				int aRowHeight = aNextDestY / 16 - aDestY / 16;
				int aNextSourceY = aSourceY + 0x100;
				int aSourceRowHeight = aNextSourceY / 16 - aSourceRowY;
				int aDestX = 0;
				int aSourceX = 0;
				const int* aRowTile = aTile;
				do
				{
					int aValue = *aRowTile;
					int aSourceColumnX = aSourceX / 16;
					int aNextDestX = aDestX + aSize;
					int aColumnX = aDestX / 16 + aBaseX;
					Rect aDest(aColumnX, aRowY, aNextDestX / 16 - aColumnX + aBaseX, aRowHeight);
					int aNextSourceX = aSourceX + 0x100;
					Rect aSource(aSourceColumnX, aSourceRowY, aNextSourceX / 16 - aSourceColumnX, aSourceRowHeight);

					if (aValue == 2)
					{
						g->SetColor(Color(0xff, 0xff, 0xff));
						g->FillRect(aDest);
					}
					else
					{
						Rect aBounds(0, 0, mWidth, mHeight);
						Rect aClip = aDest.Intersection(aBounds);
						if (aBackdrop != NULL && (aValue == 3 || aValue == 4))
							g->DrawImage(aBackdrop, aClip.mX, aClip.mY, aClip);
						if (aValue == 1 || aValue == 3)
						{
							g->SetDrawMode(1);
							aSource.mX += aFrame << 8;
							g->DrawImage(IMAGE_RING_DUDE, aDest, aSource);
							g->SetDrawMode(0);
						}
					}

					aRowTile++;
					aSourceX = aNextSourceX;
					aDestX = aNextDestX;
				} while (aSourceX < 0x1000);
				aSourceY = aNextSourceY;
				aDestY = aNextDestY;
				aTile = aRowTile;
			} while (aTile < kPortalTiles + 256);
		}
	}

	// the end screen: the polygons, the lines, the gem that flies, the gems that fall
	Point aPoints[20];
	if (mFaceState >= 6)
	{
		int i = 0;
		if ((int)mPolys.size() > 0)
		{
			do
			{
				const FacePoly& aPoly = mPolys.at(i);
				Color aColor = aPoly.mColor;
				aColor.mAlpha = FTol((double)aColor.mAlpha * (double)mFaceAlpha);
				g->SetColor(aColor);
				int aCount = (int)aPoly.mIndices.size();
				if (aCount > 20)
					aCount = 20;        // the original's array holds 20 points; the polygons of the file have at most 6
				int j = 0;
				if (aCount > 0)
				{
					do
					{
						aPoints[j].mX = mApp->ScaleNum(mShape.at(aPoly.mIndices.at(j)).mX, 0);
						aPoints[j].mY = mApp->ScaleNum(mShape.at(aPoly.mIndices.at(j)).mY, 0);
						j++;
					} while (j < aCount);
				}
				g->PolyFill(aPoints, aCount, false);
				i++;
			} while (i < (int)mPolys.size());
		}
	}

	g->SetDrawMode(1);
	int aLineNum = 0;
	if ((int)mLines.size() > 0)
	{
		do
		{
			g->SetColor(Color(0x80, 0x80, 0x80, FTol((double)mFaceAlpha * 255.0)));
			const Point& aLine = mLines.at(aLineNum);
			int aX1 = mApp->ScaleNum(mShape.at(aLine.mX).mX, 0);
			int aY1 = mApp->ScaleNum(mShape.at(aLine.mX).mY, 0);
			int aX2 = mApp->ScaleNum(mShape.at(aLine.mY).mX, 0);
			int aY2 = mApp->ScaleNum(mShape.at(aLine.mY).mY, 0);
			if (0.0f < mLineProgress)
			{
				int aEndY = FTol((double)(aY2 - aY1) * (double)mLineProgress + (double)aY1);
				int aEndX = FTol((double)(aX2 - aX1) * (double)mLineProgress + (double)aX1);
				g->DrawLine(aX1, aY1, aEndX, aEndY);
			}
			aLineNum++;
		} while (aLineNum < (int)mLines.size());
	}
	g->SetDrawMode(0);

	int aWildFrame = (mUpdateCnt / 6) % 0x28;
	if (mFaceState < 6 || mFaceState >= 8)
	{
		if (mFaceState == 8)
		{
			// the gem that flies shrinks to nothing
			int aSize = FTol(mApp->ScaleNum((float)((1.0 - (double)mWildT) * 84.0), 0.0f)) & ~1;
			int aWildWidth = mApp->ScaleNum(0x54, 0);
			int aWildHeight = mApp->ScaleNum(0x54, 0);
			Rect aSrc(mApp->ScaleNum(aWildFrame * 0x54, 0), 0, aWildHeight, aWildWidth);
			int aHalf = aSize / 2;
			int aY = mApp->ScaleNum(FTol((double)mWildY), 0) - aHalf;
			int aX = mApp->ScaleNum(FTol((double)mWildX), 0) - aHalf;
			Rect aDest(aX, aY, aSize, aSize);
			g->DrawImage(IMAGE_WILDCARD_DESTROYER, aDest, aSrc);
		}
	}
	else
	{
		int aWildWidth = mApp->ScaleNum(0x54, 0);
		int aWildHeight = mApp->ScaleNum(0x54, 0);
		Rect aSrc(mApp->ScaleNum(aWildFrame * 0x54, 0), 0, aWildHeight, aWildWidth);
		int aY = mApp->ScaleNum(FTol((double)mWildY) - 0x2a, 0);
		int aX = mApp->ScaleNum(FTol((double)mWildX) - 0x2a, 0);
		g->DrawImage(IMAGE_WILDCARD_DESTROYER, aX, aY, aSrc);
	}

	int aGemNum = 0;
	if ((int)mGems.size() > 0)
	{
		do
		{
			const FaceGem& aGem = mGems.at(aGemNum);
			int aGemHeight = mApp->ScaleNumUp(0x12, 0);
			int aGemWidth = mApp->ScaleNumUp(0x12, 0);
			Rect aSrc(0, mApp->ScaleNum(aGem.mType * 0x12, 0), aGemWidth, aGemHeight);
			int aY = mApp->ScaleNum(FTol((double)aGem.mY) - 9, 0);
			int aX = mApp->ScaleNum(FTol((double)aGem.mX) - 9, 0);
			g->DrawImage(IMAGE_SMALL_GEMS, aX, aY, aSrc);
			aGemNum++;
		} while (aGemNum < (int)mGems.size());
	}

	if (mFlagC8)
	{
		mApp->ClearUpdateBacklog(false);
		mFlagC8 = false;
	}

	if (0.0f < mFlash)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)mFlash * 255.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}

	if (mFaceState > 0)
	{
		if (mApp->mLoaded)
		{
			// a sparkle, and the credits that scroll up
			int aSparkle = mEndTimer - 2000;
			if (aSparkle >= 0)
			{
				aSparkle /= 5;
				if (aSparkle < 14)
				{
					int aSparkleSize = mApp->ScaleNumUp(0x28, 0);
					g->SetDrawMode(1);
					Rect aSrc(aSparkleSize * aSparkle, 0, aSparkleSize, aSparkleSize);
					int aY = mApp->ScaleNum(0x180, 0);
					int aX = mApp->ScaleNum(499, 0);
					g->DrawImage(IMAGE_SPARKLE, aX, aY, aSrc);
					g->SetDrawMode(0);
				}
			}

			int aCreditsY = 0x672 - mEndTimer / 2;
			if (aCreditsY < 0x300)
			{
				g->SetFont(FONT_CONTINUUM_MED_18);
				g->SetColor(Color(0, 0, 0, FTol((double)mFade * 255.0)));
				int aLineIndex = 0;
				if ((int)mCredits.size() > 0)
				{
					do
					{
						int aTextWidth = g->StringWidth(mCredits.at(aLineIndex));
						int aLineHeight = mApp->ScaleNum(0x18, 0);
						int aY = mApp->ScaleNum(aCreditsY, 0) + aLineHeight * aLineIndex;
						g->DrawString(mCredits.at(aLineIndex), mWidth / 2 - aTextWidth / 2, aY);
						aLineIndex++;
					} while (aLineIndex < (int)mCredits.size());
				}
			}
		}

		// the black bars at the top and the bottom
		int aBar = mApp->ScaleNumUp(FTol((double)mBarStep * (double)mFade), 0);
		g->SetColor(Color(0, 0, 0));
		g->FillRect(0, 0, mWidth, aBar);
		g->FillRect(0, mHeight - aBar, mWidth, aBar);
	}
}

// 0x5b9071: reads the files of the end screen and starts it. A file that is missing ends the function there (the original's
// fopen returns NULL): the state stays as far as it got.
void EffectOverlay::InitFace()
{
	mGems.clear();
	mLines.clear();
	mPolys.clear();

	// the gems (edata\gem_points.txt) and the four shapes (points.txt, points2.txt to points4.txt): "x y" per line
	int aSet = -1;
	do
	{
		std::string aFileName;
		if (aSet == -1)
		{
			aFileName = "edata\\gem_points.txt";
		}
		else
		{
			mSets[aSet].clear();
			if (aSet == 0)
				aFileName = "edata\\points.txt";
			else
				aFileName = StrFormat("edata\\points%d.txt", aSet + 1);
		}

		Buffer aBuffer;
		if (!mApp->ReadBufferFromFile(aFileName, &aBuffer, true))
			return;
		EdataText aFile(aBuffer);
		while (!aFile.AtEof())
		{
			char aLine[0x100];
			aLine[0] = '\0';
			aFile.GetLine(aLine, 0x100);
			if (aLine[0] != '\0' && !isspace((unsigned char)aLine[0]))
			{
				char* aSpace = strchr(aLine, ' ');
				if (aSpace != NULL)     // the original writes through the result unchecked
				{
					*aSpace = '\0';
					int aX = atol(aLine);
					int aY = atol(aSpace + 1);
					if (aSet == -1)
					{
						// a gem falls from a random height above the screen, with a random colour and speed
						int aSpan = FTol((double)(0x300 - aY) * 1.25);
						int aTypeDraw = Rand();
						FaceGem aGem;
						aGem.mX = (float)aX;
						double aStartY = (double)(aY - 0x300) * 1.25;
						aGem.mType = aTypeDraw % 7;
						int aHeightDraw = Rand() % 1000;
						int aHeight = FTol((double)aHeightDraw / 1000.0 * (double)aSpan);
						aGem.mY = (float)(aStartY - (double)aHeight);
						int aSpeedDraw = Rand() % 1000;
						aGem.mVY = (float)((double)aSpeedDraw / 1000.0 * -1.25);
						aGem.mTargetY = (float)aY;
						mGems.push_back(aGem);
					}
					else
					{
						mSets[aSet].push_back(Point(aX, aY));
					}
				}
			}
		}
		aSet++;
	} while (aSet < 4);

	// the lines (edata\lines.txt): "a b", two points of the shape
	{
		Buffer aBuffer;
		if (!mApp->ReadBufferFromFile("edata\\lines.txt", &aBuffer, true))
			return;
		EdataText aFile(aBuffer);
		while (!aFile.AtEof())
		{
			char aLine[0x100];
			aLine[0] = '\0';
			aFile.GetLine(aLine, 0x100);
			if (aLine[0] != '\0' && aLine[0] != ' ')
			{
				char* aSpace = strchr(aLine, ' ');
				if (aSpace != NULL)
				{
					*aSpace = '\0';
					int aA = atol(aLine);
					int aB = atol(aSpace + 1);
					mLines.push_back(Point(aA, aB));
				}
			}
		}
	}

	// the polygons (edata\polys.txt): "i,j,k,... c", the points of the shape and the colour (white or black with five alphas)
	Color aColors[10] =
	{
		Color(0xff, 0xff, 0xff, 0x0d), Color(0xff, 0xff, 0xff, 0x1a), Color(0xff, 0xff, 0xff, 0x40),
		Color(0xff, 0xff, 0xff, 0x5a), Color(0xff, 0xff, 0xff, 0x80),
		Color(0, 0, 0, 0x0d), Color(0, 0, 0, 0x1a), Color(0, 0, 0, 0x40), Color(0, 0, 0, 0x5a), Color(0, 0, 0, 0x80)
	};
	{
		Buffer aBuffer;
		if (!mApp->ReadBufferFromFile("edata\\polys.txt", &aBuffer, true))
			return;
		EdataText aFile(aBuffer);
		while (!aFile.AtEof())
		{
			char aLine[0x100];
			aLine[0] = '\0';
			aFile.GetLine(aLine, 0x100);
			if (aLine[0] != '\0' && aLine[0] != ' ')
			{
				FacePoly aPoly;
				char* aSpace = strchr(aLine, ' ');
				if (aSpace != NULL)
				{
					*aSpace = '\0';
					int aColorIndex = atol(aSpace + 1);
					if (aColorIndex >= 0 && aColorIndex < 10)   // the original indexes the array unchecked
					{
						aPoly.mColor = aColors[aColorIndex];
						char* aToken = aLine;
						while (aToken != NULL)
						{
							char* aComma = strchr(aToken, ',');
							if (aComma != NULL)
								*aComma = '\0';
							aPoly.mIndices.push_back(atol(aToken));
							aToken = (aComma != NULL) ? aComma + 1 : NULL;
						}
						mPolys.push_back(aPoly);
					}
				}
			}
		}
	}

	// the credits (edata\credits.txt): the lines as they are, with their line feeds
	mCredits.clear();
	{
		Buffer aBuffer;
		if (!mApp->ReadBufferFromFile("edata\\credits.txt", &aBuffer, true))
			return;
		EdataText aFile(aBuffer);
		while (!aFile.AtEof())
		{
			char aLine[0x100];
			aLine[0] = '\0';
			aFile.GetLine(aLine, 0x100);
			mCredits.push_back(std::string(aLine));
		}
	}

	mShape = mSets[3];
	mCurShape = mShape;
	mLineProgress = 0;
	mFaceState = 1;
	mFlash = 0;
	mFaceDelay = 0x32;
	mWildX = 513.0f;
	mMorphTimer = 0;
	mFadeHold = 0x226;
	mBarStep = 0;
	mWildY = 192.0f;
	mEndTimer = 0;
	mWildT = 0;
	mFaceAlpha = 1.0f;
	mFade = 1.0f;
}
