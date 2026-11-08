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

	mLoadingBarY = mApp->GetInteger("LoadingBarY", 389);

	mLoadProgress = 0.0;
	mLoadDone = false;

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
	delete mPlayLink;

	static const int aTitleImages[] = { IMAGE_TITLE_LOADMETER, IMAGE_BACKDROP_TITLE_LOADMETER, IMAGE_BACKDROP_TITLE,
		IMAGE_LOGO_TITLE };
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
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4334b0
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
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
	// The play link
	mApp->TitleScreenDone();
}

// 0x433e80
void TitleScreen::Update()
{
	Widget::Update();


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

	// The loading bar slides up from update 50
	if ((mUpdateCnt >= 50) && (mBarDY != 0))
	{
		mBarAngle += 0.025;
		if (mBarAngle < 1.570795)
			mBarDY = (int)((mApp->mImages[IMAGE_BACKDROP_TITLE_LOADMETER]->GetHeight() * 3) * (1.0 - sin(mBarAngle)));
		else
			mBarDY = 0;

		MarkDirty();
	}


	// The play link under the bar (331 = the bar's centre)
	mPlayLink->mLabel = GameText(GT_TITLE_CLICK_TO_PLAY);
	int aPlayWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	int aPlayHeight = mPlayLink->mFont->GetHeight() + 4;
	mPlayLink->Resize(331 - (aPlayWidth + 8) / 2, mApp->mImages[IMAGE_TITLE_LOADMETER]->GetHeight() + mLoadingBarY,
		aPlayWidth + 8, aPlayHeight);


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

	DrawLoadingBar(g);

	// The status line above the bar (the original formats it into a 256-byte buffer with sprintf): the welcome,
	// or on a first run, while loading, the unpacking of the sounds
	SexyString aText = GameText(GT_TITLE_WELCOME);
	int aTextY = mLoadingBarY - 16;
	if ((mApp->mTimesPlayed == 0) && (mLoadProgress < 1.0))
		aText = GameText(GT_TITLE_UNPACKING_SOUNDS);

	g->SetFont(mTextFont);
	g->SetColor(Color(255, 255, 255));
	int aWidth = g->GetFont()->StringWidth(aText);
	g->DrawString(aText, 331 - aWidth / 2, mBarDY + aTextY);

	SexyString aVersion = GameText(GT_TITLE_VERSION) + mApp->mProductVersion;
	g->SetFont(mVersionFont);
	g->DrawString(aVersion, (mWidth - g->GetFont()->StringWidth(aVersion)) - 35, mLogoY + 260);
}
