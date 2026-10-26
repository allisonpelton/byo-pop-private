// The full-screen widgets of Rocket Mania Deluxe (see Screens.h).

#include "Screens.h"
#include "RocketApp.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// LoadingScreen

// 0x4104b0
LoadingScreen::LoadingScreen(RocketApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mLinkShown(false)
{
	// port: the original makes a second link, "register now", for the unregistered game and hides it for a
	// registered one; this one is the play link, which starts hidden until the game has loaded
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mFonts[FONT_DOMO14_BOLD]);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xb4, 0, 0);
	mPlayLink->mOverColor = Color(0xeb, 0x7d, 5);
	mPlayLink->mUnderlineSize = 0;
}

// 0x410ab0
LoadingScreen::~LoadingScreen()
{
	delete mPlayLink;
}

// 0x4117b0 / 0x4117f0
void LoadingScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

void LoadingScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4111b0
void LoadingScreen::Update()
{
	Widget::Update();

	// the link's text and place are set again on every update
	mPlayLink->mLabel = GT(0x004e54d8);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize(0x140 - (aWidth + 8) / 2, 0x1b6, aWidth + 8, 0x1a);

	if (!mLinkShown && mUpdateCnt % 4 == 0)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if (mProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLinkShown = true;
			mProgress = 1.0;
		}
		MarkDirty();
	}
}

// 0x410bb0
void LoadingScreen::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->SetColor(Color(0, 0, 0));
	g->DrawImage(anImages[IMAGE_LOADINGGRAPHIC], 0, 0);

	int aBarWidth = anImages[IMAGE_LOADINGBAR]->GetWidth();
	int aBarX = (mWidth - aBarWidth) / 2;
	g->DrawImage(anImages[IMAGE_LOADINGBAREMPTY], aBarX, 0x186);
	g->SetFont(mApp->mFonts[FONT_ARIAL8]);

	g->SetColor(Color(100, 100, 100, 180));
	if (mProgress >= 1.0)
		g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX, 0x186);
	else if (mProgress > 0.0)
	{
		int aFilled = (int) (mProgress * aBarWidth);
		g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX, 0x186, Rect(0, 0, aFilled, 0x34));
		// the bar's leading edge: 16 columns that fade out, added on top
		for (int i = 0; i < 16; i++)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(0xff, 0xff, 0xff, (int) (cos(i * 0.0981746875) * 255.0)));
			g->DrawImage(anImages[IMAGE_LOADINGBAR], aBarX + aFilled + i, 0x186, Rect(aFilled + i, 0, 1, 0x34));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	// the line under the bar: the thanks (empty with NoReg) or, on the first run, the wait for the sounds
	SexyString aLine;
	if (!mApp->GetBoolean("NoReg", false))
		aLine = GT(0x004e5258);
	if (mApp->mTimesExecuted == 0 && mProgress < 1.0)
		aLine = GT(0x004e5234);
	if (!mPlayLink->mVisible)
	{
		g->SetFont(mApp->mFonts[FONT_ARIAL10_BOLD]);
		int aX = 0x140 - g->GetFont()->StringWidth(aLine) / 2;
		g->SetColor(Color(0xaa, 10, 10));
		g->DrawString(aLine, aX, 0x1c2);
	}
}

// 0x411830: the play link
void LoadingScreen::ButtonDepress(int theId)
{
	mApp->LoadingScreenDone();
}
