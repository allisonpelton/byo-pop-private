// HelpScreen.cpp: the "How To Play" pages (Sexy::HelpScreen, 0x5c1c7a-0x5c267d). Translated from the Ghidra export and
// the assembly. The screen is a PodDialog that is not drawn by the dialog: Update lays it out over the whole window
// and Draw paints four pages of gem demonstrations on top of the dialog's own header and button.

#include "HelpScreen.h"
#include "WinDMApp.h"
#include "Res.h"
#include "Font.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "Color.h"
#include "Rect.h"
#include <math.h>

using namespace Sexy;

// 0x5c252e: theBackOnly is true for a screen that has only the button back to the menu or the game; the other kind
// asks "Don't Show Again" or "Continue" (the player was sent here by the hint of the first game)
HelpScreen::HelpScreen(WinDMApp* theApp, bool theBackOnly) :
	PodDialog(theApp, 40, true, "How To Play", "", "", theBackOnly ? BUTTONS_FOOTER : BUTTONS_YES_NO)
{
	mState = STATE_HIDDEN;
	mDelay = 0;
	mSlide.SetMode(CurvedVal::MODE_CLAMP);
	mSlide.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mSlide.SetOutRange(0.0, 700.0);

	if (theBackOnly)
	{
		if (theApp->mBoard != nullptr)
			mYesButton->mLabel = "Back to Game";
		else
			mYesButton->mLabel = "Back to Menu";
	}
	else
	{
		mYesButton->mLabel = "Don't Show Again";
		mNoButton->mLabel = "Continue";
	}
	mNeverShowAgain = (mNoButton != nullptr);

	InitialReset();
}

// 0x5c1c7a
void HelpScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	PodDialog::Resize(theX, theY, theWidth, theHeight);
}

// 0x5c1c83
void HelpScreen::AddedToManager(WidgetManager* theManager)
{
	PodDialog::AddedToManager(theManager);
}

// 0x5c1c8c
void HelpScreen::RemovedFromManager(WidgetManager* theManager)
{
	PodDialog::RemovedFromManager(theManager);
}

// 0x5c1c95: the next page begins (the page after the last one is the first again, which also puts everything back)
void HelpScreen::Reset()
{
	mSwapAlpha1 = 0.0f;
	mSwapAlpha2 = 0.0f;
	mPage = (mPage + 1) % 4;
	mSwapDelay1 = 250;
	mSwapDelay2 = 0x122;
	mSwapTime = 0x14a;
	mAngleDelay = 0x13b;
	mGemFadeDelay = 0x168;
	mEffectDelay = 0x172;
	mFadeInTime = 600;
	mPageTime = 0x2a3;
	if (mPage == 0)
	{
		mFadeInTime = 0x226;
		mPageTime = 0x271;
	}
	else if (mPage == 1)
	{
		mFadeInTime = 500;
		mPageTime = 0x23f;
	}
	else if (mPage == 2)
	{
		mFadeInTime = 0x23f;
		mPageTime = 0x28a;
	}
	else if (mPage == 3)
		mPageTime = 0x307;

	mArrowTimer = 0;
	mArrowDelay = 0xaf;
	if (mPage == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			mSwapAngle[i] = 0.0f;
			mEffectFade[i] = 0.0f;
			mGemFade[i] = 1.0f;
			mArrowAlpha[i] = 1.0f;
		}
		mArrowTimer = 0;
	}
}

// 0x5c1daa: the first page, with the first page lit and the others dim
void HelpScreen::InitialReset()
{
	mPage = -1;
	Reset();
	mPulse = 0.0;
	mStarAngle = 0.0;
	mStarAngle2 = 0.0;
	mPageAlpha[0] = 1.0f;
	mPageAlpha[1] = 1.0f;
	mPageAlpha[2] = 1.0f;
	mPageAlpha[3] = 1.0f;
	mPageBright[0] = 1.0f;
	mPageBright[1] = 0.5f;
	mPageBright[2] = 0.5f;
	mPageBright[3] = 0.5f;
}

// 0x5c1dfc: the screen slides in from 1000 pixels below its place after 13 updates
void HelpScreen::Show()
{
	mDelay = 13;
	mSlide.SetInVal(0.0);
	mSlide.SetOutRange(1000.0, 0.0);
	mSlide.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mState = STATE_SLIDING_IN;
	InitialReset();
}

// 0x5c1e50: the screen slides out (Update kills the dialog when it is gone)
void HelpScreen::Hide()
{
	mSlide.SetInVal(0.0);
	mSlide.SetOutRange(0.0, 1000.0);
	mSlide.SetRamp(CurvedVal::RAMP_SLOW_TO_FAST);
	mState = STATE_SLIDING_OUT;
}

// 0x5c1e97
void HelpScreen::DrawArrow(Graphics* g, int theX, int theY)
{
	g->SetColorizeImages(true);
	int aBright = (int)(mPageBright[mDrawPage] * 255.0);
	g->SetColor(Color(aBright, aBright, aBright, (int)(255.0 * mPageAlpha[mDrawPage] * mArrowAlpha[mDrawPage])));
	g->DrawImage(IMAGE_ARROW_UP, theX, theY);
	g->SetColorizeImages(false);
}

// 0x5c1f1d
void HelpScreen::Update()
{
	if (!mApp->mHasFocus)
		return;

	Widget::Update();

	mStarAngle += 0.015;
	if (6.28318 < mStarAngle)
		mStarAngle -= 6.28318;
	mPulse += 0.006;
	if (1.0 < mPulse)
		mPulse -= 1.0;
	mStarAngle2 -= 0.004;
	if (mStarAngle < 0.0)
		mStarAngle += 6.28318;

	if (mState == STATE_SLIDING_IN || mState == STATE_SLIDING_OUT)
	{
		if (mDelay > 0)
			mDelay--;
		else if (!mSlide.IncInVal(0.01))
		{
			if (mState == STATE_SLIDING_IN)
				mState = STATE_SHOWN;
			else
				mApp->KillDialog(40);
		}

		int anOffset = (int)mApp->ScaleNum(mSlide.GetOutVal(), 0.0);
		Resize(mApp->ScaleNum(0x14, 0), mApp->ScaleNum(0x1e, -9) + anOffset, mApp->mWidth - mApp->ScaleNum(0x28, 0),
			mApp->mHeight - mApp->ScaleNum(0x3c, -0x12));
	}

	if (mState == STATE_SLIDING_IN)
	{
		MarkDirty();
		return;
	}

	// the page that plays now lights up while it fades in and dims when its time is over
	if (mArrowTimer > 0)
		mArrowTimer--;
	else if (mFadeInTime > 0)
	{
		mPageBright[mPage] += 0.0175f;
		if (mPageBright[mPage] >= 1.0f)
			mPageBright[mPage] = 1.0f;
	}

	if (mFadeInTime > 0)
	{
		mFadeInTime--;
		for (int i = 0; i < 4; i++)
		{
			mPageAlpha[i] += 0.015f;
			if (mPageAlpha[i] >= 1.0f)
				mPageAlpha[i] = 1.0f;
		}
	}
	else
	{
		if (mPage == 3)
		{
			for (int i = 0; i < 4; i++)
			{
				mPageAlpha[i] -= 0.01f;
				if (mPageAlpha[i] <= 0.0f)
					mPageAlpha[i] = 0.0f;
			}
		}
		mPageBright[mPage] -= 0.01f;
		if (mPageBright[mPage] <= 0.5f)
			mPageBright[mPage] = 0.5f;
	}

	// the selection boxes of the two swaps rise one after the other, then fade
	if (mSwapTime > 0)
	{
		mSwapTime--;
		if (mSwapDelay1 > 0)
			mSwapDelay1--;
		else
		{
			mSwapAlpha1 += 0.08f;
			if (mSwapAlpha1 >= 1.0f)
				mSwapAlpha1 = 1.0f;
		}
		if (mSwapDelay2 > 0)
			mSwapDelay2--;
		else
		{
			mSwapAlpha2 += 0.08f;
			if (mSwapAlpha2 >= 1.0f)
				mSwapAlpha2 = 1.0f;
		}
	}
	else
	{
		mSwapAlpha1 -= 0.04f;
		if (mSwapAlpha1 <= 0.0f)
			mSwapAlpha1 = 0.0f;
		mSwapAlpha2 -= 0.04f;
		if (mSwapAlpha2 <= 0.0f)
			mSwapAlpha2 = 0.0f;
	}

	if (mArrowDelay > 0)
		mArrowDelay--;
	else
	{
		mArrowAlpha[mPage] -= 0.025f;
		if (mArrowAlpha[mPage] <= 0.0f)
			mArrowAlpha[mPage] = 0.0f;
	}

	if (mGemFadeDelay > 0)
		mGemFadeDelay--;
	else
	{
		mGemFade[mPage] -= 0.05f;
		if (mGemFade[mPage] <= 0.0f)
			mGemFade[mPage] = 0.0f;
	}

	if (mEffectDelay > 0)
		mEffectDelay--;
	else
	{
		mEffectFade[mPage] += 0.025f;
		if (mEffectFade[mPage] >= 1.0f)
			mEffectFade[mPage] = 1.0f;
	}

	mPageTime--;
	if (mPageTime == 0)
		Reset();

	if (mAngleDelay > 0)
		mAngleDelay--;
	else if (mSwapAngle[mPage] < 1.5708)
		mSwapAngle[mPage] += 0.03f;

	MarkDirty();
}

// 0x5c2368: a gem of the demonstration; theFrac is 1.0 for a whole gem and falls to 0 as the gem shrinks away (the
// shrinking is drawn with the six frames of mShrunkenGems)
void HelpScreen::DrawGem(Graphics* g, int theX, int theY, int theColor, float theFrac)
{
	g->SetColorizeImages(true);
	int aBright = (int)(mPageBright[mDrawPage] * 255.0);
	g->SetColor(Color(aBright, aBright, aBright, (int)(255.0 * mPageAlpha[mDrawPage])));

	int aFrame = -1 - (int)((1 - theFrac) * -7.0);
	if (aFrame == -1)
	{
		int aHeight = gApp->ScaleNum(0x54, 0);
		int aWidth = gApp->ScaleNum(0x54, 0);
		Rect aSrcRect(0, 0, aWidth, aHeight);
		g->DrawImage(GetImageById(IMAGE_GEM0_ID + theColor), theX, theY, aSrcRect);
	}
	else if (aFrame < 6)
		g->DrawImage(mApp->mShrunkenGems[theColor][aFrame], theX, theY);
	g->SetColorizeImages(false);
}

// 0x5c245d: a gem that moves by (theDX, theDY) as the swap of its page goes on, with the selection box over it
void HelpScreen::DrawSwapGem(Graphics* g, int theX, int theY, int theDX, int theDY, int theColor, float theFrac, float theSelectAlpha)
{
	float anAngle = mSwapAngle[mDrawPage];
	int anOffsetX = (int)(sinf(anAngle) * theDX);
	theX += anOffsetX;
	float anAngle2 = mSwapAngle[mDrawPage];
	int anOffsetY = (int)(sinf(anAngle2) * theDY);
	theY += anOffsetY;
	DrawGem(g, theX, theY, theColor, theFrac);

	if (mDrawPage == mPage)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(theSelectAlpha * 255.0)));
		g->DrawImage(IMAGE_SELECT, theX, theY);
		g->SetColorizeImages(false);
	}
}

// 0x5c267d
void HelpScreen::Draw(Graphics* g)
{
	PodDialog::Draw(g);

	int aRow1 = 0x54;
	int aRow2 = 0x15e;
	if (mApp->mWidth == 0x280)
	{
		aRow1 = 100;
		aRow2 = 0x16e;
	}

	g->DrawImage(IMAGE_HELP_GRID_5, gApp->ScaleNum(0x27, 0), gApp->ScaleNum(aRow1, 0));
	g->DrawImage(IMAGE_HELP_GRID_5, gApp->ScaleNum(500, 0), gApp->ScaleNum(aRow1, 0));
	g->DrawImage(IMAGE_HELP_GRID_4, gApp->ScaleNum(0x27, 0), gApp->ScaleNum(aRow2, 0));
	g->DrawImage(IMAGE_HELP_GRID_5, gApp->ScaleNum(500, 0), gApp->ScaleNum(aRow2, 0));
	g->SetFont(FONT_CONTINUUM_MED_14);

	// the line of text under a page, in the brightness of the page, centered on theCenterX
	struct Caption
	{
		static void Draw(HelpScreen* theScreen, Graphics* g, const char* theText, int theCenterX, int theY)
		{
			int aBright = (int)(theScreen->mPageBright[theScreen->mDrawPage] * 255.0);
			g->SetColor(Color(aBright, aBright, aBright));
			SexyString aString = theText;
			Font* aFont = g->GetFont();
			int aY = gApp->ScaleNum(theY, 0);
			int aWidth = aFont->StringWidth(aString);
			g->DrawString(aString, gApp->ScaleNum(theCenterX, 0) - aWidth / 2, aY);
		}
	};

	// page 0: swap two adjacent gems to make a set of 3 (a row of three, then a column of three)
	mDrawPage = 0;
	int aY = aRow1 + 0x17;
	DrawGem(g, gApp->ScaleNum(0x87, 0), gApp->ScaleNum(aY, 0), 2, mGemFade[mDrawPage]);
	DrawSwapGem(g, gApp->ScaleNum(0xdb, 0), gApp->ScaleNum(aY, 0), 0, gApp->ScaleNum(0x54, 0), 3, 1.0f, mSwapAlpha2);
	DrawGem(g, gApp->ScaleNum(0x12f, 0), gApp->ScaleNum(aY, 0), 2, mGemFade[mDrawPage]);

	int aY2 = aRow1 + 0x6b;
	DrawGem(g, gApp->ScaleNum(0x87, 0), gApp->ScaleNum(aY2, 0), 0, 1.0f);
	DrawSwapGem(g, gApp->ScaleNum(0xdb, 0), gApp->ScaleNum(aY2, 0), 0, gApp->ScaleNum(-0x54, 0), 2, mGemFade[mDrawPage], mSwapAlpha1);
	DrawGem(g, gApp->ScaleNum(0x12f, 0), gApp->ScaleNum(aY2, 0), 4, 1.0f);

	int anArrowY = aRow1 + 0x50;
	DrawArrow(g, gApp->ScaleNum(0xf4, 0), gApp->ScaleNum(anArrowY, 0));
	int aTextY = aRow1 + 0xda;
	Caption::Draw(this, g, "SWAP ADJACENT GEMS TO MAKE SETS OF 3", 0x105, aTextY);

	// page 1: clearing several sets at once
	mDrawPage = 1;
	DrawGem(g, gApp->ScaleNum(0x254, 0), gApp->ScaleNum(aY, 0), 2, mGemFade[mDrawPage]);
	DrawSwapGem(g, gApp->ScaleNum(0x2a8, 0), gApp->ScaleNum(aY, 0), 0, gApp->ScaleNum(0x54, 0), 3, mGemFade[mDrawPage], mSwapAlpha2);
	DrawGem(g, gApp->ScaleNum(0x2fc, 0), gApp->ScaleNum(aY, 0), 2, mGemFade[mDrawPage]);
	DrawGem(g, gApp->ScaleNum(0x254, 0), gApp->ScaleNum(aY2, 0), 3, mGemFade[mDrawPage]);
	DrawSwapGem(g, gApp->ScaleNum(0x2a8, 0), gApp->ScaleNum(aY2, 0), 0, gApp->ScaleNum(-0x54, 0), 2, mGemFade[mDrawPage], mSwapAlpha1);
	DrawGem(g, gApp->ScaleNum(0x2fc, 0), gApp->ScaleNum(aY2, 0), 3, mGemFade[mDrawPage]);
	DrawArrow(g, gApp->ScaleNum(0x2c1, 0), gApp->ScaleNum(anArrowY, 0));
	Caption::Draw(this, g, "CLEAR MULTIPLE SETS FOR EXTRA POINTS", 0x2d2, aTextY);

	// page 2: 4 gems make a power gem (a gem swaps up into the column of three, and a star turns over the new gem)
	mDrawPage = 2;
	int aPowerY = aRow2 + 0x17;
	DrawGem(g, gApp->ScaleNum(0x5e, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);
	DrawSwapGem(g, gApp->ScaleNum(0xb2, 0), gApp->ScaleNum(aPowerY, 0), 0, gApp->ScaleNum(0x54, 0), 3, 1.0f, mSwapAlpha2);
	DrawGem(g, gApp->ScaleNum(0x106, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);
	DrawGem(g, gApp->ScaleNum(0x15a, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);

	int aPowerY2 = aRow2 + 0x6b;
	DrawGem(g, gApp->ScaleNum(0x5e, 0), gApp->ScaleNum(aPowerY2, 0), 0, 1.0f);
	int aSwapX = gApp->ScaleNum(0xb2, 0);
	int aSwapY = gApp->ScaleNum(aPowerY2, 0);
	float aSin = sinf(mSwapAngle[mDrawPage]);
	aSin = (float)(aSin * 84.0);
	aSwapY -= (int)gApp->ScaleNum(aSin, 0.0f);
	DrawSwapGem(g, aSwapX, aSwapY, 0, 0, 2, 1.0f, mSwapAlpha1);
	DrawGem(g, gApp->ScaleNum(0x106, 0), gApp->ScaleNum(aPowerY2, 0), 4, 1.0f);
	DrawGem(g, gApp->ScaleNum(0x15a, 0), gApp->ScaleNum(aPowerY2, 0), 0, 1.0f);

	// the two stars of the power gem (drawn additively, they take turns as the pulse goes from 0 to 1)
	g->SetDrawMode(1);
	g->SetColorizeImages(true);
	int aPage = mDrawPage;
	double aPulse = fabs((mPulse + mPulse) - 1.0);
	int anAlpha = (int)(mPageBright[aPage] * 255.0 * mEffectFade[aPage] * mPageAlpha[2] * aPulse);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImageRotated(IMAGE_BIGSTAR, aSwapX - gApp->ScaleNum(0x14, 0), aSwapY - gApp->ScaleNum(0x14, 0), mStarAngle, nullptr);
	aPage = mDrawPage;
	aPulse = fabs((mPulse + mPulse) - 1.0);
	anAlpha = (int)(mPageBright[aPage] * 255.0 * mEffectFade[aPage] * mPageAlpha[2] * (1 - aPulse));
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImageRotated(IMAGE_BIGSTAR, aSwapX - gApp->ScaleNum(0x14, 0), aSwapY - gApp->ScaleNum(0x14, 0), mStarAngle2, nullptr);
	g->SetColorizeImages(false);
	g->SetDrawMode(0);

	int aPowerArrowY = aRow2 + 0x50;
	DrawArrow(g, gApp->ScaleNum(0xcc, 0), gApp->ScaleNum(aPowerArrowY, 0));
	int aPowerTextY = aRow2 + 0xda;
	Caption::Draw(this, g, "4 GEMS WILL CREATE A POWER GEM", 0x105, aPowerTextY);

	// page 3: 5 gems in a line make a hyper cube (its wild card shows over the gem that swaps up)
	mDrawPage = 3;
	DrawGem(g, gApp->ScaleNum(0x200, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);
	DrawGem(g, gApp->ScaleNum(0x254, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);
	DrawSwapGem(g, gApp->ScaleNum(0x2a8, 0), gApp->ScaleNum(aPowerY, 0), 0, gApp->ScaleNum(0x54, 0), 3, 1.0f, mSwapAlpha2);
	DrawGem(g, gApp->ScaleNum(0x2fc, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);
	DrawGem(g, gApp->ScaleNum(0x350, 0), gApp->ScaleNum(aPowerY, 0), 2, mGemFade[mDrawPage]);

	DrawGem(g, gApp->ScaleNum(0x200, 0), gApp->ScaleNum(aPowerY2, 0), 4, 1.0f);
	DrawGem(g, gApp->ScaleNum(0x254, 0), gApp->ScaleNum(aPowerY2, 0), 0, 1.0f);
	int aCubeX = gApp->ScaleNum(0x2a8, 0);
	int aCubeY = gApp->ScaleNum(aPowerY2, 0);
	float aCubeSin = sinf(mSwapAngle[mDrawPage]);
	aCubeSin = (float)(aCubeSin * 84.0);
	aCubeY -= (int)gApp->ScaleNum(aCubeSin, 0.0f);
	DrawSwapGem(g, aCubeX, aCubeY, 0, 0, 2, 1.0f - mEffectFade[mDrawPage], mSwapAlpha1);
	DrawGem(g, gApp->ScaleNum(0x2fc, 0), gApp->ScaleNum(aPowerY2, 0), 4, 1.0f);
	DrawGem(g, gApp->ScaleNum(0x350, 0), gApp->ScaleNum(aPowerY2, 0), 0, 1.0f);

	if (mPage == 3)
	{
		int aFrame = (mUpdateCnt / 6) % 0x28;
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mEffectFade[mDrawPage] * 255.0 * mPageAlpha[3])));
		int aHeight = gApp->ScaleNum(0x54, 0);
		int aWidth = gApp->ScaleNum(0x54, 0);
		Rect aSrcRect(gApp->ScaleNum(aFrame * 0x54, 0), 0, aWidth, aHeight);
		g->DrawImage(IMAGE_WILDCARD_DESTROYER, aCubeX, aCubeY, aSrcRect);
		g->SetColorizeImages(false);
	}

	DrawArrow(g, gApp->ScaleNum(0x2c1, 0), gApp->ScaleNum(aPowerArrowY, 0));
	Caption::Draw(this, g, "5 GEMS WILL CREATE A HYPER CUBE", 0x2d2, aPowerTextY);
}
