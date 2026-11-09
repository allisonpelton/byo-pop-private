#include "TitleScreen.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "DDImage.h"
#include "Res.h"

using namespace Sexy;

// 0x448e10: Widget first. The images are loaded by AddControls (the original leaves their fields unset until then)
TitleScreen::TitleScreen(WinWSApp* theApp)
{
	mApp = theApp;
	mTitleImage = NULL;	// port: the original leaves +0x58..+0x70 unset until AddControls
	mBarAndLogo = NULL;
	mGoldBar = NULL;
}

// 0x4414c0 (deleting destructor 0x441ff0, slot 0): the three pictures go
TitleScreen::~TitleScreen()
{
	if (mTitleImage != NULL)
		delete mTitleImage;
	if (mBarAndLogo != NULL)
		delete mBarAndLogo;
	if (mGoldBar != NULL)
		delete mGoldBar;
}

// 0x448f40: the three pictures of the screen. When they are all there the screen is ready; otherwise the app is told to
// quit (Popup with the text, then mLoadingFailed)
void TitleScreen::AddControls()
{
	mTitleImage = mApp->GetImage("images\\titlescreen");
	mBarAndLogo = mApp->GetImage("images\\barandlogo");
	mGoldBar = mApp->GetImage("images\\goldbar");

	if (mTitleImage != NULL && mBarAndLogo != NULL && mGoldBar != NULL)
		return;

	mApp->Popup(GT(0x0052fb90));	// slot 21
	mApp->mLoadingFailed = true;	// +0x528
}

// 0x4495c0 slot 16: the title picture, the bar and logo at (0x7e, 0x162), and the gold bar cut at the width the loading
// progress gives (the progress, 0 to 1, times the bar's width, truncated; nothing when that is 0) at (0x95, 0x171)
void TitleScreen::Draw(Graphics* g)
{
	g->DrawImage(mTitleImage, 0, 0);
	g->DrawImage(mBarAndLogo, 0x7e, 0x162);

	Image* aGoldBar = mGoldBar;
	int aBarWidth = (int) (mApp->GetLoadingThreadProgress() * aGoldBar->mWidth);	// slot 51, then _ftol
	if (aBarWidth > 0)
	{
		Rect aSrcRect(0, 0, aBarWidth, aGoldBar->mHeight);
		g->DrawImage(aGoldBar, 0x95, 0x171, aSrcRect);
	}
}
