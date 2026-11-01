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
	if (mBoard->mUnknown200C == 0 && !theAtOnce)
		mUnknown194 = (float)(((double)aTop - (double)mUnknown194) / 30.0 + (double)mUnknown194);
	else
		mUnknown194 = aTop;
}
