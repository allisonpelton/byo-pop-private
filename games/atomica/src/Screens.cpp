// The full-screen widgets of Atomica Deluxe (see Screens.h).

#include "Screens.h"
#include "AtomicaApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Support.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// TitleScreen

// 0x42fe90
TitleScreen::TitleScreen(AtomicaApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mShown(0.0),
	mHighest(0),
	mLinkShown(false),
	mClicked(false),
	mSlide(-800.0f),
	mBlink(0)
{
	mTitle = mApp->mSprites->Load("titlescreen");
	mBar = mApp->mSprites->Load("progressbar");
	mLogo = mApp->mSprites->Load("atomicatitle");
	mDeluxe = mApp->mSprites->Load("deluxe");
	mPopCapLogo = mApp->mSprites->Load("popcaplogo");
	mPopCapUrl = mApp->mSprites->Load("popcapurl");
	mSponsor = mApp->mSprites->Load("sponsor");

	// port: the shareware branches (the free plays, the register link) are not ported; the screen always takes the
	// registered path (docs/GAME_NOTES.md section 3)
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mAtomicFont);
	mPlayLink->mColor = Color(0xff, 0xff, 0);
	mPlayLink->mOverColor = Color(0xff, 0xff, 0);
	mPlayLink->mLabel = "";
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetVisible(false);
	mHasTransparencies = false;
}

TitleScreen::~TitleScreen()
{
	delete mPlayLink;
}

// 0x42fb30 / 0x42fb70
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x42fbb0
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b1, aWidth, 0x18);
}

// 0x430bd0
void TitleScreen::Update()
{
	Widget::Update();

	float aSlide = mSlide;
	mSlide = aSlide * 0.9f;
	if (-3.0f < aSlide * 0.9f)
		mSlide = 0;

	// the bar's fill follows the loading
	if (mUpdateCnt % 4 == 0)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if ((mProgress == 1.0) && mApp->mLoaded && !mLinkShown)
		{
			mPlayLink->mLabel = GT(0x004b2cc0);
			mPlayLink->SetVisible(true);
			mLinkShown = true;
			mProgress = 1.0;
			int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
			mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b1, aWidth, 0x18);
		}
		MarkDirty();
	}
	MarkDirty();
}

// 0x430450
void TitleScreen::Draw(Graphics* g)
{
	mTitle->Draw(g, 0, 0);
	mLogo->Draw(g, 0x10, 8);
	mDeluxe->Draw(g, (int) (mSlide + 370.0f), 0x46);
	mPopCapLogo->Draw(g, 0x280 - mPopCapLogo->mWidth, 0x1d6 - mPopCapLogo->mHeight);
	mPopCapUrl->Draw(g, 0x282 - mPopCapLogo->mWidth, 0x1db - mPopCapUrl->mHeight);
	mSponsor->Draw(g, 0, 0x1db - mSponsor->mHeight);

	// the version, in a dark box at the right
	std::string aVersion = GT(0x004b2e08) + mApp->mProductVersion;
	g->SetFont(mApp->mAtomicFont);
	g->SetColor(Color(0, 0, 0, 200));
	int aWidth = mApp->mAtomicFont->StringWidth(aVersion);
	g->FillRect(0x218 - aWidth, 0x78, aWidth + 6, 0x13);
	g->SetColor(Color(0, 0, 0, 255));
	g->DrawString(aVersion, 0x21f - aWidth, 0x8a);
	g->SetColor(Color(0, 255, 0, 255));
	g->DrawString(aVersion, 0x21c - aWidth, 0x87);

	// registered: "thanks for registering" in green over a dark box
	{
		std::string aLine = GT(0x004b2df0);
		g->SetColor(Color(0x80, 0xff, 0x80));
		int aLineWidth = mApp->mAtomicFont->StringWidth(aLine);
		g->SetColor(Color(0, 0, 0, 0xaf));
		g->FillRect(0x13b - aLineWidth / 2, 0x164, aLineWidth + 10, 0x17);
		g->SetColor(Color(0, 0, 0, 255));
		g->DrawString(aLine, 0x142 - aLineWidth / 2, 0x177);
		g->SetColor(Color(0x80, 0xff, 0x80));
		g->DrawString(aLine, 0x140 - aLineWidth / 2, 0x175);
	}

	// the bar
	mBar->DrawPartial(g, 0x9a, 0x17f, (float) mProgress, 1, 0);

	// the line under it: what is being loaded
	if (mPlayLink->mVisible)
	{
		g->SetColor(Color(0, 0, 0, 0xaf));
		g->FillRect(mPlayLink->mX - 5, mPlayLink->mY - 5, mPlayLink->mWidth + 10, mPlayLink->mHeight + 10);
	}
	else
	{
		g->SetColor(Color(0, 0, 0, 255));
		int aTextWidth = mApp->mAtomicFont->StringWidth(mApp->mLoadingText);
		g->DrawString(mApp->mLoadingText, 0x142 - aTextWidth / 2, 0x1ce);
		g->SetColor(Color(255, 255, 0, 255));
		g->DrawString(mApp->mLoadingText, 0x140 - aTextWidth / 2, 0x1cc);
	}
}

// 0x42fc40: the play link starts the game's menus
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == 0)
		mClicked = true;
	else
		mApp->TitleScreenDone();
}
