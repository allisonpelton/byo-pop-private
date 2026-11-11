#include "TitleScreen.h"
#include "FossilizedApp.h"
#include "DDImage.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x438b20
TitleScreen::TitleScreen(FossilizedApp* theApp)
{
	mApp = theApp;
	mProgress = 0.0;
	mLoadingDone = false;
	// port: the registered copy's thanks (unless the property NoReg) is registration code and not in the port
	mCobrand = NULL;
	mCobranded = mApp->GetBoolean("Cobranded", false);
	if (mCobranded)
		mCobrand = new Pic(mApp, "images\\cobrand", true);

	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mWhiteFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xff, 0xff, 0);
	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");

}

// 0x4388d0
TitleScreen::~TitleScreen()
{
	delete mPlayLink;
	mPlayLink = NULL;
	delete mCobrand;
	mCobrand = NULL;
}

// 0x438970
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4389b0
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4389f0: the play link centred
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b6, aWidth, 0x18);
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
	g->SetColor(Color(0x80, 0xff, 0x80));

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
}

// 0x439b20: the bar follows the loader; the play link shows once it is done
void TitleScreen::Update()
{
	Widget::Update();
	if (!mLoadingDone && (mUpdateCnt % 4 == 0))
	{
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

// 0x438ab0: the play link
void TitleScreen::ButtonDepress(int theId)
{
	mApp->mSndEgglands->Play();
	mApp->TitleScreenDone();
}
