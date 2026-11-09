// Sexy::Points and Sexy::PointsManager (Points.h).
//
// +0x68, the number of font layers whose last takes the points' own colour, is set only by 0x732a41, which nothing
// calls; the floaters the board makes leave it as the heap had it. The port starts it at 0, so every layer takes its
// colour cycle's colour (NOTES.md, "Deviations").
#include "Points.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>

using namespace Sexy;

namespace {

// 0x757126: the number of layers of an ImageFont (+0x14's +0x90)
int GetFontLayerCount(Font* theFont)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return 0;
	return (int)aFont->mFontData->mFontLayerList.size();
}

int ClampByte(int theValue)
{
	if (theValue < 0)
		return 0;
	if (255 < theValue)
		return 255;
	return theValue;
}

}  // namespace

Points::Points(WinTwistApp* theApp, Font* theFont, const SexyString& theText, int theX, int theY, float theLife,
	const Color& theColor, int theKind)
{
	mStopHigh = true;
	mDelay = 0;
	mUseLastLayerCycle = false;
	mLife = theLife;
	mPhase = 0;
	mColor = theColor;
	mApp = theApp;
	mFont = theFont;
	for (int i = 0; i < 5; i++)
		mLayerOffsets[i] = 0;
	if (theKind < 3) {
		for (int i = 0; i < 5; i++) {
			mCycles[i].mPos = std::fabs((float)RandSigned());
			mCycles[i].mColors.clear();
			mCycles[i].mColor = Color(0, 0, 0, 0);
		}
	}
	for (int i = 0; i < 5; i++)
		mCycles[i].mSpeed = 1.8f;
	// The colour half again as bright and half as bright, each channel at most 255
	Color aBright = mColor;
	aBright.mRed = Ftol(std::min((double)aBright.mRed * 1.5, 255.0));
	aBright.mGreen = Ftol(std::min((double)aBright.mGreen * 1.5, 255.0));
	aBright.mBlue = Ftol(std::min((double)aBright.mBlue * 1.5, 255.0));
	Color aDim = mColor;
	aDim.mRed = Ftol(std::min((double)aDim.mRed * 0.5, 255.0));
	aDim.mGreen = Ftol(std::min((double)aDim.mGreen * 0.5, 255.0));
	aDim.mBlue = Ftol(std::min((double)aDim.mBlue * 0.5, 255.0));
	if (theKind == 0) {
		mCycles[1].mColors.push_back(aDim);
		mCycles[1].mColors.push_back(aDim);
	} else if (theKind == 1) {
		mCycles[1].mColors.push_back(aDim);
		mCycles[1].mColors.push_back(aBright);
	} else if (theKind == 2) {
		mCycles[1].mColors.push_back(aDim);
		mCycles[1].mColors.push_back(aBright);
		mCycles[1].SetPosition(0.25f);
		mCycles[2].mColors.push_back(aDim);
		mCycles[2].mColors.push_back(aBright);
		mCycles[2].SetPosition(0.75f);
	} else {
		mCycles[1].SetPosition(0.25f);
		mCycles[2].SetPosition(0.5f);
	}
	mText = theText;
	Flash();
	// (the original asks the font for the text's width and the ascent here and drops them)
	mFontHeight = 0;
	mX = (float)theX;
	mY = (float)theY;
	mScale = 0;
	mScaleVel = 0;
	mSerial = -1;
	mUpdates = 0;
	mFading = 0;
	mDrawn = false;
	mDone = false;
	mVY = (float)((double)gApp->mScale * 1.2000000476837158);
	mAlpha = 1.0f;
}

void Points::Update()
{
	mUpdates++;
	int aWidth = mFont->StringWidth(mText);
	double aHalf = (double)(aWidth / 2) * 1.5;
	float aRight = (float)gApp->mWidth;
	if (!((double)aRight >= (double)mX + aHalf))
		mX = (float)((double)aRight - aHalf);
	if (0 < mDelay) {
		mDelay--;
		return;
	}
	float aCircle = 6.2831855f;
	float aPhase = (float)((double)aCircle * 0.029999999329447746 + (double)mPhase);
	mPhase = aPhase;
	if (!(aPhase <= aCircle))
		mPhase = aPhase - aCircle;
	float aFlash = (float)((double)mFlash - 0.004999999888241291);
	mFlash = aFlash;
	if (!(0.0f <= aFlash))
		mFlash = 0;
	for (int i = 0; i < 3; i++)
		mCycles[i].Update();
	if (mFading == 0) {
		float aGap = mTargetScale - mScale;
		float aVel = aGap * mSpring + mScaleVel;
		float aStep = aVel * mDamping;
		mScaleVel = aStep;
		float aScale = mScale + aStep;
		mScale = aScale;
		if (!(mTargetScale <= aScale))
			mScale = mTargetScale;
	} else {
		float anAlpha = (float)((double)mAlpha - 0.05000000074505806);
		mAlpha = anAlpha;
		float aScale = (float)((double)mScale - 0.029999999329447746);
		mScale = aScale;
		if (!(0.0f < aScale) || !(0.0f < anAlpha))
			mDone = true;
	}
	float aTop = (float)((double)gApp->mScale * 50.0);
	double aNear = (double)mY / (double)aTop;
	if (!(1.0 > aNear))
		aNear = 1.0;
	float aSlow = (float)std::pow((double)(float)aNear, (double)0.015f);  // 0x670a49
	float aVY = mVY * aSlow;
	mVY = aVY;
	float aY = mY - aVY;
	mY = aY;
	if (mStopHigh) {
		float aMin = (float)Ftol((double)gApp->mScale * 75.0);
		mY = aY >= aMin ? aY : aMin;
	}
	float aLife = (float)((double)mLife - 0.009999999776482582);
	mLife = aLife;
	if (!(0.0f < aLife) && mFading == 0)
		mFading = 1;  // 0x7319e5
}

void Points::Draw(Graphics* g)
{
	if (0 < mDelay)
		return;
	bool is3D = gApp->Is3DAccelerated();
	g->PushState();
	// (the original takes mPhase's sine here and drops it)
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetFont(mFont);
	int anAlpha = Ftol((double)mAlpha * 255.0);
	Color aShadow(ClampByte(Ftol((double)mColor.mRed * 0.25)), ClampByte(Ftol((double)mColor.mGreen * 0.25)),
		ClampByte(Ftol((double)mColor.mBlue * 0.25)), 255 < anAlpha ? 255 : anAlpha);
	g->SetColor(aShadow);
	if (!is3D)
		g->SetFastStretch(true);
	anAlpha = Ftol((double)mAlpha * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	int aWidth = mFont->StringWidth(mText);
	int anAscent = mFont->GetAscent();
	int aLayer = 0;
	if (0 < GetFontLayerCount(mFont)) {
		do {
			Color aColor;
			if (aLayer == mLayers - 1) {
				if (mUseLastLayerCycle)
					aColor = mCycles[mLayers - 1].mColor;
				else
					aColor = mColor;
			} else {
				aColor = mCycles[aLayer].mColor;
			}
			aColor.mAlpha = Ftol((double)aColor.mAlpha * (double)mAlpha);
			SetFontLayerColor(mFont, aLayer, aColor);
			aLayer++;
		} while (aLayer < GetFontLayerCount(mFont));
	}
	g->SetColor(Color::White);
	g->PushState();
	g->SetScale(mScale, mScale, mX, mY);
	int aY = Ftol((double)anAscent * 0.20000000298023224 + (double)mY);
	int aX = Ftol((double)mX - (double)(aWidth / 2));
	g->DrawString(mText, aX, aY);
	g->PopState();
	if (!is3D)
		g->SetFastStretch(false);
	g->SetColorizeImages(false);
	mDrawn = true;
	g->PopState();
}

PointsManager::PointsManager()
{
}

PointsManager::~PointsManager()
{
	while (!mPoints.empty()) {
		delete mPoints.back();
		mPoints.pop_back();
	}
}

void PointsManager::Update()
{
	auto anIt = mPoints.begin();
	while (anIt != mPoints.end()) {
		Points* aPoints = *anIt;
		aPoints->Update();
		if (!aPoints->mDone) {
			++anIt;
		} else {
			delete aPoints;
			anIt = mPoints.erase(anIt);
			MarkDirty();
		}
	}
}

void PointsManager::DrawOverlay(Graphics* g, int thePriority)
{
	(void)thePriority;
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color::White);
	for (Points* aPoints : mPoints)
		aPoints->Draw(g);
}
