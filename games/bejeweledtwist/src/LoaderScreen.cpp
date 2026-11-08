// Sexy::LoaderScreen (LoaderScreen.h).
#include "LoaderScreen.h"

#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "EffectsManager.h"
#include "Logo.h"
#include "Menu.h"
#include "PlayButton.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "WebSettings.h"
#include "X86Math.h"

using namespace Sexy;

LoaderScreen::LoaderScreen(WinTwistApp* theApp) : mApp(theApp)
{
	mPlayButton = new PlayButton(0, nullptr, this);
	mPlayButton->mZOrder = 0;
	mPlayButton->mMouseVisible = false;
	mPlayButton->mAlpha.SetConstant(1.0);
	mPlayButton->mAppear.SetConstant(1.0);
	AddWidget(mPlayButton);

	mLogo = new Logo(gApp->mFXManager);
	mLogo->Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mApp->mWidgetManager->AddWidget(mLogo);
	mLogo->mDrop.SetConstant(0);

	mPopCapAlpha.SetCurve(GTS(0x7e6b9c));
	mTrademarkAlpha.SetCurve(GTS(0x7e6b68));
	// port: the site's "Always skip start-up logos" (-skiplogos natively): the PopCap logo and its trademark sign stay
	// out. With the port's loading done before the loader's first frames, they fade in only behind the main menu's
	// arrival, until the loader leaves; the two curves feed nothing else, and the loading and the menu's start are as
	// before
	if (Byopop::SiteSkipLogos()) {
		mPopCapAlpha.SetConstant(0.0);
		mTrademarkAlpha.SetConstant(0.0);
	}
	mCurve120.SetCurve(GTS(0x7e6b38));
	mLogoAlpha.SetCurve(GTS(0x7e6b10));
	mLogo->mTrademarkAlpha.SetCurve(GTS(0x7e6ae0));
	mLoadingAlpha.SetCurve(GTS(0x7e6ab4));
	mFadeIn.SetCurve(GTS(0x7e6a88));
	mSlide.SetCurve(GTS(0x7e6a54));
	mButtonY.SetCurve(GTS(0x7e6a34));
	mButtonAlpha.SetCurve(GTS(0x7e6a04));
	mArcAlpha.SetCurve(GTS(0x7e69d0));
	mBarGlow.SetCurve(GTS(0x7e69a4));
	mBarGlow.SetMode(1);
	mArcGlow.SetCurve(GTS(0x7e6978));
	mArcGlow.SetMode(1);
	mHasAlpha = true;
	mFinished = false;
	Update();
}

LoaderScreen::~LoaderScreen()
{
	if (mLogo != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mLogo);
		delete mLogo;
	}
	RemoveAllWidgets(true, false);
}

void LoaderScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mWidgetManager->PutInfront(mLogo, this);
	mWidgetManager->PutInfront(gApp->mFXManager, mLogo);
}

void LoaderScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
}

void LoaderScreen::Update()
{
	Widget::Update();
	if (mLogo != nullptr)
		mLogo->mAlpha = (float)mLogoAlpha.GetOutVal();
	if (mApp->mFlag784)
		mApp->mFlag764 = true;

	if (!mDone && mUpdateCnt % 4 == 0) {
		double aProgress = mApp->GetLoadingThreadProgress();
		mProgress = aProgress;
		double aSmooth = aProgress * 0.1 + (double)mSmoothProgress * 0.9;
		// port: the original caps the bar at mUpdateCnt / 800 and starts the menu only once the smoothed bar reaches
		// 0.995, so the menu waited about 8 seconds however fast the files loaded; the port loads at the machine's speed
		// (CLAUDE.md, "Port first, unmodified"): the menu starts as soon as loading has ended, the bar then full
		mSmoothProgress = (float)aSmooth;
		if (aProgress == 1.0 && mApp->mLoaded) {
			mSmoothProgress = 1.0f;
			mProgress = 1.0;
			mCountdown = 150;
			mLogo->Reset(true);
			mLogo->mTrademarkAlpha.SetCurve(GTS(0x7e6934));
			mApp->StartMainMenu();
			mWidgetManager->PutBehind(this, mApp->mMainMenu);
			mButtonY.SetCurve(GTS(0x7e690c));
			mLoadingAlpha.SetCurve(GTS(0x7e6934));
			mSlide.SetCurve(GTS(0x7e68e0));
			mDone = true;
		}
	}
	if (mCountdown == 125)
		mApp->PlaySample(SOUND_VOICE_WELCOMETOBEJEWELEDTWIST);
	if (0 < mCountdown && --mCountdown == 0)
		mFinished = true;
	if (mApp->mHasFocus || mUpdateCnt % 8 == 0)
		MarkDirty();

	// The spiral button sits in the middle, mButtonY above it; once the loader's own button is gone the main menu's
	// follows mButtonAlpha
	PlayButton* aButton = mPlayButton;
	int anOffsetX = 0;
	int anOffsetY = 0;
	if (aButton == nullptr) {
		aButton = mApp->GetMainMenuPlayButton();
		anOffsetX = mApp->mWidth;
		anOffsetY = mApp->mHeight;
		if (aButton == nullptr)
			return;
	}
	double aButtonY = mButtonY.GetOutVal() * mApp->mScale;
	// The original measures the outer ring through the loader's own button even when it is gone
	int aHeight = IMAGE_MAINMENU_OUTERRING->GetCelHeight();
	int aWidth = IMAGE_MAINMENU_OUTERRING->GetCelWidth();
	int aTop = mApp->mHeight / 2 - IMAGE_MAINMENU_OUTERRING->GetCelHeight() / 2;
	int aY = Ftol((double)aTop + aButtonY + anOffsetY);
	int aX = mApp->mWidth / 2 - IMAGE_MAINMENU_OUTERRING->GetCelWidth() / 2 + anOffsetX;
	aButton->Resize(aX, aY, aWidth, aHeight);
	aButton->mAlpha.SetConstant(mButtonAlpha.GetOutVal());
}

void LoaderScreen::Draw(Graphics* g)
{
	DeferOverlay(0);
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// The original works out the screen's middle here (0x699cfa) and drops it
	g->SetColor(Color::White);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	if (mFadeIn.GetOutVal() != 1.0) {
		g->SetColor(Color(0, 0, 0, Ftol((1.0 - mFadeIn.GetOutVal()) * 255.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
}

void LoaderScreen::DrawOverlay(Graphics* g)
{
}

void LoaderScreen::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	Widget::DrawAll(theFlags, g);
	g->SetColorizeImages(true);
	float aScale = gApp->mScale;

	if (mPopCapAlpha.GetOutVal() > 0.0) {
		g->SetColor(Color(255, 255, 255, Ftol(mPopCapAlpha.GetOutVal() * 255.0)));
		int aDY = Ftol((double)aScale * 90.0);
		DrawImageScaledCentered(g, IMAGE_LOADER_POPCAP, (float)(0.5 * mWidth), (float)(mHeight * 0.5 - aDY),
			ALIGN_CENTER, 1.0f, 1.0f);
	}
	if (mTrademarkAlpha.GetOutVal() != 0.0) {
		g->SetColor(Color(255, 255, 255, Ftol(mTrademarkAlpha.GetOutVal() * 255.0)));
		int aY = mHeight / 2 - Ftol((double)aScale * -80.0);
		int aX = mWidth / 2 - Ftol((double)aScale * -130.0);
		g->DrawImage(IMAGE_LOGOTM, aX, aY);
	}

	// The device, drawn for a 1920 wide screen and centred; mSlide moves it down
	g->SetColor(Color(255, 255, 255));
	double aSlide = mSlide.GetOutVal();
	int aSlideY = Ftol((double)Ftol((double)aScale * 0.0) + (double)aScale * aSlide);
	int anArtWidth = Ftol((double)aScale * 1920.0);
	g->Translate((gApp->mWidth - anArtWidth) / 2, aSlideY);
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_GLASS_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_GLASS_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_GLASS, aX, aY);
	}

	// The bar: two copies of the fill image scroll along it, clipped to the progress
	int aBarWidth = Ftol((double)aScale * 799.0);
	int aFillWidth = Ftol((double)mSmoothProgress * aBarWidth);
	float anUpdateCnt = (float)mUpdateCnt;
	float aFill = (float)aFillWidth;
	int aFillScroll = Ftol(anUpdateCnt * 0.20000000298023224 + aFill) % aBarWidth;
	int anAddScroll = Ftol(anUpdateCnt * 0.5 + aFill) % aBarWidth;
	g->PushState();
	g->ClipRect(Ftol((double)aScale * 570.0), 0, aFillWidth, mHeight);
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_FILL_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_FILL_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_FILL, aX + aFillScroll, aY);
	}
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_FILL_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_FILL_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_FILL, aX - aBarWidth + aFillScroll, aY);
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_FILLADD_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_FILLADD_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_FILLADD, aX + anAddScroll, aY);
	}
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_FILLADD_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_FILLADD_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_FILLADD, aX - IMAGE_LOADINGSCREEN_FILLADD->mWidth + anAddScroll, aY);
	}
	g->PopState();
	{
		int aY = Ftol((double)GetImageOffsetY(IMAGE_LOADINGSCREEN_FILLEDGE_ID) * aScale);
		int aDX = Ftol((double)aScale * -12.0);
		int aLeft = Ftol((double)aScale * -570.0);
		g->DrawImage(IMAGE_LOADINGSCREEN_FILLEDGE, aDX - aLeft + aFillWidth, aY);
	}
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_DEVICE_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_DEVICE_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_DEVICE, aX, aY);
	}

	// The glows: the arc, then the bars one after another from update 450, then the arc again
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mArcAlpha.GetOutVal() * 255.0)));
	{
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_GLOWARC_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_GLOWARC_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_GLOWARC, aX, aY);
	}
	float aGlowTime = (float)((mUpdateCnt - 450) / 150.0);
	if (aGlowTime < 0.0f)
		aGlowTime = 0;
	struct GlowImage {
		Image* mImage;
		int mId;
		double mDelay;
		CurvedVal* mCurve;
	};
	const GlowImage aGlows[] = {
		{IMAGE_LOADINGSCREEN_GLOWBAR, IMAGE_LOADINGSCREEN_GLOWBAR_ID, 0, &mBarGlow},
		{IMAGE_LOADINGSCREEN_GLOWBAR2, IMAGE_LOADINGSCREEN_GLOWBAR2_ID, 0.05000000074505806, &mBarGlow},
		{IMAGE_LOADINGSCREEN_GLOWBAR3, IMAGE_LOADINGSCREEN_GLOWBAR3_ID, 0.10000000149011612, &mBarGlow},
		{IMAGE_LOADINGSCREEN_GLOWBAR4, IMAGE_LOADINGSCREEN_GLOWBAR4_ID, 0.15000000596046448, &mBarGlow},
		{IMAGE_LOADINGSCREEN_GLOWARC, IMAGE_LOADINGSCREEN_GLOWARC_ID, 0.4000000059604645, &mArcGlow},
	};
	for (const GlowImage& aGlow : aGlows) {
		aGlow.mCurve->SetInVal((double)aGlowTime - aGlow.mDelay);
		g->SetColor(Color(255, 255, 255, Ftol(aGlow.mCurve->GetOutVal() * 255.0)));
		int aY = Ftol((double)aScale * GetImageOffsetY(aGlow.mId));
		int aX = Ftol((double)aScale * GetImageOffsetX(aGlow.mId));
		g->DrawImage(aGlow.mImage, aX, aY);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	g->Translate(0, Ftol(-(mSlide.GetOutVal() * aScale)));
	if (mLoadingAlpha.GetOutVal() > 0.0) {
		g->SetColor(Color(255, 255, 255, Ftol(mLoadingAlpha.GetOutVal() * 255.0)));
		int aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_LOADINGSCREEN_LOADING_ID));
		int aX = Ftol((double)aScale * GetImageOffsetX(IMAGE_LOADINGSCREEN_LOADING_ID));
		g->DrawImage(IMAGE_LOADINGSCREEN_LOADING, aX, aY);
	}
}

void LoaderScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// The original also reads the property RegisterLinkY (380 by default) into a static nothing reads
	Widget::Resize(theX, theY, theWidth, theHeight);
}

void LoaderScreen::MouseDown(int x, int y, int theClickCount)
{
}

void LoaderScreen::ButtonDepress(int theId)
{
	// The loader's only button is the spiral, id 0; other ids (0x75f2da, or the app's slot 42 when +0x784 is set)
	// are not used here
	if (theId == 0)
		mPlayClicked = true;
}
