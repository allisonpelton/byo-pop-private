#include "TitleScreen.h"
#include "FossilizedApp.h"
#include "DDImage.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x52b3f8: the free plays of an unregistered copy (the property MaxPlays)
static int gMaxPlays;

// 0x438b20
TitleScreen::TitleScreen(FossilizedApp* theApp)
{
	mApp = theApp;
	mProgress = 0.0;
	mFakeProgress = 0.0;
	mLoadingDone = false;
	mRegisterClicked = false;
	mShowRegistration = !mApp->GetBoolean("NoReg", false);
	mRegisterTextIdx = 0;
	mRegisterTextCountdown = 400;
	gMaxPlays = mApp->GetInteger("MaxPlays", 15);
	mSlowness = 0;
	mCobrand = NULL;
	mCobranded = mApp->GetBoolean("Cobranded", false);
	if (mCobranded)
		mCobrand = new Pic(mApp, "images\\cobrand", true);

	int aTimes = mApp->mTimesExecuted;
	if (aTimes < 11)
		mSlowness = 10;
	else if (aTimes < 21)
		mSlowness = aTimes;
	else if (aTimes < 51)
		mSlowness = aTimes * 2 - 20;
	else
		mSlowness = 80;
	aTimes = mApp->mTimesExecuted;
	if (aTimes < 2)
		mFreePlaysShown = 0;
	else if (aTimes < 10)
		mFreePlaysShown = aTimes;
	else
		mFreePlaysShown = 2;
	mPlaysExpired = false;

	mRegisterLink = new HyperlinkWidget(0, this);
	mRegisterLink->SetFont(mApp->mWhiteFontSmall);
	mRegisterLink->mColor = Color(0xff, 0xff, 0);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	mPlaysExpired = (mApp->mTimesExecuted >= gMaxPlays) && !mApp->mIsRegistered;
	mRegisterLink->mLabel = mApp->GetString("REGISTER_DYNOMITE_FOR_FASTER_STARTUP");
	if (mPlaysExpired)
		mRegisterLink->mLabel = mApp->GetString("REGISTER_DYNOMITE_TO_CONTINUE_PLAYING");

	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mWhiteFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xff, 0xff, 0);
	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");

	mQuitLink = new HyperlinkWidget(2, this);
	mQuitLink->SetFont(mApp->mWhiteFont);
	mQuitLink->SetVisible(false);
	mQuitLink->mColor = Color(0xff, 0xff, 0);
	mQuitLink->mLabel = mApp->GetString("CLICK_HERE_TO_QUIT");
	mUnused90 = 0;
}

// 0x4388d0
TitleScreen::~TitleScreen()
{
	delete mRegisterLink;
	mRegisterLink = NULL;
	delete mPlayLink;
	mPlayLink = NULL;
	delete mCobrand;
	mCobrand = NULL;
}

// 0x438970
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
	theWidgetManager->AddWidget(mQuitLink);
}

// 0x4389b0
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
	theWidgetManager->RemoveWidget(mQuitLink);
}

// 0x4389f0: the links centred, the register link above the other two (which share a place)
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
	mRegisterLink->Resize((mWidth - aWidth) / 2, 0x173, aWidth, 0x10);
	aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b6, aWidth, 0x18);
	aWidth = mQuitLink->mFont->StringWidth(mQuitLink->mLabel);
	mQuitLink->Resize((mWidth - aWidth) / 2, 0x1b6, aWidth, 0x18);
}

// 0x4391a0
void TitleScreen::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	g->DrawImage(mApp->mImgTitle, 0, 0);
	Image* aBar = mApp->mImgGreenbar;
	g->DrawImage(aBar, 0x98, 0x196, Rect(0, 0, (int) (aBar->mWidth * mProgress), aBar->mHeight));
	g->SetFont(mApp->mWhiteFontSmall);
	if (mCobrand != NULL)
		mCobrand->Draw(0, 480 - mCobrand->mHeight);
	if (!mApp->mIsRegistered)
	{
		// port: left out (the free plays left, or that they have run out, and the unregistered banner)
	}
	else
	{
		g->SetColor(Color(0x80, 0xff, 0x80));
		if (mApp->mIsRegistered && mShowRegistration)
		{
			// the original prints the text through sprintf as a format with no arguments
			SexyString aText = mApp->GetString("THANKS_FOR_REGISTERING");
			int aWidth = mApp->mWhiteFontSmall->StringWidth(aText);
			g->SetColor(Color(0, 0, 0, 0xaf));
			int aHalf = aWidth / 2;
			g->FillRect(0x13b - aHalf, 0x16e, aWidth + 10, 0x17);
			g->SetColor(Color(0, 0, 0, 0xff));
			g->DrawString(aText, 0x142 - aHalf, 0x17f);
			g->SetColor(Color(0x80, 0xff, 0x80));
			g->DrawString(aText, 0x140 - aHalf, 0x17d);
		}
	}

	std::string aVersion = "Version " + mApp->mProductVersion;
	if (mApp->mTinyFont != NULL)
	{
		int aWidth = mApp->mTinyFont->Width(aVersion.c_str());
		g->SetColor(Color(0, 0, 0, 0xa0));
		g->FillRect(0x227, 0xad, aWidth + 10, 0x11);
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, 0xff));
		mApp->mTinyFont->Draw(aVersion.c_str(), 0x22d, 0xb3);
		g->SetColorizeImages(false);
		mApp->mTinyFont->Draw(aVersion.c_str(), 0x22c, 0xb2);
	}
	if (mPlayLink->mVisible == true)
	{
		g->SetColor(Color(0, 0, 0, 0x96));
		g->FillRect(0xbe, 0x1b0, 0x104, 0x28);
	}
	if (mRegisterLink->mVisible == true)
	{
		int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
		g->SetColor(Color(0, 0, 0, 0x96));
		g->FillRect(0x13b - (aWidth + 10) / 2, 0x170, aWidth + 0x14, 0x19);
	}
	if (mQuitLink->mVisible == true)
	{
		int aWidth = mQuitLink->mFont->StringWidth(mQuitLink->mLabel);
		g->SetColor(Color(0, 0, 0, 0x96));
		g->FillRect(0x13b - (aWidth + 10) / 2, 0x1b0, aWidth + 0x14, 0x28);
	}
}

// 0x439b20
void TitleScreen::Update()
{
	Widget::Update();
	if (!mApp->mIsRegistered)
	{
		// port: left out (every 400 updates the register link shows the next of four texts)
	}
	mPlaysExpired = (mApp->mTimesExecuted >= gMaxPlays) && !mApp->mIsRegistered;
	bool aQuitVisible = mQuitLink->mVisible;
	if (mPlaysExpired == true)
	{
		if (!aQuitVisible)
		{
			mQuitLink->SetVisible(true);
			MarkDirty();
		}
	}
	else if (aQuitVisible == true)
	{
		mQuitLink->SetVisible(false);
		MarkDirty();
	}

	if (!mPlaysExpired)
	{
		mFakeProgress = 0.01 / (double) mSlowness + mFakeProgress;
		if (1.0 < mFakeProgress)
			mFakeProgress = 1.0;
		int aTimes = mApp->mTimesExecuted;
		int aPlays = (int) ((aTimes + 1) * mFakeProgress);
		if ((aPlays <= aTimes) && (mFreePlaysShown < aPlays))
		{
			mFreePlaysShown = aPlays;
			MarkDirty();
		}
		if (!mLoadingDone && (mUpdateCnt % 4 == 0))
		{
			if (!mApp->mIsRegistered)
				mProgress = (mApp->GetLoadingThreadProgress() <= mFakeProgress) ? mApp->GetLoadingThreadProgress() : mFakeProgress;
			else
				mProgress = mApp->GetLoadingThreadProgress();
			if ((mProgress == 1.0) && mApp->mLoadingThreadCompleted)
			{
				mPlayLink->SetVisible(true);
				mLoadingDone = true;
				mProgress = 1.0;
			}
			MarkDirty();
		}
	}
	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	if (mRegisterClicked)
	{
		// port: the register dialog (0x4185a0) is not ported; the link only shows in an unregistered copy
		mRegisterClicked = false;
	}
}

// 0x438ab0
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		mApp->mSndEgglands->Play();
		mRegisterClicked = true;
		return;
	}
	if (theId == 2)
	{
		mApp->Shutdown();
		return;
	}
	mApp->mSndEgglands->Play();
	mApp->TitleScreenDone();
}
