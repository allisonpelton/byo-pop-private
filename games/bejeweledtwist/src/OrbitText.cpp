// Sexy::OrbitText (OrbitText.h).
#include "OrbitText.h"

#include "GameOverShow.h"
#include "Graphics.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

OrbitText::OrbitText(Font* theFont, const SexyString& theText, const Rect& theRect)
{
	mText = new TextImage(theFont, theText, 16, 16, 0, 0, nullptr);  // 0x74e701
	mCount = 1;
	mWidth = theRect.mWidth;
	mHeight = theRect.mHeight;
	mX = mWidth / 2 + theRect.mX;
	mSpeed = 0.01f;
	mY = mHeight / 2 + theRect.mY;
	mAlpha = 0;
	mDone = false;
}

OrbitText::~OrbitText()
{
	delete mText;
}

void OrbitText::Update()
{
	Widget::Update();
	mCycle.Update();
	if (mPos < 1.0f) {
		float aPos = (float)((double)mSpeed + (double)mPos);
		mPos = aPos;
		if (aPos < 1.0f) {
			double anAlpha = (double)mAlpha + 0.019999999552965164;
			if (1.0 < anAlpha)
				anAlpha = 1.0;
			mAlpha = (float)anAlpha;
		} else {
			mCount--;
			if (0 < mCount)
				mPos = -1.0f;
		}
	}
	if (mCount < 1) {
		double anAlpha = (double)mAlpha - 0.019999999552965164;
		if (anAlpha < 0.0)
			anAlpha = 0.0;
		mAlpha = (float)anAlpha;
		if (mAlpha == 0.0f) {
			delete mText;
			mText = nullptr;
			mDone = true;
		}
	}
}

void OrbitText::Draw(Graphics* g)
{
	if (mText == nullptr || !(mPos < 1.0f))
		return;
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
	// 2D (Is3DAccelerated() false): the text flat, left of mX by its own width, a hundred art pixels above the middle
	float aX = (float)mX;
	float aY = (float)(mY + mHeight / 2);
	float aLeft = (float)((double)gApp->mScale * -1.0);
	aX = (float)((double)aLeft * (double)mText->mWidth + (double)aX);
	aY = (float)((double)aY - (double)Ftol((double)gApp->mScale * 100.0));
	g->SetColorizeImages(true);
	Color aColor = mCycle.mColor;
	int anAlpha = Ftol((double)mAlpha * 255.0);
	aColor.mAlpha = anAlpha;
	// a black shadow three art pixels down and right, then the text in the cycle's colour
	g->SetColor(Color(0, 0, 0, anAlpha));
	float aShadow = (float)Ftol((double)gApp->mScale * 3.0);
	int aShadowY = Ftol((double)aShadow + (double)aY);
	g->DrawImage(mText, Ftol((double)aShadow + (double)aX), aShadowY);
	g->SetColor(aColor);
	int aTextY = Ftol((double)aY);
	g->DrawImage(mText, Ftol((double)aX), aTextY);
}
