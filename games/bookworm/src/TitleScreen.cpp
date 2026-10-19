#include "TitleScreen.h"
#include "ShadowFont.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

#include <math.h>
#include <stdlib.h>

using namespace Sexy;

// 0x434380
TitleScreen::TitleScreen(BookWormApp* theApp)
{
	mApp = theApp;

	mPlayFont = new ShadowFont(*mApp->mFonts[FONT_LOCKERGNOME14OUTLINE], Color(255, 0, 0, 255), Point(1, 1));
	mTextFont = new ShadowFont(*mApp->mFonts[FONT_VERDANA12BOLD], Color(0, 0, 0, 255), Point(1, 1));
	mVersionFont = new ShadowFont(*mApp->mFonts[FONT_VERDANA10BOLD], Color(0, 0, 0, 255), Point(1, 1));

	mMaxPlays = mApp->GetInteger("MaxPlays", 10);
	mLoadingBarY = mApp->GetInteger("LoadingBarY", 389);

	mLoadProgress = 0.0;
	mNagProgress = 0.0;
	mLoadDone = false;
	mWantRegister = false;
	mTrialExpired = false;
	mUnused8C = 0; // port: initialised (never used)

	if (!mApp->mIsRegistered && !mApp->mBuildUnlocked)
	{
		if (mApp->mTimesPlayed < 11)
			mNagSeconds = 10;
		else
			mNagSeconds = mApp->mTimesPlayed + 5;
	}
	else
		mNagSeconds = 1;

	// The play count shown by the trial texts; drops back to 2 from 10 plays on (original quirk).
	if (mApp->mMaxTime < 1)
	{
		if (mApp->mTimesPlayed < 2)
			mShownTimesPlayed = 0;
		else if (mApp->mTimesPlayed < 10)
			mShownTimesPlayed = mApp->mTimesPlayed;
		else
			mShownTimesPlayed = 2;
	}
	else
		mShownTimesPlayed = mApp->mTimesPlayed;

	mRegisterLink = new HyperlinkWidget(REGISTER_LINK_ID, this);
	mRegisterLink->mUnderlineSize = 0;
	mRegisterLink->SetFont(mTextFont);
	mRegisterLink->mColor = Color(32, 255, 32);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	mRegisterLink->mDoFinger = true;

	// The over colour stays the framework's white.
	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetFont(mPlayFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(255, 255, 0);
	mPlayLink->mDoFinger = true;

	mLogoAngle = 0.0;
	mLogoY = mApp->mImages[IMAGE_LOGO_TITLE]->GetHeight() * -4;
	mBarAngle = 0.0;
	mBarDY = mApp->mHeight * 2;

	// Sets the link labels and positions (mUpdateCnt becomes 1); mWidth is still 0 here.
	Update();
}

// 0x433300: also frees the images only the title screen uses
TitleScreen::~TitleScreen()
{
	delete mPlayFont;
	delete mTextFont;
	delete mVersionFont;
	delete mRegisterLink;
	delete mPlayLink;

	static const int aTitleImages[] = { IMAGE_TITLE_LOADMETER, IMAGE_BACKDROP_TITLE_LOADMETER, IMAGE_BACKDROP_TITLE,
		IMAGE_LOGO_TITLE, IMAGE_TITLE_STAMP_REGISTERED, IMAGE_TITLE_STAMP_TRIAL };
	for (int i = 0; i < (int)(sizeof(aTitleImages) / sizeof(aTitleImages[0])); i++)
	{
		if (mApp->mImages[aTitleImages[i]] != NULL)
		{
			delete mApp->mImages[aTitleImages[i]];
			mApp->mImages[aTitleImages[i]] = NULL;
		}
	}
}

// 0x433470: the links are top-level widgets above the screen
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4334b0
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4334f0 (shared jump to Widget::Resize)
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
}

// 0x401450 (shared with BWGameSelector)
void TitleScreen::ButtonPress(int theId, int theClickCount)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x433500
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == REGISTER_LINK_ID)
	{
		mWantRegister = true;
		return;
	}

	// The play link (the trial-expired build would quit instead; mTrialExpired is never set when registered)
	if (mTrialExpired)
		mApp->Shutdown();
	else
		mApp->TitleScreenDone();
}

// 0x433e80
void TitleScreen::Update()
{
	Widget::Update();

	// The trial nag timer still runs; it only matters to the unregistered texts.
	mNagProgress = 0.01 / (double)mNagSeconds + mNagProgress;
	if (mNagProgress > 1.0)
		mNagProgress = 1.0;

	if (mApp->mMaxTime == 0)
	{
		int aShownCount = (int)((double)mApp->mTimesPlayed * mNagProgress);
		if ((aShownCount <= mApp->mTimesPlayed) && (aShownCount > mShownTimesPlayed))
		{
			mShownTimesPlayed = aShownCount;
			MarkDirty();
		}
	}

	// Green pulse of the (hidden) register link
	if (mUpdateCnt % 8 == 0)
	{
		int aGreenBlue = 32 - abs((mUpdateCnt * 4) % 32 - 16);
		int aGreen = 255 - abs((mUpdateCnt * 4) % 382 - 191);
		mRegisterLink->mColor = Color(aGreenBlue, aGreen, aGreenBlue);
		MarkDirty();
	}

	// The logo slides down from update 25 (port: sin() for the original's x87 fsin)
	if ((mUpdateCnt >= 25) && (mLogoY != 0))
	{
		mLogoAngle += 0.016;
		if (mLogoAngle < 1.570795)
			mLogoY = -(int)(mApp->mImages[IMAGE_LOGO_TITLE]->GetHeight() * (1.0 - sin(mLogoAngle)));
		else
			mLogoY = 0;
		MarkDirty();
	}

	// The loading bar slides up from update 50; the register link rides with it
	if ((mUpdateCnt >= 50) && (mBarDY != 0))
	{
		mBarAngle += 0.025;
		if (mBarAngle < 1.570795)
			mBarDY = (int)((mApp->mImages[IMAGE_BACKDROP_TITLE_LOADMETER]->GetHeight() * 3) * (1.0 - sin(mBarAngle)));
		else
			mBarDY = 0;

		int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
		mRegisterLink->Resize(331 - (aWidth + 4) / 2, (mLoadingBarY - mRegisterLink->mHeight) - 10 + mBarDY, aWidth + 4, 22);
		MarkDirty();
	}

	// 0x4340c8: the trial expiry (only when !mBuildUnlocked: the play limit or time limit reached hides both
	// links and, from update 250, shows dialog 4) is not ported (registered build).

	// The play link under the bar (331 = the bar's centre)
	mPlayLink->mLabel = GameText(GT_TITLE_CLICK_TO_PLAY);
	int aPlayWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	int aPlayHeight = mPlayLink->mFont->GetHeight() + 4;
	mPlayLink->Resize(331 - (aPlayWidth + 8) / 2, mApp->mImages[IMAGE_TITLE_LOADMETER]->GetHeight() + mLoadingBarY,
		aPlayWidth + 8, aPlayHeight);

	mRegisterLink->mLabel = GameText(GT_TITLE_REGISTER_NOW);
	if (!mApp->mBuildUnlocked && mWantRegister && !mApp->mIsRegistered)
		mRegisterLink->mLabel = GameText(GT_TITLE_ONE_MOMENT);

	if (!mLoadDone && (mUpdateCnt % 4 == 0))
	{
		mLoadProgress = mApp->GetLoadingThreadProgress();
		if ((mLoadProgress == 1.0) && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadDone = true;
			mLoadProgress = 1.0;
		}
		MarkDirty();
	}

	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	// 0x434320: if (mInitDone && mWantRegister) { mRegSource = "title"; ShowRegisterDialog(); mWantRegister =
	// false; } not ported (registration; the register link is hidden when registered).

	mApp->mTimedOut = mTrialExpired;
}

// 0x433570
void TitleScreen::DrawLoadingBar(Graphics* g)
{
	Image* aFill = mApp->mImages[IMAGE_TITLE_LOADMETER];
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP_TITLE_LOADMETER], 137, mLoadingBarY + mBarDY);

	if (mLoadProgress >= 1.0)
	{
		g->DrawImage(aFill, 137, mLoadingBarY + mBarDY);
	}
	else if (mLoadProgress > 0.0)
	{
		int aWidth = 25 - (int)(mLoadProgress * -348.0);
		int aHeight = aFill->GetHeight();
		g->DrawImage(aFill, 137, mLoadingBarY + mBarDY, Rect(0, 0, aWidth, aHeight));

		// A 16-column soft edge: alpha cos(i * pi/32) * 255 (port: cos() for the original's x87 fcos)
		for (int i = 0; i < 16; i++)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(cos((double)i * 0.0981746875) * 255.0)));
			g->DrawImage(aFill, aWidth + 137 + i, mLoadingBarY + mBarDY, Rect(aWidth + i, 0, 1, aHeight));
			g->SetColorizeImages(false);
		}
	}
}

// 0x433900
void TitleScreen::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;

	g->DrawImage(anImages[IMAGE_BACKDROP_TITLE], 0, 0);
	g->DrawImage(anImages[IMAGE_LOGO_TITLE], 35, mLogoY);

	// The registered / trial stamp below the logo (ShowTrialStatus is False in the retail partner.xml)
	if (mApp->GetBoolean("ShowTrialStatus", true))
	{
		if (mApp->mIsRegistered)
		{
			Image* aStamp = anImages[IMAGE_TITLE_STAMP_REGISTERED];
			g->DrawImage(aStamp, (mWidth - aStamp->GetWidth()) - 15, anImages[IMAGE_LOGO_TITLE]->GetHeight() - 80 + mLogoY);
		}
		else
		{
			Image* aStamp = anImages[IMAGE_TITLE_STAMP_TRIAL];
			g->DrawImage(aStamp, mWidth - aStamp->GetWidth(), anImages[IMAGE_LOGO_TITLE]->GetHeight() - 60 + mLogoY);
		}
	}

	if (!mTrialExpired)
		DrawLoadingBar(g);

	// The status line above the bar. The original formats it into a 256-byte buffer with sprintf.
	g->SetFont(mApp->mFonts[FONT_VERDANA10BOLD]);
	SexyString aText = GameText(GT_TITLE_WELCOME);
	int aTextY;
	if (!mApp->mBuildUnlocked)
		aTextY = mRegisterLink->mY - 16;
	else
		aTextY = mLoadingBarY - 32;

	if (mApp->mIsRegistered)
	{
		g->SetFont(mApp->mFonts[FONT_VERDANA14BOLD]);
		if (!mApp->mBuildUnlocked)
			aText = GameText(GT_TITLE_THANKS_FOR_REGISTERING);
		aTextY = mLoadingBarY - 16;
	}
	// 0x433ac4: else a 25-second cycle (mUpdateCnt % 2500) of unregistered texts (the trial status, plays
	// or minutes left, reasons to register) at aTextY; not ported (registered build).

	if ((mApp->mTimesPlayed == 0) && (mLoadProgress < 1.0))
		aText = GameText(GT_TITLE_UNPACKING_SOUNDS);

	if (mApp->GetDialog(DIALOG_TRIAL_EXPIRED) == NULL)
	{
		// The shadowed font replaces the one chosen above.
		g->SetFont(mTextFont);
		g->SetColor(Color(255, 255, 255));
		if (!mTrialExpired)
		{
			int aWidth = g->GetFont()->StringWidth(aText);
			g->DrawString(aText, 331 - aWidth / 2, mBarDY + aTextY);
		}

		SexyString aVersion = GameText(GT_TITLE_VERSION) + mApp->mProductVersion;
		g->SetFont(mVersionFont);
		g->DrawString(aVersion, (mWidth - g->GetFont()->StringWidth(aVersion)) - 35, mLogoY + 260);
	}
}
