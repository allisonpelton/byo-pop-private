// Sexy::PlayButton (PlayButton.h).
#include "PlayButton.h"

#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

PlayButton::PlayButton(int theId, Image* theImage, ButtonListener* theListener) :
	TwistButton(theId, theImage, theListener)
{
	mButtonId = theId;
	mImage = theImage;
	mButtonListener = theListener;
}

PlayButton::~PlayButton()
{
}

void PlayButton::Update()
{
	mUpdateCnt++;
	mRotation = (float)(mRotation - 0.0010000000474974513);
	// The main menu (app +0xf24) in its state 3 slows the sparkles to half: comes with the menu
	float aTarget = 1.0f;
	float aSpeed = aTarget;
	if (mSpinSpeed <= aTarget) {
		float aNext = (float)(mSpinSpeed + 0.009999999776482582);
		if (aNext <= aTarget)
			aSpeed = aNext;
	} else {
		float aNext = (float)(mSpinSpeed - 0.009999999776482582);
		if (aTarget <= aNext)
			aSpeed = aNext;
	}
	mSpinSpeed = aSpeed;

	if (mVisible) {
		mAppear.IncInVal();
		if (mAppear.GetOutVal() == 1.0 && mClickPending) {
			MouseUp(0, 0, 1);
			mClickPending = false;
			return;
		}
		float aGlow;
		if (mIsOver && mAppear.GetOutVal() == 1.0) {
			aGlow = 1.0f;
			float aNext = (float)(mGlow + 0.019999999552965164);
			if (aNext <= 1.0f)
				aGlow = aNext;
		} else {
			aGlow = 0;
			float aNext = (float)(mGlow - 0.019999999552965164);
			if (0 <= aNext)
				aGlow = aNext;
		}
		mGlow = aGlow;
	}

	if (mAppear.GetOutVal() == 1.0 && mUpdateCnt % 8 == 0) {
		Sparkle aSparkle;
		aSparkle.mAngle = (float)(RandSigned() * 3.1415927410125732);
		float aRand = std::fabs(RandSigned());
		aSparkle.mRadius = (float)((aRand * 0.10000000149011612 + 0.30000001192092896) * (float)mWidth);
		// 0.0005 instead of 0.00125 with Direct3D
		aRand = std::fabs((float)(RandSigned() * 0.0024999999441206455));
		float aSpeed = (float)(aRand + 0.0012499999720603228);
		aSparkle.mSpeed = -(aSpeed * (float)mWidth);
		aSparkle.mAlpha = 0;
		mSparkles.push_back(aSparkle);
	}

	unsigned int i = 0;
	if (mSparkles.size() != 0) {
		do {
			Sparkle& aSparkle = mSparkles[i];
			aSparkle.mSpeed = (float)(aSparkle.mSpeed - (float)mWidth * 9.999999974752427e-07);
			float anAlpha = (float)(aSparkle.mAlpha + 0.029999999329447746);
			if (1.0f < anAlpha)
				anAlpha = 1.0f;
			aSparkle.mAlpha = anAlpha;
			aSparkle.mRadius = aSparkle.mSpeed * mSpinSpeed + aSparkle.mRadius;
			aSparkle.mAngle = (float)(aSparkle.mAngle - aSparkle.mSpeed * 3.1415927410125732 * 0.004999999888241291 * mSpinSpeed);
			if (aSparkle.mRadius <= 0.0f) {
				mSparkles.erase(mSparkles.begin() + i);
				i--;
			}
			i++;
		} while (i < mSparkles.size());
	}
}

void PlayButton::DrawRings(Graphics* g, int theX, int theY, float theOuterRot, float theInnerRot, float theScale)
{
	g->SetColor(Color(255, 255, 255, Ftol(mAlpha.GetOutVal() * 255.0)));
	// The rings turn only with Direct3D
	theInnerRot = 0;
	theOuterRot = 0;
	float aX = (float)theX;
	float aY = (float)theY;
	DrawImageCentered(g, IMAGE_MAINMENU_INNERSPIRAL, aX, aY, theInnerRot, theScale, theScale);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	DrawImageCentered(g, IMAGE_MAINMENU_INNERRINGINNERGLOW, aX, aY, theInnerRot, theScale, theScale);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	DrawImageCentered(g, IMAGE_MAINMENU_INNERRING, aX, aY, theInnerRot, theScale, theScale);
	DrawImageCentered(g, IMAGE_MAINMENU_OUTERRING, aX, aY, theOuterRot, theScale, theScale);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	DrawImageCentered(g, IMAGE_MAINMENU_OUTERRINGINNERGLOW, aX, aY, theOuterRot, theScale, theScale);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void PlayButton::Draw(Graphics* g)
{
	if (mAlpha.GetOutVal() == 0.0)
		return;
	g->SetColorizeImages(true);
	if (mAppear.GetOutVal() <= 0.10000000149011612)
		return;
	{
		// DrawRings sets its own colour at once
		double aBlue = mAppear.GetOutVal();
		double aGreen = mAppear.GetOutVal();
		double aRed = mAppear.GetOutVal();
		int aB = Ftol(255.0 / aBlue);
		int aG = Ftol(255.0 / aGreen);
		int aR = Ftol(255.0 / aRed);
		g->SetColor(Color(aR, aG, aB));
	}
	DrawRings(g, mWidth / 2, mHeight / 2, mRotation, mRotation, (float)mAppear.GetOutVal());

	// The spiral: cel 0 at mAlpha, then its highlight (cel 2) at mGlow
	Image* aSpiral = IMAGE_MAINMENU_SPIRALBUTTON;
	float aSize = mAppear.GetOutVal() <= 1.0 ? (float)mAppear.GetOutVal() : 1.0f;
	float anAlpha = (float)(mAlpha.GetOutVal() * aSize);
	int aCel = 0;
	do {
		g->SetColor(Color(255, 255, 255, Ftol(anAlpha * 128.0)));
		Rect aSrcRect = aSpiral->GetCelRect(aCel);
		mAppear.GetOutVal();
		mAppear.GetOutVal();
		float aScaleY = (float)mAppear.GetOutVal();
		float aScaleX = (float)mAppear.GetOutVal();
		DrawImageCentered(g, aSpiral, aSrcRect, (float)(mWidth / 2), (float)(mHeight / 2), mRotation * 0.30000001192092896, aScaleX,
			aScaleY);
		aCel += 2;
		anAlpha = (float)(mAppear.GetOutVal() * mGlow);
	} while (aCel <= 2);

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	double anAppear1 = mAppear.GetOutVal();
	double anAppear2 = mAppear.GetOutVal();
	float aSparkleAlpha = (float)(anAppear2 * anAppear1 * mAlpha.GetOutVal());
	unsigned int i = 0;
	if (mSparkles.size() != 0) {
		do {
			Sparkle& aSparkle = mSparkles[i];
			// half the size without Direct3D
			float aScale = (float)((aSparkle.mRadius * 1.2000000476837158) / ((float)mWidth * 0.4000000059604645));
			aScale = aScale * 0.5f;
			double anAlpha2 = (double)aSparkle.mAlpha * aSparkleAlpha;
			if (1 < anAlpha2)
				anAlpha2 = 1;
			g->SetColor(Color(255, 255, 255, Ftol(anAlpha2 * 255.0)));
			float aSin = std::sin(aSparkle.mAngle);
			double aYAppear = mAppear.GetOutVal();
			float aCos = std::cos(aSparkle.mAngle);
			double aXAppear = mAppear.GetOutVal();
			float aY = aSin * aSparkle.mRadius;
			float aX = (float)((double)(mWidth / 2) + (double)aCos * aSparkle.mRadius * aXAppear);
			DrawImageScaledCentered(g, IMAGE_SPARKLET_BIG, aX, (float)(mHeight / 2) + aY * (float)aYAppear, aScale,
				aScale);
			i++;
		} while (i < mSparkles.size());
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// The button's image: cel 0, then its highlight (cel 2) at mGlow
	if (mImageAppear.GetOutVal() > 0.0) {
		double anAppear = mAppear.GetOutVal();
		double anImageAlpha = mAlpha.GetOutVal() * anAppear;
		aCel = 0;
		do {
			g->SetColor(Color(255, 255, 255, Ftol((float)anImageAlpha * 255.0)));
			Rect aSrcRect = mImage->GetCelRect(aCel);
			mAppear.GetOutVal();
			mAppear.GetOutVal();
			Transform aTransform;
			double anImageY = mImageAppear.GetOutVal();
			double anAppearY = mAppear.GetOutVal();
			double anImageX = mImageAppear.GetOutVal();
			float aScaleY = (float)(anAppearY * anImageY);
			float aScaleX = (float)(mAppear.GetOutVal() * anImageX);
			aTransform.Scale(aScaleX, aScaleY);
			g->DrawImageTransform(mImage, aTransform, aSrcRect, (float)(mWidth / 2), (float)(mHeight / 2));
			double anAppear2b = mAppear.GetOutVal();
			anImageAlpha = mGlow * anAppear2b * mAlpha.GetOutVal();
			aCel += 2;
		} while (aCel <= 2);
	}
}

void PlayButton::MouseUp(int x, int y, int theClickCount)
{
	double anAppear = mAppear.GetOutVal();
	float aShown = (float)(mImageAppear.GetOutVal() * (double)(float)anAppear);
	if (1.0f <= aShown)
		Widget::MouseUp(x, y, theClickCount);
	else if (0.6000000238418579 < aShown)
		mClickPending = true;
}

bool PlayButton::IsPointVisible(int x, int y)
{
	double anAppear = mAppear.GetOutVal();
	int aHalfWidth = mWidth / 2;
	float aDX = (float)(x - aHalfWidth);
	float aDY = (float)(y - mHeight / 2);
	float aDist = std::sqrt(aDX * aDX + aDY * aDY);
	return aDist <= (float)(anAppear * aHalfWidth);
}
