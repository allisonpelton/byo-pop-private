// The floating texts and images (see FloatText.h).

#include "FloatText.h"
#include "Draw.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Platform.h"

namespace bonnies
{

FloatTexts* gFloatTexts = nullptr;

namespace
{
inline int clampByte(int theValue)
{
	if (theValue < 0)
		return 0;
	if (theValue > 0xff)
		return 0xff;
	return theValue;
}

inline int byteOf(uint32_t theColor, int theShift)
{
	return (int)(theColor >> theShift & 0xff);
}
}

// 0x42b080
FloatText::FloatText()
{
	mLastTime = host()->getTicks();
	mFadeTime = -1;
	mColor1 = 0xffffffff;
	mEndColor1 = 0xffffffff;
	mColor2 = 0xffffffff;
	mEndColor2 = 0xffffffff;
	mStartX = 0.5f;
	mStartY = 0.5f;
	mProgress = 0.0f;
	mSpeedX = 0.0f;
	mSpeedY = 0.0f;
	mGravity = 0.0f;
	mAccelX = 0.0f;
	mAccelY = 0.0f;
	mAngle = 0.0f;
	mSpin = 0.0f;
	mScale1 = 1.0f;
	mScale2 = 1.0f;
	mDuration = 2000;
	mFlags = 0;
	mFrame = nullptr;
	mText += "";
	mFont = nullptr;
	mStarted = 0;
	mX = 0.0f;
	mY = 0.0f;
	mNext = nullptr;
	mPrev = nullptr;
}

// 0x42b0f0
void FloatText::update()
{
	if (mStarted == 0)
	{
		mX = mStartX;
		mY = mStartY;
		mStarted = 1;
	}
	if (gPlatform->isPaused() == 0)
	{
		int anElapsed = (int)(host()->getTicks() - mLastTime);
		int aDuration = 1;
		if (mDuration >= 1)
			aDuration = mDuration;
		double aProgress = (double)anElapsed / aDuration + mProgress;
		mProgress = (float)aProgress;
		if (aProgress > 1.0)
			mProgress = 1.0f;
		double aSeconds = (double)anElapsed * (double)0.001f;
		double anAccelY = aSeconds * mGravity * 1.25 + mAccelY;
		mAccelY = (float)anAccelY;
		double aSpeedX = aSeconds * mAccelX + mSpeedX;
		mSpeedX = (float)aSpeedX;
		float aSpeedY = (float)(anAccelY * aSeconds + mSpeedY);
		mSpeedY = aSpeedY;
		mX = (float)(aSpeedX * aSeconds + mX);
		mY = (float)((double)aSpeedY * aSeconds + mY);
		mAngle = (float)(aSeconds * mSpin + mAngle);
	}
	mLastTime = host()->getTicks();
}

// 0x42b1c0
void FloatText::draw()
{
	if (!(mProgress < 1.0f))
		return;
	if (mFrame == nullptr && (mFont == nullptr || mText.empty()))
		return;
	if (mStarted == 0)
		update();
	double p = mProgress;
	double anInv = 1.0 - p;
	float aFade = 1.0f;
	if (mFadeTime >= 0)
	{
		int aTime = ftol((double)mDuration * mProgress) - mFadeTime;
		int aSpan = mDuration - mFadeTime < 1 ? 1 : mDuration - mFadeTime;
		aFade = (float)(1.0 - (double)aTime / aSpan);
		if (aFade < 0.0f)
			return;
	}
	// The first colour from mColor1 to mEndColor1, faded
	int a = ftol(byteOf(mColor1, 24) * anInv + byteOf(mEndColor1, 24) * p);
	if (aFade < 1.0f)
		a = ftol((double)a * aFade);
	int r = ftol(byteOf(mColor1, 16) * anInv + byteOf(mEndColor1, 16) * p);
	int g = ftol(byteOf(mColor1, 8) * anInv + byteOf(mEndColor1, 8) * p);
	int b = ftol(byteOf(mEndColor1, 0) * p + byteOf(mColor1, 0) * anInv);
	a = clampByte(a);
	r = clampByte(r);
	g = clampByte(g);
	b = clampByte(b);
	uint32_t aColor1 = (((uint32_t)a << 8 | r) << 8 | g) << 8 | b;
	// The second colour, its alpha also scaled by the first's
	int a2 = ftol((double)a * 0.00392156862745098 * byteOf(mColor2, 24) * anInv + byteOf(mEndColor2, 24) * p);
	if (aFade < 1.0f)
		a2 = ftol((double)a2 * aFade);
	int r2 = ftol(byteOf(mEndColor2, 16) * p + byteOf(mColor2, 16) * anInv);
	int g2 = ftol(byteOf(mEndColor2, 8) * p + byteOf(mColor2, 8) * anInv);
	int b2 = ftol(byteOf(mEndColor2, 0) * p + byteOf(mColor2, 0) * anInv);
	a2 = clampByte(a2);
	r2 = clampByte(r2);
	g2 = clampByte(g2);
	b2 = clampByte(b2);
	uint32_t aColor2 = (((uint32_t)a2 << 8 | r2) << 8 | g2) << 8 | b2;
	float aScale = (float)(anInv * mScale1 + (double)mScale2 * p);
	gGraphics->mField00 = frameCount();
	if (mFrame != nullptr)
	{
		if (mX < -0.2f || !(mX < 1.2f) || mY < -0.2f || !(mY <= 1.2f))
			return;
		drawFrameRotated(mFrame, mX, mY, mAngle, aScale, aFade, 0, 0);
		return;
	}
	mFont->drawText(mText, (float)((double)mX - 1.0), (float)((double)mY - 1.0), 2.0f, 2.0f, aColor1, aColor2,
		mFlags | 9, aScale, mAngle);
}

// 0x42af30
FloatTexts::FloatTexts()
{
	mHead = nullptr;
	mTail = nullptr;
}

// 0x42af40
void FloatTexts::draw()
{
	for (FloatText* aText = mHead; aText != nullptr; aText = aText->mNext)
		aText->draw();
}

// 0x42af60
void FloatTexts::add(FloatText* theText)
{
	theText->mPrev = mTail;
	theText->mNext = nullptr;
	if (mTail != nullptr)
		mTail->mNext = theText;
	mTail = theText;
	if (mHead == nullptr)
		mHead = theText;
}

// 0x42afa0
void FloatTexts::clear()
{
	while (mHead != nullptr)
	{
		FloatText* aText = mHead;
		if (aText->mPrev != nullptr)
			aText->mPrev->mNext = aText->mNext;
		if (aText->mNext != nullptr)
			aText->mNext->mPrev = aText->mPrev;
		if (aText == mHead)
			mHead = aText->mNext;
		if (aText == mTail)
			mTail = aText->mPrev;
		delete aText;
	}
}

// 0x42b000
void FloatTexts::update()
{
	FloatText* aText = mHead;
	while (aText != nullptr)
	{
		aText->update();
		FloatText* aNext = aText->mNext;
		if (aText->mProgress >= 1.0f)
		{
			if (aText->mPrev != nullptr)
				aText->mPrev->mNext = aNext;
			if (aText->mNext != nullptr)
				aText->mNext->mPrev = aText->mPrev;
			if (aText == mHead)
				mHead = aText->mNext;
			if (aText == mTail)
				mTail = aText->mPrev;
			delete aText;
		}
		aText = aNext;
	}
}

} // namespace bonnies
