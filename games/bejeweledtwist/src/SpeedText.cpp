#include "SpeedText.h"

#include "CurvedVal.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"

using namespace Sexy;

SpeedText::SpeedText(int theLevel)
{
	if (theLevel > 2)
		theLevel = 2;
	static const int kImages[6] = { IMAGE_SPEEDBONUS_ID, IMAGE_SPEEDBONUS_OUTLINE_ID, IMAGE_HYPERSPEED_ID,
		IMAGE_HYPERSPEED_OUTLINE_ID, IMAGE_GIGASPEED_ID, IMAGE_GIGASPEED_OUTLINE_ID };
	mGlow = GetImageById(kImages[theLevel * 2]);
	mText = GetImageById(kImages[theLevel * 2 + 1]);
	mGone = false;
	mClip = false;
	mX = gApp->mWidth;
	mY = (int)((double)gApp->mScale * 5.0);
	mState = 0;
	mMouseVisible = false;
}

SpeedText::~SpeedText()
{
}

void SpeedText::Update()
{
	TwistWidget::Update();
	if (mTrail.size() > 20)
		mTrail.erase(mTrail.begin());
	if (!mGone)
		mTrail.push_back(mX);
	if (mState == 0) {
		int aMiddle = gApp->mWidth / 2 - mGlow->mWidth / 2;
		mX -= 30;
		if (mX < aMiddle) {
			mX = aMiddle;
			mHold = 1.5f;
			mState = 1;
		}
	} else if (mState == 1) {
		mHold = (float)((double)mHold - 0.01);  // 0x81d368: a double
		if (mHold <= 0.0f)
			mState = 2;
	} else if (mState == 2) {
		int aWidth = mGlow->mWidth;
		// with Direct3D the x87 rounds the product to a float (gCurveSinglePrecision): at a scale of 0.64 it is -32, not
		// the -31.99999928 that truncates to -31
		double aStep = (double)gApp->mScale * -50.0;
		if (gCurveSinglePrecision)
			aStep = (float)aStep;
		mX += (int)aStep;
		if (mX < -aWidth) {
			mX = -aWidth;
			mGone = true;
			if (mTrail.size() == 0) {
				mState = 3;
				mClicked = true;
			} else {
				mTrail.erase(mTrail.begin());
			}
		}
	}
	MarkDirty();
}

void SpeedText::Draw(Graphics* g)
{
	g->SetColor(Color(255, 255, 255));
	int aY[2] = { mY, gApp->mHeight - mGlow->mHeight - mY };
	int i = 0;
	do {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(mGlow, mX, aY[i]);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(mText, mX, aY[i]);
		i++;
	} while (i < 2);
	// The trail: the oldest place brightest (alpha 191, then 9 less each), in the normal mode and without colorizing,
	// so the colour does not show
	unsigned int anIndex = 0;
	int anAlpha = 200;
	if (mTrail.size() != 0) {
		do {
			g->SetColor(Color(0, 255, 255, anAlpha));
			anAlpha -= 9;
			if (anAlpha < 1)
				break;
			if (mTrail[anIndex] != mX) {
				g->DrawImage(mGlow, mTrail[anIndex], mY);
				g->DrawImage(mGlow, mTrail[anIndex], gApp->mHeight - mGlow->mHeight - mY);
			}
			anIndex++;
		} while (anIndex < mTrail.size());
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
