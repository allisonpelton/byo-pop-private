// The small objects the Board keeps lists of: floating texts, hint boxes, the wipe that ends a game.

#include "SmallObjects.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Particle.h"
#include <SDL.h>
#include <math.h>

using namespace Sexy;

// 0x4107e0: a text at (theX, theY) in a font. The text moves up the screen unless it is given a height to move to
// (mTargetY) and lives 250 updates; theFlag false puts it 100 pixels lower. A text that would end past the right edge of
// the 640 pixel screen is centred instead.
FloatText::FloatText(const std::string& theText, Font* theFont, float theX, float theY, bool theFlag)
{
	mOriginX = -1.0f;
	mTargetY = -1.0f;
	mSpeed = -1.0f;
	mX = theX;
	mAlphaDelay = 0;
	mNoFade = false;
	mAlpha = 0xff;
	mHue = 0;
	mLife = 0xfa;
	mHoldStart = 0;
	mHold = 500000000;
	mY = theY;
	if (!theFlag)
		mY = theY + 100.0f;
	mText = theText;
	mFont = theFont;
	mDone = false;
	mAge = 0;
	mWidth = theFont->StringWidth(theText);
	if (640.0f < (float) mWidth + mX)
		mX = (float) (0x140 - mWidth / 2);
}

// 0x4108e0: a text that rises from the bottom of the screen (y 480) to theTargetY at theSpeed, waits there theHold updates and
// goes on up (the clear bonus).
FloatText::FloatText(const std::string& theText, Font* theFont, float theX, float theTargetY, int theHold, float theSpeed)
{
	mAlphaDelay = 0;
	mAlpha = 0xff;
	mHue = 0;
	mLife = 0xfa;
	mX = theX;
	mY = 480.0f;
	mText = theText;
	mOriginX = theX;
	mTargetY = theTargetY;
	mFont = theFont;
	mNoFade = false;
	mDone = false;
	mAge = 0;
	mHold = theHold;
	mHoldStart = 0;
	mSpeed = theSpeed;
	mWidth = theFont->StringWidth(mText);
}

// 0x40b600
LevelUpText::LevelUpText(Image* theImage, int theY)
{
	mImage = theImage;
	mAlphaRiseTime = theY + 0xbe;
	mBaseScale = 0.73f;
	mActive = 1;
	mAlpha = 0.0f;
	mStretch = 1.0f;
	mStretchSpeed = 0.0f;
	mSpringRamp = 0.0f;
	mStartDelay = 0x1e;
	mSmallHoldTime = 0;
	mBigHoldTime = 100;
	mScale = 1.0f;
	mScaleSpeed = 0.025f;
}

// 0x40b6a0: one update of the picture that announces a level (or a minute): its position and size ease towards the middle
// of the screen, hold there and ease away; it is over when it has faded out or shrunk to nothing.
void LevelUpText::Update()
{
	if (mActive == 0)
		return;

	// (x87 at 24 bits, every result a float: the expressions below are written in float, each operation rounding as the original's does)
	if (mSpringRamp < 1.0f)
		mSpringRamp = mSpringRamp + 0.04f;

	float aPull = 0.975f - mSpringRamp * 0.12f;
	float aSpring = mSpringRamp * 0.01f + 0.002f;
	float aVel = aPull * ((-mStretch) * aSpring + mStretchSpeed);
	mStretchSpeed = aVel;
	float aPos = aVel + mStretch;
	mStretch = aPos;
	if (fabs((double) aPos) < 0.0010000000474974513 && fabs((double) mStretchSpeed) < 9.999999747378752e-05)
	{
		mStretch = 0.0f;
		mStretchSpeed = 0.0f;
	}

	if (mStartDelay > 0)
		mStartDelay--;
	else
	{
		mBigHoldTime--;
		mSmallHoldTime--;
		float aForce;
		if (mSmallHoldTime > 0)
			aForce = (1.2f - mScale) * 0.0015f;
		else if (mBigHoldTime > 0)
			aForce = (1.75f - mScale) * 0.0015f;
		else
			aForce = (-mScale) * 0.002f;
		mScaleSpeed = aForce + mScaleSpeed;
		mScale = mScale + mScaleSpeed;
	}

	if (mAlphaRiseTime > 0)
	{
		float aFade = mAlpha + 0.02f;
		mAlphaRiseTime--;
		mAlpha = aFade;
		if (aFade > 1.0f)
			mAlpha = 1.0f;
	}
	else
		mAlpha = mAlpha - 0.03f;

	if (mAlpha < 0.0f || mScale <= 0.0f)
		mActive = 0;
}

// 0x408890: one update of a floating text. It rises one pixel an update (-1 in the three fields: nothing else was
// asked for), or moves to its height at its speed, waits there for its hold time and then moves on up; it is done when
// it is above the screen. Unless it keeps its colour it cycles its colour phase and ends when its life has run out.
void FloatText::Update()
{
	mAge++;
	if (mDone)
		return;

	if (mSpeed == -1.0f && mOriginX == -1.0f && mTargetY == -1.0f)
		mY = mY - 1.0f;
	else if (mY > mTargetY)
	{
		float aNew = mY - mSpeed;
		mY = aNew;
		if (aNew <= mTargetY)
		{
			mY = mTargetY;
			mHoldStart = mAge;
		}
	}
	else if (mY == mTargetY)
	{
		if (mAge - mHoldStart >= mHold)
			mY = mY - mSpeed;
	}
	else
		mY = mY - mSpeed;

	if ((float) -mFont->mHeight > mY)
		mDone = true;

	if (!mNoFade)
	{
		mHue = (mHue + 7) % 0xff;
		mLife--;
		if (mLife < 1)
		{
			mLife = 0;
			mDone = true;
		}
	}
	if (mHold == 500000000)
	{
		mAlphaDelay--;
		if (mAlphaDelay < 1)
		{
			mAlpha -= 2;
			if (mAlpha < 0)
				mAlpha = 0;
		}
	}
}

// 0x456660
HintBox::HintBox()
{
	mFont = NULL;
	mFillColor = Color(0x28, 0x41, 0x2d);
	mTextColor = Color(0xd7, 0xff, 0x0a);
	mShowTime = 100;
	mHideTime = 200;
	mShown = true;
	mTimerRunning = true;
	mHidden = true;
	mDrawAllowed = true;
	mInView = false;
	mSlidIn = false;
	mCellRow = 0;
	mCellColumn = 0;
	mY = 0;
	mX = 0;
	mPhaseTimer = 0;
	mHeight = 0;
	mWidth = 0;
}

// 0x435a40: the box counts; once it is shown it waits mShowTime updates, slides in, stays mHideTime updates and goes
void HintBox::Update()
{
	if (mTimerRunning)
		mPhaseTimer++;

	if (mShown && !mSlidIn)
	{
		if (!mInView)
		{
			if (mShowTime <= mPhaseTimer)
			{
				mInView = true;
				mSlidIn = true;
				mPhaseTimer = 0;
				return;
			}
			return;
		}
	}
	else if (!mInView)
		return;

	if (mSlidIn && mHideTime <= mPhaseTimer)
	{
		mInView = false;
		mSlidIn = false;
		mTimerRunning = false;
		mPhaseTimer = 0;
	}
}

// 0x43c5d0: the panel (a frame of one colour round a fill of another) and its lines of text
void HintBox::Draw(Graphics* g)
{
	if (mInView && mDrawAllowed && !mHidden)
	{
		g->SetFont(mFont);
		g->SetColor(Color(0x69, 0xaa, 0x3c));
		g->FillRect(Rect(mX, mY, mWidth, mHeight));
		g->SetColor(mFillColor);
		g->FillRect(Rect(mX + 1, mY + 1, mWidth - 2, mHeight - 2));
		g->SetColor(mTextColor);
		for (unsigned int i = 0; i < mLines.size(); i++)
			g->DrawString(mLines[i], mX + 3, mFont->GetAscent() * (i + 1) + i * 2 + mY);
	}
}

// 0x4359f0: the box is asked to show or hide itself
void HintBox::SetShown(bool theShow)
{
	if (!theShow)
	{
		if (mShown)
		{
			mPhaseTimer = 0;
			mTimerRunning = false;
			mShown = false;
			mDrawAllowed = false;
			mHidden = true;
			mInView = false;
		}
	}
	else if (!mShown)
	{
		mTimerRunning = true;
		mShown = true;
		mSlidIn = false;
		mDrawAllowed = true;
		mHidden = false;
		mInView = false;
	}
}

// 0x43d990
void HintBox::SetFont(Font* theFont)
{
	mFont = theFont;
	mWidth = ComputeWidth();
	mHeight = ComputeHeight();
}

// 0x43c700: the widest line and 8
int HintBox::ComputeWidth()
{
	if (mFont == NULL)
		return 0;
	int aWidest = 0;
	for (unsigned int i = 0; i < mLines.size(); i++)
	{
		int aWidth = mFont->StringWidth(mLines[i]);
		if (aWidth > aWidest)
			aWidest = aWidth;
	}
	return aWidest + 8;
}

// 0x436490: the lines' heights (ascent each, and 2 between) and 4
int HintBox::ComputeHeight()
{
	if (mFont == NULL)
		return 0;
	int aLines = (int) mLines.size();
	return mFont->GetAscent() * aLines + 4 + aLines * 2;
}

// 0x460a70
void HintBox::AddLine(const std::string& theLine)
{
	mLines.push_back(theLine);
	mWidth = ComputeWidth();
	mHeight = ComputeHeight();
}

//////////////////////////////////////////////////////////////////////////
// ScreenWipe

// 0x426230
ScreenWipe::ScreenWipe()
{
	mImage = NULL;
	mKind = 0;
	mCounter = 0;
	mPos = 0;
	mRow = 0;
	mDone = false;
	mOwnsImage = false;
	mGoingDown = true;
}

// port: the wipe's destructor
ScreenWipe::~ScreenWipe()
{
	if (mOwnsImage)
		delete mImage;
}

// 0x40b280: the picture to wipe (a copy of it when theCopy), and the starting place of the kinds that sweep
void ScreenWipe::SetImage(Image* theImage, bool theCopy)
{
	mOwnsImage = theCopy;
	if (!theCopy)
		mImage = theImage;
	else
		mImage = new MemoryImage(*static_cast<MemoryImage*>(theImage));

	if (mKind == 1)
		mPos = mImage->mWidth - 2;
	else if (mKind == 2)
		mPos = 1;
	else if (mKind == 3)
		mRow = 0;
}

// 0x426250: kind 0 lists every pixel of the picture, row by row; the others only set where they start
void ScreenWipe::SetKind(int theKind)
{
	mKind = theKind;
	if (theKind == 0)
	{
		if (mImage != NULL)
		{
			for (int aRow = 0; aRow < mImage->mHeight; aRow++)
			{
				for (int aCol = 0; aCol < mImage->mWidth; aCol++)
				{
					Point aPoint;
					aPoint.mX = aCol;
					aPoint.mY = aRow;
					mPoints.push_back(aPoint);
				}
			}
		}
	}
	else if (theKind == 1)
	{
		if (mImage != NULL)
			mPos = mImage->mWidth - 2;
	}
	else if (theKind == 2)
	{
		if (mImage != NULL)
			mPos = 1;
	}
	else if (theKind == 3)
	{
		if (mImage != NULL)
			mRow = 0;
	}
}

// 0x4264a0: counts the update and runs the steps of the kind
bool ScreenWipe::Check(int theSteps)
{
	mCounter++;
	switch (mKind)
	{
	case 0:		return StepRandom(theSteps);
	case 1:		return StepToLeft(theSteps);
	case 2:		return StepToRight(theSteps);
	case 3:		return StepRows(theSteps);
	default:	return false;
	}
}

// 0x426370: clears theSteps pixels picked at random from those left (the pick is swapped with the last one and dropped)
bool ScreenWipe::StepRandom(int theSteps)
{
	if (mDone)
		return true;

	MemoryImage* anImage = static_cast<MemoryImage*>(mImage);
	ulong* aBits = anImage->GetBits();
	if (0 < theSteps)
	{
		int aStep = 0;
		do
		{
			unsigned int aNumPoints = (unsigned int) mPoints.size();
			if (aNumPoints == 0)
			{
				// (the original divides by zero here: a picture without pixels)
				mDone = true;
				break;
			}
			unsigned int anIndex = AstroRand() % aNumPoints;
			Point aPoint = mPoints[anIndex];
			mPoints[anIndex] = mPoints.back();
			mPoints.back() = aPoint;
			mPoints.pop_back();
			if (mPoints.empty())
				mDone = true;
			aBits[mImage->mWidth * aPoint.mY + aPoint.mX] = 0;
			aStep++;
		} while (!mDone && aStep < theSteps);
	}
	anImage->BitsChanged();
	anImage->CommitBits();
	return mDone;
}

// 0x40b340: the edge moves one column to the left each step; the twelve columns at it fade from clear to opaque
bool ScreenWipe::StepToLeft(int theSteps)
{
	if (mDone)
		return true;

	MemoryImage* anImage = static_cast<MemoryImage*>(mImage);
	ulong* aBits = anImage->GetBits();
	for (int aStep = 0; aStep < theSteps; aStep++)
	{
		if (mDone)
			break;
		ulong anAlpha = 0;
		int k = 0;
		do
		{
			int anIndex = (mPos - k) + 1;
			if (anIndex < 0)
				break;
			int aLimit = (((anImage->mHeight - 1) * anImage->mWidth - k) + 1) + mPos;
			for (; anIndex <= aLimit && anIndex > -1; anIndex += anImage->mWidth)
				aBits[anIndex] = (aBits[anIndex] & 0xffffff) + anAlpha;
			anAlpha += 0x15000000;
			if (anAlpha > 0xff000000UL)
				anAlpha = 0xff000000UL;
			k++;
		} while (k < 12);
		mPos--;
		if (mPos < -1)
			mDone = true;
	}
	anImage->BitsChanged();
	anImage->CommitBits();
	return mDone;
}

// 0x40b430: the same from the left to the right
bool ScreenWipe::StepToRight(int theSteps)
{
	if (mDone)
		return true;

	MemoryImage* anImage = static_cast<MemoryImage*>(mImage);
	ulong* aBits = anImage->GetBits();
	for (int aStep = 0; aStep < theSteps; aStep++)
	{
		if (mDone)
			break;
		ulong anAlpha = 0;
		int k = 0;
		do
		{
			int anIndex = (mPos - 1) + k;
			if (anIndex >= anImage->mWidth)
				break;
			int aLimit = (anImage->mHeight - 1) * anImage->mWidth + (mPos - 1) + k;
			for (; anIndex <= aLimit; anIndex += anImage->mWidth)
				aBits[anIndex] = (aBits[anIndex] & 0xffffff) + anAlpha;
			anAlpha += 0x15000000;
			if (anAlpha > 0xff000000UL)
				anAlpha = 0xff000000UL;
			k++;
		} while (k < 12);
		mPos++;
		if (anImage->mWidth < mPos)
			mDone = true;
	}
	anImage->BitsChanged();
	anImage->CommitBits();
	return mDone;
}

// 0x40b540: clears a row, then goes down two rows at a time and, at the bottom, up two at a time. The original goes on
// clearing within one call after it is done and writes rows outside the picture (-1 at the top; one past the bottom
// of a picture with an odd height); the port leaves those writes out.
bool ScreenWipe::StepRows(int theSteps)
{
	if (mDone)
		return true;

	MemoryImage* anImage = static_cast<MemoryImage*>(mImage);
	ulong* aBits = anImage->GetBits();
	for (; theSteps > 0; theSteps--)
	{
		if (mRow >= 0 && mRow < anImage->mHeight)
		{
			for (int aCol = 0; aCol < anImage->mWidth; aCol++)
				aBits[anImage->mWidth * mRow + aCol] = 0;
		}
		int aRow = mRow;
		if (!mGoingDown)
		{
			mRow = aRow - 2;
			if (aRow - 2 < 0)
				mDone = true;
		}
		else
		{
			mRow = aRow + 2;
			if (anImage->mHeight <= aRow + 2)
			{
				mRow = aRow + 1;
				mGoingDown = false;
			}
		}
	}
	anImage->BitsChanged();
	anImage->CommitBits();
	return mDone;
}

//////////////////////////////////////////////////////////////////////////
// Drawing

// 0x4089a0: the text in its font, in a colour that runs round the hues with the update (white if it does not change); the
// original also works out an alpha from the life left and then sets the colour with the alpha field (+0x50) instead
void FloatText::Draw(Graphics* g)
{
	if (mDone)
		return;

	g->SetFont(mFont);
	Color aColor(0xff, 0xff, 0xff);
	if (!mNoFade)
		aColor = Color((int) (gApp->HSLToRGB(mHue, 0xff, 0x80) & 0xffffff) | 0xff000000);
	aColor.mAlpha = mAlpha;
	g->SetColor(aColor);
	g->DrawString(mText, (int) mX, (int) ((float) mFont->GetAscent() + mY));
}

// 0x40dab0
void LevelUpText::Draw(Graphics* g)
{
	DrawAt(g, 0x182, 0xf0);
}

// 0x40c8a0: the picture stretched round the point (theX, theY): wider as it springs out and shorter, never below 6 pixels
void LevelUpText::DrawAt(Graphics* g, int theX, int theY)
{
	if (mActive == 0)
		return;

	bool is3D = gApp->Is3DAccelerated();
	// (x87 at 24 bits: every product and sum below is rounded to a float, in the order of the original's instructions)
	float aWidth = ((mStretch * 4.0f + 1.0f) * (float) mImage->mWidth) * mBaseScale * mScale;
	float aHeight = (((float) mImage->mHeight - ((float) mImage->mHeight * mStretch) * 1.8f) * mBaseScale) * mScale;
	if (aHeight < 6.0f)
		aHeight = 6.0f;

	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, (int) (mAlpha * 255.0f)));
	if (!is3D)
		g->SetFastStretch(true);
	Rect aSrc(0, 0, mImage->mWidth, mImage->mHeight);
	Rect aDest(theX - (int) (aWidth * 0.5f), theY - (int) (aHeight * 0.5f), (int) aWidth, (int) aHeight);
	g->DrawImage(mImage, aDest, aSrc);
	if (!is3D)
		g->SetFastStretch(false);
	g->SetColorizeImages(false);
}

// 0x40b5e0
void ScreenWipe::Draw(Graphics* g, int theX, int theY)
{
	g->DrawImage(mImage, theX, theY);
}
