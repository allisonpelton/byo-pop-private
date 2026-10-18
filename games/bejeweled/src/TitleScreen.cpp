#include "TitleScreen.h"
#include "WinDMApp.h"
#include "Res.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

// 0x4d41c8
TitleScreen::TitleScreen(WinDMApp* theApp)
{
	mLoadingProgress = 0.0;
	mDelayProgress = 0.0;
	mApp = theApp;
	mLoadingComplete = false;
	mRegisterClicked = false;

	int aTimesPlayed = mApp->mTimesPlayed;
	if (aTimesPlayed <= 10)
		mTotalTimerSec = 10;
	else if (aTimesPlayed <= 20)
		mTotalTimerSec = aTimesPlayed;
	else if (aTimesPlayed <= 50)
		mTotalTimerSec = aTimesPlayed*2 - 20;
	else
		mTotalTimerSec = 80;
	mTotalTimerSec = mApp->GetInteger("TotalTimerSec", mTotalTimerSec);

	// Drops back to 2 from 10 plays on (original quirk; the counter has no visible effect).
	if (mApp->mTimesPlayed < 2)
		mDisplayedPlayCount = 0;
	else if (mApp->mTimesPlayed < 10)
		mDisplayedPlayCount = mApp->mTimesPlayed;
	else
		mDisplayedPlayCount = 2;

	mRegisterLink = new HyperlinkWidget(REGISTER_LINK_ID, this);
	mRegisterLink->SetFont(mApp->mMainFont);
	mRegisterLink->mColor = Color(32, 255, 32);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->SetFont(mApp->mHeaderFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(32, 255, 32);

	// Sets the link labels (the register link's width in Resize depends on it); mWidth is still 0 here.
	Update();
}

// 0x4d360b
TitleScreen::~TitleScreen()
{
	delete mRegisterLink;
	delete mPlayLink;
}

// 0x4d3661
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4d3691
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4d36c1
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == REGISTER_LINK_ID)
		mRegisterClicked = true;
	else
		mApp->TitleScreenIsFinished(); // the nag mode's quit link (mApp->Shutdown()) is unregistered only
}

// 0x4d370b
void TitleScreen::Draw(Graphics* g)
{
	static int sLoadingBarY = mApp->GetInteger("LoadingBarY", 275);

	g->DrawImage(mApp->mImages[IMAGE_TITLESCREEN], 0, 0);
	// Unregistered only: the trial circle IMAGE_TRIALCIRCLE at (31, 346), unless the property Cobranded is set.

	Image* aBarImage = mApp->mImages[IMAGE_TITLEGREENBAR];
	g->DrawImage(aBarImage, LOADING_BAR_X, sLoadingBarY, Rect(0, 0, (int)(mLoadingProgress * LOADING_BAR_WIDTH), aBarImage->GetHeight()));

	g->SetFont(mApp->mMainFont);
	// Only when !mBuildUnlocked: the magenta (255,64,255) status line (THANKS_FOR_REGISTERING, plays left or YOU_MUST_REG*) centred at y RegisterStringY (319).

	g->SetColor(Color(128, 128, 128));
	SexyString aVersion = mApp->GetString("VERSION") + _S(" ") + StringToSexyStringFast(mApp->mProductVersion);
	g->DrawString(aVersion, 537 - g->GetFont()->StringWidth(aVersion)/2, 221);
}

// 0x4d3c6a
void TitleScreen::Update()
{
	static int sPlayLinkY = mApp->GetInteger("PlayLinkY", 386);

	Widget::Update();

	// Unregistered only: the trial-expired nag mode (MaxPlays/MaxExecutions; mTimedOut, frozen bar, CLICK_HERE_TO_QUIT at tick 1000).

	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth)/2, sPlayLinkY, aWidth, 24);

	mRegisterLink->mLabel = mApp->GetString("REGISTER_NOW_TO_REMOVE_THIS_DELAY");

	mDelayProgress = 0.01/(double)mTotalTimerSec + mDelayProgress;
	if (mDelayProgress > 1.0)
		mDelayProgress = 1.0;

	int aPlayCount = (int)((double)(mApp->mTimesPlayed + 1) * mDelayProgress);
	if (aPlayCount <= mApp->mTimesPlayed && aPlayCount > mDisplayedPlayCount)
	{
		mDisplayedPlayCount = aPlayCount;
		MarkDirty();
	}

	if (!mLoadingComplete && mUpdateCnt % 4 == 0)
	{
		// Unregistered, the bar shows min(progress, mDelayProgress) instead.
		mLoadingProgress = mApp->GetLoadingThreadProgress();
		if (mLoadingProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadingProgress = 1.0;
			mLoadingComplete = true;
		}
		MarkDirty();
	}

	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	if (mApp->mLoaded && mRegisterClicked)
	{
		mApp->ShowRegisterDialog();
		mRegisterClicked = false;
	}
}

// 0x4d4118
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	static int sRegisterLinkY = mApp->GetInteger("RegisterLinkY", 324);

	Widget::Resize(theX, theY, theWidth, theHeight);

	// Laid out once, with the label the constructor's Update() set.
	int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
	mRegisterLink->Resize((mWidth - aWidth)/2, sRegisterLinkY, aWidth, 16);
}
