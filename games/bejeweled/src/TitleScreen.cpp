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
	mApp = theApp;
	mLoadingComplete = false;

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->SetFont(mApp->mHeaderFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(32, 255, 32);

	// One update, as the original's constructor ran (it sets the play link's label and counts in mUpdateCnt, which paces
	// the bar's reads); mWidth is still 0 here.
	Update();
}

// 0x4d360b
TitleScreen::~TitleScreen()
{
	delete mPlayLink;
}

// 0x4d3661
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4d3691
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4d36c1
void TitleScreen::ButtonDepress(int theId)
{
	mApp->TitleScreenIsFinished();
}

// 0x4d370b
void TitleScreen::Draw(Graphics* g)
{
	static int sLoadingBarY = mApp->GetInteger("LoadingBarY", 275);

	g->DrawImage(mApp->mImages[IMAGE_TITLESCREEN], 0, 0);

	Image* aBarImage = mApp->mImages[IMAGE_TITLEGREENBAR];
	g->DrawImage(aBarImage, LOADING_BAR_X, sLoadingBarY, Rect(0, 0, (int)(mLoadingProgress * LOADING_BAR_WIDTH), aBarImage->GetHeight()));

	g->SetFont(mApp->mMainFont);

	g->SetColor(Color(128, 128, 128));
	SexyString aVersion = mApp->GetString("VERSION") + _S(" ") + StringToSexyStringFast(mApp->mProductVersion);
	g->DrawString(aVersion, 537 - g->GetFont()->StringWidth(aVersion)/2, 221);

	// port(variant): code-drawn art plays on without the game's sound files (features.codeArt), and says so
	if (mApp->mMissingSounds > 0)
	{
		SexyString aSilent = mApp->GetString("CODEART_SILENT", _S("No sound files found: playing silent"));
		g->DrawString(aSilent, (mWidth - g->GetFont()->StringWidth(aSilent)) / 2, 452);
	}
}

// 0x4d3c6a
void TitleScreen::Update()
{
	static int sPlayLinkY = mApp->GetInteger("PlayLinkY", 386);

	Widget::Update();

	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth)/2, sPlayLinkY, aWidth, 24);

	if (!mLoadingComplete && mUpdateCnt % 4 == 0)
	{
		mLoadingProgress = mApp->GetLoadingThreadProgress();
		if (mLoadingProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadingProgress = 1.0;
			mLoadingComplete = true;
		}
		MarkDirty();
	}
}
