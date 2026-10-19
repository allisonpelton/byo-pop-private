#include "TitleScreen.h"
#include "WinBMApp.h"
#include "Res.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

// 0x41b120
TitleScreen::TitleScreen(WinBMApp* theApp)
{
	mApp = theApp;
	mCountdownTrial = true;

	// 0x41b163..0x41b293: mMaxPlays from mVariation ("CntDn30"/"CntUp30" 30, "CntDn10"/"CntUp10" 10, else
	// GetInteger("MaxPlays", 10)) not ported: it only feeds the trial expiry and the unregistered texts.
	mMaxPlays = 0;

	mLoadProgress = 0.0;
	mNagProgress = 0.0;
	mLoadDone = false;
	mWantRegister = false;
	mTrialExpired = false;

	if (mApp->mTimesPlayed > 10)
		mNagSeconds = mApp->mTimesPlayed + 5;
	else
		mNagSeconds = 10;
	if (mApp->GetBoolean("RealArcade", false))
		mNagSeconds = 0;

	// Drops back to 2 from 10 plays on (original quirk; the number is only shown unregistered).
	if (mApp->mTimesPlayed < 2)
		mShownTimesPlayed = 0;
	else if (mApp->mTimesPlayed < 10)
		mShownTimesPlayed = mApp->mTimesPlayed;
	else
		mShownTimesPlayed = 2;

	mRegisterLink = new HyperlinkWidget(REGISTER_LINK_ID, this);
	mRegisterLink->mUnderlineSize = 0;
	mRegisterLink->SetFont(mApp->mFont3OutlineUnderlined);
	mRegisterLink->mColor = Color(32, 255, 32);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetFont(mApp->mFont1OutlineUnderlined);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(32, 255, 32);

	mStripScroll = 0;
	mLogoAngle = 0.0;
	mLogoY = -mApp->mImages[IMAGE_BIGMONEYLOGO]->GetHeight();
	mMoneybagsAngle = 0.0;
	mMoneybagsDY = mApp->mImages[IMAGE_MRMONEYBAGS]->GetHeight();
	mBarAngle = 0.0;
	mBarDY = mApp->mImages[IMAGE_BARANDLOGO]->GetHeight();
	mCigarPhase = 0.0;
	mEyePhase = 0.0;

	// The "Deluxe" badge: an HSL copy of IMAGE_DELUXOR whose luminosity Draw raises by mDeluxeLum.
	MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(mApp->mImages[IMAGE_DELUXOR]);
	mDeluxeImage = new MemoryImage(mApp);
	mDeluxeImage->Create(aSrcImage->GetWidth(), aSrcImage->GetHeight());
	mDeluxeImage->SetImageMode(true, true);
	mDeluxeImage->SetVolatile(true);
	int aSize = mDeluxeImage->GetWidth() * mDeluxeImage->GetHeight();
	mDeluxeHSL = new ulong[aSize];
	mApp->RGBToHSL(aSrcImage->GetBits(), mDeluxeHSL, aSize);
	mDeluxeWork = new ulong[aSize];
	mDeluxeLum = 256;
	mDeluxeDirty = false; // port: left uninitialised by the original (only read after Update has set it)
	mDeluxeShown = false;

	// Sets the link labels and moves Mr. Moneybags one step (mUpdateCnt becomes 1); mWidth is still 0 here.
	Update();
}

// 0x419630
TitleScreen::~TitleScreen()
{
	delete mRegisterLink;
	delete mPlayLink;
	delete[] mDeluxeHSL;
	delete[] mDeluxeWork;
	delete mDeluxeImage;
}

// 0x4196d0
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x419700
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x412c50 (COMDAT shared with HighScoreWidget: a jump to Widget::Resize)
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
}

// 0x419730
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == REGISTER_LINK_ID)
		mWantRegister = true;
	else
		mApp->TitleScreenIsFinished(); // trial expired (unregistered only): the quit link calls mApp->Shutdown() instead
}

// 0x41a810
void TitleScreen::Update()
{
	Widget::Update();

	bool aRealArcade = mApp->GetBoolean("RealArcade", false);

	if (mUpdateCnt % 8 == 0)
	{
		mCigarPhase += 0.05;
		if (mCigarPhase > 1.0)
			mCigarPhase -= 1.0;

		mEyePhase += 0.06;
		if (mEyePhase > 1.0)
			mEyePhase -= 1.0;

		if (mStripScroll == 0)
			mStripScroll = 64;
		else
			mStripScroll--;

		if (mUpdateCnt >= 180)
			mDeluxeShown = true;

		if (mDeluxeShown && mDeluxeLum > 0)
		{
			mDeluxeLum -= 16;
			if (mDeluxeLum < 0)
				mDeluxeLum = 0;
			mDeluxeDirty = true;
		}

		MarkDirty();
	}

	// port: sin() for the original's x87 fsin (same doubles; may differ only at an exact truncation boundary).
	if (mUpdateCnt >= 25 && mLogoY != 0)
	{
		mLogoAngle += 0.016;
		if (mLogoAngle < 1.570795)
			mLogoY = -(int)(mApp->mImages[IMAGE_BIGMONEYLOGO]->GetHeight() * (1.0 - sin(mLogoAngle)));
		else
			mLogoY = 0;
		MarkDirty();
	}

	if (mMoneybagsDY != 0)
	{
		mMoneybagsAngle += 0.02;
		if (mMoneybagsAngle < 1.570795)
			mMoneybagsDY = (int)(mApp->mImages[IMAGE_MRMONEYBAGS]->GetHeight() * (1.0 - sin(mMoneybagsAngle)));
		else
			mMoneybagsDY = 0;
		MarkDirty();
	}

	if (mUpdateCnt >= 50 && mBarDY != 0)
	{
		mBarAngle += 0.025;
		if (mBarAngle < 1.570795)
			mBarDY = (int)(mApp->mImages[IMAGE_BARANDLOGO]->GetHeight() * (1.0 - sin(mBarAngle)));
		else
			mBarDY = 0;

		// The register link (hidden when registered) rides up with the bar.
		int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel) + 4;
		mRegisterLink->Resize(308 - aWidth / 2, mBarDY + 374, aWidth, 26);
		MarkDirty();
	}

	// 0x41ab41: trial expiry (!RealArcade && mTimesPlayed >= mMaxPlays && !mIsRegistered -> mTrialExpired,
	// mWantRegister) not ported: never true when registered.
	// 0x41aef2: the expired branch (CLICK_HERE_TO_QUIT play link at update 1000, CLICK_HERE_TO_REGISTER_NOW)
	// not ported for the same reason.

	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
	int aPlayWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel) + 8;
	mPlayLink->Resize(308 - aPlayWidth / 2, 435, aPlayWidth, 32);

	mRegisterLink->mLabel = mApp->GetString(aRealArcade ? "CLICK_HERE_TO_REGISTER_NOW" : "REGISTER_NOW_TO_UNLOCK_THIS_GAME");

	// The nag timer still runs; it only limits the bar when unregistered. mNagSeconds 0 gives +inf.
	mNagProgress = 0.01 / (double)mNagSeconds + mNagProgress;
	if (mNagProgress > 1.0)
		mNagProgress = 1.0;

	int aShownCount = (int)((double)mApp->mTimesPlayed * mNagProgress);
	if (aShownCount <= mApp->mTimesPlayed && aShownCount > mShownTimesPlayed)
	{
		mShownTimesPlayed = aShownCount;
		MarkDirty();
	}

	if (!mLoadDone && mUpdateCnt % 4 == 0)
	{
		// Unregistered, the bar shows min(progress, mNagProgress) instead (not ported).
		mLoadProgress = mApp->GetLoadingThreadProgress();
		if (mLoadProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadDone = true;
			mLoadProgress = 1.0;
		}
		MarkDirty();
	}

	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	// 0x41b0d7: if (mLoaded && mWantRegister) { ShowRegisterDialog(); mWantRegister = false; } not ported
	// (registration; the register link is never visible when registered).

	mApp->mTimedOut = mTrialExpired;
}

// 0x419950
void TitleScreen::Draw(Graphics* g)
{
	// The original also calls GetBoolean("RealArcade", false) here and ignores the result.
	Image** anImages = mApp->mImages;

	g->DrawImage(anImages[IMAGE_TITLEBACK], 0, 0);

	// Rows of dollar signs, every other row shifted 13 px, scrolling up 1 px per 8 updates (period 64).
	for (int i = 0; i < (mHeight + 31) / 32 + 2; i++)
		g->DrawImage(anImages[IMAGE_DOLLARSTRIP], (i % 2 != 0) ? 13 : 0, mStripScroll + 32*i - 64);

	Image* aMoneybags = anImages[IMAGE_MRMONEYBAGS];
	g->DrawImage(aMoneybags, 0, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 6);

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(fabs(mCigarPhase - 0.5) * 510.0)));
	g->DrawImage(anImages[IMAGE_TITLECIGARTIP], 26, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 184);
	g->SetColorizeImages(false);

	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(0, 200, 0, (int)(fabs(mEyePhase - 0.5) * 510.0)));
	g->DrawImage(anImages[IMAGE_TITLEEYEGLOW], 122, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 90);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);

	Image* aBar = anImages[IMAGE_BARANDLOGO];
	g->DrawImage(aBar, mWidth - aBar->GetWidth(), mHeight + mBarDY - aBar->GetHeight(), Rect(0, 0, 540, 149));

	// Rows 125..129 of the bar drawn once more with fading alphas (212, 170, 127, 85, 42).
	int aRow = 0;
	for (int anAlpha = 1275; anAlpha > 0; anAlpha -= 255)
	{
		Graphics aRowG(*g);
		aRowG.SetColorizeImages(true);
		aRowG.SetColor(Color(255, 255, 255, anAlpha / 6));
		aRowG.ClipRect(mWidth - aBar->GetWidth(), mHeight + mBarDY + aRow - aBar->GetHeight() + 125, 540, 1);
		aRowG.DrawImage(aBar, mWidth - aBar->GetWidth(), mHeight + mBarDY - aBar->GetHeight());
		aRowG.SetColorizeImages(false);
		aRow++;
	}

	g->DrawImage(anImages[IMAGE_BIGMONEYLOGO], 84, mLogoY - 8);

	if (mApp->mCobrandImage != NULL)
		g->DrawImage(mApp->mCobrandImage, 8, mHeight + mBarDY - mApp->mCobrandImage->GetHeight());
	// 0x419dd1: else, unregistered only, IMAGE_TRIALVERSION at (0, mHeight + mBarDY - its height): not ported.

	Image* aGoldBar = anImages[IMAGE_GOLDBAR];
	if (mLoadProgress >= 1.0)
	{
		g->DrawImage(aGoldBar, 120, mBarDY + 381);
	}
	else if (mLoadProgress > 0.0)
	{
		int aBarWidth = (int)(mLoadProgress * 318.0 + 25.0);
		g->DrawImage(aGoldBar, 120, mBarDY + 381, Rect(0, 0, aBarWidth, 76));

		// A 16-column soft edge: alpha cos(i * pi/32) * 255 (port: cos() for the original's x87 fcos).
		for (int i = 0; i < 16; i++)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(cos((double)i * 0.0981746875) * 255.0)));
			g->DrawImage(aGoldBar, aBarWidth + i + 120, mBarDY + 381, Rect(aBarWidth + i, 0, 1, 76));
			g->SetColorizeImages(false);
		}
	}

	if (mDeluxeShown)
	{
		if (mDeluxeDirty)
		{
			ulong* aBits = mDeluxeImage->GetBits();
			int aSize = mDeluxeImage->GetWidth() * mDeluxeImage->GetHeight();
			for (int i = 0; i < aSize; i++)
			{
				ulong aHSL = mDeluxeHSL[i];
				int aLum = (int)((aHSL >> 16) & 0xff) + mDeluxeLum;
				if (aLum > 255)
					mDeluxeWork[i] = aHSL | 0xff0000;
				else
					mDeluxeWork[i] = (aHSL & 0xff00ffff) | ((ulong)aLum << 16);
			}
			mApp->HSLToRGB(mDeluxeWork, aBits, aSize);
			mDeluxeImage->BitsChanged();
			mDeluxeDirty = false;
		}
		g->DrawImage(mDeluxeImage, 478, 18);
	}

	// Status line. Registered path only: THANKS_FOR_REGISTERING unless the build is unlocked (empty on
	// Steam). 0x41a072: the unregistered texts at y 370 (plays remaining, registration required,
	// UNREGISTERED_VERSION, YOU_HAVE_PLAYED_X_TIMES_WITHOUT_REGISTERING) are not ported.
	g->SetFont(mApp->mFont3Outline);
	SexyString aText;
	if (!mApp->mBuildUnlocked)
		aText = mApp->GetString("THANKS_FOR_REGISTERING");
	int aTextY = 397;

	if (mApp->mTimesPlayed == 0 && mLoadProgress < 1.0)
		aText = mApp->GetString("PLEASE_WAIT");

	// port: the original formats these through sprintf(buf, text) with no arguments; neither string has a '%'.
	int aTextWidth = g->GetFont()->StringWidth(aText);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(aText, 308 - aTextWidth / 2, mBarDY + aTextY);

	g->SetFont(mApp->mFont3);
	g->SetColor(Color(255, 255, 255));
	SexyString aVersion = mApp->GetString("VERSION_") + _S(" ") + StringToSexyStringFast(mApp->mProductVersion);
	g->DrawString(aVersion, mWidth - g->GetFont()->StringWidth(aVersion) - 16, 211);
}
