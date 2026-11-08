// TitleScreen.cpp: TitleScreen (TitleScreen.h).
#include "TitleScreen.h"

#include <math.h>
#include "FancyBackdrop.h"
#include "DDImage.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinDMApp.h"

using namespace Sexy;

static const int PLAY_LINK_ID = 1;

// 0x5daef5
TitleScreen::TitleScreen(WinDMApp* theApp)
{
	mApp = theApp;
	mLoadProgress = 0;
	mLoadedFlag = false;

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->SetFont(FONT_1);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(32, 255, 32);

	mLogo2GlowImage = MakeGlowImage(IMAGE_LOGO_2);
	mDeluxeGlowImage = MakeGlowImage(IMAGE_LOGO_DELUXE);
	SetupVals();
	mLoadingCompleted = false;
	mSlide.SetRamp(CurvedVal::RAMP_SLOW_TO_FAST);
	mSlide.SetOutRange(0, 400.0);
	mSettled = false;
	Update();
}

TitleScreen::~TitleScreen()
{
	delete mPlayLink;
	delete mLogo2GlowImage;
	delete mDeluxeGlowImage;
}

// 0x5d98a5: a copy of the image with the red of each pixel in all three colors, the alpha kept
Image* TitleScreen::MakeGlowImage(Image* theImage)
{
	MemoryImage* aCopy = mApp->CopyImage(theImage);
	unsigned long* aBits = aCopy->GetBits();
	int aCount = aCopy->mWidth * aCopy->mHeight;
	for (; aCount > 0; aCount--, aBits++)
	{
		unsigned long aRed = (*aBits >> 16) & 0xff;
		*aBits = ((aRed << 8) | aRed) | (*aBits & 0xffff0000);
	}
	aCopy->BitsChanged();
	return aCopy;
}

// 0x5d98fa
void TitleScreen::SetupVals()
{
	mLogoDelay = 0x50;
	mLogoAlpha.SetRamp(CurvedVal::RAMP_SLOW_TO_FAST);
	mLogoAlpha.SetInVal(0);
	mLogoY.SetOutRange(800.0, 0);
	mLogoY.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mLogoY.SetInVal(0);
	mBarDelay = 0x32;
	mBarAlpha.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mBarAlpha.SetInVal(0);
	mUnusedDelay = 0x50;
	mUnused.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mUnused.SetInVal(0);
	mDeluxeDelay = 0x140;
	mDeluxeAlpha.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mDeluxeAlpha.SetInVal(0);
	mDeluxeGlowDelay = 0x140;
	mDeluxeGlow.SetMode(CurvedVal::MODE_PING_PONG);
	mDeluxeGlow.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mDeluxeGlow.SetInVal(0);
	mLogo2Delay = 0x104;
	mLogo2Alpha.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mLogo2Alpha.SetInVal(0);
	mLogo2GlowDelay = 0x104;
	mLogo2Glow.SetMode(CurvedVal::MODE_PING_PONG);
	mLogo2Glow.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mLogo2Glow.SetInVal(0);
	mBarFlashStart = 0x140;
	mBarFlash.SetMode(CurvedVal::MODE_PING_PONG);
	mBarFlash.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	mBarFlash.SetInVal(0);
	mBarPurple = 0;
	mFade = 0;
	mLinkGlow = 0;
	mBarFlashDelay = 0x50;
}

// 0x5d9ac8
void TitleScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mPlayLink);
}

// 0x5d9aeb
void TitleScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mPlayLink);
}

// 0x5d9b2b
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == PLAY_LINK_ID)
		mApp->LoadingCompleted();
}

// 0x5d9b7b
void TitleScreen::Draw(Graphics* g)
{
	mApp->mFancyBackdrop->DrawBottom(g);
	int anOfs = (int)mSlide.GetOutVal();

	// the logos: the retail layout, and the one of the Diamond Mine edition
	bool isDM = mApp->mIsDiamondMine;
	int aLogoX = isDM ? 0x32 : 0x28;
	int aLogo2X = isDM ? 0x19c : 0x1c2;
	int aLogo2Y = (isDM ? 0xda : 0x82) - anOfs;
	int aDeluxeX = isDM ? 0x70 : 0x66;
	int aDeluxeY = (isDM ? 0x172 : 0xdc) - anOfs;

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(mLogoAlpha.GetOutVal() * 255.0)));
	g->DrawImage(IMAGE_LOGO, mApp->ScaleNum(aLogoX, 0), mApp->ScaleNum(((int)mLogoY.GetOutVal() - anOfs) + 0x14, 0));
	g->SetColor(Color(255, 255, 255, (int)(mLogo2Alpha.GetOutVal() * 255.0)));
	g->DrawImage(IMAGE_LOGO_2, mApp->ScaleNum(aLogo2X, 0), mApp->ScaleNum(aLogo2Y, 0));
	if (mLogo2Glow.mInVal < 0.98 && mLogo2Glow.GetOutVal() > 0.001)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, (int)(mLogo2Glow.GetOutVal() * 255.0)));
		g->DrawImage(mLogo2GlowImage, mApp->ScaleNum(aLogo2X, 0), mApp->ScaleNum(aLogo2Y, 0));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColor(Color(255, 255, 255, (int)(mDeluxeAlpha.GetOutVal() * 255.0)));
	g->DrawImage(IMAGE_LOGO_DELUXE, mApp->ScaleNum(aDeluxeX, 0), mApp->ScaleNum(aDeluxeY, 0));
	if (mDeluxeGlow.mInVal < 0.98 && mDeluxeGlow.GetOutVal() > 0.001)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, (int)(mDeluxeGlow.GetOutVal() * 255.0)));
		g->DrawImage(mDeluxeGlowImage, mApp->ScaleNum(aDeluxeX, 0), mApp->ScaleNum(aDeluxeY, 0));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	g->SetColorizeImages(false);
	mApp->mFancyBackdrop->DrawTop(g);
	g->SetColorizeImages(true);

	// the PopCap logo and the loading bar
	double aBarOut = mBarAlpha.GetOutVal();
	double aPulse = cos(mUpdateCnt * 0.07) * 0.07 + 0.93;
	int anAlpha = (int)(aBarOut * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImage(IMAGE_LOGO_POPCAP, mApp->ScaleNum(0x348, 0), mApp->ScaleNum(anOfs + 0x23a, 0));
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImage(IMAGE_LOADER_BAR, mApp->ScaleNum(0xf3, 0), mApp->ScaleNum(0x250, 0) + anOfs);

	if (mBarPurple < 1.0f)
	{
		// the blue bar: three pieces, the left end, the right end and the middle repeated to the length
		g->SetColor(Color(255, 255, 255, (int)((1 - mBarPurple) * aPulse * mBarAlpha.GetOutVal() * 255.0)));
		int aWidth = (int)(mApp->ScaleNum(0x226, 0) * mLoadProgress);
		if (mLoadProgress == 1.0)
			aWidth = mApp->ScaleNumUp(0x226, 0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aCap = mApp->ScaleNum(0x28, 0);
		int aHeight = mApp->ScaleNumUp(0x45, 0);
		int aX = mApp->ScaleNum(0xee, 0);
		int aY = mApp->ScaleNum(0x263, 0) + anOfs;
		if (aWidth < aCap * 2)
		{
			int aHalf = aWidth / 2;
			g->DrawImage(IMAGE_LOADER_BAR_LIT_BLUE, aX, aY, Rect(0, 0, aHalf, aHeight));
			int aRest = aWidth - aHalf;
			g->DrawImage(IMAGE_LOADER_BAR_LIT_BLUE, aHalf + aX, aY, Rect(aCap * 3 - aRest, 0, aRest, aHeight));
		}
		else
		{
			g->DrawImage(IMAGE_LOADER_BAR_LIT_BLUE, aX, aY, Rect(0, 0, aCap, aHeight));
			g->DrawImage(IMAGE_LOADER_BAR_LIT_BLUE, (aX - aCap) + aWidth, aY, Rect(aCap * 2, 0, aCap, aHeight));
			aWidth -= aCap * 2;
			Graphics aMid(*g);
			aMid.ClipRect(aX + aCap, aY, aWidth, aHeight);
			int aPieces = (aWidth - 1 + aCap) / aCap;
			int aPieceX = aX + aCap;
			for (int i = 0; i < aPieces; i++)
			{
				aMid.DrawImage(IMAGE_LOADER_BAR_LIT_BLUE, aPieceX, aY, Rect(aCap, 0, aCap, aHeight));
				aPieceX += aCap;
			}
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	if (mBarPurple > 0.0f)
	{
		g->SetColor(Color(255, 255, 255, (int)(aPulse * 255.0 * aPulse * aBarOut * mBarPurple)));
		g->DrawImage(IMAGE_LOADER_BAR_LIT_PURPLE, mApp->ScaleNum(0xee, 0), mApp->ScaleNum(0x263, 0) + anOfs);
	}
	if (mBarFlash.mInVal < 0.98 && mBarFlash.GetOutVal() > 0.001)
	{
		g->SetColor(Color(255, 255, 255, (int)(mBarFlash.GetOutVal() * 255.0)));
		g->DrawImage(IMAGE_LOADER_BAR_FLASH, mApp->ScaleNum(0x8c, 0), mApp->ScaleNum(0x213, 0) + anOfs);
	}
	if (mBarFlashDelay == 0)
	{
		if (mLinkGlow > 0.0)
		{
			g->SetColor(Color(255, 255, 255, (int)(mLinkGlow * aBarOut * 255.0)));
			g->DrawImage(IMAGE_LOADER_BAR_LIT_PURPLE, mApp->ScaleNum(0xee, 0), mApp->ScaleNum(0x263, 0) + anOfs);
		}
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(IMAGE_LOADER_BAR_TEXT, mApp->ScaleNum(0x127, 0), mApp->ScaleNum(0x251, 0) + anOfs);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x5da77e
void TitleScreen::Update()
{
	Widget::Update();

	if (mBarFlash.mInVal >= 0.98)
	{
		if (mApp->mHasFocus)
			mSettled = true;
		else if (mSettled)
			return;
	}

	if (mLogoDelay < 1)
	{
		mLogoY.IncInVal(0.0055);
		mLogoAlpha.IncInVal(0.0065);
	}
	else
		mLogoDelay--;
	if (mBarDelay < 1)
		mBarAlpha.IncInVal(0.005);
	else
		mBarDelay--;
	if (mUnusedDelay < 1)
		mUnused.IncInVal(0.005);
	else
		mUnusedDelay--;
	if (mDeluxeDelay < 1)
		mDeluxeAlpha.IncInVal(0.01);
	else
		mDeluxeDelay--;
	if (mDeluxeGlowDelay < 1)
	{
		if (mDeluxeGlow.mInVal < 0.98)
			mDeluxeGlow.IncInVal(0.012);
	}
	else
		mDeluxeGlowDelay--;
	if (mLogo2Delay < 1)
		mLogo2Alpha.IncInVal(0.01);
	else
		mLogo2Delay--;
	if (mLogo2GlowDelay < 1)
	{
		if (mLogo2Glow.mInVal < 0.98)
			mLogo2Glow.IncInVal(0.012);
	}
	else
		mLogo2GlowDelay--;

	if (mLoadingCompleted && !mSlide.IncInVal(0.016))
		mApp->KillTitleScreen();

	if (mBarFlashStart > 0)
		mBarFlashStart--;

	if (mApp->mLoaded && mBarFlashStart == 0)
	{
		if (mBarFlash.mInVal < 0.98)
			mBarFlash.IncInVal(0.012);
		mBarPurple = mBarPurple + 0.01f;
		if (mBarPurple >= 1.0f)
			mBarPurple = 1.0f;
		mFade = mFade + 0.05;
		if (mFade >= 1.0)
			mFade = 1.0;
		if (mBarFlashDelay > 0)
			mBarFlashDelay--;
		if (!mPlayLink->mIsOver)
			mLinkGlow = mLinkGlow - 0.06 > 0.0 ? mLinkGlow - 0.06 : 0.0;
		else
			mLinkGlow = mLinkGlow + 0.08 < 1.0 ? mLinkGlow + 0.08 : 1.0;
	}

	int anOfs = (int)mSlide.GetOutVal();
	mPlayLink->SetDisabled(false);
	int aWidth = mApp->ScaleNum(400, 0);
	mPlayLink->Resize((mWidth - aWidth) / 2, mApp->ScaleNum(0x269, 0) + anOfs, aWidth, mApp->ScaleNumUp(0x35, 0));
	mPlayLink->SetDisabled(false);

	if (!mLoadedFlag && (mUpdateCnt & 3) == 0)
	{
		mLoadProgress = mApp->GetLoadingThreadProgress();
		if (mLoadProgress == 1.0 && mApp->mLoaded && mBarFlashStart == 0)
		{
			mPlayLink->SetVisible(true);
			mLoadProgress = 1.0;
			mLoadedFlag = true;
		}
	}

	if (!(!mApp->mHasFocus && mApp->mLoaded && (mUpdateCnt & 7) != 0))
		MarkDirty();
}
