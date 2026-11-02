// Sexy::FlyingText (FlyingText.h).
#include "FlyingText.h"

#include "TwistD3D.h"

#include "GameOverShow.h"
#include "Graphics.h"
#include "Image.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

FlyingText::FlyingText(int theKind, Image* theImage, int theX, int theY, float theUnused, int theSoundId,
	float theDelay)
{
	(void)theUnused;
	mKind = theKind;
	mSoundId = theSoundId;
	mTextImage = theImage;
	mClicked = false;  // 0x6fc3a9 sets the rest of the defaults
	mClip = false;
	if (theDelay <= 0.0f) {
		theDelay = 1.0f;
		mState = 1;
	} else {
		mState = 0;
	}
	mDelay = theDelay;
	if (theX < 0)
		theX = gApp->mWidth / 2;
	mX = theX;
	if (theY < 0)
		theY = gApp->mHeight / 2;
	mY = theY;
}

FlyingText::FlyingText(Font* theFont, const SexyString& theText, int theX, int theY)
{
	// (0x6fc3a9 sets the rest of the defaults)
	mKind = 0;
	mSoundId = 0;
	mNoticeClicks = true;
	mClicked = false;
	mClip = false;
	mCycling = false;
	mQueued = false;
	mScale = 1.0f;
	mWidth = gApp->mWidth;
	mHeight = gApp->mHeight;
	if (theX == -1000)
		theX = mWidth / 2;
	mX = theX;
	if (theY == -1000)
		theY = mHeight / 2;
	mY = theY;
	mState = 1;
	mImageOverride = new TextImage(theFont, theText, 0, 0, 0, 0, nullptr);  // 0x74e701
}

FlyingText::~FlyingText()
{
	delete mImageOverride;
}

void FlyingText::Update()
{
	if (mQueued)
		return;
	if (mState == 0) {
		float aDelay = (float)((double)mDelay - 0.01);
		mDelay = aDelay;
		if (0.0f < aDelay)
			return;
		mState = 1;
		mDelay = 1.0f;
	}
	if (mSoundId != 0) {
		gApp->PlayVoice(mSoundId, false);
		mSaid = true;
		mSoundId = 0;
	}
	float aFadeStep = -0.022f;
	if (mKind == 0) {
		if (mCycling)
			mCycle.Update();
		mFading = mFadeScale <= mScale;
		float aScale = (float)((double)(mState == 1 ? mGrowFast : mGrowSlow) + (double)mScale);
		mScale = aScale;
		if (mFadeScale <= aScale)
			aFadeStep = mFadeStep;
		else
			aFadeStep = 0.0f;
		if (mSlowScale <= aScale)
			mState = 2;
	} else if (mKind == 1) {
		// Never reached: both makers (0x6fd22c from 0x6ba73a, 0x6fd358 from 0x6ba0c2) are given kind 0
		NotPorted("FlyingText::Update: the letters");
	}
	if (mFading) {
		float anAlpha = (float)((double)mTextAlpha + (double)aFadeStep);
		mTextAlpha = anAlpha;
		if (0.0f >= anAlpha)
			mClicked = true;
	}
}

void FlyingText::Draw(Graphics* g)
{
	if (mQueued || mState == 0 || mClicked)
		return;
	Image* anImage = mImageOverride;
	if (anImage == nullptr)
		anImage = mTextImage;
	if (gApp->Is3DAccelerated())
		gD3DInterface->SetTextureWrap(0, false);  // slot 0xcc: texture 0 does not wrap
	if (mKind == 0) {
		if (anImage != nullptr && 0.0f < mTextAlpha) {
			// (the original asks for the image's size here and drops it)
			g->SetColorizeImages(true);
			Color aColor = mCycling ? mCycle.mColor : Color(255, 255, 255);
			aColor.mAlpha = Ftol((double)mTextAlpha * 255.0);
			g->SetColor(aColor);
			Transform aTransform;
			aTransform.Scale(mScale, mScale);
			g->DrawImageTransformF(anImage, aTransform, (float)mX, (float)mY);
		}
	} else if (mKind == 1) {
		// Never reached (no FlyingText of kind 1 is made)
		NotPorted("FlyingText::Draw: the letters");
	}
}

void FlyingText::MouseDown(int x, int y, int theClickCount)
{
	TwistWidget::MouseDown(x, y, theClickCount);
	mFading = true;
}
