#include "LoadingScreen.h"
#include "NcApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "Image.h"
#include "DDImage.h"
#include "WidgetManager.h"
#include <math.h>

using namespace Sexy;

static const char* gLoadingImageNames[3] = { "loadinggraphic", "loadingbar", "loadingbarempty" };

// 0x4e77e0 (settings from 0x418960)
LoadingScreen::LoadingScreen(NcApp* theApp)
{
	mApp = theApp;
	mFade = 255.0;
	mDelay = 100;
	mLoadDone = false;
	mPlayRequested = false;
	mProgress = 0.0;
	mProgressCap = 1.0;

	// 0x4e7540: the three pictures of the Nc framework
	for (int i = 0; i < 3; i++)
	{
		SexyString aPath = SexyString("NcFramework\\images\\") + gLoadingImageNames[i];
		mImages[i] = mApp->GetImage(aPath);
		if (mImages[i] == NULL)
		{
			mApp->Popup("Failed to load " + aPath);
			mApp->mLoadingFailed = true;
			break;
		}
	}

	mStatusFont = new ImageFont(mApp, "data\\Praitor24.txt");
	mLinkFont = new ImageFont(mApp, "data\\Praitor36.txt");

	// the register link is hidden for good in a registered game; kept so the ids and the layout stay the original's
	mRegisterLink = new HyperlinkWidget(REGISTER_LINK_ID, this);
	mRegisterLink->mUnderlineSize = 0;
	mRegisterLink->SetFont(mLinkFont);
	mRegisterLink->mColor = Color(0, 255, 0);
	mRegisterLink->mOverColor = Color(255, 0x91, 5);
	mRegisterLink->SetVisible(false);

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetFont(mLinkFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(235, 235, 235);
	mPlayLink->mOverColor = Color(255, 255, 255);
}

// 0x4e6a50
LoadingScreen::~LoadingScreen()
{
	delete mRegisterLink;
	delete mPlayLink;
	delete mStatusFont;
	delete mLinkFont;
	for (int i = 0; i < 3; i++)
		delete mImages[i];
}

// 0x4e67b0
void LoadingScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4e67e0
void LoadingScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4e6810: the bar fills from the left by 20 + 332 * progress pixels; its end is feathered with 16 columns whose
// alpha is the original's truncated cosine (255 for the first, 0 for the others)
void LoadingScreen::DrawBar(Graphics* g)
{
	int aBarY = 0x1bb;
	int aBarX = (mWidth - mImages[1]->GetWidth()) / 2;

	if (mProgress < 1.0)
	{
		g->DrawImage(mImages[2], aBarX, aBarY);
		if (mProgress > 0.0)
		{
			int aFill = 0x14 - (int)(mProgress * -332.0);
			g->DrawImage(mImages[1], aBarX, aBarY, Rect(0, 0, aFill, mImages[1]->GetHeight()));
			for (int i = 0; i < 0x10; i++)
			{
				g->SetColorizeImages(true);
				int anAlpha = (int)cos(i * 0.0981746875) * 0xff;
				g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
				g->DrawImage(mImages[1], aBarX + aFill + i, aBarY, Rect(aFill + i, 0, 1, mImages[1]->GetHeight()));
				g->SetColorizeImages(false);
			}
		}
		return;
	}

	// finished: the full bar, fading out after a while
	if (!mLoadDone)
		g->DrawImage(mImages[1], aBarX, aBarY);
	else if (mFade > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, (int)mFade));
		g->DrawImage(mImages[1], aBarX, aBarY);
		g->SetColorizeImages(false);
	}
}

// 0x4e6f30 (and the status line 0x4e6bf0, empty in a registered game)
void LoadingScreen::Draw(Graphics* g)
{
	Widget::Draw(g);
	g->DrawImage(mImages[0], 0, 0);
	DrawBar(g);
}

// 0x418820 + 0x4e6f70
void LoadingScreen::Update()
{
	// only while the window has the focus
	if (!mApp->mHasFocus)
		return;

	Widget::Update();

	if (mLoadDone)
	{
		if (mDelay == 0)
		{
			if (mFade > 0.0)
			{
				double aFade = mFade;
				mFade = aFade - 1.4;
				if (aFade - 1.4 < 0.0)
					mFade = 0.0;
				MarkDirty();
			}
		}
		else
			mDelay--;
		mPlayLink->SetVisible(true);
	}
	else
	{
		if ((mUpdateCnt % 4) == 0)
		{
			mProgress = mApp->GetLoadingThreadProgress();
			if (mProgress == 1.0 && mApp->mLoaded)
			{
				mPlayLink->SetVisible(true);
				mLoadDone = true;
				mProgress = 1.0;
			}
			MarkDirty();
		}
	}

	// the play link lies over the finished bar
	mPlayLink->mLabel = GT(0x5288f8);
	int aWidth = mImages[1]->GetWidth();
	mPlayLink->Resize(0x140 - aWidth / 2, 0x1ae, aWidth, 0x2e);
}

// 0x4e73c0
void LoadingScreen::ButtonDepress(int theId)
{
	if (theId == PLAY_LINK_ID)
		mApp->PlayClicked();
}
