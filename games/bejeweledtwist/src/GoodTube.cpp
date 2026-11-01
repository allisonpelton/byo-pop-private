// Sexy::GoodTube (0x702736-0x70a9bb): being ported, see GoodTube.h.
#include "GoodTube.h"

#include "Board.h"
#include "GameText.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

GoodTube::GoodTube(Board* theBoard)
{
	mBoard = theBoard;
	mUnknown308 = 1.0f;
	mName = "GOODTUBE";
	mColorCycle.mColors.clear();
	mColorCycle.mPos = 0;
	mColorCycle.mColors.push_back(Color::White);
	// port: the constructor's other fields come with the tube's update and drawing
	UpdateImages();
	SetUnknown14D(false);
	SetShown(false);
	for (int i = 16; i != 0; i--) {
		Bubble* aBubble = new Bubble();
		aBubble->mUnknown0 = 0;
		aBubble->mPhase = (float)((double)(Rand() % 100) * 0.001);
		mBubbles.push_back(aBubble);  // 0x707ad5
	}
	// port: the level's text (0x74e701, FONT_INGAME), the 0x158-byte object at +0x32c (0x701efb), the menu button
	// (0x754172, IMAGE_INGAMEUI_MENUBUTTON) and the effects manager at +0x28c come with the tube's drawing; none of
	// them draws from Rand
	mUnknown2E0 = std::fabs((float)RandSigned());
	mUnknown338.SetCurve(GTS(0x7e3e80));  // 0x704d9c
}

GoodTube::~GoodTube()
{
	for (Bubble* aBubble : mBubbles)
		delete aBubble;
}

void GoodTube::SetShown(bool theShown)
{
	mShown = theShown;
	// port: and +0x32c's +0x54 and +0x55, once that object is ported
}

void GoodTube::SetUnknown158(int theValue)
{
	mUnknown158 = theValue;
	if (theValue == 3) {
		// port: sets a CurvedVal to 1 (CurvedVal::SetConstant), once it is known which
		NotPorted("GoodTube::SetUnknown158(3)");
	}
}

void GoodTube::SetChallenge(bool on)
{
	mChallenge = on;
	UpdateImages();
}

void GoodTube::SetTime(int theTime, bool theAtOnce)
{
	mUnknown4B4 = theTime;
	if (mUnknown4BC == 0)
		mUnknown4BC = theTime;
	if (theAtOnce) {
		mUnknown4B8 = theTime;
		UpdateFill(theAtOnce);
	}
}

void GoodTube::StartUnknown4AC(bool theFour, bool theUnknown4A9)
{
	if (mUnknown4AC < 1) {
		mUnknown4B0 = 0;
		mUnknown4C0 = 0;
		mUnknown4AC = 1;
		mUnknown400.SetCurve(GTS(0x7e3fe0));
		if (theFour)
			mUnknown400.mInVal = mUnknown400.mInMax;
		mUnknown400.mIncRate = 0;
	}
	if (theFour)
		mUnknown4AC = 4;
	mUnknown4A9 = theUnknown4A9;
}

void GoodTube::SetColor(const Color& theColor, bool theAtOnce)
{
	Color aLast = mColorCycle.mColors[mColorCycle.mColors.size() - 1];
	mColorCycle.mColors.clear();
	mColorCycle.mPos = 0;
	mColorCycle.mColors.push_back(aLast);
	mColorCycle.mColors.push_back(theColor);
	mColorCycle.mLoop = false;
	if (theAtOnce)
		mColorCycle.SetPosition(1.0f);
}

void GoodTube::UpdateImages()
{
	mTopImage = mChallenge ? IMAGE_INGAMEUI_TUBE_TOP_CHALLENGE : IMAGE_INGAMEUI_TUBE_TOP;
	mBottomImage = IMAGE_INGAMEUI_TUBE_BOTTOM;
	mGlassImage = IMAGE_INGAMEUI_TUBE_GLASS;
	mFillImage = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	mFillImage2 = IMAGE_INGAMEUI_TUBE_FILL_GOLD;
	if (mGlassImage == nullptr)
		mUnknown16C = 0;
	else
		mUnknown16C = GetBottomTop() - GetGlassTop();
	float aTop = (float)(GetGlassTop() + mUnknown16C);
	mUnknown198 = aTop;
	mUnknown194 = aTop;
}

int GoodTube::GetTubeHeight()
{
	if (mUnknown16C == 0)
		return 0;
	return Ftol((double)(mGlassImage->mHeight + mBottomImage->mHeight + mTopImage->mHeight) * 0.9300000071525574);
}

int GoodTube::GetTubeWidth()
{
	if (mUnknown16C == 0)
		return 0;
	int aWidth = IMAGE_INGAMEUI_TOP_LIGHTGLOW->mWidth;
	if (IMAGE_INGAMEUI_TOP_LIGHTGLOW->mWidth <= IMAGE_INGAMEUI_BOT_LIGHTGLOW->mWidth)
		aWidth = IMAGE_INGAMEUI_BOT_LIGHTGLOW->mWidth;
	return aWidth;
}

int GoodTube::GetGlassTop()
{
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	return Ftol((double)GetImageOffsetY(GetIdByImage(mGlassImage)) * (double)aScale);
}

int GoodTube::GetBottomTop()
{
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	int aTop = Ftol((double)GetImageOffsetY(GetIdByImage(mBottomImage)) * (double)aScale);
	return Ftol((double)aTop + (double)mUnknown2D0);
}

void GoodTube::SetProgress(float theProgress, bool theAtOnce)
{
	if (1.0f < theProgress)
		theProgress = 1.0f;
	mProgress = theProgress;
	UpdateFill(theAtOnce);
}

void GoodTube::UpdateFill(bool theAtOnce)
{
	if (mUnknown15C == 1) {
		// Blitz: the time left
		float aProgress;
		if (mUnknown4BC == 0)
			aProgress = 0.0f;
		else
			aProgress = (float)mUnknown4B8 / (float)mUnknown4BC;
		mProgress = aProgress;
	}
	int aMargin = Ftol((double)gApp->mScale * 35.0);
	mFillTop = GetGlassTop() + aMargin;
	int aBottom = Ftol((double)gApp->mScale * 5.0) + GetBottomTop();
	mFillBottom = aBottom;
	int aHeight = Ftol((double)(aBottom - mFillTop) * (double)mProgress);
	if (0 < mUnknown300)
		aHeight = 0;
	float aTop = (float)(aBottom - aHeight);
	mUnknown198 = aTop;
	if (mBoard->mHyperspace == nullptr && !theAtOnce)
		mUnknown194 = (float)(((double)aTop - (double)mUnknown194) / 30.0 + (double)mUnknown194);
	else
		mUnknown194 = aTop;
}

void GoodTube::SetLevelSteps(int theSteps)
{
	if (mUnknown274 < theSteps) {
		int aFlash = mUnknown1D8 + 30;
		int aMax = 200;
		if (aFlash < 200)
			aMax = aFlash;
		mUnknown1D8 = aMax;
		mUnknown1E0 = 0.5f;
	}
	if (theSteps < mUnknown274) {
		MarkLevelChange();
		mUnknown29C = mUnknown29C * 0.5f;
	}
	mUnknown274 = theSteps;
}

void GoodTube::SetLevelShown(int theLevel)
{
	if (!mUnknown14D)
		return;
	if (10 < theLevel)
		theLevel = 10;
	if (theLevel != mUnknown27C) {
		float aSteps;
		if (mUnknown27C < theLevel) {
			if (!mBoard->IsBusy()) {
				int aSound = 3;
				if (theLevel < 4)
					aSound = theLevel;
				// SOUND_MULTIPLIER_UP2_1 is resource 0x397 (0x397 + the level, at most 3)
				gApp->PlaySample(GetSoundById(SOUND_MULTIPLIER_UP2_1_ID + aSound));
			}
			mUnknown29C = 1.0f;
			gBoard->mUnknown20B8 = 1.0f;
			int aFlash = mUnknown1D8 + 64;
			int aMax = 255;
			if (aFlash < 255)
				aMax = aFlash;
			mUnknown1D8 = aMax;
			mUnknown1E0 = 0.5f;
			aSteps = 0.0f;
		} else {
			if (!mBoard->IsBusy())
				gApp->PlaySample(SOUND_MULTIPLIER_DOWN);
			aSteps = (float)Board::GetLevelSteps(theLevel);
		}
		mUnknown278 = aSteps;
	}
	mUnknown27C = theLevel;
	mUnknown280 = theLevel;
}

void GoodTube::GetStepPoint(int* thePoint, int theStep, bool theCentre)
{
	// The first light of each level (0x84f028 in the exe): IMAGE_INGAMEUI_ARC_4_FILL_1 for the first, then 8, 12 and 16
	static const int kFirstLight[11] = {IMAGE_INGAMEUI_BONUS_BLUE_LIT_ID, IMAGE_INGAMEUI_ARC_4_FILL_1_ID,
		IMAGE_INGAMEUI_ARC_8_FILL_1_ID, IMAGE_INGAMEUI_ARC_12_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID,
		IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID,
		IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID, IMAGE_INGAMEUI_ARC_16_FILL_1_ID};
	int aLevel = mUnknown27C;
	int aSteps = Board::GetLevelSteps(aLevel);
	int aStep = theStep % aSteps;
	int aLast = Board::GetLevelSteps(aLevel) - 1;
	if (aLast < aStep)
		aStep = aLast;
	int anId = kFirstLight[aLevel] + aStep;
	Image* anImage = GetImageById(anId);
	int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
	int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
	if (theCentre) {
		aX += anImage->mWidth / 2;
		aY += anImage->mHeight / 2;
	}
	thePoint[0] = aX;
	thePoint[1] = aY;
}

void GoodTube::ResetBonus()
{
	mUnknown3B0 = 0;
	for (unsigned int i = 0; i < mUnknown3D0.size(); i++) {
		mUnknown3D0[i] = 0;
		mUnknown3E0[i] = 0;
	}
	mUnknown3B4 = 0;
	mUnknown3F0 = 1.0f;
	mUnknown3B8 = 0;
	mUnknown3C4 = 0;
}
