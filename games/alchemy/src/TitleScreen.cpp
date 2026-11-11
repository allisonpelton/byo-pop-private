// The title screen (see TitleScreen.h).

#include "TitleScreen.h"
#include "WinAlchApp.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x4103e2
TitleScreen::TitleScreen(WinAlchApp* theApp)
{
	mApp = theApp;
	mProgress = 0;
	mDone = false;

	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(theApp->mAFont1);
	mPlayLink->mColor = Color(255, 255, 0);
	mPlayLink->SetVisible(false);
	mPlayLink->mLabel = theApp->GetString("CLICK_HERE_TO_PLAY");
}

// 0x41022c: deletes the links (the original's other one is the registration link, which the port does not have)
TitleScreen::~TitleScreen()
{
	delete mPlayLink;
}

// 0x410286
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4102b3
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4102e0: the link is centred under the bar
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1ad, aWidth, 0x18);
}

// 0x410622
void TitleScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_TITLE], 0, 0);

	// the bar: 321 pixels wide when full, drawn from (158, 372)
	Rect aBarRect(0, 0, (int) (mProgress * 321.0), 0x18);
	g->DrawImage(mApp->mImages[IMAGE_GREENBAR], 0x9e, 0x174, aBarRect);

	g->SetFont(mApp->mFont1);
	// (a registered copy: the original's "THANKS FOR REGISTERING!" is drawn only for a build that is not unlocked)

	g->SetColor(Color(255, 255, 0));
	SexyString aVersion = mApp->GetString("VERSION") + " " + mApp->mProductVersion;
	g->DrawString(aVersion, 0x219 - g->GetFont()->StringWidth(aVersion) / 2, 0xdd);
}

// 0x4109d8
void TitleScreen::Update()
{
	Widget::Update();

	if (mDone)
		return;

	if (mUpdateCnt % 4 == 0)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if ((mProgress == 1.0) && (mApp->mLoadingThreadCompleted))
		{
			mPlayLink->SetVisible(true);
			mProgress = 1.0;
			mDone = true;
		}
		MarkDirty();
	}
}

// 0x41035e: the play link
void TitleScreen::ButtonDepress(int theId)
{
	if (theId != 0)
		mApp->LoadingCompleted();
}
